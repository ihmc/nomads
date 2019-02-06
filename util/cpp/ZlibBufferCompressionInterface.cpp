/*
* ZlibBufferCompressionInterface.h
* Author: Roberto Fronteddu
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
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
#include"ZlibBufferCompressionInterface.h"

namespace NOMADSUtil
{
    int ZlibCompressionInterface::deflate (const char *pInflated, const uint32 ui32InflatedSize, char **ppDeflated)
    {
        ZlibCompressionInterface zli;
        BufferWriter bw;
        int rc = zli.writeCompressedBuffer (pInflated, ui32InflatedSize, &bw);
        if (rc < 0) {
            return rc;
        }
        memcpy (*ppDeflated, bw.getBuffer(), bw.getBufferLength());
        return bw.getBufferLength();
    }

    int ZlibCompressionInterface::inflate (
        const char *pDeflatedBuffer,
        const uint32 ui32DeflatedSize,
        char **ppInflatedBuffer)
    {
        ZlibCompressionInterface zli;
        // Owned by Buffer reader
        char * pDeflatedBufferCpy = new char[ui32DeflatedSize];
        memcpy (pDeflatedBufferCpy, pDeflatedBuffer, ui32DeflatedSize);
        BufferReader br (pDeflatedBufferCpy, ui32DeflatedSize, true);
        return zli.readCompressedBuffer (&br, ppInflatedBuffer);
    }

    int ZlibCompressionInterface::readCompressedBuffer (
        Reader  *pReader,
        char    **ppDestBuffer)
    {
        if (pReader == NULL) {
            return -1;
        }

        // Read buffer size from buffer
        int iDeflatedSize = 0;
        if (pReader->read32 (&iDeflatedSize) < 0) {
            return -2;
        }

        if (iDeflatedSize == 0) {
            return 0;
        }

        *ppDestBuffer = new char[iDeflatedSize];
        if (*ppDestBuffer == NULL) {
            printf ("new Failed, iDeflatedSize:%d\n", iDeflatedSize);
            return -3;
        }

        CompressedReader cr (pReader, false, false);
        if (cr.readBytes (*ppDestBuffer, iDeflatedSize) < 0) {
            delete[](*ppDestBuffer);
            printf("readBytes Failed, iDeflatedSize: %d\n", iDeflatedSize);
            return -4;
        }
        return iDeflatedSize;
    }

    int ZlibCompressionInterface::writeCompressedBuffer (
        const char *pb,
        const uint32 ui32BufferSize,
        Writer *pWriter)
    {
        if (pWriter == NULL) {
            return -1;
        }
        uint32 ui32InflatedSize = ui32BufferSize;
        // Write length of the uncompressed string
        if (pWriter->write32(&ui32InflatedSize) < 0) {
            return -2;
        }

        CompressedWriter cw(pWriter);
        if ((cw.writeBytes(pb, ui32BufferSize) < 0) || (cw.flush() < 0)) {
            return -3;
        }
        return 0;
    }

}