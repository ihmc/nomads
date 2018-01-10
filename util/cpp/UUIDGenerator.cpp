/*
 * UUIDGenerator.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include "UUIDGenerator.h"

#include "MD5.h"

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#if defined (WIN32)
	#define NOMINMAX
	#include <winsock2.h>
    #include <windows.h>
#elif defined (LINUX)
    #include <sys/sysinfo.h>
    #include <sys/time.h>
    #include <unistd.h>
#elif defined (SOLARIS)
    #include <sys/systeminfo.h>
    #include <unistd.h>
#elif defined (OSX)
    #include <sys/time.h>
    #include <unistd.h>
#endif

#include "EndianHelper.h"     // NOTE: This include should be here after all the system includes
                              //       because it undefines htons, htonl, ntohs, and ntohl

using namespace NOMADSUtil;

UUIDGenerator::UUIDGenerator (void)
{
    /* set the following to the number of 100ns ticks of the actual resolution of
       your system's clock */
    UUIDS_PER_TICK=1;
}

UUIDGenerator::~UUIDGenerator (void)
{
}

const char * UUIDGenerator::create (void)
{
    local_uuid_t u;
    local_uuid_create (&u);
    sprintf (szUUID, "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
             u.time_low, u.time_mid,
             u.time_hi_and_version, u.clock_seq_hi_and_reserved,
             u.clock_seq_low,
             u.node[0], u.node[1], u.node[2], u.node[3], u.node[4], u.node[5]);
    return szUUID;
}

int UUIDGenerator::create (unsigned char *puchUUID)
{
    local_uuid_t u;
    local_uuid_create (&u);
    memcpy (puchUUID, &u, 16);
    return 0;
}

const char * UUIDGenerator::convertToString (const unsigned char *puchUUID)
{
    local_uuid_t u;
    memcpy (&u, puchUUID, 16);
    sprintf (szUUID, "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
             u.time_low, u.time_mid,
             u.time_hi_and_version, u.clock_seq_hi_and_reserved,
             u.clock_seq_low,
             u.node[0], u.node[1], u.node[2], u.node[3], u.node[4], u.node[5]);
    return szUUID;
}

#define LOCK _m.lock();
#define UNLOCK _m.unlock();

UUIDGenerator::uuid_state UUIDGenerator::st;

/* uuid_create -- generator a UUID */
int UUIDGenerator::local_uuid_create (local_uuid_t * uuid)
{
    uuid_time_t timestamp, last_time;
    uint16 clockseq;
    uuid_node_t node;
    uuid_node_t last_node;
    int f;

    /* acquire system wide lock so we're alone */
    LOCK;

    /* get current time */
    get_current_time(&timestamp);

    /* get node ID */
    get_ieee_node_identifier(&node);

    /* get saved state from NV storage */
    f = read_state(&clockseq, &last_time, &last_node);

    /* if no NV state, or if clock went backwards, or node ID changed
    (e.g., net card swap) change clockseq */
    if (!f || memcmp(&node, &last_node, sizeof(uuid_node_t)))
        clockseq = true_random();
    else if (timestamp < last_time)
        clockseq++;

    /* stuff fields into the UUID */
    format_uuid_v1(uuid, clockseq, timestamp, node);

    /* save the state for next time */
    write_state(clockseq, timestamp, node);

    UNLOCK;
    return(1);
};

/* format_uuid_v1 -- make a UUID from the timestamp, clockseq,
                     and node ID */
void UUIDGenerator::format_uuid_v1 (local_uuid_t * uuid, uint16 clock_seq, uuid_time_t timestamp, uuid_node_t node)
{
    /* Construct a version 1 uuid with the information we've gathered
     * plus a few constants. */
    uuid->time_low = (unsigned long)(timestamp & 0xFFFFFFFF);
    uuid->time_mid = (unsigned short)((timestamp >> 32) & 0xFFFF);
    uuid->time_hi_and_version = (unsigned short)((timestamp >> 48) & 0x0FFF);
    uuid->time_hi_and_version |= (1 << 12);
    uuid->clock_seq_low = clock_seq & 0xFF;
    uuid->clock_seq_hi_and_reserved = (clock_seq & 0x3F00) >> 8;
    uuid->clock_seq_hi_and_reserved |= 0x80;
    memcpy(&uuid->node, &node, sizeof uuid->node);
};

/* read_state -- read UUID generator state from non-volatile store */
int UUIDGenerator::read_state(uint16 *clockseq, uuid_time_t *timestamp, uuid_node_t *node)
{
    if (st.cs == 0) {
        return 0;
    }
    *clockseq = st.cs;
    *timestamp = st.ts;
    *node = st.node;
    return 1;
};

/* write_state -- save UUID generator state back to non-volatile storage */
void UUIDGenerator::write_state(uint16 clockseq, uuid_time_t timestamp, uuid_node_t node)
{
    /* always save state to volatile shared state */
    st.cs = clockseq;
    st.ts = timestamp;
    st.node = node;
};

/* get-current_time -- get time as 60 bit 100ns ticks since whenever.
   Compensate for the fact that real clock resolution is
   less than 100ns. */
void UUIDGenerator::get_current_time(uuid_time_t * timestamp)
{
    uuid_time_t         time_now;
    static uuid_time_t  time_last;
    static uint16   uuids_this_tick;
    static int          inited = 0;

    if (!inited) {
        get_system_time(&time_last);
        uuids_this_tick = UUIDS_PER_TICK;
        inited = 1;
    };

    while (1) {
        get_system_time(&time_now);

        /* if clock reading changed since last UUID generated... */
        if (time_last != time_now) {
            /* reset count of uuids gen'd with this clock reading */
            uuids_this_tick = 0;
            time_last = time_now;
            break;
        };
        if (uuids_this_tick < UUIDS_PER_TICK) {
            uuids_this_tick++;
            break;
        };
        /* going too fast for our clock; spin */
    };
    /* add the count of uuids to low order bits of the clock reading */
    *timestamp = time_now + uuids_this_tick;
};

/* true_random -- generate a crypto-quality random number.
   This sample doesn't do that. */
uint16 UUIDGenerator::true_random (void)
{
    static int inited = 0;
    uuid_time_t time_now;

    if (!inited) {
        get_system_time(&time_now);
        time_now = time_now/UUIDS_PER_TICK;
        srand((unsigned int)(((time_now >> 32) ^ time_now)&0xffffffff));
        inited = 1;
    };
    return (rand());
}

/* uuid_create_from_name -- create a UUID using a "name" from a "name
space" */
void UUIDGenerator::uuid_create_from_name (
  local_uuid_t * uuid,        /* resulting UUID */
  local_uuid_t nsid,          /* UUID to serve as context, so identical
                           names from different name spaces generate
                           different UUIDs */
  void * name,          /* the name from which to generate a UUID */
  int namelen)          /* the length of the name */
{
  MD5 md5;
  local_uuid_t net_nsid;      /* context UUID in network byte order */

  /* put name space ID in network byte order so it hashes the same
      no matter what endian machine we're on */
  net_nsid = nsid;
  net_nsid.time_low = EndianHelper::htonl (net_nsid.time_low);
  net_nsid.time_mid = EndianHelper::htons (net_nsid.time_mid);
  net_nsid.time_hi_and_version = EndianHelper::htons (net_nsid.time_hi_and_version);

  md5.init();
  md5.update (&net_nsid, sizeof(local_uuid_t));
  md5.update (name, namelen);
  unsigned char *hash = (unsigned char*) md5.getChecksum();

  /* the hash is in network byte order at this point */
  format_uuid_v3(uuid, hash);
};

/* format_uuid_v3 -- make a UUID from a (pseudo)random 128 bit number
*/
void UUIDGenerator::format_uuid_v3 (local_uuid_t * uuid, unsigned char hash[16])
{
    /* Construct a version 3 uuid with the (pseudo-)random number
     * plus a few constants. */

    memcpy(uuid, hash, sizeof(local_uuid_t));

  /* convert UUID to local byte order */
  uuid->time_low = EndianHelper::ntohl (uuid->time_low);
  uuid->time_mid = EndianHelper::ntohs (uuid->time_mid);
  uuid->time_hi_and_version = EndianHelper::ntohs (uuid->time_hi_and_version);

  /* put in the variant and version bits */
    uuid->time_hi_and_version &= 0x0FFF;
    uuid->time_hi_and_version |= (3 << 12);
    uuid->clock_seq_hi_and_reserved &= 0x3F;
    uuid->clock_seq_hi_and_reserved |= 0x80;
};

/* uuid_compare --  Compare two UUID's "lexically" and return
       -1   u1 is lexically before u2
        0   u1 is equal to u2
        1   u1 is lexically after u2
    Note:   lexical ordering is not temporal ordering!
*/
int UUIDGenerator::uuid_compare (local_uuid_t *u1, local_uuid_t *u2)
{
  int i;

#define CHECK(f1, f2) if (f1 != f2) return f1 < f2 ? -1 : 1;
  CHECK(u1->time_low, u2->time_low);
  CHECK(u1->time_mid, u2->time_mid);
  CHECK(u1->time_hi_and_version, u2->time_hi_and_version);
  CHECK(u1->clock_seq_hi_and_reserved, u2->clock_seq_hi_and_reserved);
  CHECK(u1->clock_seq_low, u2->clock_seq_low)
  for (i = 0; i < 6; i++) {
      if (u1->node[i] < u2->node[i])
           return -1;
      if (u1->node[i] > u2->node[i])
      return 1;
    }
  return 0;
};

/* system dependent call to get IEEE node ID.
   This sample implementation generates a random node ID
   */
void UUIDGenerator::get_ieee_node_identifier(uuid_node_t *node)
{
    char seed[16];
    static int inited = 0;
    static uuid_node_t saved_node;

    if (!inited) {
        get_random_info(seed);
        seed[0] |= 0x80;
        memcpy(&saved_node, seed, sizeof(uuid_node_t));
        inited = 1;
    };

    *node = saved_node;
};

/* system dependent call to get the current system time.
   Returned as 100ns ticks since Oct 15, 1582, but resolution may be
   less than 100ns.
*/
#ifdef WIN32

void UUIDGenerator::get_system_time (uuid_time_t *uuid_time)
{
  ULARGE_INTEGER time;

  GetSystemTimeAsFileTime((FILETIME *)&time);

  /* NT keeps time in FILETIME format which is 100ns ticks since
     Jan 1, 1601.  UUIDs use time in 100ns ticks since Oct 15, 1582.
     The difference is 17 Days in Oct + 30 (Nov) + 31 (Dec)
     + 18 years and 5 leap days.
  */

  time.QuadPart +=
          (unsigned __int64) (1000*1000*10)       // seconds
        * (unsigned __int64) (60 * 60 * 24)       // days
        * (unsigned __int64) (17+30+31+365*18+5); // # of days

  *uuid_time = time.QuadPart;

};

void UUIDGenerator::get_random_info(char seed[16])
{
  MD5 md5;
  md5.init();
  typedef struct {
      MEMORYSTATUS m;
      SYSTEM_INFO s;
      FILETIME t;
      LARGE_INTEGER pc;
      DWORD tc;
      DWORD l;
      char hostname[MAX_COMPUTERNAME_LENGTH + 1];
  } randomness;
  randomness r;

  /* memory usage stats */
  GlobalMemoryStatus(&r.m);
  /* random system stats */
  GetSystemInfo(&r.s);
  /* 100ns resolution (nominally) time of day */
  GetSystemTimeAsFileTime(&r.t);
  /* high resolution performance counter */
  QueryPerformanceCounter(&r.pc);
  /* milliseconds since last boot */
  r.tc = GetTickCount();
  r.l = MAX_COMPUTERNAME_LENGTH + 1;
  GetComputerName(r.hostname, &r.l );
  md5.update (&r, sizeof(randomness));
  memcpy (seed, md5.getChecksum(), 16);
};
#else

void UUIDGenerator::get_system_time(uuid_time_t *uuid_time)
{
    struct timeval tp;

    gettimeofday(&tp, (struct timezone *)0);

    /* Offset between UUID formatted times and Unix formatted times.
       UUID UTC base time is October 15, 1582.
       Unix base time is January 1, 1970.
    */
    *uuid_time = (tp.tv_sec * 10000000) + (tp.tv_usec * 10) +
                 0x01B21DD213814000LL;
};

void UUIDGenerator::get_random_info(char seed[16])
{
    MD5 md5;
    typedef struct {
        #if defined (LINUX)
            struct sysinfo s;
        #elif defined (SOLARIS)
            char szHWSerial[255];
        #endif
        struct timeval t;
        char hostname[257];
    } randomness;
    randomness r;
	
    #if defined (LINUX)
        #if defined (ANDROID)
          //COMMENTED OUT BECAUSE NOT WORKING ON ANDROID X86
          //SHOULD NOT CAUSE ISSUES
	        //sysinfo (r.s);
        #else  
          sysinfo (&r.s);
        #endif
    #elif defined (SOLARIS)
        sysinfo (SI_HW_SERIAL, r.szHWSerial, sizeof (r.szHWSerial));
    #endif

    gettimeofday(&r.t, (struct timezone *)0);
    gethostname(r.hostname, 256);

    md5.init();
    md5.update (&r, sizeof(randomness));
    memcpy (seed, md5.getChecksum(), 16);
};

#endif
