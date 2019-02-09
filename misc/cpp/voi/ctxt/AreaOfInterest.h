/*
 * AreaOfInterest.h
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

#ifndef INCL_AREA_OF_INTEREST_H
#define	INCL_AREA_OF_INTEREST_H

#include "GeoUtils.h"
#include "StrClass.h"
#include "PtrLList.h"

namespace IHMC_VOI
{
    class AreaOfInterest
    {
        public:
            AreaOfInterest (const char *pszAreaName, const NOMADSUtil::BoundingBox &bb);
            ~AreaOfInterest (void);

            NOMADSUtil::BoundingBox getBoundingBox (void) const;

            void setStartTime (int64 i64StartTime);
            void setEndTime (int64 i64EndTime);
            NOMADSUtil::String getName (void) const;

        private:
            int64 _i64StartTime;
            int64 _i64EndTime;
            const NOMADSUtil::String _areaName;
            const NOMADSUtil::BoundingBox _bb;
    };

    typedef NOMADSUtil::PtrLList<AreaOfInterest> AreaOfInterestList;
}

#endif  /* INCL_AREA_OF_INTEREST_H */

