/*
 * BMPImage.cpp
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

#include "BMPImage.h"

#include "Logger.h"
#include "Reader.h"
#include "Writer.h"

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace NOMADSUtil
{
    bool isValidBitCount (uint16 ui16BitsPerPixel)
    {
        switch (ui16BitsPerPixel) {
        case 0:   // The number of bits-per-pixel is specified or is implied by
            // the JPEG or PNG format.
        case 1:   // The bitmap is monochrome
        case 4:   // The bitmap has a maximum of 16 colors
        case 8:   // The bitmap has a maximum of 256 colors
        case 16:  // The bitmap has a maximum of 2^16 colors
            return false;

        case 24:  // The bitmap has a maximum of 2^24 colors
            return true;

        case 32:  // The bitmap has a maximum of 2^32 colors
            return false;

        default:
            return false;
        }
    }
}

BMPImage::BMPImage (bool bEnablePadding)
    : _img (bEnablePadding)
{
}

BMPImage::~BMPImage (void)
{
}

int BMPImage::readHeader (Reader *pReader)
{
    int rc;
    if (pReader == NULL) {
        checkAndLogMsg ("BMPImage::readHeader", Logger::L_MildError,
                        "pReader is NULL\n");
        return -1;
    }
    if ((rc = readBMPFileHeader (pReader)) < 0) {
        checkAndLogMsg ("BMPImage::readHeader", Logger::L_MildError,
                        "readBMPFileHeader() failed with rc = %d\n", rc);
        return -2;
    }
    if ((rc = readBMPInformationHeader (pReader)) < 0) {
        checkAndLogMsg ("BMPImage::readHeader", Logger::L_MildError,
                        "readBMPInformationHeader() failed with rc = %d\n", rc);
        return -3;
    }
    return 0;
}

int BMPImage::readHeaderAndImage (Reader *pReader)
{
    int rc;
    if (pReader == NULL) {
        checkAndLogMsg ("BMPImage::readHeaderAndImage", Logger::L_MildError,
                        "pReader is NULL\n");
        return -1;
    }
    if ((rc = readHeader (pReader)) < 0) {
        checkAndLogMsg ("BMPImage::readHeaderAndImage", Logger::L_MildError,
                        "readHeader() failed with rc = %d\n", rc);
        return -2;
    }
    _img.initNewImage (_bih.ui32Width, _bih.ui32Height, _bih.ui16BitsPerPixel);
    if ((rc = _img.readRGBImageData (pReader)) < 0) {
        checkAndLogMsg ("BMPImage::readHeaderAndImage", Logger::L_MildError,
                        "readBMPImageData() failed with rc = %d\n", rc);
        return -3;
    }
    return 0;
}

int BMPImage::initNewImage (uint32 ui32Width, uint32 ui32Height, uint8 ui8BitsPerPixel)
{
    if ((ui32Width == 0) || (ui32Height == 0) || (!isValidBitCount(ui8BitsPerPixel))) {
        return -1;
    }
    if (_img.initNewImage (ui32Width, ui32Height, ui8BitsPerPixel) < 0) {
        return -2;
    } 

    _bih.ui32HeaderSize = _bih.size(); // Should be 40
    _bih.ui32Width = ui32Width;
    _bih.ui32Height = ui32Height;
    _bih.ui16Planes =  NUMBER_OF_PLANES;
    _bih.ui16BitsPerPixel = ui8BitsPerPixel;
    _bih.ui32Compression =  0;
    _bih.ui32ImageSize = _img.getImageSize();
    _bih.ui32XPelsPerMeter = 0;
    _bih.ui32YPelsPerMeter = 0;
    _bih.ui32ClrUsed =  0;
    _bih.ui32ClrImportant =  0;

    _bfh.ui16Signature = BMP_MAGIC_NUMBER;
    _bfh.ui32FileSize = _bfh.size() + _bih.size() + _bih.ui32ImageSize;
    _bfh.ui16Reserved1 = 0;
    _bfh.ui16Reserved2 = 0;
    _bfh.ui32Offset   = _bfh.size() + _bih.size();    // Should be 55

    return 0;
}

BMPImage & BMPImage::operator = (const BMPImage &bmpImg)
{
    if (initNewImage (bmpImg.getWidth(), bmpImg.getHeight(), bmpImg.getBitsPerPixel()) < 0) {
        return *this;
    }
    _img.setImage (bmpImg._img.getImage(), bmpImg._img.getImageSize());
    return *this;
}

int BMPImage::writeHeader (Writer *pWriter) const
{
    int rc;
    if (pWriter == NULL) {
        checkAndLogMsg ("BMPImage::writeHeader", Logger::L_MildError,
                        "pWriter is NULL\n");
        return -1;
    }
    if ((rc = writeBMPFileHeader (pWriter)) < 0) {
        checkAndLogMsg ("BMPImage::writeHeader", Logger::L_MildError,
                        "writeBMPFileHeader() failed with rc = %d\n", rc);
        return -2;
    }
    if ((rc = writeBMPInformationHeader (pWriter)) < 0) {
        checkAndLogMsg ("BMPImage::writeHeader", Logger::L_MildError,
                        "writeBMPInformationHeader() failed with rc = %d\n", rc);
        return -3;
    }
    return 0;
}

int BMPImage::writeHeaderAndImage (Writer *pWriter) const
{
    int rc;
    if (pWriter == NULL) {
        checkAndLogMsg ("BMPImage::writeHeaderAndImage", Logger::L_MildError,
                        "pWriter is NULL\n");
        return -1;
    }
    if ((rc = writeHeader (pWriter)) < 0) {
        checkAndLogMsg ("BMPImage::writeHeaderAndImage", Logger::L_MildError,
                        "writeHeader() failed with rc = %d\n", rc);
        return -2;
    }
    if ((rc = _img.writeRGBImageData (pWriter)) < 0) {
        checkAndLogMsg ("BMPImage::writeHeaderAndImage", Logger::L_MildError,
                        "writeBMPImageData() failed with rc = %d\n", rc);
        return -3;
    }
    return 0;
}

uint16 BMPImage::getWidth() const
{
    return _bih.ui32Width;
}

uint16 BMPImage::getHeight() const
{
    return _bih.ui32Height;
}

uint8 BMPImage::getBitsPerPixel() const
{
    return (uint8) _bih.ui16BitsPerPixel;
}

uint32 BMPImage::getLineSize (void) const
{
    return getImageSize () / getHeight();
}

uint32 BMPImage::getTotalSize (void) const
{
    return getImageSize() + _bfh.size() + _bih.size();
}

uint32 BMPImage::getImageSize() const
{
    return _bih.ui32ImageSize;
}

const void * BMPImage::getImage() const
{
    return _img.getImage();
}

void * BMPImage::relinquishImage (void)
{
    return _img.relinquishImage();
}

int BMPImage::setPixel (uint32 ui32X, uint32 ui32Y, uint8 ui8Red, uint8 ui8Green, uint8 ui8Blue)
{
    return _img.setPixel (ui32X, ui32Y, ui8Red, ui8Green, ui8Blue);
}

int BMPImage::getPixel (uint32 ui32X, uint32 ui32Y, uint8 *pui8Red, uint8 *pui8Green, uint8 *pui8Blue) const
{
    return _img.getPixel (ui32X, ui32Y, pui8Red, pui8Green, pui8Blue);
}

int BMPImage::readBMPFileHeader (Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }
    if (pReader->read16 (&_bfh.ui16Signature) < 0) {  // Signature is not read in Little-Endian form
        return -2;
    }
    if (_bfh.ui16Signature != BMP_MAGIC_NUMBER) {     // BM in ASCII
        return -3;
    }
    if (pReader->readLE32 (&_bfh.ui32FileSize) < 0 ||
        pReader->readLE16 (&_bfh.ui16Reserved1) < 0 ||
        pReader->readLE16 (&_bfh.ui16Reserved2) < 0 ||
        pReader->readLE32 (&_bfh.ui32Offset) < 0) {
        return -4;
    }
    return 0;
}

int BMPImage::writeBMPFileHeader (Writer *pWriter) const
{
    if (pWriter == NULL) {
        return -1;
    }
    BMPFileHeader bfh = _bfh;     // To get around this method being const
    if (pWriter->write16 (&bfh.ui16Signature) < 0 ||      // NOTE: Signature is not written in Little-Endian Form
        pWriter->writeLE32 (&bfh.ui32FileSize) < 0 ||
        pWriter->writeLE16 (&bfh.ui16Reserved1) < 0 ||
        pWriter->writeLE16 (&bfh.ui16Reserved2) < 0 ||
        pWriter->writeLE32 (&bfh.ui32Offset) < 0) {
        return -2;
    }
    return 0;
}

int BMPImage::readBMPInformationHeader (Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }
    if (pReader->readLE32 (&_bih.ui32HeaderSize) < 0 ||
        pReader->readLE32 (&_bih.ui32Width) < 0 ||
        pReader->readLE32 (&_bih.ui32Height) < 0 ||
        pReader->readLE16 (&_bih.ui16Planes) < 0) {
        return -2;
    }
    if (_bih.ui16Planes != NUMBER_OF_PLANES) {
        return -3;
    }
    if (pReader->readLE16 (&_bih.ui16BitsPerPixel) < 0) {
        return -4;
    }
    if (!isValidBitCount (_bih.ui16BitsPerPixel)) {
        return -5;
    }
    if (pReader->readLE32 (&_bih.ui32Compression) < 0 ||
        pReader->readLE32 (&_bih.ui32ImageSize) < 0 ||
        pReader->readLE32 (&_bih.ui32XPelsPerMeter) < 0 ||
        pReader->readLE32 (&_bih.ui32YPelsPerMeter) < 0 ||
        pReader->readLE32 (&_bih.ui32ClrUsed) < 0 ||
        pReader->readLE32 (&_bih.ui32ClrImportant) < 0) {
        return -6;
    }

    return 0;
}

int BMPImage::writeBMPInformationHeader (Writer *pWriter) const
{
    if (pWriter == NULL) {
        return -1;
    }
    BMPInformationHeader bih = _bih;        // To get around this method being const
    if (pWriter->writeLE32 (&bih.ui32HeaderSize) < 0 ||
        pWriter->writeLE32 (&bih.ui32Width) < 0 ||
        pWriter->writeLE32 (&bih.ui32Height) < 0 ||
        pWriter->writeLE16 (&bih.ui16Planes) < 0 ||
        pWriter->writeLE16 (&bih.ui16BitsPerPixel) < 0 ||
        pWriter->writeLE32 (&bih.ui32Compression) < 0 ||
        pWriter->writeLE32 (&bih.ui32ImageSize) < 0 ||
        pWriter->writeLE32 (&bih.ui32XPelsPerMeter) < 0 ||
        pWriter->writeLE32 (&bih.ui32YPelsPerMeter) < 0 ||
        pWriter->writeLE32 (&bih.ui32ClrUsed) < 0 ||
        pWriter->writeLE32 (&bih.ui32ClrImportant) < 0) {
        return -2;
    }
    return 0;
}

bool BMPImage::isValidBitCount (uint16 ui16BitsPerPixel) const
{
    switch (ui16BitsPerPixel) {
        case 0:   // The number of bits-per-pixel is specified or is implied by
                  // the JPEG or PNG format.
        case 1:   // The bitmap is monochrome
        case 4:   // The bitmap has a maximum of 16 colors
        case 8:   // The bitmap has a maximum of 256 colors
        case 16:  // The bitmap has a maximum of 2^16 colors
            return false;

        case 24:  // The bitmap has a maximum of 2^24 colors
            return true;

        case 32:  // The bitmap has a maximum of 2^32 colors
            return false;

        default:
            return false;
    }
}

BMPImage::BMPFileHeader::BMPFileHeader (void)
{
    ui16Signature = 0;
    ui32FileSize = 0;
    ui16Reserved1 = 0;
    ui16Reserved2 = 0;
    ui32Offset = 0;
}

uint32 BMPImage::BMPFileHeader::size (void) const
{
    return  sizeof (ui16Signature)      +
            sizeof (ui32FileSize)      +
            sizeof (ui16Reserved1) +
            sizeof (ui16Reserved2) +
            sizeof (ui32Offset);
}

BMPImage::BMPInformationHeader::BMPInformationHeader (void)
{
    ui32HeaderSize = 0;
    ui32Width = 0;
    ui32Height = 0;
    ui16Planes = 0;
    ui16BitsPerPixel = 0;
    ui32Compression = 0;
    ui32ImageSize = 0;
    ui32XPelsPerMeter = 0;
    ui32YPelsPerMeter = 0;
    ui32ClrUsed = 0;
    ui32ClrImportant = 0;
}

uint32 BMPImage::BMPInformationHeader::size (void) const
{
    return  sizeof (ui32HeaderSize)    +
            sizeof (ui32Width)         +
            sizeof (ui32Height)        +
            sizeof (ui16Planes)        +
            sizeof (ui16BitsPerPixel)  +
            sizeof (ui32Compression)   +
            sizeof (ui32ImageSize)     + 
            sizeof (ui32XPelsPerMeter) +
            sizeof (ui32YPelsPerMeter) +
            sizeof (ui32ClrUsed)       +
            sizeof (ui32ClrImportant);
}
