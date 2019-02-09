/*
 * Chunks.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2016 IHMC.
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

#include "Chunks.h"

#include "Writer.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

Chunks::Chunks (bool bDeallocatedBuffers)
    : DArray2<BufferReader>(),
      _bDeallocatedBuffers (bDeallocatedBuffers)
{
}

Chunks::Chunks (bool bDeallocatedBuffers, unsigned int uiSize)
    : DArray2<BufferReader> (uiSize),
      _bDeallocatedBuffers (bDeallocatedBuffers)
{
}

Chunks::Chunks (bool bDeallocatedBuffers, DArray2<BufferReader> &SourceArray)
    : DArray2<BufferReader> (SourceArray),
      _bDeallocatedBuffers (bDeallocatedBuffers)
{
}

int Chunks::read (Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }
    uint8 ui8ChunkId;
    do {
        ui8ChunkId = 0;
        if (pReader->read8 (&ui8ChunkId) < 0) {
            return -2;
        }
        if (ui8ChunkId > 0) {
            uint32 ui32BufLen = 0U;
            if (pReader->read32 (&ui32BufLen) < 0) {
                return -2;
            }
            if (ui32BufLen == 0U) {
                return -2;
            }
            void *pBuf = malloc (ui32BufLen);
            if (pBuf == NULL) {
                return -2;
            }
            if (pReader->readBytes (pBuf, ui32BufLen) < 0) {
                return -2;
            }
            operator[](ui8ChunkId).init (pBuf, ui32BufLen, _bDeallocatedBuffers);
        }
    } while (ui8ChunkId > 0);
    return 0;
}

int Chunks::write (Writer *pWriter)
{
    if (pWriter == NULL) {
        return -1;
    }
    for (unsigned int i = 0; i < size(); i++) {
        if (used (i)) {
            uint8 ui8ChunkId = i;
            if (pWriter->write8 (&ui8ChunkId) < 0) {
                return -2;
            }
            uint32 ui32BufLen = operator[](i).getBufferLength();
            if (pWriter->write32 (&ui32BufLen) < 0) {
                return -2;
            }
            const void *pBuf = operator[](i).getBuffer();
            if (pWriter->writeBytes (pBuf, ui32BufLen) < 0) {
                return -2;
            }
        }
    }
    uint8 ui8 = 0;
    if (pWriter->write8 (&ui8) < 0) {
        return -2;
    }
    return 0;
}

