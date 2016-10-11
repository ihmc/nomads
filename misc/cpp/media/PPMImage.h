/*
 * PPMImage.h
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

#ifndef INCL_PPM_IMAGE_H
#define INCL_PPM_IMAGE_H

#include "RGBImage.h"

namespace NOMADSUtil
{
    class PPMImage
    {
        public:
            PPMImage (bool bEnablePadding = false);
            ~PPMImage (void);

            int initNewImage (uint32 ui32Width, uint32 ui32Height, uint8 ui8BitsPerPixel);
            PPMImage & operator = (const PPMImage &ppmImg);
            PPMImage & operator = (const RGBImage &rgbImg);

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

            // Returns the total size of the PPM (should it be written out)
            uint32 getTotalSize (void) const;

            // Returns the length of the image array (without including the headers)
            uint32 getImageSize (void) const;

            const RGBImage * getImage (void) const;

            // Returns the image array
            const void * getImageBytes (void) const;

            // Relinquishes and returns the image array
            // NOTE: The caller is then responsible for deallocating the memory
            void * relinquishImage (void);

            // Set the pixel value based on the specified red, green, and blue components
            int setPixel (uint32 ui32X, uint32 ui32Y, uint8 ui8Red, uint8 ui8Green, uint8 ui8Blue);

            // Get the pixel value for the specified pixel
            int getPixel (uint32 ui32X, uint32 ui32Y, uint8 *pui8Red, uint8 *pui8Green, uint8 *pui8Blue) const;

            static const uint16 BMP_MAGIC_NUMBER = 16973;

       private:
           // Write the PPM File Header
           // Returns 0 if successful or a negative number in case of error
           int readPPMFileHeader (Reader *pReader) const;
           int readPPMInformationHeader (Reader *pReader);

            // Write the PPM File Header
            // Returns 0 if successful or a negative number in case of error
            int writePPMFileHeader (Writer *pWriter) const;
            int writePPMInformationHeader (Writer *pWriter) const;

        private:
            const bool _bEnablePadding;
            uint32 _ui32Width;
            uint32 _ui32Height;
            RGBImage _img;
    };
}

#endif  // INCL_PPM_IMAGE_H

