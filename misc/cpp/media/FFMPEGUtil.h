/*
 * FFMPEGUtil.h
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
 */

#ifndef INCL_FFMPEG_UTIL_H
#define INCL_FFMPEG_UTIL_H

#include  "DArray2.h"
#include  "StrClass.h"

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;

namespace NOMADSUtil
{
    class FFMPEGUtil
    {
        public:
            static const unsigned int VIFO_INTERLACED;

            struct Audio
            {
                int reset (void) { return 0; } // TODO: implement this
                int audioStream;
                AVFormatContext *pFormatCtx;
                AVCodecContext  *aCodecCtx;
                AVCodec *pCodec;
            };

            struct VideoFormat
            {
                // const char * name;
                VideoFormat (void);
                VideoFormat (unsigned int uiWidth, unsigned int uiHeight,
                             unsigned int uiFrame_rate_num, unsigned int uiFrame_rate_den,
                             unsigned int uiSample_aspect_num, unsigned int uiSample_aspect_den,
                             unsigned int uiDisplay_aspect_num, unsigned int uiDisplay_aspect_den,
                             unsigned int uiFlags, unsigned int uiColorspace, int64 i64Duration,
                             int64 iNFrames);
                VideoFormat (const FFMPEGUtil::VideoFormat &rhsFormat);
                VideoFormat (const AVCodecContext &videoCodecCtx, int64 i64Duration, int64 iNFrames);
                ~VideoFormat (void);

                int64 getDurationInMillis (void) const;
                int64 getNumberOfFrames (void) const;
                bool interlaced (void) const;

                unsigned int width;
                unsigned int height;
                unsigned int frame_rate_num;
                unsigned int frame_rate_den;
                unsigned int sample_aspect_num;
                unsigned int sample_aspect_den;
                unsigned int display_aspect_num;
                unsigned int display_aspect_den;
                unsigned int flags;
                unsigned int colorspace;
                int64 duration;
                int64 nFrames;
            };

            class VideoEncodingProfile
            {
                public:
                    enum {
                        BITRATE_LOW,
                        BITRATE_MEDIUM,
                        BITRATE_HIGH
                    };

                    enum Type {
                        TYPE_FILE,
                        TYPE_DEVICE,
                        TYPE_BLURAY,
                        TYPE_DVD,
                        TYPE_WEB
                    };

                    VideoEncodingProfile (void);
                    VideoEncodingProfile (const VideoEncodingProfile &hrsProfile);
                    ~VideoEncodingProfile (void);

                    Type _type;
                    String _name;

                    // Video params
                    String _videoContainer;	// i.e. avi, mp4, flv
                    String _videoContainerExtension;
                    String _videoCodec;		// i.e. h264, libtheora
                    DArray2<String> limitFormats;	// limited to specific video encoding params

                    // Audio params
                    String _audioCodec;		 // i.e. ac3, mp3
                    unsigned int _sampleRate; // 44100
                    unsigned int _channels;	 // 1 or 2

                    // Bitrates
                    bool _bitratesEnabled[3];
                    unsigned int _bitratesVideo[3]; // in KILOBYTES
                    unsigned int _bitratesAudio[3]; // in KILOBYTES
            };

            static void log (const char *pszSource, unsigned char uchLevel, int errnum);
            static void logCodecList (void);

            // Register all codecs _once_, and set ups logging level
            static void setup (void);

            static int setVideoCodeContext (const VideoFormat *pVideoFormat, const AVCodec *pVideoCodec,
                                            const unsigned int uiVideoBitRate, const bool bHasGlobalHeader,
                                            AVCodecContext *pVideoCodecCtx);
            static int setVideoFormat (const AVCodecContext *pVideoCodecCtx, const AVCodec *pVideoCodec,
                                       const unsigned int uiVideoBitRate, const bool bHasGlobalHeader,
                                       VideoFormat *pVideoFormat);

            static bool supportsDecoder (const String &codecName);
            static bool supportsEncoder (const String &codecName);
            static String getDefaultVideoCodec (void);
    };
}

#endif  // INCL_FFMPEG_UTIL_H

