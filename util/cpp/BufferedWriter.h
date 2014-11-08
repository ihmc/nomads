/*
 * BufferedWriter.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_BUFFERED_WRITER_H
#define INCL_BUFFERED_WRITER_H

#include "Writer.h"

namespace NOMADSUtil
{
    class BufferedWriter : public Writer
    {
        public:
            BufferedWriter (Writer *pw, unsigned long ulBufSize = 8192);
            ~BufferedWriter (void);
            int flush (void);
            int writeBytes (const void *pBuf, unsigned long ulCount);
        protected:
            Writer *_pWriter;
            char *_pBuf;
            unsigned long _ulBufSize;
            unsigned long _ulBufCount;
    };
}

#endif   // #ifndef INCL_BUFFERED_WRITER_H
