/*
 * MPEGElements.cpp
 *
 * This file is part of the IHMC Misc Media Library
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#include "MPEGElements.h"

#include "Logger.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_MISC;
using namespace NOMADSUtil;

SequenceInfo::SequenceInfo (int halfw, int halfh, int w, int h, int seqw, int seqh, int picSize, int remw, int remh,
                            double dAspectRatio, double dPicRate, double iBitRate, int iConstrained)
    : _halfw (halfw),
      _halfh (halfh),
      _w (w),
      _h (h),
      _seqw (seqw),
      _seqh (seqh),
      _picSize (picSize),
      _remw (remw),
      _remh (remh),
      _dAspectRatio (dAspectRatio),
      _dPicRate (dPicRate),
      _iBitRate (iBitRate),
      _iConstrained (iConstrained)
{
}

SequenceInfo::~SequenceInfo (void)
{
}

void SequenceInfo::log (void)
{
    const char *pszMethodName = "SequenceInfo::log";
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "width: %d\theight: %d\tseq width: %d\tseq height: %d\n",
                    _w, _h, _seqw, _seqh);
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "aspect ratio: %f\tpicture rate: %f fps\tbit rate: %d bps"
                    "\tconstrained: %d\n", _dAspectRatio, _dPicRate, _iBitRate, _iConstrained);
}

//-----------------------------------------------------------------------------

RawFrame::RawFrame (void)
    : _type (UNKNOWN)
{
}

RawFrame::~RawFrame (void)
{
}

FrameType RawFrame::getType (void)
{
    return _type;
}

const char * IHMC_MISC::toString (const FrameType type)
{
    switch (type) {
        case I: return "I_FRAME";
        case P: return "P_FRAME";
        case B: return "B_FRAME";
        case D: return "D_FRAME";
        case UNKNOWN:
        default: return "UNKOWM_FRAME";
    }
}


