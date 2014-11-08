/*
 * CompressedWriter.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef COMPRESSED_WRITER_H
#define COMPRESSED_WRITER_H

#include "zlib.h"

#include "Writer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (WIN32)
    #include <windows.h>
#elif defined (UNIX)

#endif

namespace NOMADSUtil
{

    typedef const char * (*VOIDFP)(void);

    class CompressedWriter : public Writer
    {
        public:
            CompressedWriter (Writer *pw, bool bDeleteWhenDone = false, unsigned long ulOutBufSize = 32768);
            ~CompressedWriter (void);
            int flush (void);
            int writeBytes (const void *pBuf, unsigned long  ulCount);
        protected:
            static void * alloc_mem (void* userdata, uInt items, uInt size);
            static void free_mem (void *userdata, void* data);
        protected:
            Writer *_pWriter;
            bool _bDeleteWhenDone;
            char *_pOutputBuffer;
            unsigned long _ulOutBufSize;
            z_stream _zsCompStream;
            bool _bFlushed;
    };

}

#endif // COMPRESSED_WRITER_H
