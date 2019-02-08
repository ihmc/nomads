/*
 * JPEGLibWrapper.cpp
 *
 *This file is part of the IHMC Misc Library
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

#include "JPEGLibWrapper.h"

#include "BufferReader.h"
#include "Logger.h"
#include "BMPImage.h"

#include "jpeglib.h"

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#ifdef WIN32
    #define _CRT_SECURE_NO_WARNINGS
#endif

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace JPEGLibWrapperNS
{
    struct JPEGLibErrorMgr
    {
        struct jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
    };

    METHODDEF (void) jpegLibErrorExitHandler (j_common_ptr cinfo)
    {
        JPEGLibWrapperNS::JPEGLibErrorMgr *pErrorMgr = (JPEGLibWrapperNS::JPEGLibErrorMgr*) cinfo->err;
        longjmp (pErrorMgr->setjmp_buffer, 1);
    }
}

BMPImage * JPEGLibWrapper::convertJPEGToBMP (const void *pInputBuf, uint32 ui32InputBufLen)
{
    if ((pInputBuf == NULL) || (ui32InputBufLen == 0)) {
        checkAndLogMsg ("JPEGLibWrapper::convertJPEGToBMP1", Logger::L_MildError,
                        "invalid parameters\n");
        return NULL;
    }
    return convertJPEGToBMP (pInputBuf, ui32InputBufLen, NULL);
}

BMPImage * JPEGLibWrapper::convertJPEGToBMP (const char *pszFile)
{
    if (pszFile == NULL) {
        checkAndLogMsg ("JPEGLibWrapper::convertJPEGToBMP2", Logger::L_MildError,
                        "invalid parameters\n");
        return NULL;
    }
    FILE *fileInput = fopen (pszFile, "rb");
    if (fileInput == NULL) {
        checkAndLogMsg ("JPEGLibWrapper::convertJPEGToBMP2", Logger::L_MildError,
                        "failed to open file <%s>\n", pszFile);
        return NULL;
    }
    BMPImage *pImage = convertJPEGToBMP (NULL, 0, fileInput);
    fclose (fileInput);
    return pImage;
}

BufferReader * JPEGLibWrapper::convertBMPToJPEG (const BMPImage *pInputImage, uint8 ui8CompressionQuality)
{
    const char *pszMethodName = "JPEGLibWrapper::convertBMPToJPEG";
    if (pInputImage == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "input image is NULL\n");
        return NULL;
    }
    else if ((pInputImage->getWidth() <= 0) || (pInputImage->getHeight() <= 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "input image is empty\n");
        return NULL;
    }
    else if (pInputImage->getBitsPerPixel() != 24) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "cannot handle BMP with %d bits per pixel\n",
                        (int) pInputImage->getBitsPerPixel());
        return NULL;
    }
    else if (ui8CompressionQuality > 100) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "invalid compression quality (%d) - must be between 0 and 100\n",
                        (int) ui8CompressionQuality);
        return NULL;
    }

    unsigned char *puchOutputBuf = NULL;
    unsigned long ulOutputLen = 0;
    JSAMPROW sampleRow[1];
    sampleRow[0] = (JSAMPROW) malloc (pInputImage->getWidth() * 3);
    if (sampleRow[0] == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "could not allocate memory to hold one row of sample data given image width of %lu\n",
                        pInputImage->getWidth());
        return NULL;
    }

    // Setup a custom error handler for the JPEG Library
    struct jpeg_compress_struct cinfo;
    struct JPEGLibWrapperNS::JPEGLibErrorMgr errMgr;
    cinfo.err = jpeg_std_error (&errMgr.pub);
    errMgr.pub.error_exit = JPEGLibWrapperNS::jpegLibErrorExitHandler;
    if (setjmp (errMgr.setjmp_buffer)) {
        char szErrorBuf[JMSG_LENGTH_MAX];
        (*cinfo.err->format_message) ((j_common_ptr) &cinfo, szErrorBuf);
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "jpeglib operation failed with message <%s>\n",
                        szErrorBuf);
        jpeg_destroy_compress (&cinfo);
        if (puchOutputBuf != NULL) {
            free (puchOutputBuf);
        }
        if (sampleRow[0] != NULL) {
            free (sampleRow[0]);
        }
        return NULL;
    }

    // Create the JPEG Compression structure
    jpeg_create_compress (&cinfo);

    // Initialize the output source to be a buffer
    jpeg_mem_dest (&cinfo, &puchOutputBuf, &ulOutputLen);

    // Configure the input image parameters
    cinfo.image_width = pInputImage->getWidth();
    cinfo.image_height = pInputImage->getHeight();
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    // Configure the defaults for the compressor
    jpeg_set_defaults (&cinfo);

    // Override the compression quality ratio
    jpeg_set_quality (&cinfo, ui8CompressionQuality, TRUE);

    // Start the compression
    jpeg_start_compress (&cinfo, TRUE);

    // Write the data for the image
    while (cinfo.next_scanline < cinfo.image_height) {
        uint32 ui32Index = 0;
        for (uint32 ui32XPos = 0; ui32XPos < cinfo.image_width; ui32XPos++) {
            pInputImage->getPixel (ui32XPos, ((cinfo.image_height - 1) - cinfo.next_scanline), &sampleRow[0][ui32Index+2], &sampleRow[0][ui32Index+1], &sampleRow[0][ui32Index+0]);
            ui32Index += 3;
        }
        jpeg_write_scanlines (&cinfo, sampleRow, 1);
    }

    // Cleanup
    jpeg_finish_compress (&cinfo);
    free (sampleRow[0]);
    jpeg_destroy_compress (&cinfo);

    checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug, "compressed bmp image of size: %u.\n", ulOutputLen);

    return new BufferReader (puchOutputBuf, ulOutputLen, true);
}

BMPImage * JPEGLibWrapper::convertJPEGToBMP (const void *pInputBuf, uint32 ui32InputBufLen, FILE *fileInput)
{
    if (((pInputBuf == NULL) || (ui32InputBufLen == 0)) && (fileInput == NULL)) {
        checkAndLogMsg ("JPEGLibWrapper::convertJPEGToBMP", Logger::L_MildError,
                        "either pInputBuf/ui32InputBufLen or fileInput must be specified\n");
        return NULL;
    }
    int rc;
    struct jpeg_decompress_struct cinfo;
    struct JPEGLibWrapperNS::JPEGLibErrorMgr errMgr;
    JSAMPARRAY pSampleArray = NULL;
    BMPImage *pBMPImage = NULL;

    // Setup a custom error handler for the JPEG Library
    cinfo.err = jpeg_std_error (&errMgr.pub);
    errMgr.pub.error_exit = JPEGLibWrapperNS::jpegLibErrorExitHandler;
    if (setjmp (errMgr.setjmp_buffer)) {
        char szErrorBuf[JMSG_LENGTH_MAX];
        (*cinfo.err->format_message) ((j_common_ptr) &cinfo, szErrorBuf);
        checkAndLogMsg ("JPEGLibWrapper::convertJPEGToBMP", Logger::L_MildError,
                        "jpeglib operation failed with message <%s>\n", szErrorBuf);
        jpeg_destroy_decompress (&cinfo);
        if (pBMPImage) {
            delete pBMPImage;
        }
        if (pSampleArray) {
            if (pSampleArray[0]) {
                free (pSampleArray[0]);
            }
            free (pSampleArray);
        }
        return NULL;
    }

    // Create the JPEG Decompression structure
    jpeg_create_decompress (&cinfo);

    // Initialize the input source to be either a file or a buffer
    if (fileInput != NULL) {
        jpeg_stdio_src (&cinfo, fileInput);
    }
    else {
        jpeg_mem_src (&cinfo, (unsigned char *) pInputBuf, ui32InputBufLen);
    }

    // Read te header to determine the size of the image
    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);

    // Initialize a BMP Image to hold the decompressed data
    pBMPImage = new BMPImage (true);
    if (0 != (rc = pBMPImage->initNewImage (cinfo.output_width, cinfo.output_height, 24))) {
        checkAndLogMsg ("JPEGLibWrapper::convertJPEGToBMP", Logger::L_MildError,
                        "failed to initialize BMPImage; rc = %d\n", rc);
        delete pBMPImage;
        jpeg_destroy_decompress (&cinfo);
        return NULL;
    }

    // Setup the output array for the JPEG decoder output
    pSampleArray = (JSAMPARRAY) malloc (sizeof (JSAMPROW) * 1);
    if (pSampleArray == NULL) {
        checkAndLogMsg ("JPEGLibWrapper::convertJPEGToBMP", Logger::L_MildError,
                        "failed to allocate JSAMPARRAY\n");
        delete pBMPImage;
        jpeg_destroy_decompress (&cinfo);
        return NULL;
    }
    JDIMENSION row_stride = cinfo.output_width * cinfo.output_components;
    pSampleArray[0] = (JSAMPROW) malloc (sizeof (JSAMPLE) * row_stride);
    if (pSampleArray[0] == NULL) {
        checkAndLogMsg ("JPEGLibWrapper::convertJPEGToBMP", Logger::L_MildError,
                        "failed to allocate JSAMPROW of size %lu\n", (uint32) (sizeof (JSAMPLE) * row_stride));
        delete pBMPImage;
        free (pSampleArray);
        pSampleArray = NULL;
        jpeg_destroy_decompress (&cinfo);
        return NULL;
    }

    // Decode the JPEG and store the result in the BMP
    for (uint32 ui32YPos = 0; cinfo.output_scanline < cinfo.output_height; ui32YPos++) {
        if (jpeg_read_scanlines (&cinfo, pSampleArray, 1) != 1) {
            checkAndLogMsg ("JPEGLibWrapper::convertJPEGToBMP", Logger::L_MildError,
                            "jpeg_read_scanlines() did not decode a line of data\n");
            delete pBMPImage;
            free (pSampleArray[0]);
            free (pSampleArray);
            jpeg_destroy_decompress (&cinfo);
            return NULL;
        }
        uint32 ui32Index = 0;
        for (uint32 ui32XPos = 0; ui32XPos < cinfo.output_width; ui32XPos++) {
            if (0 != (rc = pBMPImage->setPixel (ui32XPos, ((cinfo.output_height - 1) - ui32YPos), pSampleArray[0][ui32Index+2], pSampleArray[0][ui32Index+1], pSampleArray[0][ui32Index+0]))) {
                checkAndLogMsg ("JPEGLibWrapper::convertJPEGToBMP", Logger::L_MildError,
                                "failed to set pixel (%lu, %lu), rc = %d\n", ui32XPos, ui32YPos, rc);
                delete pBMPImage;
                free (pSampleArray[0]);
                free (pSampleArray);
                jpeg_destroy_decompress (&cinfo);
                return NULL;
            }
            ui32Index += 3;
        }
    }

    // Cleanup
    jpeg_finish_decompress (&cinfo);
    free (pSampleArray[0]);
    free (pSampleArray);
    jpeg_destroy_decompress (&cinfo);

    return pBMPImage;
}
