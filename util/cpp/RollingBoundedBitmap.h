/*
 * RollingBoundedBitmap.h
 *
 * This file is part of the IHMC Util Library
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
 * Maintains a bounded bitmap that only keeps track of the bits within a specified history window.
 * Assumes that any bits tested below the window are set.
 * Attempting to set a bit below the window is ignored.
 * Setting a bit above the window causes the window to slide to accommodate the new bit being set.
 * Bitmap history window size must be a multiple of 32 for efficiency reasons. If a different
 * number is specified, it is rounded up to the nearest multiple of 32.
 */

#ifndef INCL_ROLLING_BOUNDED_BITMAP_H
#define INCL_ROLLING_BOUNDED_BITMAP_H

#include "FTypes.h"

namespace NOMADSUtil
{

    class RollingBoundedBitmap
    {
        public:
            // Creates a rolling bounded bitmap with the specified number of bits
            // NOTE: Number of bits must be a multiple of 32 (or will increased to be a multiple of 32)
            RollingBoundedBitmap (uint32 ui32NumberOfBits);
            virtual ~RollingBoundedBitmap (void);

            void reset (void);

            void set (uint32 ui32Bit);
            bool isSet (uint32 ui32Bit);

            void display (void);

        protected:
            uint32 _ui32Floor;
            uint32 _ui32Ceiling;
            uint32 _ui32ArraySize;
            uint32 *_pui32Bits;
    };

}

#endif   // #ifndef INCL_ROLLING_BOUNDED_BITMAP_H
