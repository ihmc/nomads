/*
 * FFMPEGReader.cpp
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

extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libswresample/swresample.h"
    #include "libavutil/opt.h"
};

#include "FFMPEGWriter.h"

#include "FTypes.h"
#include "Logger.h"
#include "NLFLib.h"
#include "RGBImage.h"

#include <limits.h>

// #define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#define checkAndLogMsg if (pLogger) pLogger->logMsg
#define RNDTO2(X) ( (X) & 0xFFFFFFFE )
#define RNDTO32(X) ( ( (X) % 32 ) ? ( ( (X) + 32 ) & 0xFFFFFFE0 ) : (X) )

namespace NOMADSUtil
{
    class FFMPEGWriterImpl
    {
        public:
            FFMPEGWriterImpl (void);
            ~FFMPEGWriterImpl (void);

            bool createFile (const String &filename);
            bool close (void);
            int encodeAudio (void);
            int encodeImage (const RGBImage &img, int64 time);
            void flush (void);

        public:
            // Video output parameters
            const FFMPEGUtil::VideoEncodingProfile *_pVideoProfile;
            const FFMPEGUtil::VideoFormat *_pVideoFormat;
            bool _bConvertAudio;
            unsigned int _uiVideoBitRate;
            unsigned int _uiAudioBitRate;

            // Do we also have an audio source?
            FFMPEGUtil::Audio *pAudioSrc;

            // Error message
            String sErrMsg;

        private:
            bool addAudioStreamToFile (void);

            /**
             * Convert RGBImage to the internal YUV format
             */
            bool convertImage_sws (const RGBImage &img);
            bool resampleAndAudioStreamToFile (void);

        private:
            // FFmpeg stuff
            AVFormatContext *_pOutputFormatCtx;
            AVOutputFormat *_pOutputFormat;
            AVCodecContext *_pVideoCodecCtx;
            AVCodecContext *_pAudioCodecCtx;
            AVStream *_pVideoStream;
            AVStream *_pAudioStream;
            AVCodec *_pVideoCodec;
            AVCodec *_pAudioCodec;

            // Video frame data
            AVFrame *_pVideoFrame;
            uint8_t *_pVideoImageBuffer;

            // Audio frame data
            AVFrame *_pAudioFrame;
            uint8_t *_pAudioSampleBuffer;
            unsigned int _uiAudioSampleBufLen;

            // Conversion
            SwsContext *_pVideoConvertCtx;

            // FIXME: Audio resample
            SwrContext *_pAudioResampleCtx;

            // File has been _bOutputFileOpened successfully
            bool _bOutputFileOpened;

            // Video frame PTS
            unsigned int _uiVideoFrameNumber;
            unsigned int _uiAudioPTStracker;
            unsigned int _uiOutputTotalSize;
    };

    bool isAudioSampleFormatSupported (const enum AVSampleFormat *pSamppleFormats, AVSampleFormat format)
    {
        while (*pSamppleFormats != AV_SAMPLE_FMT_NONE) {
            if (*pSamppleFormats == format) {
                return true;
            }
            pSamppleFormats++;
        }
        return false;
    }
}

using namespace NOMADSUtil;

FFMPEGWriter::FFMPEGWriter (void)
    : _pImpl (new FFMPEGWriterImpl())
{
}

FFMPEGWriter::~FFMPEGWriter (void)
{
    if (_pImpl != NULL) {
        delete _pImpl;
        _pImpl = NULL;
    }
}

bool FFMPEGWriter::close (void)
{
    return ((_pImpl == NULL) || _pImpl->close());
}

int FFMPEGWriter::encodeImage (const RGBImage &img, int64 time)
{
    if (_pImpl == NULL) {
        return -1;
    }
    return _pImpl->encodeImage (img, time);
}

int FFMPEGWriter::createFile (const String &filename,
                              const FFMPEGUtil::VideoEncodingProfile *pProfile,
                              const FFMPEGUtil::VideoFormat *pVideoFormat,
                              unsigned int uiQuality, bool bConvertAudio,
                              FFMPEGUtil::Audio *pAudio)
{
    const char *pszMethodName = "FFMPEGWriter::createFile";
    if ((_pImpl == NULL) || (filename.length() <= 0) || (pProfile == NULL) || (pVideoFormat == NULL)) {
        return -1;
    }
    _pImpl->pAudioSrc = pAudio;
    _pImpl->_pVideoProfile = pProfile;
    _pImpl->_pVideoFormat = pVideoFormat;
    _pImpl->_bConvertAudio = bConvertAudio;

    _pImpl->_uiVideoBitRate = pProfile->_bitratesVideo[uiQuality] * 1000;
    _pImpl->_uiAudioBitRate = pProfile->_bitratesAudio[uiQuality] * 1000;

    if (_pImpl->createFile (filename)) {
        return 0;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Warning, "%s\n", _pImpl->sErrMsg.c_str());
    return -2;
}

//-----------------------------------------------------------------------------
// FFMPEGWriterImpl
//-----------------------------------------------------------------------------

FFMPEGWriterImpl::FFMPEGWriterImpl (void)
    : _pVideoProfile (NULL),
      _pVideoFormat (NULL),
      _bConvertAudio (false),
      _uiVideoBitRate (0U),
      _uiAudioBitRate (0U),
      _pOutputFormatCtx (NULL),
      _pOutputFormat (NULL),
      _pVideoCodecCtx (NULL),
      _pAudioCodecCtx (NULL),
      _pVideoStream (NULL),
      _pAudioStream (NULL),
      _pVideoCodec (NULL),
      _pAudioCodec (NULL),
      _pVideoFrame (NULL),
      _pVideoImageBuffer (NULL),
      _pAudioFrame (NULL),
      _pAudioSampleBuffer (NULL),
      _uiAudioSampleBufLen (0U),
      _pVideoConvertCtx (NULL),
      _pAudioResampleCtx (NULL),
      _bOutputFileOpened (false),
      _uiVideoFrameNumber (0U),
      _uiAudioPTStracker (0U),
      _uiOutputTotalSize (0U)
{
    FFMPEGUtil::setup();
}

FFMPEGWriterImpl::~FFMPEGWriterImpl (void)
{
	close();
}

bool FFMPEGWriterImpl::close (void)
{
    if (_pOutputFormatCtx) {
        if (_bOutputFileOpened) {
            flush();
            av_write_trailer (_pOutputFormatCtx);

            // close video and audio
            if ((_pVideoStream != NULL) && (_pVideoStream->codec != NULL)) {
                avcodec_close (_pVideoStream->codec);
            }
            if ((_pAudioStream != NULL) && (_pAudioStream->codec != NULL)) {
                avcodec_close (_pAudioStream->codec);
            }

            // Close the file
            if (_pOutputFormatCtx->pb != NULL) {
                avio_close (_pOutputFormatCtx->pb);
            }
        }

        // free the streams
        for (unsigned int i = 0; i < _pOutputFormatCtx->nb_streams; i++) {
            av_freep (&_pOutputFormatCtx->streams[i]->codec);
            av_freep (&_pOutputFormatCtx->streams[i]);
        }

        // Free the format
        av_free (_pOutputFormatCtx);
    }
    delete[] _pVideoImageBuffer;
    delete[] _pAudioSampleBuffer;
    if (_pVideoFrame) {
        av_free (_pVideoFrame);
    }
    if (_pAudioFrame) {
        av_free (_pAudioFrame);
    }
    _pOutputFormatCtx = NULL;
    _pOutputFormat = NULL;
    _pVideoCodecCtx = NULL;
    _pVideoStream = NULL;
    _pAudioStream = NULL;
    _pVideoCodec = NULL;
    _pVideoFrame = NULL;
    _pAudioFrame = NULL;
    _pVideoImageBuffer = NULL;
    _pAudioSampleBuffer = NULL;
    _pVideoConvertCtx = NULL;
    return true;
}

void FFMPEGWriterImpl::flush (void)
{
    const char *pszMethodName = "FFMPEGWriter::flush";
    int rc;

    AVPacket pkt;
    av_init_packet (&pkt);

    // packet data will be allocated by the encoder - av_init_packet() does NOT do that!
    pkt.data = NULL;
    pkt.size = 0;

    // Get the delayed frames
    for (int iGotOutput = 1; iGotOutput;) {
        if (_pAudioCodecCtx == NULL) {
            iGotOutput = 0;
        }
        else if ((rc = avcodec_encode_audio2 (_pAudioCodecCtx, &pkt, 0, &iGotOutput)) < 0) {
            FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
            return;
        }

        if (iGotOutput) {
            // Set up the packet index
            pkt.stream_index = _pAudioStream->index;

            // Newer ffmpeg versions do it anyway, but just in case
            pkt.flags |= AV_PKT_FLAG_KEY;

            if (_pAudioCodecCtx != NULL) {
                if (_pAudioCodecCtx->coded_frame && _pAudioCodecCtx->coded_frame->pts != AV_NOPTS_VALUE) {
                    pkt.pts = av_rescale_q (_pAudioCodecCtx->coded_frame->pts, _pAudioCodecCtx->time_base, _pAudioStream->time_base);
                }
            }

            // And write the file
            if ((rc = av_interleaved_write_frame (_pOutputFormatCtx, &pkt)) < 0) {
                FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "Failed to write the audio packet: error %d", rc);
                return;
            }

            _uiOutputTotalSize += pkt.size;
            av_free_packet (&pkt);
        }
    }
}

bool FFMPEGWriterImpl::resampleAndAudioStreamToFile (void)
{
    const char *pszMethodName = "FFMPEGWriterImpl::resampleAndAudioStreamToFile";
    // Find the audio codec
    _pAudioCodec = avcodec_find_encoder_by_name (_pVideoProfile->_audioCodec);

    if (_pAudioCodec == NULL) {
        sErrMsg = "Could not use the audio codec ";
        sErrMsg += _pVideoProfile->_audioCodec;
        return false;
    }

    // Hack to use the fixed AC3 codec if available
    if (_pAudioCodec->id == AV_CODEC_ID_AC3 && avcodec_find_encoder_by_name ("ac3_fixed")) {
        _pAudioCodec = avcodec_find_encoder_by_name ("ac3_fixed");
    }

    // Allocate the audio context
    _pAudioCodecCtx = avcodec_alloc_context3 (_pAudioCodec);
    if (_pAudioCodecCtx == NULL) {
        sErrMsg = "Context for audio codec ";
        sErrMsg += _pVideoProfile->_audioCodec;
        sErrMsg += " cannot be allocated";
        return false;
    }

    _pAudioCodecCtx->codec_id = _pAudioCodec->id;
    _pAudioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    _pAudioCodecCtx->bit_rate = _uiAudioBitRate;
    _pAudioCodecCtx->channels = _pVideoProfile->_channels;
    _pAudioCodecCtx->sample_rate = _pVideoProfile->_sampleRate;
    _pAudioCodecCtx->channel_layout = av_get_channel_layout (_pVideoProfile->_channels == 1 ? "mono" : "stereo");
    _pAudioCodecCtx->time_base.num = 1;
    _pAudioCodecCtx->time_base.den = _pVideoProfile->_sampleRate;

    if (_pOutputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        _pAudioCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    // Since different audio codecs support different sample formats, look up which one is supported by this specific codec
    if (isAudioSampleFormatSupported (_pAudioCodec->sample_fmts, AV_SAMPLE_FMT_FLTP)) {
        checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug, "Audio format %s: using AV_SAMPLE_FMT_FLTP",
                        _pVideoProfile->_audioCodec.c_str());
        _pAudioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    }
    else if (isAudioSampleFormatSupported (_pAudioCodec->sample_fmts, AV_SAMPLE_FMT_S16)) {
        checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug, "Audio format %s: using AV_SAMPLE_FMT_S16",
                        _pVideoProfile->_audioCodec.c_str());
        _pAudioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
    }
    else if (isAudioSampleFormatSupported (_pAudioCodec->sample_fmts, AV_SAMPLE_FMT_S16P)) {
        checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug, "Audio format %s: using AV_SAMPLE_FMT_S16P",
                        _pVideoProfile->_audioCodec.c_str ());
        _pAudioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16P;
    }
    else {
        String supported;
        for (const enum AVSampleFormat * fmt = _pAudioCodec->sample_fmts; *fmt != AV_SAMPLE_FMT_NONE; fmt++) {
            supported += av_get_sample_fmt_name (*fmt);
        }
        sErrMsg = "Could not find the sample format supported by the audio codec ";
        sErrMsg += _pVideoProfile->_audioCodec;
        sErrMsg += " supported: ";
        sErrMsg += supported;
        return false;
    }

    // Open the audio codec
    int rc = avcodec_open2 (_pAudioCodecCtx, _pAudioCodec, NULL);
    if (rc < 0) {
        FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
        char chErr[22];
        i64toa (chErr, rc);
        sErrMsg = "Could not open the audio codec: ";
        sErrMsg += chErr;
        return false;
    }

    // Allocate the audio stream
    _pAudioStream = avformat_new_stream (_pOutputFormatCtx, _pAudioCodec);
    if (_pAudioStream == NULL) {
        sErrMsg = "Could not allocate audio stream";
        return false;
    }

    // Remember the codec for the stream
    _pAudioStream->codec = _pAudioCodecCtx;

    // Setup the audio resampler
    uint64_t ui64InChLayout = pAudioSrc->aCodecCtx->channel_layout;
    if (pAudioSrc->aCodecCtx->channel_layout == 0) {
        // Some formats (i.e. WAV) do not produce the proper channel layout
        ui64InChLayout = av_get_channel_layout (_pVideoProfile->_channels == 1 ? "mono" : "stereo");
    }
    _pAudioResampleCtx = swr_alloc_set_opts (NULL,  // we're allocating a new context
                                             _pAudioCodecCtx->channel_layout,   // out_ch_layout
                                             _pAudioCodecCtx->sample_fmt,       // out_sample_fmt
                                             _pAudioCodecCtx->sample_rate,      // out_sample_rate
                                             ui64InChLayout,                    // in_ch_layout
                                             pAudioSrc->aCodecCtx->sample_fmt,  // in_sample_fmt
                                             pAudioSrc->aCodecCtx->sample_rate, // in_sample_rate
                                             0,                                 // log_offset
                                             NULL);                             // log_ctx
    if (_pAudioResampleCtx == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Could not open the audio resampler\n");
        return false;
    }
    av_opt_set_int (_pAudioResampleCtx, "in_channels", pAudioSrc->aCodecCtx->channels, 0);
    av_opt_set_int (_pAudioResampleCtx, "out_channels", _pAudioCodecCtx->channels, 0);
    rc = swr_init (_pAudioResampleCtx);
    if (rc < 0) {
        FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Could not init the audio resampler: %d\n", rc);
        return false;
    }

    /*rc = avresample_open (_pAudioResampleCtx);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Could not open the audio resampler: %d\n", rc);
        return false;
    }*/

    _pAudioFrame = av_frame_alloc();
    if (_pAudioFrame == NULL) {
        sErrMsg = "Could not allocate audio frame";
        return false;
    }

    _pAudioFrame->nb_samples = _pAudioStream->codec->frame_size;
    _pAudioFrame->format = _pAudioStream->codec->sample_fmt;
    _pAudioFrame->channel_layout = _pAudioStream->codec->channel_layout;

    // Tthe codec gives us the frame size, in samples,we calculate the size of the samples buffer in bytes
    _uiAudioSampleBufLen = av_samples_get_buffer_size (NULL, _pAudioCodecCtx->channels, _pAudioCodecCtx->frame_size, _pAudioCodecCtx->sample_fmt, 0);
    _pAudioSampleBuffer = static_cast<uint8_t*>(av_malloc (_uiAudioSampleBufLen));
    if (_pAudioSampleBuffer == NULL) {
        sErrMsg = "Could not allocate audio buffer";
        return false;
    }

    // Setup the data pointers in the AVFrame
    if (avcodec_fill_audio_frame (_pAudioFrame, _pAudioStream->codec->channels, _pAudioStream->codec->sample_fmt,
        static_cast<const uint8_t*>(_pAudioSampleBuffer), _uiAudioSampleBufLen, 0) < 0) {
        sErrMsg = "Could not set up audio frame";
        return false;
    }

    if (_pAudioStream->codec->block_align == 1 && _pAudioStream->codec->codec_id == AV_CODEC_ID_MP3) {
        _pAudioStream->codec->block_align = 0;
    }
    if (_pAudioStream->codec->codec_id == AV_CODEC_ID_AC3) {
        _pAudioStream->codec->block_align = 0;
    }
    return true;
}

bool FFMPEGWriterImpl::addAudioStreamToFile (void)
{
    // Are we copying the stream data?
    // Add the audio stream, index 1
    _pAudioStream = avformat_new_stream (_pOutputFormatCtx, 0);

    if (_pAudioStream == NULL) {
        sErrMsg = "Could not allocate audio stream";
        return false;
    }

    AVStream * pOrigAudioStream = pAudioSrc->pFormatCtx->streams[pAudioSrc->audioStream];

    _pAudioStream->time_base = pOrigAudioStream->time_base;
    _pAudioStream->disposition = pOrigAudioStream->disposition;
    _pAudioStream->pts.num = pOrigAudioStream->pts.num;
    _pAudioStream->pts.den = pOrigAudioStream->pts.den;

    AVCodecContext *pNewCtx = _pAudioStream->codec;

    // Copy the stream
    memcpy (pNewCtx, pAudioSrc->aCodecCtx, sizeof (AVCodecContext));
    if (pNewCtx->block_align == 1 && pNewCtx->codec_id == AV_CODEC_ID_MP3) {
        pNewCtx->block_align = 0;
    }
    if (pNewCtx->codec_id == AV_CODEC_ID_AC3) {
        pNewCtx->block_align = 0;
    }
    return true;
}

bool FFMPEGWriterImpl::createFile (const String &fileName)
{
    const char *pszMethodName = "FFMPEGWriterImpl::createFile";
    int rc;

    // If we had an open video, close it.
    close();

    // Find the output container format
    _pOutputFormat = av_guess_format (_pVideoProfile->_videoContainer, _pVideoProfile->_videoContainer, NULL);
    if (_pOutputFormat == NULL) {
        sErrMsg = "Could not guess the output format ";
        sErrMsg += _pVideoProfile->_videoContainer;
        close();
        return false;
    }

	// Allocate the output context
    _pOutputFormatCtx = avformat_alloc_context();
    if (_pOutputFormatCtx == NULL) {
        sErrMsg = "Error allocating format context";
        close();
        return false;
    }

    _pOutputFormatCtx->oformat = _pOutputFormat;

    // Find the video encoder
    _pVideoCodec = avcodec_find_encoder_by_name (_pVideoProfile->_videoCodec);
    if (_pVideoCodec == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "Video codec %s not found. Available codecs are:\n", _pVideoProfile->_videoCodec.c_str());
        FFMPEGUtil::logCodecList();
        sErrMsg = "Video codec ";
        sErrMsg += _pVideoProfile->_videoCodec;
        sErrMsg += " not found";
        close();
        return false;
    }

    // Allocate the video codec context
    _pVideoCodecCtx = avcodec_alloc_context3 (_pVideoCodec);
    if (_pVideoCodecCtx == NULL) {
        sErrMsg = "Context for video codec ";
        sErrMsg += _pVideoProfile->_videoCodec;
        sErrMsg += "cannot be allocated";
        close();
        return false;
    }

    // Set _pVideoCodecCtx parameters
    const bool bHasGlobalHeader = (_pOutputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER);
    if (FFMPEGUtil::setVideoCodeContext (_pVideoFormat, _pVideoCodec, _uiVideoBitRate, bHasGlobalHeader, _pVideoCodecCtx) < 0) {
        sErrMsg += "Could not set encoding parameters";
        close();
        return false;
    }

    // Open the codec
    if ((rc = avcodec_open2 (_pVideoCodecCtx, _pVideoCodec, NULL)) < 0) {
        FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
        char chErr[22];
        i64toa (chErr, rc);
        sErrMsg = "Could not open video codec: error ";
        sErrMsg += chErr;
        close();
        return false;
    }

    // Create the video stream, index
    _pVideoStream = avformat_new_stream (_pOutputFormatCtx, _pVideoCodecCtx->codec);
    if (_pVideoStream == NULL) {
        sErrMsg = "Could not allocate video stream";
        close();
        return false;
    }

    // Specify the coder for the stream
    _pVideoStream->codec = _pVideoCodecCtx;

    // Set the video stream timebase if not set
    if (_pVideoStream->time_base.den == 0) {
        _pVideoStream->time_base = _pVideoCodecCtx->time_base;
    }

    // Do we also have audio stream?
    if (pAudioSrc != NULL) {
        if (!(_bConvertAudio ? resampleAndAudioStreamToFile() : addAudioStreamToFile())) {
            close();
            return false;
        }
        // Rewind the audio player
        pAudioSrc->reset();
    }

    // Allocate the buffer for the picture
    const int iSize = avpicture_get_size (_pVideoCodecCtx->pix_fmt, _pVideoCodecCtx->width, _pVideoCodecCtx->height);
    _pVideoImageBuffer = new uint8_t[iSize];
    if (_pVideoImageBuffer == NULL) {
        sErrMsg = "Could not open allocate picture buffer";
        close();
        return false;
    }

    // Allocate the YUV frame
    _pVideoFrame = av_frame_alloc();
    if (_pVideoFrame == NULL) {
        sErrMsg = "Could not open allocate picture frame buffer";
        close();
        return false;
    }

    // Reset the PTS
    _pVideoFrame->pts = 0;

    // Setup the planes
    if (avpicture_fill ((AVPicture *) _pVideoFrame, _pVideoImageBuffer, _pVideoCodecCtx->pix_fmt,
                        _pVideoCodecCtx->width, _pVideoCodecCtx->height) <  0) {
        sErrMsg = "Could not fill picture";
        close();
        return false;
    }

    // Create the file and write the header
    strncpy (_pOutputFormatCtx->filename, fileName, sizeof (_pOutputFormatCtx->filename));

    if (avio_open (&_pOutputFormatCtx->pb, fileName, AVIO_FLAG_WRITE) < 0) {
        String msg ("Could not create video file ");
        sErrMsg = msg + fileName.c_str();
        close();
        return false;
    }

    if ((rc = avformat_write_header (_pOutputFormatCtx, NULL)) < 0) {
        FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
        sErrMsg = "Could not write header";
        close();
        return false;
    }

    _uiVideoFrameNumber = 0;
    _uiAudioPTStracker = 0;
    _uiOutputTotalSize = 0;
    _bOutputFileOpened = true;

    return true;
}

int FFMPEGWriterImpl::encodeAudio (void)
{
    return 0;
    int rc, iGotPacket;
    AVPacket pkt;

    const char *pszMethodName = "FFMPEGWriterImpl::encodeAudio";
    while (true) {
        const double dAudioPts = static_cast<double>(_pAudioStream->pts.val) * av_q2d (_pAudioStream->time_base);
        const double dVideoPts = static_cast<double>(_pVideoStream->pts.val) * av_q2d (_pVideoStream->time_base);
        if (dVideoPts < dAudioPts) {
            break;
        }

        // Read a frame
        if (av_read_frame (pAudioSrc->pFormatCtx, &pkt) < 0) {
            return false;  // Frame read failed (e.g. end of stream)
        }

        if (pkt.stream_index != pAudioSrc->audioStream) {
            av_free_packet (&pkt);
            continue;
        }

        // Initialize the frame
        AVFrame srcaudio;
        av_frame_unref (&srcaudio);    // avcodec_get_frame_defaults (&srcaudio);

        // Decode the original audio into the srcaudio frame
        int got_audio;
        rc = avcodec_decode_audio4 (pAudioSrc->aCodecCtx, &srcaudio, &got_audio, &pkt);
        if (rc < 0) {
            FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "Error decoding audio frame: %d\n", rc);
            return -1;
        }

        // We don't need the AV packet anymore
        av_free_packet (&pkt);

        // Next packet if we didn't get audio
        if (!got_audio) {
            continue;
        }

        // TODO: uncomment this
        /*
        // Resample the input into the _pAudioSampleBuffer until we proceed the whole decoded data
        if ((rc = avresample_convert (_pAudioResampleCtx, NULL, 0, 0, srcaudio.data, 0, srcaudio.nb_samples)) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "Error resampling decoded audio: %d\n", rc);
            return -1;
        }

        while (avresample_available (_pAudioResampleCtx) >= _pAudioFrame->nb_samples) {
            // Read a frame audio data from the resample fifo
            if (avresample_read (_pAudioResampleCtx, _pAudioFrame->data, _pAudioFrame->nb_samples) != _pAudioFrame->nb_samples) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "Error reading resampled audio: %d\n", rc);
                return -1;
            }

            // Prepare the packet
            av_init_packet (&pkt);

            // packet data will be allocated by the encoder - av_init_packet() does NOT do that!
            pkt.data = NULL;
            pkt.size = 0;

            // this only works for theora+vorbis
            //				if ( _pVideoCodec->id == AV_CODEC_ID_THEORA && _pAudioCodec->id == AV_CODEC_ID_VORBIS )
            //				{
            _pAudioFrame->pts = av_rescale_q (_uiAudioPTStracker, (AVRational){ 1, _pAudioCodecCtx->sample_rate }, _pAudioCodecCtx->time_base);
            _uiAudioPTStracker += _pAudioFrame->nb_samples;
            //				}

            // and encode the audio into the audiopkt
            if ((rc = avcodec_encode_audio2 (_pAudioCodecCtx, &pkt, _pAudioFrame, &iGotPacket)) < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "Audio encoder failed with error %d\n", rc);
                return -2;
            }

            if (iGotPacket) {
                // Set up the packet index
                pkt.stream_index = _pAudioStream->index;

                // Newer ffmpeg versions do it anyway, but just in case
                pkt.flags |= AV_PKT_FLAG_KEY;

                // Rescale output packet timestamp values from codec to stream timebase
                pkt.pts = av_rescale_q_rnd (pkt.pts, _pAudioCodecCtx->time_base, _pAudioStream->time_base,
                                            static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt.dts = av_rescale_q_rnd (pkt.dts, _pAudioCodecCtx->time_base, _pAudioStream->time_base,
                                            static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt.duration = av_rescale_q (pkt.duration, _pAudioCodecCtx->time_base, _pAudioStream->time_base);

                // And write the file
                if ((rc = av_interleaved_write_frame (_pOutputFormatCtx, &pkt)) < 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_Warning, "Failed to write the audio packet: error %d\n", rc);
                    return -3;
                }

                _uiOutputTotalSize += pkt.size;
                av_free_packet (&pkt);
            }
        }  */ // TODO: uncomment this
    }

    return 0;
}

int FFMPEGWriterImpl::encodeImage (const RGBImage &img, int64)
{
    const char *pszMethodName = "FFMPEGWriterImpl::encodeImage";

    // If we have audio, first add all audio packets for this time
    if ((pAudioSrc != NULL) && encodeAudio()) {
        return -1;
    }
    if (_pVideoFormat == NULL) {
        return -2;
    }
    // SWS conversion: modifies _pVideoFrame
    if (!convertImage_sws (img)) {
        return -3;
    }

    // Setup frame data
    _pVideoFrame->interlaced_frame = (_pVideoFormat->interlaced() ? 1 : 0);
    _pVideoFrame->pts = _uiVideoFrameNumber++;
    if (_pVideoFrame->width <= 0) {
        _pVideoFrame->width = _pVideoFormat->width;
    }
    if (_pVideoFrame->height <= 0) {
        _pVideoFrame->height = _pVideoFormat->height;
    }
    if (_pVideoFrame->format <= 0) {
        _pVideoFrame->format = AV_PIX_FMT_YUV420P;
    }

    AVPacket pkt;
    av_init_packet (&pkt);  // packet data will be allocated by the encoder.
    pkt.data = NULL;        // av_init_packet() does NOT do that!
    pkt.size = 0;

    int iGotPacket = 0;
    int rc = avcodec_encode_video2 (_pVideoCodecCtx, &pkt, _pVideoFrame, &iGotPacket);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Error encoding video: %d\n", rc);
        return -4;
    }
    if (iGotPacket == 1) {
        if (pkt.pts != AV_NOPTS_VALUE) {
            // Convert the PTS from the packet base to stream base
            pkt.pts = av_rescale_q (pkt.pts, _pVideoCodecCtx->time_base, _pVideoStream->time_base);
        }
        if (pkt.dts != AV_NOPTS_VALUE) {
            // Convert the DTS from the packet base to stream base
            pkt.dts = av_rescale_q (pkt.dts, _pVideoCodecCtx->time_base, _pVideoStream->time_base);
        }
        if (pkt.duration > 0) {
            pkt.duration = av_rescale_q (pkt.duration, _pVideoCodecCtx->time_base, _pVideoStream->time_base);
        }
        if (_pVideoCodecCtx->coded_frame->key_frame) {
            pkt.flags |= AV_PKT_FLAG_KEY;
        }
        pkt.stream_index = _pVideoStream->index;
        if ((rc = av_interleaved_write_frame (_pOutputFormatCtx, &pkt)) < 0) {
            return -5;
        }
        _uiOutputTotalSize += pkt.size;
        av_free_packet (&pkt);
    }
    return _uiOutputTotalSize;
}

bool FFMPEGWriterImpl::convertImage_sws (const RGBImage &img)
{
    const char *pszMethodName = "FFMPEGWriterImpl::convertImage_sws";

    // Check if the image matches the size
    const uint16 ui16SrcWidth = img.getWidth();
    const uint16 ui16SrcHeight = img.getHeight();
    if ((ui16SrcWidth != _pVideoFormat->width) || (ui16SrcHeight != _pVideoFormat->height)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Wrong image size!\n");
        return false;
    }

    const uint32 ui32LineSize = img.getLineSize();
    if (ui32LineSize > INT_MAX) {
        return false;
    }

    int iDstWidth = RNDTO2 (ui16SrcWidth); assert (iDstWidth == ui16SrcWidth);
    int iDstHeight = RNDTO2 (ui16SrcHeight); assert (iDstHeight == ui16SrcHeight);
    _pVideoConvertCtx = sws_getCachedContext (_pVideoConvertCtx, ui16SrcWidth, ui16SrcHeight, AV_PIX_FMT_RGB24,
                                              iDstWidth, iDstHeight, AV_PIX_FMT_YUV420P,
                                              SWS_BICUBIC, NULL, NULL, NULL);
    if (_pVideoConvertCtx == NULL) {
        printf ("Cannot initialize the conversion context\n");
        return false;
    }
    const int ystride = RNDTO32 (iDstWidth);
    const int uvstride = RNDTO32 (iDstWidth / 2);
    const int ysize = ystride * iDstHeight;
    const int vusize = uvstride * (iDstHeight / 2);
    const int iYUVFrameSize = ysize + (2 * vusize);
    uint8 *pFrameYUV = static_cast<uint8*>(malloc (iYUVFrameSize));
    uint8_t *plane[4] = { pFrameYUV, pFrameYUV + ysize, pFrameYUV + ysize + vusize, NULL };
    int stride[4] = { ystride, uvstride, uvstride, 0 };
    const uint8_t *const pSrcData = img.getLinePtr (0);
    int srcstride[3] = { static_cast<int>(ui32LineSize), 0, 0 };
    int rc = sws_scale (_pVideoConvertCtx, &pSrcData, srcstride, 0, ui16SrcHeight, plane, stride);
    // int rc = sws_scale (_pVideoConvertCtx, &pSrcData, srcstride, 0, ui16SrcHeight, _pVideoFrame->data, _pVideoFrame->linesize);
    if (rc <= 0) {
        return false;
    }
    for (uint8 i = 0; i < 4; i++) {
        _pVideoFrame->data[i] = plane[i];
        _pVideoFrame->linesize[i] = stride[i];
    }
    return true;
}
