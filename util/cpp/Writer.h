/*
 * Writer.h
 *
 * This file is part of the IHMC Util Library
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

#ifndef INCL_WRITER_H
#define INCL_WRITER_H

#include "Reader.h"

namespace NOMADSUtil
{

    class Writer
    {
        public:
            Writer (void);
            virtual ~Writer (void);

            // Write a bool value
            int writeBool (const bool *pbVal);

            // Write an 8 byte value
            int write8 (const void *pBuf);

            // Write a 16 bit (2 byte) value
            // NOTE: The data is converted to big-endian format if this is a little-endian machine
            virtual int write16 (void *pBuf);

            // Write a 16 bit (2 byte) value
            // NOTE: The data is converted to little-endian format if this is a big-endian machine
            virtual int writeLE16 (void *pBuf);

            // Write a 32 bit (4 byte) value
            // NOTE: The data is converted to big-endian format if this is a little-endian machine
            virtual int write32 (void *pBuf);

            // Write a 32 bit (4 byte) value
            // NOTE: The data is converted to little-endian format if this is a big-endian machine
            virtual int writeLE32 (void *pBuf);

            // Write a 64 bit (8 byte) value
            // NOTE: The data is converted to big-endian format if this is a little-endian machine
            virtual int write64 (void *pBuf);
            
            // Write a 64 bit (8 byte) value
            // NOTE: The data is converted to little-endian format if this is a big-endian machine
            virtual int writeLE64 (void *pBuf);

            // Returns 0 if successful or -1 in case of an error
            virtual int writeBytes (const void *pBuf, unsigned long ulCount) = 0;

            // Flush any buffered data
            virtual int flush (void);

            // Close the underlying 'stream'
            virtual int close();

            static void byteSwap16 (void *pBuf);
            static void byteSwap32 (void *pBuf);
            static void byteSwap64 (void *pBuf);
    };

    inline Writer::Writer (void)
    {
    }

    inline Writer::~Writer (void)
    {
    }

    inline int Writer::flush (void)
    {
        return 0;
    }

    inline void Writer::byteSwap16 (void *pBuf)
    {
        Reader::byteSwap16 (pBuf);
    }

    inline void Writer::byteSwap32 (void *pBuf)
    {
        Reader::byteSwap32 (pBuf);
    }

    inline void Writer::byteSwap64 (void *pBuf)
    {
        Reader::byteSwap64 (pBuf);
    }

}

#endif   // #ifndef INCL_WRITER_H
