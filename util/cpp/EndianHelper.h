/*
 * EndianHelper.h
 *
 *This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

// The following are required because gcc when using -O seems
// to define these as macros

#if defined (htons)
    #undef htons
#endif
#if defined (htonl)
    #undef htonl
#endif
#if defined (ntohs)
    #undef ntohs
#endif
#if defined (ntohl)
    #undef ntohl
#endif

#ifndef INCL_ENDIAN_HELPER_H
#define INCL_ENDIAN_HELPER_H

#include "FTypes.h"

#if !defined (LITTLE_ENDIAN_SYSTEM) && !defined (BIG_ENDIAN_SYSTEM)
    #error Must Define LITTLE_ENDIAN_SYSTEM or BIG_ENDIAN_SYSTEM!
#endif

namespace NOMADSUtil
{

    class EndianHelper
    {
        public:
            static void byteSwap16 (void *pBuf);
            static void byteSwap32 (void *pBuf);
            static void byteSwap64 (void *pBuf);
            static uint16 htons (uint16 ui16);
            static uint32 htonl (uint32 ui32);
            static float htonl (float f);
            static int64 hton64 (int64 i64);
            static uint16 ntohs (uint16 ui16);
            static int32 ntohl (int32 ui32);
            static uint32 ntohl (uint32 ui32);
            static float ntohl (float f);
            static int64 ntoh64 (int64 i64);
    };

    inline uint16 EndianHelper::htons (uint16 ui16)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            byteSwap16 (&ui16);
        #endif
        return ui16;
    }

    inline uint32 EndianHelper::htonl (uint32 ui32)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            byteSwap32 (&ui32);
        #endif
        return ui32;
    }

    inline float EndianHelper::htonl (float f)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            byteSwap32 (&f);
        #endif
        return f;
    }

    inline int64 EndianHelper::hton64 (int64 i64)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            byteSwap64 (&i64);
        #endif
        return i64;
    }

    inline uint16 EndianHelper::ntohs (uint16 ui16)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            byteSwap16 (&ui16);
        #endif
        return ui16;
    }

    inline int32 EndianHelper::ntohl (int32 i32)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            byteSwap32 (&i32);
        #endif
        return i32;
    }

    inline uint32 EndianHelper::ntohl (uint32 ui32)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            byteSwap32 (&ui32);
        #endif
        return ui32;
    }

    inline float EndianHelper::ntohl (float f)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            byteSwap32 (&f);
        #endif
        return f;
    }

    inline int64 EndianHelper::ntoh64 (int64 i64)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            byteSwap64 (&i64);
        #endif
        return i64;
    }

}

#endif   // #ifndef INCL_ENDIAN_HELPER_H
