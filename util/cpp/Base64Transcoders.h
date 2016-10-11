/*
 * Base64Transcoders.h
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

#ifndef INCL_BASE_64_TRANSCODERS_H
#define INCL_BASE_64_TRANSCODERS_H

namespace NOMADSUtil
{
    class Base64Transcoders
    {
        public:
            // Encodes a binary block of data (of the specified length) into a Base64
            // null-terminated string
            // Caller must deallocate the returned string buffer using free()
            static char * encode (const void *pBuf, unsigned int uiLen);

            // Decodes the Base64 encoded block into a newly allocated block of memory
            // The length of the decoded block is written into the integer pointed to by puiLen
            // Caller must deallocate returned block using free()
            static void * decode (const char *pszBuf, unsigned int *puiLen);
    };
}

#endif   // #ifndef INCL_BASE_64_TRANSCODERS_H
