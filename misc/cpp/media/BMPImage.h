/*
 * BMPImage.h
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

#ifndef INCL_BMP_IMAGE_H
#define INCL_BMP_IMAGE_H

#include "RGBImage.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;

    class BMPImage
    {
        public:
            BMPImage (bool bEnablePadding);
            virtual ~BMPImage (void);

            // Read the Bitmap File Header and the Bitmap Information Header
            // Returns 0 if successful or a negative number in case of error
            int readHeader (Reader *pReader);

            // Read the Bitmap File Header, Bitmap Information Header, and the data for the image
            // Returns 0 if successful or a negative number in case of error
            int readHeaderAndImage (Reader *pReader);

            // Initialize a new image
            int initNewImage (uint32 ui32Width, uint32 ui32Height, uint8 ui8BitsPerPixel);
            BMPImage & operator = (const BMPImage &bmpImg);

            // Write the Bitmap File Header and the Bitmap Information Header
            // Returns 0 if successful or a negative number in case of error
            int writeHeader (Writer *pWriter) const;

            // Write the Bitmap File Header, Bitmap Information Header, and the data for the image
            // Returns 0 if successful or a negative number in case of error
            int writeHeaderAndImage (Writer *pWriter) const;

            // Return the width of the image in pixels
            uint16 getWidth (void) const;

            // Return the height of the image in pixels
            uint16 getHeight (void) const;

            // Returns the number of bits per pixel in the image
            uint8 getBitsPerPixel (void) const;

            // Returns the size of a line of the BMP
            uint32 getLineSize (void) const;

            // Returns the total size of the BMP (should it be written out)
            uint32 getTotalSize (void) const;

            // Returns the length of the image array (without including the headers)
            uint32 getImageSize (void) const;

            // Returns the image array
            const void * getImage (void) const;

            // Relinquishes and returns the image array
            // NOTE: The caller is then responsible for deallocating the memory
            void * relinquishImage (void);

            // Set the pixel value based on the specified red, green, and blue components
            // NOTE: Following the BitMap specification, this method assumes that the 0 coordinate
            //       is at the bottom left of the image, not the top left
            // NOTE: RGBA or RGB pixels should be stored in the common order RGBARGBARGBA... or RGBRGBRGBRGB...,
            //       from top to bottom, without any special byte boundaries. BMP images and bitmaps in win32
            //       use a different format. They do three things differently: Instead of being from top to
            //       bottom, it goes from bottom to top (upside down). Instead of using the order red, green, blue,
            //       it uses the order blue, green, red, or BGRBGRBGR..., (can result in wrong colors).
            //       It has a limitation where rows always must be a multiple of 4 bytes, so if the width of the
            //       image is not a multiple of 4 some unused bytes are introduced at the end of each row (can result
            //       in a skewed image).
            int setPixel (uint32 ui32X, uint32 ui32Y, uint8 ui8Red, uint8 ui8Green, uint8 ui8Blue);

            // Get the pixel value for the specified pixel
            // NOTE: Following the BitMap specification, this method assumes that the 0 coordinate
            //       is at the bottom left of the image, not the top left
            // NOTE: RGBA or RGB pixels should be stored in the common order RGBARGBARGBA... or RGBRGBRGBRGB...,
            //       from top to bottom, without any special byte boundaries. BMP images and bitmaps in win32
            //       use a different format. They do three things differently: Instead of being from top to
            //       bottom, it goes from bottom to top (upside down). Instead of using the order red, green, blue,
            //       it uses the order blue, green, red, or BGRBGRBGR..., (can result in wrong colors).
            //       It has a limitation where rows always must be a multiple of 4 bytes, so if the width of the
            //       image is not a multiple of 4 some unused bytes are introduced at the end of each row (can result
            //       in a skewed image).
            int getPixel (uint32 ui32X, uint32 ui32Y, uint8 *pui8Red, uint8 *pui8Green, uint8 *pui8Blue) const;

            static const uint16 BMP_MAGIC_NUMBER = 16973;
            static const uint16 NUMBER_OF_PLANES = 1;

            struct BMPFileHeader
            {
                uint16 ui16Signature;
                uint32 ui32FileSize;
                uint16 ui16Reserved1;
                uint16 ui16Reserved2;
                uint32 ui32Offset;

                BMPFileHeader (void);
                uint32 size (void) const;
            };

            struct BMPInformationHeader
            {
                uint32 ui32HeaderSize;
                uint32 ui32Width;
                uint32 ui32Height;
                uint16 ui16Planes;
                uint16 ui16BitsPerPixel;
                uint32 ui32Compression;
                uint32 ui32ImageSize;
                uint32 ui32XPelsPerMeter;
                uint32 ui32YPelsPerMeter;
                uint32 ui32ClrUsed;
                uint32 ui32ClrImportant;

                BMPInformationHeader (void);
                uint32 size (void) const;
            };

        private:
            // Read the BMP File Header
            // Returns 0 if successful or a negative number in case of error
            int readBMPFileHeader (Reader *pReader);

            // Write the BMP File Header
            // Returns 0 if successful or a negative number in case of error
            int writeBMPFileHeader (Writer *pWriter) const;

            // Read the BMP Information Header
            // Assumes the Bitmap File Header has already been read and the
            // reader is pointing to the Bitmap Information Header section
            // Returns 0 if successful or a negative number in case of error
            int readBMPInformationHeader (Reader *pReader);

            // Write the BMP Information Header
            // Returns 0 if successful or a negative number in case of error
            int writeBMPInformationHeader (Writer *pWriter) const;

            // Determines whether the bitcount specified is valid and supported by this class
            bool isValidBitCount (uint16 ui16BitCount) const;

        private:
            BMPFileHeader _bfh;
            BMPInformationHeader _bih;
            RGBImage _img;
    };
}

#endif   // #ifndef INCL_BMP_IMAGE_H
