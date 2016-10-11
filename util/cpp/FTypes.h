/*
 * FTypes.h
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
 *
 * This file defines a set of commonly used data types that are suffixed by their
 * sizes - useful when working across multiple platforms with different word sizes
 *
 * Author: Niranjan Suri
 * Date: June 17, 1999
 */

#ifndef INCL_FIXED_TYPES_H
#define INCL_FIXED_TYPES_H

typedef char               int8;
typedef unsigned char     uint8;
typedef short             int16;
typedef unsigned short   uint16;
typedef int               int32;
typedef unsigned int     uint32;

#if defined (WIN32)
    typedef __int64             int64;
    typedef unsigned __int64   uint64;
#elif defined (UNIX)
    typedef long long int       int64;
    typedef unsigned long long uint64;
#else
    struct int64
    {
        int32 lowWord;
        int32 highWord;
    };
#endif

#endif   // #ifndef INCL_FIXED_TYPES_H
