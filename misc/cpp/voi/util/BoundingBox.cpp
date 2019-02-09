/*
 * BoundingBox.cpp
 *
 * This file is part of the IHMC Voi Library/Component
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

#include <math.h>
#include <assert.h>

using namespace IHMC_VOI;
using namespace NOMADSUtil;


BoundingBoxAccumulator::BoundingBoxAccumulator (const BoundingBox &bbox)
    : _bEmpty (bbox._bEmpty),_leftUpperLatitude (bbox._leftUpperLatitude),
      _leftUpperLongitude (bbox._leftUpperLongitude), _rightLowerLatitude (bbox._rightLowerLatitude),
      _rightLowerLongitude (bbox._rightLowerLongitude)
{
}

BoundingBoxAccumulator::~BoundingBoxAccumulator (void)
{
}

BoundingBoxAccumulator & BoundingBoxAccumulator::operator+=(const BoundingBox &rhs)
{
    if (rhs._bEmpty) {
        return *this;
    }
    _bEmpty = false;
    if (rhs._leftUpperLatitude > _leftUpperLatitude) {
        _leftUpperLatitude = rhs._leftUpperLatitude;
    }
    if (rhs._leftUpperLongitude < _leftUpperLongitude) {
        _leftUpperLongitude = rhs._leftUpperLongitude;
    }
    if (rhs._rightLowerLatitude < _rightLowerLatitude) {
        _rightLowerLatitude = rhs._rightLowerLatitude;
    }
    if (rhs._rightLowerLongitude > _rightLowerLongitude) {
        _rightLowerLongitude = rhs._rightLowerLongitude;
    }
    return *this;
}

BoundingBox BoundingBoxAccumulator::getBoundingBox (void)
{
    return BoundingBox (_leftUpperLatitude, _leftUpperLongitude,
                        _rightLowerLatitude, _rightLowerLongitude);
}

