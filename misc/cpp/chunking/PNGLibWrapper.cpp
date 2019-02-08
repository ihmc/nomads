/*
 * PNGLibWrapper.cpp
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

#include "PNGLibWrapper.h"

#include "png.h"

#include "BMPImage.h"
#include "BufferReader.h"
#include "Logger.h"

#include <stdlib.h>

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace PNGLibWrapperNS
{
    struct InputData
    {
        const uint8 *pui8InputBuf;
        uint32 ui32InputBufLen;
        uint32 ui32CurrPos;
    };

    struct OutputData
    {
        uint8 *pBuffer;
        size_t size;
    };

    void user_error_fn (png_structp png_ptr, png_const_charp error_msg);

    void user_warning_fn (png_structp png_ptr, png_const_charp warning_msg);

    void user_read_data (png_structp png_ptr, png_bytep data, png_size_t length);

    void user_png_write_data (png_structp png_ptr, png_bytep data, png_size_t length);

    void user_png_flush (png_structp png_ptr);
}

BMPImage * PNGLibWrapper::convertPNGToBMP (const void *pInputBuf, uint32 ui32InputBufLen)
{
    #if defined (ANDROID)
        checkAndLogMsg("Chunker::fragmentBuffer", Logger::L_MildError, "PNG not supported on the Android platform\n");
        return NULL;
    #endif
    if ((pInputBuf == NULL) || (ui32InputBufLen == 0)) {
        checkAndLogMsg ("PNGLibWrapper::convertPNGToBMP", Logger::L_MildError,
                        "input buf is null or input buf len is 0\n");
        return NULL;
    }

    png_structp png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, PNGLibWrapperNS::user_error_fn, PNGLibWrapperNS::user_warning_fn);
    if (png_ptr == NULL) {
        checkAndLogMsg ("PNGLibWrapper::convertPNGToBMP", Logger::L_MildError,
                        "png_create_read_struct() failed\n");
        return NULL;
    }

    png_infop info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        checkAndLogMsg ("PNGLibWrapper::convertPNGToBMP", Logger::L_MildError,
                        "png_create_info_struct() failed\n");
        png_destroy_read_struct (&png_ptr, NULL, NULL);
        return NULL;
    }

    if (setjmp (png_jmpbuf (png_ptr))) {
        checkAndLogMsg ("PNGLibWrapper::convertPNGToBMP", Logger::L_MildError,
                        "libpng terminated with an error\n");
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    PNGLibWrapperNS::InputData inputData;
    inputData.pui8InputBuf = (const uint8*) pInputBuf;
    inputData.ui32CurrPos = 0;
    inputData.ui32InputBufLen = ui32InputBufLen;
    png_set_read_fn (png_ptr, &inputData, PNGLibWrapperNS::user_read_data);

    png_read_png (png_ptr, info_ptr, PNG_TRANSFORM_SCALE_16, NULL);

    int bit_depth = png_get_bit_depth (png_ptr, info_ptr);
    if (bit_depth != 8) {
        checkAndLogMsg ("PNGLibWrapper::convertPNGToBMP", Logger::L_MildError,
                        "cannot handle PNG with a bit depth of %d\n", (int) bit_depth);
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    int color_type = png_get_color_type (png_ptr, info_ptr);
    if ((color_type != PNG_COLOR_TYPE_RGB) && (color_type != PNG_COLOR_TYPE_RGBA)) {
        checkAndLogMsg ("PNGLibWrapper::convertPNGToBMP", Logger::L_MildError,
                        "cannot handle PNG with a non-RGB color type; color type is %d\n", (int) color_type);
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    int interlace_type = png_get_interlace_type (png_ptr, info_ptr);
    if (interlace_type != PNG_INTERLACE_NONE) {
        checkAndLogMsg ("PNGLibWrapper::convertPNGToBMP", Logger::L_MildError,
                        "cannot handle an interlaced PNG - interlace type is %d\n", (int) interlace_type);
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    int channels = png_get_channels (png_ptr, info_ptr);
    png_uint_32 width = png_get_image_width (png_ptr, info_ptr);
    png_uint_32 height = png_get_image_height (png_ptr, info_ptr);

    checkAndLogMsg ("PNGLibWrapper::convertPNGToBMP", Logger::L_Info,
                    "converting PNG of size %lux%lu with %d channels, bit depth %d, color type %d, and interlace type %d\n",
                    width, height, channels, bit_depth, color_type, interlace_type);

    // Initialize a BMP Image to hold the decompressed data
    int rc;
    BMPImage *pBMPImage = new BMPImage (true);
    if (0 != (rc = pBMPImage->initNewImage (width, height, 24))) {
        checkAndLogMsg ("JPEGLibWrapper::convertPNGToBMP", Logger::L_MildError,
                        "failed to initialize BMPImage; rc = %d\n", rc);
        delete pBMPImage;
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    png_bytepp row_pointers = png_get_rows (png_ptr, info_ptr);
    for (uint32 ui32Y = 0; ui32Y < height; ui32Y++) {
        for (uint32 ui32X = 0, ui32ByteIndex = 0; ui32X < width; ui32X++) {
            if (0 != (rc = pBMPImage->setPixel (ui32X, height - ui32Y - 1, row_pointers[ui32Y][ui32ByteIndex+2], row_pointers[ui32Y][ui32ByteIndex+1], row_pointers[ui32Y][ui32ByteIndex+0]))) {
                checkAndLogMsg ("JPEGLibWrapper::convertPNGToBMP", Logger::L_MildError,
                                "failed to set pixel (%lu,%lu); rc = %d\n", ui32X, ui32Y, rc);
                delete pBMPImage;
                png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
                return NULL;
            }
            ui32ByteIndex += channels;
        }
    }

    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

    return pBMPImage;
}

BufferReader * PNGLibWrapper::convertBMPtoPNG (const NOMADSUtil::BMPImage *pInputImage)
{
    if (pInputImage == NULL) {
        checkAndLogMsg ("PNGLibWrapper::convertBMPtoPNG", Logger::L_MildError,
                        "input image is NULL\n");
        return NULL;
    }
    else if ((pInputImage->getWidth() <= 0) || (pInputImage->getHeight() <= 0)) {
        checkAndLogMsg ("PNGLibWrapper::convertBMPtoPNG", Logger::L_MildError,
                        "input image is empty\n");
        return NULL;
    }
    else if (pInputImage->getBitsPerPixel() != 24) {
        checkAndLogMsg ("PNGLibWrapper::convertBMPtoPNG", Logger::L_MildError,
                        "cannot handle BMP with %d bits per pixel\n",
                        (int) pInputImage->getBitsPerPixel());
        return NULL;
    }

    png_structp png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, PNGLibWrapperNS::user_error_fn, PNGLibWrapperNS::user_warning_fn);
    if (png_ptr == NULL) {
        return NULL;
    }
    png_infop info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        return NULL;
    }
    if (setjmp (png_jmpbuf (png_ptr))) {
        checkAndLogMsg ("PNGLibWrapper::convertBMPtoPNG", Logger::L_MildError,
                        "libpng terminated with an error\n");
        png_destroy_write_struct (&png_ptr, &info_ptr);
        return NULL;
    }

    // Register functions to write PNG image to buffer
    PNGLibWrapperNS::OutputData state;
    state.pBuffer = NULL;
    state.size = 0;
    png_set_write_fn (png_ptr, &state, PNGLibWrapperNS::user_png_write_data, PNGLibWrapperNS::user_png_flush);

    static const uint8 PIXEL_SIZE = 3;
    const int depth = 8;

    // Write header
    if (setjmp (png_jmpbuf (png_ptr))) {
        checkAndLogMsg ("PNGLibWrapper::convertBMPtoPNG", Logger::L_MildError,
                        "libpng terminated with an error while writing header\n");
        png_destroy_write_struct (&png_ptr, &info_ptr);
        return NULL;
    }
    png_set_IHDR (png_ptr,
                  info_ptr,
                  pInputImage->getWidth(),
                  pInputImage->getHeight(),
                  depth,
                  PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);
    png_write_info (png_ptr, info_ptr);

    // Write image
    if (setjmp (png_jmpbuf (png_ptr))) {
        checkAndLogMsg ("PNGLibWrapper::convertBMPtoPNG", Logger::L_MildError,
                        "libpng terminated with an error while writing image\n");
        png_destroy_write_struct (&png_ptr, &info_ptr);
        return NULL;
    }

    // Initialize rows of PNG image
    png_byte **row_pointers = (png_byte **) png_malloc (png_ptr, pInputImage->getHeight() * sizeof (png_byte *));
    if (row_pointers == NULL) {
        return NULL;
    }
    for (unsigned int y = 0; y < pInputImage->getHeight(); ++y) {
        png_byte *row = (png_byte *) png_malloc (png_ptr, sizeof (uint8) * pInputImage->getWidth() * PIXEL_SIZE);
        const unsigned int actualY = pInputImage->getHeight () - y - 1;
        row_pointers[actualY] = row;
        for (unsigned int x = 0; x < pInputImage->getWidth(); ++x) {
            uint8 ui8Red = 0;
            uint8 ui8Green = 0;
            uint8 ui8Blue = 0;
            if (pInputImage->getPixel (x, y, &ui8Red, &ui8Green, &ui8Blue) != 0) {
                return NULL;
            }
            *row++ = ui8Red;
            *row++ = ui8Green;
            *row++ = ui8Blue;
        }
    }

    // Get Buffer
    // png_voidp puchOutputBuf = png_get_io_ptr (png_ptr);
    // sleepForMilliseconds (10000);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    // End Write
    if (setjmp (png_jmpbuf (png_ptr))) {
        checkAndLogMsg ("PNGLibWrapper::convertBMPtoPNG", Logger::L_MildError,
                        "libpng terminated with an error while writing image\n");
        png_destroy_write_struct (&png_ptr, &info_ptr);
        return NULL;
    }

    // Deallocate image
    for (unsigned int y = 0; y < pInputImage->getHeight(); y++) {
        png_free (png_ptr, row_pointers[y]);
    }
    png_free (png_ptr, row_pointers);

    return new BufferReader (state.pBuffer, state.size, true);
}

void PNGLibWrapperNS::user_error_fn (png_structp png_ptr, png_const_charp error_msg)
{
    checkAndLogMsg ("PNGLibWrapper::user_error_fn", Logger::L_MildError,
                    "libpng reports error: %s\n", error_msg);
}

void PNGLibWrapperNS::user_warning_fn (png_structp png_ptr, png_const_charp warning_msg)
{
    checkAndLogMsg ("PNGLibWrapper::user_warning_fn", Logger::L_MildError,
                    "libpng reports warning: %s\n", warning_msg);
}

void PNGLibWrapperNS::user_read_data (png_structp png_ptr, png_bytep data, png_size_t length)
{
    InputData *pInputData = (InputData*) png_get_io_ptr (png_ptr);
    if (length > (pInputData->ui32InputBufLen - pInputData->ui32CurrPos)) {
        checkAndLogMsg ("PNGLibWrapper::user_read_data", Logger::L_MildError,
                        "libpng wants to read %lu bytes but there are only (%lu-%lu) bytes available\n",
                        (unsigned long) length, pInputData->ui32InputBufLen, pInputData->ui32CurrPos);
        longjmp (png_jmpbuf (png_ptr), -1);
    }
    memcpy (data, pInputData->pui8InputBuf+pInputData->ui32CurrPos, length);
    pInputData->ui32CurrPos += (uint32) length;
}

void PNGLibWrapperNS::user_png_write_data (png_structp png_ptr, png_bytep data, png_size_t length)
{
    OutputData *pOutputData = (OutputData*) png_get_io_ptr (png_ptr);
    const size_t nsize = pOutputData->size + length;

    // allocate or increase buffer size
    pOutputData->pBuffer = (uint8 *) realloc (pOutputData->pBuffer, nsize);
    if (pOutputData->pBuffer == NULL) {
        png_error (png_ptr, "Write Error");
    }

    // copy new bytes to end of buffer
    memcpy ((pOutputData->pBuffer + pOutputData->size), data, length);
    pOutputData->size += length;
}

void PNGLibWrapperNS::user_png_flush (png_structp png_ptr)
{
}

