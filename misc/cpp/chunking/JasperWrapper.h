/*
 * JasperWrapper.h
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

#ifndef INCL_JASPER_WRAPPER_H
#define INCL_JASPER_WRAPPER_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class BufferReader;
}

class JasperWrapper
{
    public:
        static NOMADSUtil::BufferReader * convertToBMP (const void *pInputBuf, uint32 ui32InputBufLen);
        static NOMADSUtil::BufferReader * convertToJPEG (const void *pInputBuf, uint32 ui32InputBufLen);
        static NOMADSUtil::BufferReader * convertToJPEG2000 (const void *pInputBuf, uint32 ui32InputBufLen);

    private:
        static void checkAndInitializeJasper (void);
        static NOMADSUtil::BufferReader * convertImage (const void *pInputBuf, uint32 ui32InputBufLen, int iTargetFormat);
        static bool _bJasperInitialized;
};

#endif   // #ifndef INCL_JASPER_WRAPPER_H
