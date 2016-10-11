/*
 * CompressedWriter.h
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

#ifndef COMPRESSED_WRITER_H
#define COMPRESSED_WRITER_H

#include "zlib.h"

#include "Writer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (WIN32)
	#include <winsock2.h>
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
