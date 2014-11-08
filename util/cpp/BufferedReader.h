/*
 * BufferedReader.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
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
