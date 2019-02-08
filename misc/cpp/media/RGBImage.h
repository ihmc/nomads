/*
 * RGBImage.h
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

#ifndef INCL_RGB_IMAGE_H
#define	INCL_RGB_IMAGE_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;

    class RGBImage
    {
        public:
            RGBImage (bool bEnablePadding = false);
            virtual ~RGBImage (void);

            // Initialize a new image
            int initNewImage (uint32 ui32Width, uint32 ui32Height, uint8 ui8BitsPerPixel);

            // Return the width of the image in pixels
            uint16 getWidth (void) const;

            // Return the height of the image in pixels
            uint16 getHeight (void) const;

            // Returns the number of bits per pixel in the image
            uint8 getBitsPerPixel (void) const;

            // Returns the length of the image array (without including the headers)
            uint32 getImageSize (void) const;

            // Returns the length of a line if the image array (without including the headers)
            uint32 getLineSize (void) const;

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

            int setImage (const void *pData, uint32 ui32Len);

            uint8 * getLinePtr (unsigned int line);
            const uint8 * getLinePtr (unsigned int line) const;

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

            // Compute the row padding - which is used to ensure that the size of each row of pixel data is a multiple of 4 bytes
            // NOTE: This does not update any class variables
            uint8 computeRowPadding (uint32 ui32ImageWidth, uint16 ui16BitsPerPixel) const;

            // Read the actual image bytes
            // Assumes both the Bitmap File Header and the Bitmap Information Header have both been
            // read and that the reader is pointing to the data section
            // Returns 0 if successful or a negative number in case of error
            //
            // NOTE: the image must be initialized before calling readRGBImageData()
            int readRGBImageData (Reader *pReader);

            // Write the actual image bytes
            // Returns 0 if successful or a negative number in case of error
            int writeRGBImageData (Writer *pWriter) const;

        private:
            const bool _bEnablePadding;
            uint8 _ui8RowPadding; // Must be 0, 1, 2, or 3 - to make each row be a multiple of 4 bytes
            uint16 _ui16BitsPerPixel;
            uint32 _ui32Width;
            uint32 _ui32Height;
            uint32 _ui32ImageSize;
            uint8 *_pui8Data;
    };
}

#endif	/* INCL_RGB_IMAGE_H */

