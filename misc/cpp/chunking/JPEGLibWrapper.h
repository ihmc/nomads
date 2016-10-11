/*
 * JPEGLibWrapper.h
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

#ifndef INCL_JPEGLIB_WRAPPER_H
#define INCL_JPEGLIB_WRAPPER_H

#include "FTypes.h"

#include <stdio.h>

namespace NOMADSUtil
{
    class BMPImage;
    class BufferReader;
}

class JPEGLibWrapper
{
    public:
        static NOMADSUtil::BMPImage * convertJPEGToBMP (const void *pInputBuf, uint32 ui32InputBufLen);
        static NOMADSUtil::BMPImage * convertJPEGToBMP (const char *pszFile);

        // Compresses the specified BMP into a JPEG image using the specified compression quality
        // NOTE: The compression quality must range between 0 and 100
        static NOMADSUtil::BufferReader * convertBMPToJPEG (const NOMADSUtil::BMPImage *pInputImage, uint8 ui8CompressionQuality);

    private:
        // Internal function that handles either a memory buffer or an input file
        static NOMADSUtil::BMPImage * convertJPEGToBMP (const void *pInputBuf, uint32 ui32InputBufLen, FILE *fileInput);
};

#endif   // #ifndef INCL_JPEGLIB_WRAPPER_H
