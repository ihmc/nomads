/* 
 * BoundingBox.h
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Created on August 12, 2014, 6:37 PM
 */

#ifndef INCL_BOUNDING_BOX_H
#define	INCL_BOUNDING_BOX_H

#include "MetadataInterface.h"

namespace IHMC_ACI
{
    class Point
    {
        public:
            Point (const Point &point);
            Point (float fLatitude, float fLongitude);
            ~Point (void);

         public:
             const float _fLatitude;
             const float _fLongitude;
    };

    class BoundingBox
    {
        public:
            BoundingBox (float leftUpperLatitude, float leftUpperLongitude,
                         float rightLowerLatitude, float rightLowerLongitude);
            BoundingBox (const BoundingBox &bbox);
            ~BoundingBox (void);

            float getArea (void) const;
            Point getBaricenter (void) const;
            bool isValid (void) const;

            static BoundingBox getBoundingBox (MetadataInterface &metadata, uint32 ui32RangeOfInfluence);
            static BoundingBox getBoundingBox (float fLatitude, float fLongitude, uint32 ui32RangeOfInfluence);

            const bool _bEmpty;
            const float _leftUpperLatitude;
            const float _leftUpperLongitude;
            const float _rightLowerLatitude;
            const float _rightLowerLongitude;

        private:
            BoundingBox (void);
            
    };
}

#endif	/* INCL_BOUNDING_BOX_H */

