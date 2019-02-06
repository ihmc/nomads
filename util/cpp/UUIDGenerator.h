/*
 * UUIDGenerator.h
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

#ifndef INCL_UUID_Generator_H
#define INCL_UUID_Generator_H

#include "FTypes.h"
#include "Mutex.h"

namespace NOMADSUtil
{

    class UUIDGenerator
    {
        public:
            UUIDGenerator (void);
            ~UUIDGenerator (void);

            // Creates a new UUID and copies the UUID into the specified buffer,
            // which must be at least 16 bytes long
            // NOTE: The UUID is not a string representation and is not null-terminated
            int create (unsigned char *puchUUID);

            // Creates a new UUID and returns the UUID in string form
            // The UUID string is held in a buffer which is reused the
            //     next time create() or convertToString() is called
            const char * create (void);

            // Converts a UUID from binary form into a string representation
            // The UUID string is held in a buffer which is reused the
            //     next time create() or convertToString() is called
            const char * convertToString (const unsigned char *puchUUID);

        private:
            /*
            ** Copyright (c) 1990- 1993, 1996 Open Software Foundation, Inc.
            ** Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, Ca. &
            ** Digital Equipment Corporation, Maynard, Mass.
            ** Copyright (c) 1998 Microsoft.
            ** To anyone who acknowledges that this file is provided "AS IS"
            ** without any express or implied warranty: permission to use, copy,
            ** modify, and distribute this file for any purpose is hereby
            ** granted without fee, provided that the above copyright notices and
            ** this notice appears in all source code copies, and that none of
            ** the names of Open Software Foundation, Inc., Hewlett-Packard
            ** Company, or Digital Equipment Corporation be used in advertising
            ** or publicity pertaining to distribution of the software without
            ** specific, written prior permission.  Neither Open Software
            ** Foundation, Inc., Hewlett-Packard Company, Microsoft, nor Digital Equipment
            ** Corporation makes any representations about the suitability of
            ** this software for any purpose.
            */

            int UUIDS_PER_TICK;

            typedef uint64 uuid_time_t;

            typedef struct {
                char nodeID[6];
            } uuid_node_t;

            typedef struct _local_uuid_t {
                uint32          time_low;
                uint16          time_mid;
                uint16          time_hi_and_version;
                unsigned char   clock_seq_hi_and_reserved;
                unsigned char   clock_seq_low;
                unsigned char   node[6];
            } local_uuid_t;

            /* data type for UUID generator persistent state */
            typedef struct {
                uuid_time_t ts;       /* saved timestamp */
                uuid_node_t node;     /* saved node ID */
                uint16 cs;        /* saved clock sequence */
            } uuid_state;

            static uuid_state st;

            /* uuid_create -- generate a UUID */
            int local_uuid_create (local_uuid_t * uuid);

            /* uuid_create_from_name -- create a UUID using a "name"
               from a "name space" */
            void uuid_create_from_name (
                local_uuid_t * uuid,        /* resulting UUID */
                local_uuid_t nsid,          /* UUID to serve as context, so identical
                                         names from different name spaces generate
                                         different UUIDs */
                void * name,          /* the name from which to generate a UUID */
                int namelen);         /* the length of the name */

            /* uuid_compare --  Compare two UUID's "lexically" and return
                    -1   u1 is lexically before u2
                     0   u1 is equal to u2
                     1   u1 is lexically after u2
               Note:   lexical ordering is not temporal ordering!
            */
            int uuid_compare (local_uuid_t *u1, local_uuid_t *u2);

            int read_state (uint16 *clockseq, uuid_time_t *timestamp, uuid_node_t * node);
            void write_state (uint16 clockseq, uuid_time_t timestamp, uuid_node_t node);
            void format_uuid_v1 (local_uuid_t * uuid, uint16 clockseq, uuid_time_t timestamp, uuid_node_t node);
            void format_uuid_v3 (local_uuid_t * uuid, unsigned char hash[16]);
            void get_current_time (uuid_time_t * timestamp);
            uint16 true_random (void);

            // From sysdep.h
            void get_ieee_node_identifier(uuid_node_t *node);
            void get_system_time(uuid_time_t *uuid_time);
            void get_random_info(char seed[16]);

        private:
            Mutex _m;
            char szUUID[80];
    };

}

#endif   // #ifndef INCL_UUID_Generator_H
