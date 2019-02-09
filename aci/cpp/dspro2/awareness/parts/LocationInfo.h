/**
 * LocationInfo.h
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
 * Created on December 23, 2016, 10:20 PM
 */

#ifndef INCL_LOCATION_INFO_H
#define INCL_LOCATION_INFO_H

#include "NodePath.h"
#include "NodeContext.h"
#include "Part.h"

namespace IHMC_ACI
{
    class LocationInfo : public Part
    {
        public:
            static const char * LOCATION_INFO_OBJECT_NAME;

            LocationInfo (double dTooFarCoeff, double dApproxCoeff);
            ~LocationInfo (void);

            IHMC_VOI::AdjustedPathWrapper * getAdjustedPath (IHMC_VOI::NodeContext *pNodeContext);
            float getClosestPointOnPathLatitude (void) const;
            float getClosestPointOnPathLongitude (void) const;
            int getCurrentWayPointInPath (void) const;
            int getCurrentPosition (float &latitude, float &longitude, float &altitude,
                                    const char *&pszLocation, const char *&pszNote,
                                    uint64 &timeStamp);
            IHMC_VOI::NodeContext::PositionApproximationMode getPathAdjustingMode (void) const;
            IHMC_VOI::NodeContext::NodeStatus getStatus (void) const;

            /*
             * Return true if the value was updated, false otherwise
             */
            bool setCurrentPosition (IHMC_VOI::NodePath *pPath, float latitude, float longitude, float altitude, uint32 ui32MaxUsefulDistance);
            void setCurrentPath (float closestPointOnPathLat, float closestPointOnPathLong);

            void reset (void);

            /*
             * Serialization
             */
            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

        private:
            IHMC_VOI::NodeContext::NodeStatus _status;
            int _iCurrWayPointInPath;  // Current position of the node along the path
            double _closestPointOnPathLat;
            double _closestPointOnPathLong;
            IHMC_VOI::NodeContext::PositionApproximationMode _pathAdjustingMode;
            IHMC_VOI::AdjustedPathWrapper *_pPathWrapper;
            NOMADSUtil::JsonObject *_pCurrLocation;
            IHMC_VOI::NodePath _history;
    };
}

#endif  /* INCL_LOCATION_INFO_H */
