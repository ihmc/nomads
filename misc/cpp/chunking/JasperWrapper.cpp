/*
 * JasperWrapper.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2016 IHMC.
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

#include "JasperWrapper.h"

#include "jasper/jasper.h"

#include "BMPImage.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "Logger.h"
#include "ChunkingUtils.h"

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg
#define unsupportedOnAndroid Logger::L_MildError, "JPEG2000 not supported on the Android platform\n"

bool JasperWrapper::_bJasperInitialized;

BufferReader * JasperWrapper::convertToBMP (const void *pInputBuf, uint32 ui32InputBufLen)
{
    #if defined (ANDROID)
        checkAndLogMsg ("JasperWrapper::convertToBMP", unsupportedOnAndroid);
        return NULL;
    #endif
    checkAndInitializeJasper();
    return convertImage (pInputBuf, ui32InputBufLen, jas_image_strtofmt ("bmp"));
}

BMPImage * JasperWrapper::convertToBMPAsImg (const void *pInputBuf, uint32 ui32InputBufLen)
{
    BufferReader *pReader = convertToBMP (pInputBuf, ui32InputBufLen);
    if (pReader == NULL) {
        checkAndLogMsg ("JasperWrapper::convertToBMPAsImg", Logger::L_MildError,
                        "failed to convert JPEG2000 image to BMP\n");
        return NULL;
    }
    BMPImage *pBMPImage = IHMC_MISC::ChunkingUtils::toBMPImage (pReader);
    if (pBMPImage == NULL) {
        checkAndLogMsg ("JasperWrapper::convertToBMPAsImg", Logger::L_MildError,
                        "failed to read BMP image after converting JPEG2000 to BMP\n");
    }
    delete pReader;
    return pBMPImage;
}

BufferReader * JasperWrapper::convertToJPEG (const void *pInputBuf, uint32 ui32InputBufLen)
{
    #if defined (ANDROID)
        checkAndLogMsg ("JasperWrapper::convertToJPEG", unsupportedOnAndroid);
        return NULL;
    #endif
    checkAndInitializeJasper();
    return convertImage (pInputBuf, ui32InputBufLen, jas_image_strtofmt ("jpeg"));
}

BufferReader * JasperWrapper::convertToJPEG2000 (const BMPImage *pBMPImage)
{
    #if defined (ANDROID)
        checkAndLogMsg ("JasperWrapper::convertToJPEG2000", unsupportedOnAndroid);
        return NULL;
    #endif
    int rc = 0;
    BufferWriter bw (pBMPImage->getTotalSize(), 1024);
    if (0 != (rc = pBMPImage->writeHeaderAndImage (&bw))) {
        checkAndLogMsg ("JasperWrapper::convertToJPEG2000", Logger::L_MildError,
                        "failed to write chunk BMP into buffer for creating JPEG2000; rc = %d\n", rc);
        return NULL;
    }
    return convertToJPEG2000 (bw.getBuffer(), bw.getBufferLength());
}

BufferReader * JasperWrapper::convertToJPEG2000 (const void *pInputBuf, uint32 ui32InputBufLen)
{
    #if defined (ANDROID)
        checkAndLogMsg ("JasperWrapper::convertToJPEG2000", unsupportedOnAndroid);
        return NULL;
    #endif
    checkAndInitializeJasper();
    return convertImage (pInputBuf, ui32InputBufLen, jas_image_strtofmt ("jp2"));
}

void JasperWrapper::checkAndInitializeJasper (void)
{
    int rc;
    if (!_bJasperInitialized) {
        if (0 != (rc = jas_init())) {
            checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_MildError,
                            "jas_init() failed with rc = %d\n", rc);
        }
        else {
            _bJasperInitialized = true;
        }
    }
}

NOMADSUtil::BufferReader * JasperWrapper::convertImage (const void *pInputBuf, uint32 ui32InputBufLen, int iTargetFormat)
{
    int rc;
    if ((pInputBuf == NULL) || (ui32InputBufLen == 0)) {
        checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_MildError,
                        "invalid parameters\n");
        return NULL;
    }
    jas_stream_t *pJASInputStream = jas_stream_memopen ((char*)pInputBuf, (int)ui32InputBufLen);
    if (pJASInputStream == NULL) {
        checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_MildError,
                        "jas_stream_memopen() failed for the input stream\n");
        return NULL;
    }
    int iFormat;
    if ((iFormat = jas_image_getfmt (pJASInputStream)) < 0) {
        checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_MildError,
                        "jas_image_getfmt() failed with rc = %d\n", iFormat);
        jas_stream_close (pJASInputStream);
        return NULL;
    }
    checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_LowDetailDebug,
                    "reading image of type %s\n", jas_image_fmttostr (iFormat));
    jas_image_t *pJASImage = jas_image_decode (pJASInputStream, iFormat, NULL);
    if (pJASImage == NULL) {
        checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_MildError,
                        "jas_image_decode() failed\n");
        jas_stream_close (pJASInputStream);
        return NULL;
    }
    jas_stream_close (pJASInputStream);
    int iWidth = jas_image_width (pJASImage);
    int iHeight = jas_image_height (pJASImage);
    checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_LowDetailDebug,
                    "image is of size %dx%d\n", iWidth, iHeight);
    jas_stream_t *pJASOutputStream = jas_stream_memopen (NULL, 0);
    if (pJASOutputStream == NULL) {
        checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_MildError,
                        "jas_stream_memopen() failed for the output stream\n");
        jas_image_destroy (pJASImage);
        return NULL;
    }
    if (0 != (rc = jas_image_encode (pJASImage, pJASOutputStream, iTargetFormat, NULL))) {
        checkAndLogMsg ("JEGUtils::convertJPEGToBMP", Logger::L_MildError,
                        "jas_image_encode() failed with rc = %d\n", rc);
        jas_stream_close (pJASOutputStream);
        return NULL;
    }
    jas_image_destroy (pJASImage);
    int iBMPImageLength = jas_stream_length (pJASOutputStream);
    if (iBMPImageLength <= 0) {
        checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_MildError,
                        "encoded BMP image length (%d) is invalid\n", iBMPImageLength);
        jas_stream_close (pJASOutputStream);
        return NULL;
    }
    checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_LowDetailDebug,
                    "size of image decoded into BMP is %lu\n", iBMPImageLength);
    jas_stream_rewind (pJASOutputStream);
    void *pBuf = malloc (iBMPImageLength);
    if (pBuf == NULL) {
        checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_MildError,
                        "malloc() failed to allocate %d bytes of memory\n", iBMPImageLength);
        jas_stream_close (pJASOutputStream);
        return NULL;
    }
    if ((rc = jas_stream_read (pJASOutputStream, pBuf, jas_stream_length (pJASOutputStream))) != iBMPImageLength) {
        checkAndLogMsg ("JPEGUtils::convertJPEGToBMP", Logger::L_MildError,
                        "jas_stream_read() failed with rc = %d\n", rc);
        jas_stream_close (pJASOutputStream);
        free (pBuf);
        return NULL;
    }
    return new BufferReader (pBuf, iBMPImageLength, true);
}
