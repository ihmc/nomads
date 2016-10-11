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
 * Created on May 28, 2015, 12:27 AM
 */

#ifndef INCL_FFMPEG_READER_H
#define	INCL_FFMPEG_READER_H

#include "FTypes.h"
#include "FFMPEGUtil.h"

namespace NOMADSUtil
{
    class FFMPEGReaderImpl;
    class RGBImage;

    class FFMPEGReader
    {
        public:
            FFMPEGReader (void);
            virtual ~FFMPEGReader (void);

            void close (void);
            int read (const void *pBuffer, unsigned int uiLen);
            int openFile (const char *pszFileName, unsigned int seekto = 0);
            const char * errorMsg (void) const;

            RGBImage * getFrame (int iFrameIdx);
            RGBImage * getFrameAtTime (int64 i64TimeInMilliseconds);

            int getFrameIndexByTime (int64 i64TimeInMilliseconds);
            int64 getFrameTimeInMillis (int iFrameIdx);

            const FFMPEGUtil::VideoEncodingProfile * getVideoEncProfile (void);
            const FFMPEGUtil::VideoFormat * getVideoFormat (void);

        private:
            FFMPEGReaderImpl *_pImpl;
            String _tmpfilename;
    };
}

#endif	/* INCL_FFMPEG_READER_H */

