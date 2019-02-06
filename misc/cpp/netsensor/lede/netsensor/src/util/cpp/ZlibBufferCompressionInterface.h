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

#ifndef INCL_ZLIB_COMPRESSION_INTERFACE_H
#define INCL_ZLIB_COMPRESSION_INTERFACE_H

#include "BufferReader.h"
#include "BufferWriter.h"
#include "zlib.h"
#include "FTypes.h"
#include "StrClass.h"
#include "Writer.h"
#include "Reader.h"
#include "CompressedReader.h"
#include "CompressedWriter.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


namespace NOMADSUtil
{
    class ZlibCompressionInterface
    {
    public:
        ZlibCompressionInterface();
        ~ZlibCompressionInterface();

        // returns size of pDeflated/Inflated
        uint32 deflate(const char *pInflated, const uint32 ui32InflatedSize, char **ppDeflated);
        uint32 inflate(const char *pDeflated, const uint32 ui32DeflatedSize, char **ppInflated);

        // Returns size
        uint32 readCompressedBuffer(
            Reader *pReader,
            char **pDestBuffer);

        // returns 0 if successfull
        int writeCompressedBuffer(
            const char *pb,
            const uint32 ui32BufferSize,
            Writer *pWriter);

    private:
        static const int GZIP_ENCODING = 16;
        static const int CHUNK = 0x4096;
    };

}
#endif
