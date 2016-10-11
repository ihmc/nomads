/*
 * PPMImage.cpp
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
 * Created on July 21, 2015, 2:50 PM
 */

#include "PPMImage.h"

#include "LineOrientedReader.h"
#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"
#include "StringTokenizer.h"
#include "Writer.h"

#include <stdio.h>
#include <string.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg
#define fail(methodName,rc) Logger::L_MildError, "%s failed with rc = %d\n", methodName, rc

using namespace NOMADSUtil;

PPMImage::PPMImage (bool bEnablePadding)
    : _bEnablePadding (bEnablePadding),
      _ui32Width (0U),
      _ui32Height (0U)
{
}

PPMImage::~PPMImage(void)
{
}

int PPMImage::initNewImage (uint32 ui32Width, uint32 ui32Height, uint8 ui8BitsPerPixel)
{
    if ((ui32Width == 0) || (ui32Height == 0)) {
        return -1;
    }
    if (_img.initNewImage (ui32Width, ui32Height, ui8BitsPerPixel) < 0) {
        return -2;
    }
    _ui32Width = ui32Width;
    _ui32Height = ui32Height;

    return 0;
}

PPMImage & PPMImage::operator = (const PPMImage &ppmImg)
{
    return operator = (ppmImg._img);
}

PPMImage & PPMImage::operator = (const RGBImage &rgbImg)
{
    if (initNewImage (rgbImg.getWidth(), rgbImg.getHeight(), rgbImg.getBitsPerPixel()) < 0) {
        return *this;
    }
    _img.setImage (rgbImg.getImage(), rgbImg.getImageSize());
    return *this;
}

int PPMImage::writeHeader (Writer *pWriter) const
{
    const char *pszMethodName = "PPMImage::writeHeader";
    int rc;
    if (pWriter == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "pWriter is NULL\n");
        return -1;
    }
    if ((rc = writePPMFileHeader (pWriter)) < 0) {
        checkAndLogMsg (pszMethodName, fail ("writeBMPFileHeader()", rc));
        return -2;
    }
    if ((rc = writePPMInformationHeader (pWriter)) < 0) {
        checkAndLogMsg(pszMethodName, fail ("writeBMPInformationHeader()", rc));
        return -3;
    }
    return 0;
}

int PPMImage::writeHeaderAndImage (Writer *pWriter) const
{
    const char *pszMethodName = "PPMImage::writeHeaderAndImage";
    int rc;
    if (pWriter == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "pWriter is NULL\n");
        return -1;
    }
    if ((rc = writeHeader(pWriter)) < 0) {
        checkAndLogMsg (pszMethodName, fail("writeHeader()", rc));
        return -2;
    }
    if ((rc = _img.writeRGBImageData (pWriter)) < 0) {
        checkAndLogMsg (pszMethodName, fail("writeBMPImageData()", rc));
        return -3;
    }
    return 0;
}

uint16 PPMImage::getWidth (void) const
{
    return _ui32Width;
}

uint16 PPMImage::getHeight (void) const
{
    return _ui32Height;
}

uint8 PPMImage::getBitsPerPixel (void) const
{
    return 1;
}

uint32 PPMImage::getTotalSize (void) const
{
    return getImageSize();
}

uint32 PPMImage::getImageSize (void) const
{
    return (_ui32Width * _ui32Height);
}

const RGBImage * PPMImage::getImage (void) const
{
    return &_img;
}

const void * PPMImage::getImageBytes (void) const
{
    return _img.getImage();
}

void * PPMImage::relinquishImage (void)
{
    return _img.relinquishImage();
}

int PPMImage::setPixel (uint32 ui32X, uint32 ui32Y, uint8 ui8Red, uint8 ui8Green, uint8 ui8Blue)
{
    return _img.setPixel (ui32X, ui32Y, ui8Red, ui8Green, ui8Blue);
}

int PPMImage::getPixel (uint32 ui32X, uint32 ui32Y, uint8 *pui8Red, uint8 *pui8Green, uint8 *pui8Blue) const
{
    return _img.getPixel (ui32X, ui32Y, pui8Red, pui8Green, pui8Blue);
}

int PPMImage::readPPMFileHeader (Reader *pReader) const
{
    if (pReader == NULL) {
        return -1;
    }
    char header[1024];
    LineOrientedReader lr (pReader, false);
    for (int rc; (rc = lr.readLine (header, 1024)) < 0; ) {
        if (header[0] != '#') {
            // if not a comment, it must be the header
            if (rc < 2) {
                return -2;
            }
            String sHeader (header);
            sHeader.trim();
            if (sHeader != "P6") {
                return -3;
            }
            return 0;
        }
    }
    return -4;
}

int PPMImage::writePPMFileHeader (Writer *pWriter) const
{
    if (pWriter == NULL) {
        return -1;
    }

    uint8 header[3] = { 'P', '6', '\n' };
    if (pWriter->writeBytes (header, 3) < 0) {
        return -2;
    }

    return 0;
}

int PPMImage::readPPMInformationHeader (Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }

    StringTokenizer tokenizer;
    unsigned int iNonComment = 0;
    char header[1024];
    LineOrientedReader lr(pReader, false);
    for (int rc; (rc = lr.readLine (header, 1024)) < 0;) {
        if (header[0] != '#') {
            switch (iNonComment) {
                // if not a comment, it must be the header
                case 0: {
                    tokenizer.init (header);
                    const char *pszWidth = tokenizer.getNextToken();
                    if (pszWidth == NULL) {
                        return -2;
                    }
                    const char *pszHeight = tokenizer.getNextToken();
                    if (pszHeight == NULL) {
                        return -3;
                    }
                    _ui32Width = atoui32 (pszWidth);
                    _ui32Height = atoui32 (pszHeight);
                }

                case 1: {
                    tokenizer.init (header);
                    const String s255 = (tokenizer.getNextToken());
                    if (s255 != "255") {
                        return -4;
                    }
                    return 0;
                }

                default:
                    break;
            }
            iNonComment++;
        }
    }
    return -5;
}

int PPMImage::writePPMInformationHeader (Writer *pWriter) const
{
    if (pWriter == NULL) {
        return -1;
    }
    char header[128];
    sprintf (header, "%d %d\n255\n", _ui32Width, _ui32Height);
    if (pWriter->writeBytes(header, strlen(header)) < 0) {
        return -2;
    }
    return 0;
}

