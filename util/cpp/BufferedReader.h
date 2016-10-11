/*
 * BufferedReader.h
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

#ifndef INCL_BUFFERED_READER_H
#define INCL_BUFFERED_READER_H

#include "Reader.h"

namespace NOMADSUtil
{
    class BufferedReader : public Reader
    {
        public:
            BufferedReader (Reader *pr, bool bDeleteWhenDone = false, uint32 ui32BufSize = 8192);
            ~BufferedReader (void);
            int read (void *pBuf, int iCount);
            int readBytes (void *pBuf, uint32 ui32Count);

            // Returns the number of bytes available by taking into account the number of
            // bytes in the buffer plus the number of bytes available in the nested reader
            uint32 getBytesAvailable (void);

            Reader * getNestedReader (void);

        protected:
            Reader *_pReader;
            bool _bDeleteReaderWhenDone;
            char *_pBuf;
            uint32 _ui32BufSize;
            uint32 _ui32BufStart;
            uint32 _ui32BufEnd;
    };

    inline Reader * BufferedReader::getNestedReader (void)
    {
        return _pReader;
    }
}

#endif   // #ifndef INCL_BUFFERED_READER_H
