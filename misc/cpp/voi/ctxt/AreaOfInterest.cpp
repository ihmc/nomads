/*
 * AreaOfInterest.cpp
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
 * Created on September 27, 2011, 2:00 PM
 */

#include "AreaOfInterest.h"

using namespace IHMC_VOI;
using namespace NOMADSUtil;

AreaOfInterest::AreaOfInterest (const char *pszAreaName, const BoundingBox &bb)
    : _i64StartTime (0),
      _i64EndTime (0x7FFFFFFFFFFFFFFF),
      _areaName (pszAreaName),
      _bb (bb)
{
}

AreaOfInterest::~AreaOfInterest (void)
{
}

BoundingBox AreaOfInterest::getBoundingBox (void) const
{
    return _bb;
}

void AreaOfInterest::setStartTime (int64 i64StartTime)
{
    _i64StartTime = i64StartTime;
}

void AreaOfInterest::setEndTime (int64 i64EndTime)
{
    _i64EndTime = i64EndTime;
}

String AreaOfInterest::getName (void) const
{
    return _areaName;
}

