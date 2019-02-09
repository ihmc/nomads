/*
 * AreasOfIntInfo.h
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

#ifndef INCL_AREAS_OF_INTEREST_H
#define INCL_AREAS_OF_INTEREST_H

#include "Part.h"

#include "AreaOfInterest.h"

namespace NOMADSUtil
{
    class JsonArray;
}

namespace IHMC_ACI
{
    class AreasOfInterestsInfo : public Part
    {
        public:
            static const char * AREAS_OF_INTEREST_OBJ;

            AreasOfInterestsInfo (void);
            ~AreasOfInterestsInfo (void);

            bool add (const char *pszAreaName, NOMADSUtil::BoundingBox &bb, int64 i64StatTime, int64 int64EndTime=0x7FFFFFFFFFFFFFFF);
            IHMC_VOI::AreaOfInterestList * getAreasOfInterest (void) const;

            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

            void reset (void);

        private:
            NOMADSUtil::JsonObject *_pJson;
    };
}

#endif  /* INCL_AREAS_OF_INTEREST_H */
