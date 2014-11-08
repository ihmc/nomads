/*
 * Base64Transcoders.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
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
