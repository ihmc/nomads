/*
 * MatchMakingPolicies.h
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
 * Created on July 5, 2013, 4:02 AM
 */

#ifndef INCL_MATCHMAKING_POLICIES_H
#define	INCL_MATCHMAKING_POLICIES_H

#include "AreaOfInterest.h"
#include "Match.h"

#include "DArray2.h"
#include "StrClass.h"

namespace IHMC_C45
{
    class Prediction;
}

namespace IHMC_VOI
{
    class NodeContext;
    class Path;

    class MatchMakingPolicies
    {
        public:
            /**
             * Ranks a metadata based on the amount of overlap between the area
             * of relevance of the metadata, and the area of interest of the user.
             * 0 = no overlap
             * 10 = complete overlap
             * 1 - 9 = partial overlap
             */
            static Match rankByAreasOfInterest (const NOMADSUtil::BoundingBox &bbox,
                                                AreaOfInterestList *pAreasOfInterest);

            /**
             * Ranks a metadata based on the distance between the metadata
             * coordinates and the path. Returns the rank value and stores in
             * "metadataWayPoint" the wayPointIndex closest to the metadata
             * coordinates. In this way is possible to order metadata with the
             * same rank value based on the distance between the actual
             * wayPointIndex and the wayPointReturned.
             * 0 = the metadata is close to a past location in the path
             * 10 = the metadata has an intersection with the future path
             * 1 - 9 = the metadata is near a point in the future path
             */
            static Match rankByPathCoordinates (const NOMADSUtil::BoundingBox &area, Path *pPath,
                                                int iCurrentWayPoint, uint32 ui32UsefulDistance,
                                                int *pIMetadataWayPoint, NOMADSUtil::String &comment);

            /*
             * Use for parameter "metadataWayPoint" the value returned by
             * "rankingByPathCoordinates()" in the parameter with the same name.
             * The method ranks the metadata based on the time stamps in the path.
             * If the path has no time stamps, the rank value is based on
             * distances between the actual coordinates and the
             * "metadataWayPoint" coordinates.
             * 0 = the metadata will be useful after the calculated timestamp
             *     for "usefulDistance"
             * 10 = the metadata is useful right now
             * 1 - 9 = estimate when the metadata will be useful and rank
             *     it based on timestamps or distances
             */
            static Match rankByPathTimeStamps (Path *pPath, int iCurrentWayPoint, int iMetadataWayPoint,
                                               uint32 ui32UsefulDistance, bool bConsiderFuturePathSegmentForMatchmacking);

            /*
             * Ranks a metadata based on the available time before the
             * expiration. If the expiration is not set (expiration time = 0)
             * the method always return 5. The expiration time is a number
             * of milliseconds measured from the source time stamp.
             * 0 = the data is old and close to its expiration
             * 10 = the data is new
             */
            static Match rankByExpirationTime (uint64 ui64SourceTimeStamp, uint64 ui64ExpirationTimeStamp);

            /*
             * "Importance" is a value in the interval [1.0 - 5.0] given in metadata.
             * 5.0 means maximum importance, 1.0 means minimum importance.
             */
            static Match rankByImportance (double dmportance);

            /**
             *
             * Ranks by the the id of the target node
             * - pMetaData is the metadata of the node that is being ranked
             * - pNodeContext is the node context of the node
             */
            static Match rankByTarget (const char *pMetaDataTarget, const char *pMetaDataTargetRole,
                                       const char *pMetaDataTargetTeam, const char *pMetaDataTargetMission,
                                       NodeContext *pNodeContext);

            static bool isNodeOrUserTarget (const char *pMetaDataTarget, NodeContext *pNodeContext);

            /**
             * Ranking using a prediction given by a generic learning algorithm.
             */
            static Match rankByPrediction (IHMC_C45::Prediction *pPrediction);

            /**
             * Simply scales a numeric value of an attribute to a [MIN_RANK, MAX_RANK] value of rank.
             * IF scaling is not possible, it returns a NOT_SURE match.
             */
            static Match toRank (float fValue, float fOldMin, float fOldMax);

        private:
            /**
             * Returns true whether there is a segment in the future path that
             * overlaps or that is contained in the area identified by
             * (fMetaLatC, fMetaLongC) as upper left corner and
             * (fMetaLatE, fMetaLongE) as bottom right corner.
             * Returns false if such a segment does not exist.
             *
             * The method also stores into pIMetadataWayPoint the number of the
             * first segment that matches.
             */
            static bool futureSegmentAreaOverlap (float fMetaLatC, float fMetaLongC,
                                                  float fMetaLatE, float fMetaLongE,
                                                  Path *pPath, int iCurrentWayPoint,
                                                  int *pIMetadataWayPoint,
                                                  NOMADSUtil::String &comment);

            /**
             * Returns true whether there is a segment in the past path that
             * overlaps or that is contained in the area identified by
             * (fMetaLatC, fMetaLongC) as upper left corner and
             * (fMetaLatE, fMetaLongE) as bottom right corner.
             * Returns false if such a segment does not exist.
             *
             * The method also stores into pIMetadataWayPoint the number of the
             * first segment that matches.
             */
            static bool pastSegmentAreaOverlap (float fMetaLatC, float fMetaLongC,
                                                float fMetaLatE, float fMetaLongE,
                                                Path *pPath, int iCurrentWayPoint,
                                                int *pIMetadataWayPoint,
                                                NOMADSUtil::String &comment);

            static void rankByTarget (const char *pszNodeTargetValue, const char *pMetaDataTargetValue,
                                      float &fCurrentRank, float &fMaximumMatch);

            /**
             * Returns true whether the segment (fWayPointLatA, fWayPointLongA),
             * (fWayPointLatB, fWayPointLong) overlaps or is contained in the
             * area identified by (fMetaLatC, fMetaLongC) as upper left corner
             * and (fMetaLatE, fMetaLongE) as bottom right corner.
             * Returns false if such a segment does not exist.
             */
            static bool segmentAreaOverlap (float fWayPointLatA, float fWayPointLongA,
                                            float fWayPointLatB, float fWayPointLongB,
                                            float fMetaLatC, float fMetaLongC,
                                            float fMetaLatE, float fMetaLongE,
                                            int i, int iCurrentWayPoint,
                                            NOMADSUtil::String &comment);
    };
}

#endif	/* INCL_MATCHMAKING_POLICIES_H */

