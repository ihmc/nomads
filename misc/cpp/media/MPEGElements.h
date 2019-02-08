/*
 * MPEGElements.h
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
 */

#ifndef INCL_MPEG_ELEMENTS_H
#define	INCL_MPEG_ELEMENTS_H

namespace NOMADSUtil
{
    class RGBImage;
}

namespace IHMC_MISC
{
    enum FrameType {
        I = 0x00,
        P = 0x01,
        B = 0x02,
        D = 0x03,

        UNKNOWN = 0xFF
    };

    const char * toString (const FrameType type);

    struct SequenceInfo
    {
        SequenceInfo (int halfw, int halfh, int w, int h, int seqv, int seqh, int picSize, int remw, int remh,
                      double dAspectRatio, double dPicRate, double iBitRate, int iConstrained);
        ~SequenceInfo (void);

        void log (void);

        const int _halfw;
        const int _halfh;
        const int _w;
        const int _h;
        const int _seqw;
        const int _seqh;
        const int _picSize;
        const int _remw;
        const int _remh;
        const double _dAspectRatio;
        const double _dPicRate;
        const int _iBitRate;
        const int _iConstrained;
    };

    class RawFrame
    {
        public:
            RawFrame (void);
            virtual ~RawFrame (void);

            virtual NOMADSUtil::RGBImage * toRGBImage (void) = 0;

            FrameType getType (void);

        protected:
            FrameType _type;
    };

    class GroupOfFrames
    {
        public:
            GroupOfFrames (void) {};
            virtual ~GroupOfFrames (void) {};

            virtual RawFrame * getFrame (int iFrameIdx) = 0;

        private:
    };

    class Sequence
    {
        Sequence (SequenceInfo *pInfo);
        virtual ~Sequence (void);

        virtual GroupOfFrames * getGroupOfFrames (void) = 0;
    };
}

#endif    /* INCL_MPEG_ELEMENTS_H */


