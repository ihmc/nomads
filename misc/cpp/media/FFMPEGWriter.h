/*
 * FFMPEGReader.h
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
 * Author: Giacomo Benincasa	(gbenincasa@ihmc.us)
 * Created on May 28, 2015, 12:30 AM
*/

#ifndef INCL_FFMPEG_WRITER_H
#define INCL_FFMPEG_WRITER_H

#include "FFMPEGUtil.h"
#include "StrClass.h"
#include "RGBImage.h"

namespace NOMADSUtil
{
    class FFMPEGWriterImpl;

    class FFMPEGWriter
    {
        public:
            FFMPEGWriter (void);
            virtual ~FFMPEGWriter (void);

            int createFile (const String& filename,
                            const FFMPEGUtil::VideoEncodingProfile *pProfile,
                            const FFMPEGUtil::VideoFormat *pVideoformat,
                            unsigned int uiQuality, bool bConvertAudio,
                            FFMPEGUtil::Audio *pAudio);

            bool close (void);
            int encodeImage (const RGBImage &img, int64 time);

        private:
            FFMPEGWriterImpl * _pImpl;
    };
}

#endif // INCL_FFMPEG_WRITER_H

