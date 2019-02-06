/*
 * MD5.h
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

#ifndef INCL_MD5_H
#define INCL_MD5_H

/*
 * MD5.h
 *
 * Header file for MD5 class that can be used to compute MD5 checksums
 *
 * This code is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm
 *
 */

#include "FTypes.h"

namespace NOMADSUtil
{
    class BufferWriter;
    class Reader;

    #define MD5_LENGTH 16

    class MD5
    {
        public:
            MD5 (void);
            ~MD5 (void);

            // Init may be called multiple times to reuse the object to compute
            //     multiple checksums
            int init (void);

            // Update the checksum given the next set of data
            // Returns failure only if pBuf is NULL
            int update (const void *pBuf, unsigned long ulBufSize);

            // Get the checksum - may be called multiple times
            // Cannot call update() anymore until init() is called again
            // Returns a const pointer to an array of 16 unsigned chars
            const unsigned char * getChecksum (void);

            // NOTE: the returned \0-terminated string must be deallocated by
            // the caller
            char * getChecksumAsString (void);

            int length (void) {  return MD5_LENGTH;  }

        private:
            /* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
               rights reserved.

               License to copy and use this software is granted provided that it
               is identified as the "RSA Data Security, Inc. MD5 Message-Digest
               Algorithm" in all material mentioning or referencing this software
               or this function.

               License is also granted to make and use derivative works provided
               that such works are identified as "derived from the RSA Data
               Security, Inc. MD5 Message-Digest Algorithm" in all material
               mentioning or referencing the derived work.

               RSA Data Security, Inc. makes no representations concerning either
               the merchantability of this software or the suitability of this
               software for any particular purpose. It is provided "as is"
               without express or implied warranty of any kind.

               These notices must be retained in any copies of any part of this
               documentation and/or software.
             */

            /* MD5 context. */
            typedef struct {
                uint32 state[4];                                   /* state (ABCD) */
                uint32 count[2];        /* number of bits, modulo 2^64 (lsb first) */
                unsigned char buffer[64];                          /* input buffer */
            } MD5_CTX;

            void MD5Init (MD5_CTX *);
            void MD5Update (MD5_CTX *, const unsigned char *, unsigned int);
            void MD5Final (unsigned char [16], MD5_CTX *);
            void MD5Transform (uint32[4], const unsigned char [64]);
            void Encode (unsigned char *, const uint32 *, unsigned int);
            void Decode (uint32 *, const unsigned char *, unsigned int);
            void MD5_memcpy (unsigned char *, const unsigned char *, unsigned int);
            void MD5_memset (unsigned char *, int, unsigned int);

            static const char * PRINT_PATTERN;

        private:
            MD5_CTX ctx;
            unsigned char auchChecksum[MD5_LENGTH];
    };

    class MD5Utils
    {
        public:
            /*
             * Utility methods to get the MD5 of a Reader/BufferWriter's buffer/buffer.
             * All the methods allocated a new MD5 object, so these functions may not be
             * efficient if several subsequent calls have to be made.
             *
             * NOTE: the returned string mmust be deallcated by the caller.
             */
            static char * getMD5Checksum (Reader *pReader);
            static char * getMD5Checksum (BufferWriter *pWriter);
            static char * getMD5Checksum (const void *pBuf, uint32 ui32Len);
    };
}

#endif   // #ifndef INCL_MD5_H

