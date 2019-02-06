/*
 * BufferReader.h
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

#ifndef INCL_BUFFER_READER_H
#define INCL_BUFFER_READER_H

#include <stdio.h>
#include "Reader.h"

namespace NOMADSUtil
{
    class BufferReader : public Reader
    {
        public:
            BufferReader (const void *pBuf = NULL, uint32 ui32BufLen = 0U, bool bDeleteWhenDone = false);
            virtual ~BufferReader (void);

            int init (const void *pBuf, uint32 ui32BufLen, bool bDeleteWhenDone = false);

            // Returns a pointer to the internal buffer
            // NOTE: The buffer returned by this method will be deallocated in the destructor
            const uint8 * getBuffer (void) const;

            // Returns the length of the buffer (i.e., the number of bytes that have been
            // written into the buffer)
            uint32 getBufferLength (void) const;

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

    inline const uint8 * BufferReader::getBuffer (void) const
    {
        return _pui8Buf;
    }

    inline uint32 BufferReader::getBufferLength (void) const
    {
        return _ui32BufLen;
    }
}

#endif   // #ifndef INCL_BUFFER_READER_H
