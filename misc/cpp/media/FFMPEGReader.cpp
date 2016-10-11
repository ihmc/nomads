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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 28, 2015, 12:27 AM
 */

extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
};

#include "FFMPEGReader.h"

#include "BufferReader.h"
#include "FileUtils.h"
#include "FFMPEGUtil.h"
#include "Logger.h"
#include "RGBImage.h"
#include "StringTokenizer.h"
#include <limits.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace NOMADSUtil
{
    class FFMPEGReaderImpl
    {
    public:
        FFMPEGReaderImpl (void);
        ~FFMPEGReaderImpl (void);

        void init (void);
        bool readFrame (int frame);

    public:
        int _iVideoStreamIdx;
        int	_iMaxFrame;
        int	_fpsDen;
        int	_fpsNum;
        int	_iCurrFrameNum;
        unsigned int _uiFramesToSkip;
        uint8_t * _pBuf;
        AVFormatContext *_pFormatCtx;
        AVCodecContext *_pCodecCtx;
        AVCodec *_pCodec;
        AVFrame *_pFrame;    // YUV frame
        AVFrame *_pFrameRGB; // RGB frame
        FFMPEGUtil::VideoFormat *_pVideoFormat;
        FFMPEGUtil::VideoEncodingProfile *_pVideoEncProfile;
        SwsContext *_pImgConvertCtx;
        String _sErrMsg;
        RGBImage _currFrameImg;
    };

    class BufferIOContext
    {
    public:
        static const unsigned int OUTPUT_BUFFER_SIZE = 32678;

        BufferIOContext (const void *pBuf, unsigned int uiLen);
        ~BufferIOContext (void);
        void reset_inner_context (void);

        static int read (void *opaque, unsigned char *buf, int buf_size);

        // - offset: The new position within the stream. This is relative
        //           to the loc parameter, and can be positive or negative. 
        // - whence: A value of type SeekOrigin, which acts as the seek
        //           reference point.
        // It returns the new position within the stream, calculated by
        // combining the initial reference point and the offset
        static int64_t seek (void *opaque, int64_t offset, int whence);
        AVIOContext * getAVIO (void);

    private:
        BufferIOContext (BufferIOContext const &);
        BufferIOContext& operator = (BufferIOContext const &);

    private:
        unsigned char *_pOutBuf;
        BufferReader _inputBuf;
        AVIOContext *_pCodecCtx;
    };

    int decode (FFMPEGReaderImpl *_pImpl, unsigned int seekto);
}

using namespace NOMADSUtil;

FFMPEGReader::FFMPEGReader (void)
    : _pImpl (new FFMPEGReaderImpl())
{
    FFMPEGUtil::setup();
}

FFMPEGReader::~FFMPEGReader (void)
{
    delete _pImpl;
    if (_tmpfilename.length() > 0) {
        FileUtils::deleteFile (_tmpfilename);
    }
}

void FFMPEGReader::close (void)
{
    _pImpl->init(); // Reset pointers and params
}

NOMADSUtil::String createTempFileName (void)
{
    String tmpDir;
    String prefix ("dstmp");  // 5 chars at most
    #ifdef LINUX
    if (FileUtils::directoryExists("/tmp")) {
        tmpDir = "/tmp";
    }
    #endif
    String tmpFileName (tempnam (tmpDir, prefix));
    while (tmpDir.startsWith ("\\")) {
        tmpDir = tmpDir.substring (1, tmpDir.length ());
    }
    return tmpFileName;
}

int FFMPEGReader::read (const void *pBuffer, unsigned int uiLen)
{
    if ((pBuffer == NULL) || (uiLen == 0)) {
        return -1;
    }
    _tmpfilename = createTempFileName();
    if (_tmpfilename.length() <= 0) {
        return -2;
    }
    if (FileUtils::dumpBufferToFile (pBuffer, uiLen, _tmpfilename) < 0) {
        return -3;
    }
    if (openFile (_tmpfilename) < 0) {
        return -4;
    }
    return 0;

    /*
    const char *pszMethodName = "FFMPEGReader::read";
    close();

    if ((pBuffer == NULL) || (uiLen == 0U)) {
        return -1;
    }

    BufferIOContext bufCtx (pBuffer, uiLen);
    _pImpl->_pFormatCtx = avformat_alloc_context();
    _pImpl->_pFormatCtx->pb = bufCtx.getAVIO();

    int rc = avformat_open_input (&_pImpl->_pFormatCtx, "stream", NULL, NULL);
    if (rc < 0) {
        // avformat_open_input frees _pImpl->_pFormatCtx on failure: set to
        // NULL to avoid double free
        _pImpl->_pFormatCtx = NULL;
        FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
        _pImpl->_sErrMsg = "Could not open video file";
        return -2;
    }

    rc = decode (_pImpl, 0U);
    if (0 != rc) {
        return -3;
    }*/
    return 0;
}

int FFMPEGReader::openFile (const char *pszFileName, unsigned int seekto)
{
    const char *pszMethodName = "FFMPEGReader::openFile";
    close();

    if (pszFileName == NULL) {
        return -1;
    }

    int rc = avformat_open_input (&_pImpl->_pFormatCtx, pszFileName, NULL, NULL);
    if (rc < 0) {
        FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
        _pImpl->_sErrMsg = "Could not open video file";
        return -2;
    }

    rc = decode (_pImpl, seekto);
    if (0 != rc) {
        return -3;
    }

    return 0;
}

const char * FFMPEGReader::errorMsg (void) const
{
    if (_pImpl == NULL) {
        return NULL;
    }
    return _pImpl->_sErrMsg;
}

RGBImage * FFMPEGReader::getFrame (int iFrameIdx)
{
    if (_pImpl == NULL) {
        return NULL;
    }
    while (!_pImpl->readFrame (iFrameIdx)) {
        // End of video; restart
        _pImpl->_iMaxFrame = _pImpl->_iCurrFrameNum - 1;
        _pImpl->_iCurrFrameNum = 0;
        iFrameIdx = 0;

        if (av_seek_frame (_pImpl->_pFormatCtx, _pImpl->_iVideoStreamIdx, 0, 0) < 0) {
            return NULL;
        }
        _pImpl->_uiFramesToSkip = 0;
        avcodec_flush_buffers (_pImpl->_pCodecCtx);
    }
    return &(_pImpl->_currFrameImg);
}

RGBImage * FFMPEGReader::getFrameAtTime (int64 i64TimeInMilliseconds)
{
    if (_pImpl == NULL) {
        return NULL;
    }

    // Use current frame?
    return getFrame (getFrameIndexByTime (i64TimeInMilliseconds));
}

int FFMPEGReader::getFrameIndexByTime (int64 i64TimeInMilliseconds)
{
    const int64 i64FrameForTime = ((i64TimeInMilliseconds * _pImpl->_fpsNum) / _pImpl->_fpsDen) / 1000;
    if (i64FrameForTime > INT_MAX) {
        return -1;
    }

    int iFrameForTime = static_cast<int>(i64FrameForTime);
    if (iFrameForTime == 0) {
        iFrameForTime = 1;
    }

    iFrameForTime += _pImpl->_uiFramesToSkip;

    // Loop if we know how many frames we have total
    if (_pImpl->_iMaxFrame > 0) {
        iFrameForTime %= _pImpl->_iMaxFrame;
    }

    return iFrameForTime;
}

int64 FFMPEGReader::getFrameTimeInMillis (int iFrameIdx)
{
    return ((1000 * iFrameIdx * _pImpl->_fpsDen) / _pImpl->_fpsNum);
}

const FFMPEGUtil::VideoEncodingProfile * FFMPEGReader::getVideoEncProfile (void)
{
    if (_pImpl == NULL) {
        return NULL;
    }
    return _pImpl->_pVideoEncProfile;
}

const FFMPEGUtil::VideoFormat * FFMPEGReader::getVideoFormat (void)
{
    if (_pImpl == NULL) {
        return NULL;
    }
    return _pImpl->_pVideoFormat;
}

//-----------------------------------------------------------------------------
// FFMPEGReaderImpl
//-----------------------------------------------------------------------------

FFMPEGReaderImpl::FFMPEGReaderImpl (void)
    : _iVideoStreamIdx (-1),
      _iMaxFrame (0),
      _fpsDen (0),
      _fpsNum (0),
      _iCurrFrameNum (0),
      _uiFramesToSkip (0),
      _pBuf (NULL),
      _pFormatCtx (NULL),
      _pCodecCtx (NULL),
      _pCodec (NULL),
      _pFrame (NULL),
      _pFrameRGB (NULL),
      _pVideoFormat (NULL),
      _pVideoEncProfile (NULL),
      _pImgConvertCtx (NULL)
{ 
}

FFMPEGReaderImpl::~FFMPEGReaderImpl (void)
{
    if (_pFrame != NULL) {
        av_frame_free (&_pFrame);
    }
    if (_pFrameRGB != NULL) {
        av_frame_free (&_pFrameRGB);
    }
    if (_pCodecCtx != NULL) {
        avcodec_close (_pCodecCtx);
        _pCodecCtx = NULL;
    }
    if (_pFormatCtx != NULL) {
        avformat_close_input (&_pFormatCtx);
        _pFormatCtx = NULL;
    }
    if (_pVideoFormat != NULL) {
        delete _pVideoFormat;
        _pVideoFormat = NULL;
    }
    if (_pVideoEncProfile != NULL) {
        delete _pVideoEncProfile;
        _pVideoEncProfile = NULL;
    }
}

void FFMPEGReaderImpl::init (void)
{
    _uiFramesToSkip = 0;
    _pFormatCtx = NULL;
    _iVideoStreamIdx = -1;
    _pCodecCtx = NULL;
    _pCodec = NULL;
    _pFrame = NULL;
    _pFrameRGB = NULL;
    _pImgConvertCtx = NULL;
    _pVideoFormat = NULL;
    _pVideoEncProfile = NULL;
    _pBuf = NULL;
    _iMaxFrame = 0;
    _fpsDen = 0;
    _fpsNum = 0;
    _iCurrFrameNum = 0;
}

bool FFMPEGReaderImpl::readFrame (int iFrame)
{
    AVPacket packet;
    int iFrameFinished;

    while (_iCurrFrameNum < iFrame) {
        // Read a frame
        if (av_read_frame (_pFormatCtx, &packet) < 0) {
            return false;  // Frame read failed (e.g. end of stream)
        }

        if (packet.stream_index == _iVideoStreamIdx) {
            // Is this a packet from the video stream -> decode video frame
            avcodec_decode_video2 (_pCodecCtx, _pFrame, &iFrameFinished, &packet);

            if (iFrameFinished != 0) {
                _iCurrFrameNum++;

                if (_iCurrFrameNum >= iFrame) {
                    if ((_pCodecCtx->width < 0) || (_pCodecCtx->height < 0)) {
                        return false;
                    }
                    const int iWidth = _pCodecCtx->width;
                    const int iHeight = _pCodecCtx->height;
                    _pImgConvertCtx = sws_getCachedContext (_pImgConvertCtx, iWidth, iHeight,
                                                            _pCodecCtx->pix_fmt, iWidth, iHeight,
                                                            AV_PIX_FMT_RGB24, SWS_BICUBIC,
                                                            NULL,  // source filter
                                                            NULL,  // destination filter
                                                            NULL); // parameter

                    if (_pImgConvertCtx == NULL) {
                        printf ("Cannot initialize the conversion context!\n");
                        return false;
                    }

                    int rc = sws_scale (_pImgConvertCtx, _pFrame->data, _pFrame->linesize, 0,
                                        _pCodecCtx->height, _pFrameRGB->data, _pFrameRGB->linesize);
                    if (rc < 0) {
                        return false;
                    }

                    // Convert the frame to RGBImage
                    rc = _currFrameImg.initNewImage (iWidth, iHeight, 24);
                    if (rc < 0) {
                        return false;
                    }

                    for (int y = 0; y < iHeight; y++) {
                        uint8 *pDst = _currFrameImg.getLinePtr (y);
                        int iOffset = y * _pFrameRGB->linesize[0];
                        memcpy (pDst, _pFrameRGB->data[0] + iOffset, iWidth * 3);
                    }
                }
            }
        }

        av_free_packet (&packet);
    }

    return true;
}

//-----------------------------------------------------------------------------
// BufferIOContext
//-----------------------------------------------------------------------------

BufferIOContext::BufferIOContext (const void *pBuf, unsigned int uiLen)
    : _pOutBuf (static_cast<unsigned char*> (av_malloc (OUTPUT_BUFFER_SIZE))),
      _inputBuf (pBuf, uiLen, false)
{
    _pCodecCtx = avio_alloc_context (_pOutBuf,           // Memory block for input / output operations via AVIOContext
                                     OUTPUT_BUFFER_SIZE, // The buffer size is very important for performance.
                                                         // For protocols with fixed blocksize it should be set to this blocksize.
                                                         // For others a typical size is a cache page, e.g. 4kb
                                     AVIO_FLAG_READ,     // Read-only
                                     this,               // Opaque pointer to user-specific data
                                     &read,              // A function for refilling the buffer
                                     NULL,               // A function for writing the buffer contents
                                     &seek);             // A function for seeking to specified byte position
}

BufferIOContext::~BufferIOContext (void)
{
    if (_pCodecCtx != NULL) {
        av_free (_pCodecCtx);
        _pCodecCtx = NULL;
    }
    if (_pOutBuf != NULL) {
        av_free (_pOutBuf);
        _pOutBuf = NULL;
    }
}

void BufferIOContext::reset_inner_context (void)
{
    _pCodecCtx = NULL;
    _pOutBuf = NULL;
}

int BufferIOContext::read (void *pOpaque, unsigned char *pBuf, int iBufSize)
{
    if ((pBuf == NULL) || (iBufSize < -0)) {
        return -1;
    }
    BufferIOContext *pBufCtx = static_cast<BufferIOContext*>(pOpaque);
    return pBufCtx->_inputBuf.readBytes (pBuf, iBufSize);
}

int64_t BufferIOContext::seek (void *pOpaque, int64_t i64Offset, int iWhence)
{
    BufferIOContext *pBufCtx = static_cast<BufferIOContext*>(pOpaque);
    if (0x10000 == iWhence) {
        return pBufCtx->_inputBuf.getBufferLength ();
    }
    const int64_t i64Pos = iWhence + i64Offset;
    if ((i64Pos < 0) || (i64Pos > 0xFFFFFFFF)) {
        return -1;
    }
    const uint32 ui32Pos = static_cast<uint32>(i64Pos);
    if (ui32Pos > pBufCtx->_inputBuf.getBufferLength()) {
        return -2;
    }
    if (pBufCtx->_inputBuf.setPosition (ui32Pos) < 0) {
        return -3;
    }
    return ui32Pos;
}

AVIOContext * BufferIOContext::getAVIO (void)
{
    return _pCodecCtx;
}

//-----------------------------------------------------------------------------

int NOMADSUtil::decode (FFMPEGReaderImpl *pImpl, unsigned int seekto)
{
    const char *pszMethodName = "FFMPEGReader::decode";
    if (pImpl == NULL) {
        return -1;
    }

    // Retrieve stream information
    int rc = avformat_find_stream_info (pImpl->_pFormatCtx, NULL);
    if (rc < 0) {
        FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
        pImpl->_sErrMsg = "Could not find stream information in the video file";
        return -2;
    }

    // Find the first video stream
    pImpl->_iVideoStreamIdx = -1;

    for (unsigned i = 0; i < pImpl->_pFormatCtx->nb_streams; i++) {
        if (pImpl->_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            pImpl->_iVideoStreamIdx = i;
            break;
        }
    }

    if (pImpl->_iVideoStreamIdx < 0) {
        return 43; // Didn't find a video stream
    }

    if (Logger::L_Info < pLogger->getDebugLevel()) {
        static const int INPUT = 0;
        av_dump_format (pImpl->_pFormatCtx, pImpl->_iVideoStreamIdx, "", INPUT);
    }

    pImpl->_fpsDen = pImpl->_pFormatCtx->streams[pImpl->_iVideoStreamIdx]->r_frame_rate.den;
    pImpl->_fpsNum = pImpl->_pFormatCtx->streams[pImpl->_iVideoStreamIdx]->r_frame_rate.num;

    if (pImpl->_fpsDen == 60000) {
        pImpl->_fpsDen = 30000;
    }

    // Get a pointer to the codec context for the video stream
    if ((pImpl->_pCodecCtx = pImpl->_pFormatCtx->streams[pImpl->_iVideoStreamIdx]->codec) == NULL) {
        pImpl->_sErrMsg = "Could not initialize codec context for the video file";
        return -5;
    }

    // Find the decoder for the video stream
    if ((pImpl->_pCodec = avcodec_find_decoder (pImpl->_pCodecCtx->codec_id)) == NULL) {
        pImpl->_sErrMsg = "Could not find the decoder for the video file";
        return -6;
    }
    pImpl->_pVideoFormat = new FFMPEGUtil::VideoFormat (*(pImpl->_pCodecCtx), pImpl->_pFormatCtx->duration,
                                                        pImpl->_pFormatCtx->streams[pImpl->_iVideoStreamIdx]->nb_frames);
    pImpl->_pVideoEncProfile = new FFMPEGUtil::VideoEncodingProfile;
    if (pImpl->_pVideoEncProfile != NULL) {

        StringTokenizer tokenizer (pImpl->_pFormatCtx->iformat->name, ',', ',');
        pImpl->_pVideoEncProfile->_videoContainer = tokenizer.getNextToken();
        tokenizer.init (pImpl->_pFormatCtx->iformat->extensions, ',', ',');
        pImpl->_pVideoEncProfile->_videoContainerExtension = tokenizer.getNextToken();
        pImpl->_pVideoEncProfile->_videoCodec = pImpl->_pCodec->name;
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Video container: %s (extension %s). Selecting %s.\n",
                        pImpl->_pFormatCtx->iformat->name, pImpl->_pFormatCtx->iformat->extensions,
                        pImpl->_pVideoEncProfile->_videoContainer.c_str());
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Video codec: %s.\n",
                        pImpl->_pVideoEncProfile->_videoCodec.c_str());
    }

    // Open codec
    rc = avcodec_open2 (pImpl->_pCodecCtx, pImpl->_pCodec, NULL);
    if (rc < 0) {
        FFMPEGUtil::log (pszMethodName, Logger::L_MildError, rc);
        pImpl->_sErrMsg = "Could not open the codec for the video file";
        return -7;
    }

    // Allocate video frame
    if ((pImpl->_pFrame = av_frame_alloc()) == NULL) {
        pImpl->_sErrMsg = "Could not allocate memory for video frames";
        return -8;
    }
    if ((pImpl->_pFrameRGB = av_frame_alloc()) == NULL) {
        pImpl->_sErrMsg = "Could not allocate memory for video frames";
        return -9;
    }

    // Determine required buffer size and allocate buffer
    const int iBufSize = avpicture_get_size (AV_PIX_FMT_RGB24, pImpl->_pCodecCtx->width, pImpl->_pCodecCtx->height);
    pImpl->_pBuf = static_cast<uint8_t *>(calloc (iBufSize, sizeof (uint8_t)));

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    avpicture_fill ((AVPicture *) pImpl->_pFrameRGB, static_cast<uint8_t*>(pImpl->_pBuf),
                     AV_PIX_FMT_RGB24, pImpl->_pCodecCtx->width, pImpl->_pCodecCtx->height);

    pImpl->_uiFramesToSkip = seekto;
    return 0;
}

