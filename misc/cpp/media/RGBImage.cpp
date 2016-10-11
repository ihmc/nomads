/* 
 * RGBImage.cpp
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
 * Created on May 28, 2015, 5:47 PM
 */

#include "RGBImage.h"

#include "Writer.h"

#include <stdlib.h>
#include <string.h>

namespace NOMADSUtil
{
    // Compute the row padding - which is used to ensure that the size of each row of pixel data is a multiple of 4 bytes
    // NOTE: This does not update any class variables
    uint32 computeImgSize (uint32 ui32Width, uint32 ui32Height, uint8 ui8BitsPerPixel, uint8 ui8RowPadding)
    {
        return (ui32Width * (ui8BitsPerPixel / 8) + ui8RowPadding) * ui32Height;
    }
}

using namespace NOMADSUtil;

RGBImage::RGBImage (bool bEnablePadding)
    : _bEnablePadding (bEnablePadding),
      _ui8RowPadding (0),
      _ui16BitsPerPixel (0),
      _ui32Width (0U),
      _ui32Height (0U),
      _ui32ImageSize (0U),
      _pui8Data (NULL)
{
}

RGBImage::~RGBImage (void)
{
    if (_pui8Data) {
        free (_pui8Data);
        _pui8Data = NULL;
    }
    _ui8RowPadding = 0;
}

int RGBImage::initNewImage (uint32 ui32Width, uint32 ui32Height, uint8 ui8BitsPerPixel)
{
    if ((ui32Width == 0) || (ui32Height == 0)) {
        return -1;
    }

    _ui16BitsPerPixel = ui8BitsPerPixel;
    _ui8RowPadding = computeRowPadding (ui32Width, _ui16BitsPerPixel);
    _ui32Width = ui32Width;
    _ui32Height = ui32Height;
    const uint32 ui32OldImgSize = _ui32ImageSize;
    _ui32ImageSize = computeImgSize (_ui32Width, _ui32Height, _ui16BitsPerPixel, _ui8RowPadding);
    if (_pui8Data == NULL) {
        _pui8Data = static_cast<uint8*>(calloc (_ui32ImageSize, 1));
    }
    else if (ui32OldImgSize < _ui32ImageSize) {
        _pui8Data = static_cast<uint8*>(realloc (_pui8Data, _ui32ImageSize));
    }
    else {
        memset (_pui8Data, 0, _ui32ImageSize);
    }
    if (_pui8Data == NULL) {
        return -2;
    }

    return 0;
}

uint16 RGBImage::getWidth (void) const
{
    return _ui32Width;
}

uint16 RGBImage::getHeight (void) const
{
    return _ui32Height;
}

uint8 RGBImage::getBitsPerPixel (void) const
{
    return (uint8) _ui16BitsPerPixel;
}

uint32 RGBImage::getImageSize (void) const
{
    return _ui32ImageSize;
}

uint32 RGBImage::getLineSize (void) const
{
    return getImageSize() / getHeight();
}

const void * RGBImage::getImage() const
{
    return _pui8Data;
}

void * RGBImage::relinquishImage (void)
{
    void *pTmp = _pui8Data;
    _pui8Data = NULL;
    return pTmp;
}

int RGBImage::setPixel (uint32 ui32X, uint32 ui32Y, uint8 ui8Red, uint8 ui8Green, uint8 ui8Blue)
{
    if (_pui8Data == NULL) {
        // Uninitialized image
        return -1;
    }
    if ((ui32X >= _ui32Width) || (ui32Y >= _ui32Height)) {
        // Invalid pixel address
        return -2;
    }
    uint32 ui32StartingIndex = ui32Y * (_ui32Width * (_ui16BitsPerPixel / 8) + _ui8RowPadding) + ui32X * (_ui16BitsPerPixel / 8);
    if ((ui32StartingIndex+2) >= _ui32ImageSize) {
        // Should not have happened!
        return -3;
    }
    _pui8Data[ui32StartingIndex+0] = ui8Red;
    _pui8Data[ui32StartingIndex+1] = ui8Green;
    _pui8Data[ui32StartingIndex+2] = ui8Blue;
    return 0;
}

int RGBImage::setImage (const void *pData, uint32 ui32Len)
{
    if (pData == NULL) {
        return -1;
    }
    if (ui32Len != _ui32ImageSize) {
        return -2;
    }
    memcpy (_pui8Data, pData, ui32Len);
    return 0;
}

uint8 * RGBImage::getLinePtr (unsigned int line)
{
    uint32 ui32StartingIndex = line * (_ui32Width * (_ui16BitsPerPixel / 8) + _ui8RowPadding);
    if (ui32StartingIndex >= _ui32ImageSize) {
        return NULL;
    }
    return _pui8Data + ui32StartingIndex;
}

const uint8 * RGBImage::getLinePtr (unsigned int line) const
{
    uint32 ui32StartingIndex = line * (_ui32Width * (_ui16BitsPerPixel / 8) + _ui8RowPadding);
    if (ui32StartingIndex >= _ui32ImageSize) {
        return NULL;
    }
    return _pui8Data + ui32StartingIndex;
}

int RGBImage::getPixel (uint32 ui32X, uint32 ui32Y, uint8 *pui8Red, uint8 *pui8Green, uint8 *pui8Blue) const
{
    if (_pui8Data == NULL) {
        // Uninitialized image
        return -1;
    }
    if ((ui32X >= _ui32Width) || (ui32Y >= _ui32Height)) {
        // Invalid pixel address
        return -2;
    }
    uint32 ui32StartingIndex = ui32Y * (_ui32Width * (_ui16BitsPerPixel / 8) + _ui8RowPadding) + ui32X * (_ui16BitsPerPixel / 8);
    if ((ui32StartingIndex+2) >= _ui32ImageSize) {
        // Should not have happened!
        return -3;
    }
    if ((pui8Red == NULL) || (pui8Green == NULL) || (pui8Blue == NULL)) {
        return -4;
    }
    *pui8Red = _pui8Data[ui32StartingIndex+0];
    *pui8Green = _pui8Data[ui32StartingIndex+1];
    *pui8Blue = _pui8Data[ui32StartingIndex+2];
    return 0;
}

uint8 RGBImage::computeRowPadding (uint32 ui32ImageWidth, uint16 ui16BitsPerPixel) const
{
    uint8 ui8RowPadding;
    uint32 ui32RowSize = (ui32ImageWidth * (ui16BitsPerPixel / 8));
    if ((ui32RowSize % 4) == 0) {
        ui8RowPadding = 0;
    }
    else {
        ui8RowPadding = 4 - (ui32RowSize % 4);
    }
    return ui8RowPadding;
}

int RGBImage::readRGBImageData (Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }
    if (_ui32ImageSize == 0) {
        return -2;
    }
    if (_pui8Data == NULL) {
        return -3;
    }
    int rc = pReader->readBytes (_pui8Data, _ui32ImageSize);
    if (rc < 0) {
        return -4;
    }
    return rc;
}

int RGBImage::writeRGBImageData (Writer *pWriter) const
{
    if (pWriter == NULL) {
        return -1;
    }
    if (_pui8Data == NULL) {
        return -2;
    }
    if (_ui32ImageSize == 0) {
        return -3;
    }
    if (pWriter->writeBytes (_pui8Data, _ui32ImageSize) < 0) {
        return -4;
    }
    return 0;
}

