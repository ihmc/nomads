/*
 * PNGLibWrapper.h
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2014 IHMC.
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

#ifndef INCL_PNG_LIB_WRAPPER_H
#define INCL_PNG_LIB_WRAPPER_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class BMPImage;
    class BufferReader;
}

class PNGLibWrapper
{
    public:
        static NOMADSUtil::BMPImage * convertPNGToBMP (const void *pInputBuf, uint32 ui32InputBufLen);
        static NOMADSUtil::BufferReader * convertBMPtoPNG (const NOMADSUtil::BMPImage *pInputImage);
};

#endif   // #ifndef INCL_PNG_LIB_WRAPPER_H
