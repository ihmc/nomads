/*
 * EndianHelper.h
 *
 *This file is part of the IHMC Util Library
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

/* The following are required (before the guard) because,
 * when using -Ox, gcc seems to define these as macros */
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
        static uint16 htons (const uint16 ui16);
        static uint16 ntohs (const uint16 ui16);

        static int32 htonl (const int32 i32);
        static uint32 htonl (const uint32 ui32);
        static int32 ntohl (const int32 i32);
        static uint32 ntohl (const uint32 ui32);

        static int64 hton64 (const int64 i64);
        static uint64 hton64 (const uint64 ui64);
        static int64 ntoh64 (const int64 i64);
        static uint64 ntoh64 (const uint64 ui64);

        static float htonl (float f);
        static float ntohl (float f);

        static uint16 byteSwap16 (const uint16 ui16);
        static uint32 byteSwap32 (const uint32 ui32);
        static uint64 byteSwap64 (const uint64 ui64);
    };


    inline uint16 EndianHelper::htons (const uint16 ui16)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            return EndianHelper::byteSwap16 (ui16);
        #else
            return ui16;
        #endif
    }

    inline uint16 EndianHelper::ntohs (const uint16 ui16)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            return EndianHelper::byteSwap16 (ui16);
        #else
            return ui16;
        #endif
    }

    inline int32 EndianHelper::htonl (const int32 i32)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            return EndianHelper::byteSwap32 (i32);
        #else
            return i32;
        #endif
    }

    inline uint32 EndianHelper::htonl (const uint32 ui32)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            return EndianHelper::byteSwap32 (ui32);
        #else
            return ui32;
        #endif
    }

    inline int32 EndianHelper::ntohl (const int32 i32)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            return EndianHelper::byteSwap32 (i32);
        #else
            return i32;
        #endif
    }

    inline uint32 EndianHelper::ntohl (const uint32 ui32)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            return EndianHelper::byteSwap32 (ui32);
        #else
            return ui32;
        #endif
    }

    inline int64 EndianHelper::hton64 (const int64 i64)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            return EndianHelper::byteSwap64 (i64);
        #else
            return i64;
        #endif
    }

    inline uint64 EndianHelper::hton64 (const uint64 ui64)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            return EndianHelper::byteSwap64 (ui64);
        #else
            return ui64;
        #endif
    }

    inline int64 EndianHelper::ntoh64 (const int64 i64)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            return EndianHelper::byteSwap64 (i64);
        #else
            return i64;
        #endif
    }

    inline uint64 EndianHelper::ntoh64 (const uint64 ui64)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            return EndianHelper::byteSwap64 (ui64);
        #else
            return ui64;
        #endif
    }

    inline float EndianHelper::htonl (float f)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            union v {
                float f;
                unsigned int i;
            };

            reinterpret_cast<v *> (&f)->i =
                EndianHelper::byteSwap32 (reinterpret_cast<v *> (&f)->i);
            return reinterpret_cast<v *> (&f)->f;
        #endif
        return f;
    }

    inline float EndianHelper::ntohl (float f)
    {
        #if defined (LITTLE_ENDIAN_SYSTEM)
            union v {
                float f;
                unsigned int i;
            };

            reinterpret_cast<v *> (&f)->i =
                EndianHelper::byteSwap32 (reinterpret_cast<v *> (&f)->i);
            return reinterpret_cast<v *> (&f)->f;
        #endif
        return f;
    }

    inline uint16 EndianHelper::byteSwap16 (const uint16 ui16)
    {
        return ((((ui16) & 0xff00) >> 8) |
                (((ui16) & 0x00ff) << 8));
    }

    inline uint32 EndianHelper::byteSwap32 (const uint32 ui32)
    {
        return ((((ui32) & 0xff000000) >> 24) |
                (((ui32) & 0x00ff0000) >>  8) |
                (((ui32) & 0x0000ff00) <<  8) |
                (((ui32) & 0x000000ff) << 24));
    }

    inline uint64 EndianHelper::byteSwap64 (const uint64 ui64)
    {
        return ((((ui64) & 0xff00000000000000ull) >> 56) |
                (((ui64) & 0x00ff000000000000ull) >> 40) |
                (((ui64) & 0x0000ff0000000000ull) >> 24) |
                (((ui64) & 0x000000ff00000000ull) >>  8) |
                (((ui64) & 0x00000000ff000000ull) <<  8) |
                (((ui64) & 0x0000000000ff0000ull) << 24) |
                (((ui64) & 0x000000000000ff00ull) << 40) |
                (((ui64) & 0x00000000000000ffull) << 56));
    }
}

#endif   // #ifndef INCL_ENDIAN_HELPER_H
