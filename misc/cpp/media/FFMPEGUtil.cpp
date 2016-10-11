/*
 * FFMPEGUtil.cpp
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

#include "FFMPEGUtil.h"

extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavfilter/avfilter.h"
    #include "libavutil/error.h"
    #include "libavutil/opt.h"
};

#include  "Logger.h"

#include <stdarg.h>

namespace NOMADSUtil
{
    int toFFMPegLogLevel (Logger::Level level)
    {
        switch (level) {
            case Logger::L_SevereError: return AV_LOG_FATAL;
            case Logger::L_MildError: return AV_LOG_ERROR;
            case Logger::L_Warning: return AV_LOG_WARNING;
            case Logger::L_Info: return AV_LOG_INFO;
            case Logger::L_NetDetailDebug: return AV_LOG_VERBOSE;
            case Logger::L_LowDetailDebug:
            case Logger::L_MediumDetailDebug: return AV_LOG_DEBUG;
            case Logger::L_HighDetailDebug: return AV_LOG_TRACE;
            default: return AV_LOG_WARNING;
        }
    }

    Logger::Level toLogLevel (int iLogLevel)
    {
        switch (iLogLevel) {
            case AV_LOG_FATAL: return Logger::L_SevereError;
            case AV_LOG_ERROR: return  Logger::L_MildError;
            case AV_LOG_WARNING: return Logger::L_Warning;
            case AV_LOG_INFO: return Logger::L_Info;
            case AV_LOG_VERBOSE: return Logger::L_NetDetailDebug;
            case AV_LOG_DEBUG: return Logger::L_LowDetailDebug;
            case AV_LOG_TRACE: return Logger::L_HighDetailDebug;
            default: return Logger::L_Warning;
        }
    }

    /*
    void logCback (void *avcl, int level, const char *fmt, va_list vl)
    {
        if (pLogger == NULL) {
            return;
        }
        char buf[1024];
        va_start (vl, fmt);
        vsnprintf (buf, 1024, fmt, vl);
        va_end (vl);
        pLogger->logMsg ("FFMPEG", toLogLevel (level), buf);
    }
    */

    bool supportsCodec (const String &codecName, bool bCheckDecoder, bool bCheckEncoder)
    {
        bool bIsDec = false;
        bool bIsEnc = false;
        AVCodec *pCurrCodec = NULL;
        while ((pCurrCodec = av_codec_next (pCurrCodec)) != NULL) {
            if (codecName != pCurrCodec->name) {
                continue;
            }
            if (av_codec_is_decoder (pCurrCodec)) {
                bIsDec = true;
            }
            if (av_codec_is_encoder (pCurrCodec)) {
                bIsEnc = true;
            }
        }
        return (!bCheckDecoder || bIsDec) && (!bCheckEncoder || bIsEnc);
    }
}

using namespace NOMADSUtil;

const unsigned int FFMPEGUtil::VIFO_INTERLACED = (1 << 0);

void FFMPEGUtil::log (const char *pszSource, unsigned char uchLevel, int errnum)
{
    if ((pLogger == NULL) || (pszSource == NULL)) {
        return;
    }
    char buf[1024];
    pLogger->logMsg (pszSource, uchLevel, "%s\n", av_make_error_string (buf, 1024, errnum));
}

void FFMPEGUtil::logCodecList (void)
{
    static String codecs;
    if (codecs.length () <= 0) {
        AVCodec *pCurrCodec = NULL;
        while ((pCurrCodec = av_codec_next (pCurrCodec)) != NULL) {
            if (av_codec_is_encoder (pCurrCodec)) {
                if (codecs.length() > 0) {
                    codecs += ", ";
                }
                codecs += pCurrCodec->name;
            }
        }
    }
    pLogger->logMsg ("FFMPEGUtil::logCodecList", Logger::L_Info,  "encoder list: %s.\n", codecs.c_str());
}

void FFMPEGUtil::setup (void)
{
    static bool bRegistered = false;
    if (!bRegistered) {
        av_register_all();
        avfilter_register_all();
        // av_log_set_callback (logCback);
        if (pLogger == NULL) {
            av_log_set_level (AV_LOG_ERROR);
        }
        /*else {
            av_log_set_level (toFFMPegLogLevel (static_cast<Logger::Level>(pLogger->getDebugLevel())));
        }*/
        bRegistered = true;
    }
}

int FFMPEGUtil::setVideoCodeContext (const VideoFormat *pVideoFormat, const AVCodec *pVideoCodec,
                                     const unsigned int uiVideoBitRate, const bool bHasGlobalHeader,
                                     AVCodecContext *pVideoCodecCtx)
{
    if ((pVideoFormat == NULL) || (pVideoCodecCtx == NULL) || (pVideoCodec == NULL)) {
        return -1;
    }
    pVideoCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pVideoCodecCtx->width = pVideoFormat->width;
    pVideoCodecCtx->height = pVideoFormat->height;
    pVideoCodecCtx->sample_aspect_ratio.den = pVideoFormat->sample_aspect_den;
    pVideoCodecCtx->sample_aspect_ratio.num = pVideoFormat->sample_aspect_num;
    pVideoCodecCtx->time_base.num = pVideoFormat->frame_rate_num;
    pVideoCodecCtx->time_base.den = pVideoFormat->frame_rate_den;
    pVideoCodecCtx->gop_size = (pVideoFormat->frame_rate_den / pVideoFormat->frame_rate_num) / 2;	// GOP size is framerate / 2
    pVideoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pVideoCodecCtx->bit_rate = uiVideoBitRate;
    pVideoCodecCtx->bit_rate_tolerance = uiVideoBitRate * av_q2d (pVideoCodecCtx->time_base);

    // Set color space
    if (pVideoFormat->colorspace == 601) {
        pVideoCodecCtx->colorspace = (576 % pVideoCodecCtx->height) ? AVCOL_SPC_SMPTE170M : AVCOL_SPC_BT470BG;
    }
    else if (pVideoFormat->colorspace == 709) {
        pVideoCodecCtx->colorspace = AVCOL_SPC_BT709;
    }

    // Enable interlacing if needed
    if (pVideoFormat->flags & VIFO_INTERLACED) {
        pVideoCodecCtx->flags |= CODEC_FLAG_INTERLACED_DCT;
    }

    // Enable multithreaded encoding: breaks FLV!
    //_VideoCodecCtx->thread_count = 4;

    // Video format-specific hacks
    switch (pVideoCodec->id) {
        case AV_CODEC_ID_H264:
            av_opt_set (pVideoCodecCtx->priv_data, "preset", "slow", 0);
            break;

        case AV_CODEC_ID_MPEG2VIDEO:
            pVideoCodecCtx->max_b_frames = 2;
            pVideoCodecCtx->bit_rate_tolerance = uiVideoBitRate * av_q2d (pVideoCodecCtx->time_base) * 2;
            break;

        case AV_CODEC_ID_MPEG1VIDEO:
            // Needed to avoid using macroblocks in which some coeffs overflow.
            pVideoCodecCtx->mb_decision = 2;
            break;

        default:
            break;
    }

    // If we have a global header for the format, no need to duplicate the codec info in each keyframe
    if (bHasGlobalHeader) {
        pVideoCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    return 0;
}

int FFMPEGUtil::setVideoFormat (const AVCodecContext *pVideoCodecCtx, const AVCodec *pVideoCodec,
                                const unsigned int uiVideoBitRate, const bool bHasGlobalHeader,
                                VideoFormat *pVideoFormat)
{
    if ((pVideoFormat == NULL) || (pVideoCodecCtx == NULL) || (pVideoCodec == NULL)) {
        return -1;
    }

    pVideoFormat->width = pVideoCodecCtx->width;
    pVideoFormat->height = pVideoCodecCtx->height;

    pVideoFormat->sample_aspect_den = pVideoCodecCtx->sample_aspect_ratio.den;
    pVideoFormat->sample_aspect_num = pVideoCodecCtx->sample_aspect_ratio.num;
    pVideoFormat->frame_rate_num = pVideoCodecCtx->time_base.num;
    pVideoFormat->frame_rate_den = pVideoCodecCtx->time_base.den;

    pVideoFormat->colorspace = pVideoCodecCtx->colorspace; // TODO: check this

    if (pVideoCodecCtx->flags & CODEC_FLAG_INTERLACED_DCT) {
        pVideoFormat->flags |= CODEC_FLAG_INTERLACED_DCT; // TODO: check this
    }

    return 0;
}

bool FFMPEGUtil::supportsDecoder (const String &codecName)
{
    return supportsCodec (codecName, true, false);
}

bool FFMPEGUtil::supportsEncoder (const String &codecName)
{
    return supportsCodec (codecName, false, true);
}

String FFMPEGUtil::getDefaultVideoCodec (void)
{
    logCodecList();
    return String ("h263p");
    //return String ("mpeg4");
}

//-----------------------------------------------------------------------------
// FFMPEGUtil::VideoFormat::VideoFormat
//-----------------------------------------------------------------------------

FFMPEGUtil::VideoFormat::VideoFormat (void)
{
}

FFMPEGUtil::VideoFormat::VideoFormat (unsigned int uiWidth, unsigned int uiHeight,
                                      unsigned int uiFrame_rate_num, unsigned int uiFrame_rate_den,
                                      unsigned int uiSample_aspect_num, unsigned int uiSample_aspect_den,
                                      unsigned int uiDisplay_aspect_num, unsigned int uiDisplay_aspect_den,
                                      unsigned int uiFlags, unsigned int uiColorspace, int64 i64Duration,
                                      int64 iNFrames)
    : width (uiWidth), height (uiHeight),
      frame_rate_num (uiFrame_rate_num), frame_rate_den (uiFrame_rate_den),
      sample_aspect_num (uiSample_aspect_num), sample_aspect_den (uiSample_aspect_den),
      display_aspect_num (uiDisplay_aspect_num), display_aspect_den (uiDisplay_aspect_den),
      flags (uiFlags), colorspace (uiColorspace), duration (i64Duration), nFrames (iNFrames)
{
}

FFMPEGUtil::VideoFormat::VideoFormat (const FFMPEGUtil::VideoFormat &rhsFormat)
    : width (rhsFormat.width), height (rhsFormat.height),
      frame_rate_num (rhsFormat.frame_rate_num), frame_rate_den (rhsFormat.frame_rate_den),
      sample_aspect_num (rhsFormat.sample_aspect_num), sample_aspect_den (rhsFormat.sample_aspect_den),
      display_aspect_num (rhsFormat.display_aspect_num), display_aspect_den (rhsFormat.display_aspect_den),
      flags (rhsFormat.flags), colorspace (rhsFormat.colorspace), duration (rhsFormat.duration),
      nFrames (rhsFormat.nFrames)
{
}

FFMPEGUtil::VideoFormat::VideoFormat (const AVCodecContext &rhsFormat, int64 i64Duration, int64 iNFrames)
    : width (rhsFormat.width), height (rhsFormat.height),
      frame_rate_num (rhsFormat.time_base.num), frame_rate_den (rhsFormat.time_base.den),
      sample_aspect_num (rhsFormat.sample_aspect_ratio.num), sample_aspect_den (rhsFormat.sample_aspect_ratio.den),
      display_aspect_num (0U), display_aspect_den (0U),
      flags (rhsFormat.flags), colorspace (rhsFormat.colorspace), duration (i64Duration), nFrames (iNFrames)
{
}

FFMPEGUtil::VideoFormat::~VideoFormat (void)
{
}

int64 FFMPEGUtil::VideoFormat::getDurationInMillis (void) const
{
    const double time_base = static_cast<double>(frame_rate_num) / static_cast<double>(frame_rate_den);
    const double actualDuration = static_cast<double>(duration) * time_base * 1000.0;
    return static_cast<int64>(floor (actualDuration));
}

int64 FFMPEGUtil::VideoFormat::getNumberOfFrames (void) const
{
    return nFrames;
}

bool FFMPEGUtil::VideoFormat::interlaced (void) const
{
    return (flags & VIFO_INTERLACED);
}

//-----------------------------------------------------------------------------
// FFMPEGUtil::VideoEncodingProfile::VideoEncodingProfile
//-----------------------------------------------------------------------------

FFMPEGUtil::VideoEncodingProfile::VideoEncodingProfile (void)
{
}

FFMPEGUtil::VideoEncodingProfile::VideoEncodingProfile (const VideoEncodingProfile &hrsProfile)
    : _type (hrsProfile._type),
      _name (hrsProfile._name),
      _videoContainer (hrsProfile._videoContainer),
      _videoCodec (hrsProfile._videoCodec),
      _audioCodec (hrsProfile._audioCodec),
      _sampleRate (hrsProfile._sampleRate),
      _channels (hrsProfile._channels)
{
}

FFMPEGUtil::VideoEncodingProfile::~VideoEncodingProfile (void)
{
}

