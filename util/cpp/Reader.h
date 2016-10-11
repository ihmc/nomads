/*
 * Reader.h
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

#ifndef INCL_READER_H
#define INCL_READER_H

#include "FTypes.h"

#include <stddef.h>

namespace NOMADSUtil
{

    class Reader
    {
        public:
            Reader (void);
            virtual ~Reader (void);

            // Read a bool value
            int readBool (bool *pBuf);

            // Read an 8-bit value
            int read8 (void *pBuf);

            // Read a 16-bit value (byte-swap if on a little-endian system)
            virtual int read16 (void *pBuf);

            // Read a 16-bit value (byte-swap if on a big-endian system)
            virtual int readLE16 (void *pBuf);

            // Read a 32-bit value (byte-swap if on a little-endian system)
            virtual int read32 (void *pBuf);

            // Read a 32-bit value (byte-swap if on a big-endian system)
            virtual int readLE32 (void *pBuf);

            // Read a 64-bit value (byte-swap if on a little-endian system)
            virtual int read64 (void *pBuf);

            // Read a 64-bit value (byte-swap if on a big-endian system)
            virtual int readLE64 (void *pBuf);

            // Read upto iCount bytes; returns number of bytes read or
            //     a negative number in case of error
            virtual int read (void *pBuf, int iCount) = 0;

            // Read ulCount bytes repeating calls to read if necessary
            // Returns 0 if successful or a negative number in case of error
            virtual int readBytes (void *pBuf, uint32 ui32Count);

            // Read a float value
            // NOTE: The data is converted to big-endian format if this is a little-endian machine
            int readFloat (void *pBuf);
            virtual int readUI8 (void *pBuf);
            virtual int readUI16 (void *pBuf);
            virtual int readUI32 (void *pBuf);
            virtual int readUI64 (void *pBuf);
            virtual int readString (char **pBuf);

            // Returns the number of bytes available
            virtual uint32 getBytesAvailable (void);

            // Returns the total bytes read so far
            virtual uint32 getTotalBytesRead (void);

            // Return the nested reader (if any)
            virtual Reader * getNestedReader (void);

            // Close the underlying 'stream'
            virtual int close (void);

            static void byteSwap16 (void *pBuf);
            static void byteSwap32 (void *pBuf);
            static void byteSwap64 (void *pBuf);

        protected:
            uint32 _ui32TotalBytesRead;
    };

    inline Reader::Reader (void)
    {
        _ui32TotalBytesRead = 0;
    }

    inline Reader::~Reader (void)
    {
    }

    inline int Reader::readFloat (void *pBuf)
    {
        // Assume a float is 4 bytes. It may be architecture-dependent
        return read32 (pBuf);
    }

    inline uint32 Reader::getBytesAvailable (void)
    {
        return 0;
    }

    inline uint32 Reader::getTotalBytesRead (void)
    {
        return _ui32TotalBytesRead;
    }

    inline Reader * Reader::getNestedReader (void)
    {
        return NULL;
    }

}

#endif   // #ifndef INCL_READER_H
