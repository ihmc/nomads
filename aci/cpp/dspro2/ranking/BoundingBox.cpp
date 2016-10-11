/* 
 * BoundingBox.cpp
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

#include "BoundingBox.h"

#include "GeoUtils.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    float validateLatitude (float fLatitude)
    {
        if (fLatitude > MAX_LATITUDE) {
            return MAX_LATITUDE;
        }
        else if (fLatitude < MIN_LATITUDE) {
            return MIN_LATITUDE;
        }
        return fLatitude;
    }

    float validateLongitude (float fLongitude)
    {
        if (fLongitude > MAX_LONGITUDE) {
            return MAX_LONGITUDE;
        }
        else if (fLongitude < MIN_LONGITUDE) {
            return MIN_LONGITUDE;
        }
        return fLongitude;
    }
}

Point::Point (const Point &point)
    : _fLatitude (point._fLatitude),
      _fLongitude (point._fLongitude)
{
}

Point::Point (float fLatitude, float fLongitude)
    : _fLatitude (fLatitude),
      _fLongitude (fLongitude)
{
}

Point::~Point (void)
{
    
}
        
BoundingBox::BoundingBox (void)
    : _bEmpty (true), _leftUpperLatitude (0U), _leftUpperLongitude (0U),
      _rightLowerLatitude (0U), _rightLowerLongitude (0U)
{
}

BoundingBox::BoundingBox (float leftUpperLatitude, float leftUpperLongitude, float rightLowerLatitude, float rightLowerLongitude)
    : _bEmpty (!isValidLatitude (leftUpperLatitude) || !isValidLongitude (leftUpperLongitude) ||
                   !isValidLatitude (rightLowerLatitude) || !isValidLongitude (rightLowerLongitude)),
      _leftUpperLatitude (leftUpperLatitude), _leftUpperLongitude (leftUpperLongitude),
      _rightLowerLatitude (rightLowerLatitude), _rightLowerLongitude (rightLowerLongitude)
{
}

BoundingBox::BoundingBox (const BoundingBox &bbox)
    : _bEmpty (bbox._bEmpty), _leftUpperLatitude (bbox._leftUpperLatitude),
      _leftUpperLongitude (bbox._leftUpperLongitude), _rightLowerLatitude (bbox._rightLowerLatitude),
      _rightLowerLongitude (bbox._rightLowerLongitude)
{    
}

BoundingBox::~BoundingBox()
{
}

float BoundingBox::getArea (void) const
{
    return fabsf ((_leftUpperLatitude - _rightLowerLatitude) *
            (_rightLowerLongitude - _leftUpperLongitude));
}

Point BoundingBox::getBaricenter (void) const
{
    float fLat = _rightLowerLatitude + (fabs (_leftUpperLatitude - _rightLowerLatitude) / 2.0f);
    float fLon = _leftUpperLongitude + (fabs (_rightLowerLongitude - _leftUpperLongitude) / 2.0f);
    return Point (fLat, fLon);
}

bool BoundingBox::isValid (void) const
{
    return !_bEmpty;
}

BoundingBox BoundingBox::getBoundingBox (MetadataInterface &metadata, uint32 ui32RangeOfInfluence)
{
    float fLatitude, fLongitude;
    if (0 != metadata.getFieldValue (MetadataInterface::LATITUDE, &fLatitude)) {
        fLatitude = MetadataInterface::LEFT_UPPER_LATITUDE_UNSET;
    }
    if (0 != metadata.getFieldValue (MetadataInterface::LONGITUDE, &fLongitude)) {
        fLongitude = MetadataInterface::LEFT_UPPER_LONGITUDE_UNSET;
    }
    BoundingBox bbox1 = getBoundingBox (fLatitude, fLongitude, ui32RangeOfInfluence);

    float leftUpperLatitude, leftUpperLongitude, rightLowerLatitude, rightLowerLongitude;
    if (0 != metadata.getFieldValue (MetadataInterface::LEFT_UPPER_LATITUDE, &leftUpperLatitude)) {
        leftUpperLatitude = MetadataInterface::LEFT_UPPER_LATITUDE_UNSET;
    }
    if (0 != metadata.getFieldValue (MetadataInterface::LEFT_UPPER_LONGITUDE, &leftUpperLongitude)) {
        leftUpperLongitude = MetadataInterface::LEFT_UPPER_LONGITUDE_UNSET;
    }
    if (0 != metadata.getFieldValue (MetadataInterface::RIGHT_LOWER_LATITUDE, &rightLowerLatitude)) {
        rightLowerLatitude = MetadataInterface::LEFT_UPPER_LATITUDE_UNSET;
    }
    if (0 != metadata.getFieldValue (MetadataInterface::RIGHT_LOWER_LONGITUDE, &rightLowerLongitude)) {
        rightLowerLongitude = MetadataInterface::LEFT_UPPER_LONGITUDE_UNSET;
    }
    BoundingBox bbox2 (leftUpperLatitude, leftUpperLongitude, rightLowerLatitude, rightLowerLongitude);

    if (bbox2.isValid() && (ui32RangeOfInfluence > 0U)) {
        Point baricenter (bbox2.getBaricenter());
        BoundingBox bbox3 = getBoundingBox (baricenter._fLatitude, baricenter._fLongitude, ui32RangeOfInfluence);
        if (bbox3.isValid() && (bbox3.getArea() > bbox2.getArea())) {
            return bbox3;
        }
        return bbox2;
    }

    if (bbox1.isValid() && bbox2.isValid()) {
        assert (false); // this case should never happen
        if (bbox1.getArea() > bbox2.getArea()) {
            return bbox1;
        }
        return bbox2;
    }
    if (bbox1.isValid()) {
        return bbox1;
    }
    return bbox2;
}

BoundingBox BoundingBox::getBoundingBox (float fLatitude, float fLongitude, uint32 ui32RangeOfInfluence)
{
    if (!isValidLatitude (fLatitude) || !isValidLongitude (fLongitude)) {
        return BoundingBox();
    }
    double dLatDegDisplacement = 0.0;
    double dLonDegDisplacement = 0.0;
    metersToLatitudeDegrees (fLatitude, ui32RangeOfInfluence, dLatDegDisplacement, dLonDegDisplacement);
    float leftUpperLatitude = validateLatitude (fLatitude + dLatDegDisplacement);
    float leftUpperLongitude = validateLongitude (fLongitude - dLonDegDisplacement);
    float rightLowerLatitude = validateLatitude (fLatitude - dLatDegDisplacement);
    float rightLowerLongitude = validateLongitude (fLongitude + dLonDegDisplacement);
    return BoundingBox (leftUpperLatitude, leftUpperLongitude, rightLowerLatitude, rightLowerLongitude);
}
    
