/*
 * BufferReader.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_BUFFER_READER_H
#define INCL_BUFFER_READER_H

#include <stdio.h>
#include "Reader.h"

namespace NOMADSUtil
{
    class BufferReader : public Reader
    {
        public:
            BufferReader (const void *pBuf, uint32 ui32BufLen, bool bDeleteWhenDone = false);
            ~BufferReader (void);

            // Returns a pointer to the internal buffer
            // NOTE: The buffer returned by this method will be deallocated in the destructor
            const uint8 * getBuffer (void);

            // Returns the length of the buffer (i.e., the number of bytes that have been
            // written into the buffer)
            uint32 getBufferLength (void);

            int read (void *pBuf, int iCount);
            int readBytes (void *pBuf, uint32 ui32Count);
            int skipBytes (uint32 ui32Count);
            uint32 getBytesAvailable (void);
            int setPosition (uint32);

        protected:
            const uint8 *_pui8Buf;
            uint32 _ui32BufLen;
            uint32 _ui32NextPtr;
            bool _bDeleteWhenDone;
    };

    inline const uint8 * BufferReader::getBuffer (void)
    {
        return _pui8Buf;
    }

    inline uint32 BufferReader::getBufferLength (void)
    {
        return _ui32BufLen;
    }
}

#endif   // #ifndef INCL_BUFFER_READER_H
