/*
 * CompressedReader.h
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

#ifndef COMPRESSED_READER_H
#define COMPRESSED_READER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Reader.h"
#include "zlib.h"

namespace NOMADSUtil
{

    class CompressedReader : public Reader
    {
        public:
            // Initializes the CompressedReader object
            // If bDeleteWhenDone is true, the reader that is passed into the constructor will be
            //     deleted when this object is deleted
            // If bNoWrapMode is true, the zlib library will not use a header or a checksum. This
            //     mode should be used for compatibility with .zip files
            // NOTE: If bNoWrapMode is set to true, then the data that is passed in must be padded with
            //       an extra byte in order to prevent the zlib library from complaining with a Z_BUF_ERROR
            CompressedReader (Reader *pr, bool bDeleteWhenDone, bool bNoWrapMode, unsigned long ulInBufSize = 32768);

            ~CompressedReader (void);

            int read (void *pBuf, int iCount);

            int readBytes (void *pBuf, uint32 ui32Count);

            Reader * getNestedReader (void);

        protected:
            static void * alloc_mem (void *userdata, uInt items, uInt size);
            static void free_mem (void *userdata, void *data);

        protected:
            Reader *_pReader;
            bool _bDeleteWhenDone;
            char *_pInputBuffer;
            unsigned long _ulInBufSize;
            z_stream _zsDecompStream;
    };

    inline Reader * CompressedReader::getNestedReader (void)
    {
        return _pReader;
    }

}

#endif // COMPRESSED_READER_H
