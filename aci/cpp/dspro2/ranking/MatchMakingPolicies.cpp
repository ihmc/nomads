/* 
 * MatchMakingPolicies.cpp
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
 * Created on July 5, 2013, 4:02 AM
 */

#include "MatchMakingPolicies.h"

#include "BoundingBox.h"
#include "Defs.h"
#include "MetaData.h"
#include "MetadataRankerConfiguration.h"
#include "NodeContext.h"
#include "NodePath.h"

#include "Prediction.h"

#include "GeoUtils.h"
#include "Logger.h"

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

#define COMMENT_LEN 512

using namespace IHMC_ACI;
using namespace NOMADSUtil;

bool enableMathDebug;

Match MatchMakingPolicies::rankByPathCoordinates (const BoundingBox &metadataBBox,
                                                  Path *pPath, int iCurrentWayPoint,
                                                  uint32 ui32UsefulDistance, int *pIMetadataWayPoint,
                                                  String &comment)
{
    const char *pszMethodName = "MatchMakingPolicies::rankingByPathCoordinates";
    if (pPath == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "No NodePath for the peer.\n");
        return Match (Match::NOT_SURE);
    }
    if (!metadataBBox.isValid()) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "Some of the coordinates in "
                        "the given MetaData contain unknown values. Could not rank it.\n");
        return Match (Match::NOT_SURE);
    }

    float fMetaLatC, fMetaLongC, fMetaLatD, fMetaLongD,
          fMetaLatE, fMetaLongE, fMetaLatF, fMetaLongF;

    // Metadata's bounding box:        |          // Path Segment:
    //                                 |
    //  C                 D            |                 B (currentWayPoint+1)
    //   +---------------+             |               /
    //   |               |             |              /
    //   +---------------+             |             /
    // F                  E            |           A (currentWayPoint)

    fMetaLatC = fMetaLatF = metadataBBox._leftUpperLatitude;
    fMetaLongC = fMetaLongD = metadataBBox._leftUpperLongitude;
    fMetaLatE = fMetaLatD = metadataBBox._rightLowerLatitude;
    fMetaLongE = fMetaLongF = metadataBBox._rightLowerLongitude;

    static float MIN_DISTANCE_UNSET = -1.0;
    float minDistance = MIN_DISTANCE_UNSET;

    char szComment[COMMENT_LEN];
    szComment[0] = '\0';

    if (pPath->getPathType() != NodePath::FIXED_LOCATION && pPath->getPathLength() > 1) {
        // First evaluate the future path, assuming that the current position is
        // defined by currentWayPoint 
        if (futureSegmentAreaOverlap (fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE,
                                      pPath, iCurrentWayPoint, pIMetadataWayPoint,
                                      comment)) {
            return Match (Match::YES, MetadataRankerLocalConfiguration::MAX_RANK);
        }

        // If the bounding box of the metadata does not intersect the future
        // path, search the segment, which is the closest to the current way
        // point, and that intersects the path
        if (pastSegmentAreaOverlap (fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE,
                                    pPath, iCurrentWayPoint, pIMetadataWayPoint,
                                    comment)) {
            return Match (Match::YES, MetadataRankerLocalConfiguration::MAX_RANK);
        }

        // Rank based on distance between the metadata and the path
        int index = -1;
        for (int i = 0; i < pPath->getPathLength() - 1; i ++) {
            float fWayPointLatA = pPath->getLatitude (i);
            float fWayPointLongA = pPath->getLongitude (i);
            float fWayPointLatB = pPath->getLatitude (i + 1);
            float fWayPointLongB = pPath->getLongitude (i + 1);
            float distance = minDistanceSegmentArea (fWayPointLatA, fWayPointLongA, fWayPointLatB, fWayPointLongB,
                                                     fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE);

            if ((minDistance < 0.0f) || (distance < minDistance)) {
                minDistance = distance;
                index = i;
                sprintf (szComment, "Segment %d-%d (%f, %f)(%f, %f) min distance to metadata with bounding box (%f, %f)(%f, %f) is %f",
                         index, (index+1), fWayPointLatA, fWayPointLongA, fWayPointLatB, fWayPointLongB,
                         fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE, minDistance);
            }
        }
        (*pIMetadataWayPoint) = index;
    }
    else {
        // when the path is a fixed point, the rank is based on the minimum
        // distance between a metadata segment and the only point in path
        (*pIMetadataWayPoint) = 0;

        if (pointContained (pPath->getLatitude (0), pPath->getLongitude (0),
                            fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE)) {
            return Match (Match::YES, MetadataRankerLocalConfiguration::MAX_RANK);
        }
        minDistance = minDistancePointArea (pPath->getLatitude (0),
                                            pPath->getLongitude (0),
                                            fMetaLatC, fMetaLongC,
                                            fMetaLatE, fMetaLongE);
    }

    if (minDistance > ui32UsefulDistance) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "******** minumum dist %f > "
                        "useful dist %u\n", minDistance, ui32UsefulDistance);
        return Match (Match::NO, MetadataRankerLocalConfiguration::MIN_RANK);
    }

    // Scale minDistance to be in the ] MIN_RANK + 1.0f, MAX_RANK - 1.0 [ range.
    bool bError;
    float fTmpMinDistance = minDistance;  // Only for logging the error
    minDistance = inverseScale (minDistance, 0.0f, (float)ui32UsefulDistance, MetadataRankerLocalConfiguration::MIN_RANK + 1.0f,
                                MetadataRankerLocalConfiguration::MAX_RANK - 1.0f, bError);
    if (bError) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "inverseScale returned "
                        "an error for arguments %f, 0.0, %f, %f, %f\n", fTmpMinDistance,
                        (float) ui32UsefulDistance, MetadataRankerLocalConfiguration::MIN_RANK + 1.0f,
                        MetadataRankerLocalConfiguration::MAX_RANK - 1.0);
        return Match (Match::NOT_SURE);
    }

    comment = szComment;
    return Match (Match::YES, minDistance);
}

Match MatchMakingPolicies::rankByPathTimeStamps (Path *pPath, int iCurrentWayPoint,
                                                 int iMetadataWayPoint, uint32 ui32UsefulDistance,
                                                 bool bConsiderFuturePathSegmentForMatchmacking)
{
    const char *pszMethodName = "MatchMakingPolicies::rankByPathTimeStamps";
    enableMathDebug = true;
    if (enableMathDebug) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "closest waypoint is: %d; current waypoint is "
                        "%d; useful distance is %d\n", iMetadataWayPoint, iCurrentWayPoint, ui32UsefulDistance);
    }

    if (iCurrentWayPoint < -1) {
        return Match (Match::NOT_SURE);
    }

    if (iCurrentWayPoint == -1) {
        // Path has not started yet
        if (enableMathDebug) {
            checkAndLogMsg (pszMethodName, Logger::L_Info,  "current waypoint is -1\n");
        }
        if (iMetadataWayPoint < 1) {
            if (enableMathDebug) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "returning MAX_RANK "
                                "as object is close to waypoint 0\n");
            }
            return Match (Match::YES, MetadataRankerLocalConfiguration::MAX_RANK);
        }
        else {
            bool bError = false;
            float fRawRank = (float) iMetadataWayPoint / (float) pPath->getPathLength();
            float fRank = scale (fRawRank, 1.0, 0.0, MetadataRankerLocalConfiguration::MIN_RANK,
                                 MetadataRankerLocalConfiguration::MAX_RANK, bError);
            if (bError) {
                return Match (Match::NOT_SURE);
            }
            else if (fRank > MetadataRankerLocalConfiguration::MIN_RANK) {
                return Match (Match::YES, fRank);
            }
            else {
                return Match (Match::NO);
            }
        }
    }

    if (iMetadataWayPoint == iCurrentWayPoint) {
        if (enableMathDebug) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "returning MAX_RANK as "
                            "object is close to current waypoint or next waypoint\n");
        }
        return Match (Match::YES, MetadataRankerLocalConfiguration::MAX_RANK);
    }
    else if (iMetadataWayPoint > iCurrentWayPoint) {
        if (!bConsiderFuturePathSegmentForMatchmacking) {
            Match match (Match::NO);
            return match;
        }
        bool bError = false;
        float fRawRank = (float) (iMetadataWayPoint - iCurrentWayPoint) / (float) (pPath->getPathLength() - iCurrentWayPoint);
        float fRank = scale (fRawRank, 1.0, 0.0, MetadataRankerLocalConfiguration::MIN_RANK,
                             MetadataRankerLocalConfiguration::MAX_RANK, bError);
        if (bError) {
            return Match (Match::NOT_SURE);
        }
        else if (fRank > MetadataRankerLocalConfiguration::MIN_RANK) {
            return Match (Match::YES, fRank);
        }
        else {
            return Match (Match::NO);
        }
    }
    else {
        // Item is close to a past waypoint
        return Match (Match::YES, MetadataRankerLocalConfiguration::DEF_RANK);
    }
}

/*float MatchMakingPolicies::rankByPathTimeStamps(NodePath *pNodePath, int iCurrentWayPoint,
                                           int iMetadataWayPoint, uint32 ui32UsefulDistance)
{
    if (enableMathDebug) {
        checkAndLogMsg ("MatchMakingPolicies::rankByPathTimeStamps", Logger::L_Info, "current waypoint is %d, closest waypoint is %d, useful distance is set to %d\n",
                         iCurrentWayPoint, iMetadataWayPoint, (int) ui32UsefulDistance);
    }
    if (pNodePath == NULL) {
        return -1;
    }
    if (iMetadataWayPoint == iCurrentWayPoint) {
        if (enableMathDebug) {
            checkAndLogMsg("MatchMakingPolicies::rankByPathTimeStamps", Logger::L_Info, "metadata waypoint matches current waypoint - returning MAX_RANK\n");
        }

        return MAX_RANK;
    }
    if(iMetadataWayPoint < iCurrentWayPoint) {
        if (enableMathDebug) {
            checkAndLogMsg ("MatchMakingPolicies::rankByPathTimeStamps", Logger::L_Info, "metadata waypoint occurs earlier than current waypoint - returning MAX_RANK/2\n");
        }
        return (MAX_RANK + MIN_RANK)/2;
    }

    bool bAllTimeStamps = true;
    uint64 *pUi64TimeStamps;
    float *pFDistances;
    pUi64TimeStamps = (uint64 *) calloc(pNodePath->getPathLength() - iCurrentWayPoint, sizeof(uint64));
    pFDistances = (float *) calloc(pNodePath->getPathLength() - iCurrentWayPoint - 1, sizeof(float));
    float fTotalDistance = 0.0;
    for(int i = iCurrentWayPoint; i < pNodePath->getPathLength(); i ++) {
        pUi64TimeStamps[i - iCurrentWayPoint] = pNodePath->getTimeStamp(i);
        if(pUi64TimeStamps[i - iCurrentWayPoint] == 0) {
            bAllTimeStamps = false;
        }
    }

    // Not all the waypoints have been assigned a timestamp, use the distances
    // to infer the missing timestamps
    if(!bAllTimeStamps) {
        if (enableMathDebug) {
            checkAndLogMsg ("MatchMakingPolicies::rankByPathTimeStamps", Logger::L_Info, "all timestamps have not been set - computing\n");
        }
        for (int i = iCurrentWayPoint; i < pNodePath->getPathLength() - 1; i ++) {
            pFDistances[i - iCurrentWayPoint] = greatCircleDistance (pNodePath->getLatitude(i), pNodePath->getLongitude(i),
                                                                     pNodePath->getLatitude(i + 1), pNodePath->getLongitude(i + 1));
            fTotalDistance += pFDistances[i - iCurrentWayPoint];
        }
        // Timestamp estimation
        if(pUi64TimeStamps[pNodePath->getPathLength() - iCurrentWayPoint - 1] != 0) {
            if(pUi64TimeStamps[0] == 0) {
                pUi64TimeStamps[0] = getTimeInMilliseconds();
            }
            for(int i = iCurrentWayPoint + 1; i < pNodePath->getPathLength() - 1; i ++) {
                if(pUi64TimeStamps[i] != 0) {
                    continue;
                }
                pUi64TimeStamps[i - iCurrentWayPoint] = (pUi64TimeStamps[pNodePath->getPathLength() - iCurrentWayPoint - 1] -
                pUi64TimeStamps[i - iCurrentWayPoint - 1]) *pFDistances[i - iCurrentWayPoint] / fTotalDistance;
                pUi64TimeStamps[i - iCurrentWayPoint] += pUi64TimeStamps[i - iCurrentWayPoint - 1];
            }
            bAllTimeStamps = true;
        }
    }

    // Find the ID of the furthest (future) way point withing the useful distance
    // from the current waypoint and store it in iFarthestUsefulWaypointID
    int iFarthestUsefulWaypointID = iCurrentWayPoint;
    float fRadiusDistance;
    for (int i = iCurrentWayPoint + 1; i < pNodePath->getPathLength(); i ++) {
        // Distance between the current way point and the i-th way point after it
        fRadiusDistance = greatCircleDistance(pNodePath->getLatitude(iCurrentWayPoint),
                                              pNodePath->getLongitude(iCurrentWayPoint),
                                              pNodePath->getLatitude(i),
                                              pNodePath->getLongitude(i));
        if (fRadiusDistance > (ui32UsefulDistance * 2.0)) {
            // exit when the way point is too far away
            break;
        }
        iFarthestUsefulWaypointID = i;
    }
    if (enableMathDebug) {
        checkAndLogMsg ("MatchMakingPolicies::rankByPathTimeStamps", Logger::L_Info, "farthest waypoint within the useful distance threshold is: %d\n", iWayPointUD);
    }

    if(iMetadataWayPoint > iFarthestUsefulWaypointID) {
    //if (iMetadataWayPoint > iWayPointUD) {
        //---------------------------------
        // iMetadataWayPoint > iWayPointUD
        //---------------------------------
        // iMetadataWayPoint is to far away from the current position
        free(pUi64TimeStamps);
        pUi64TimeStamps = NULL;
        free(pFDistances);
        pFDistances = NULL;
        if (enableMathDebug) {
            checkAndLogMsg ("MatchMakingPolicies::rankByPathTimeStamps", Logger::L_Info, "metadata waypoint exceeds farthest waypoint within the useful distance - return 0\n");
        }
        return 0;
    }

    float fRawRank;     // rank for iFarthestUsefulWaypointID
    bool bError;
    if(iFarthestUsefulWaypointID == (pNodePath->getPathLength() - 1)) {
        fRawRank = inverseScale(fRadiusDistance, 0.0, ui32UsefulDistance * 2.0, 1.0, 9.0, bError);
        if (bError) {
            checkAndLogMsg ("MatchMakingPolicies::rankByPathTimeStamps", Logger::L_SevereError,
                            "inverseScale returned an error for arguments %f, 0.0, %f, 1.0, 9.0",
                            fRadiusDistance, (ui32UsefulDistance * 2.0));
            // There was an error.  Set it to average rank
            fRawRank = 5.0;
        }
    }
    else {
        // This nodes are close to the path, but they are very far away.
        // Assign the minumum rank
        fRawRank = 1.0;
    }
    
    float fRank = 0;
    if(iMetadataWayPoint < iFarthestUsefulWaypointID) {
        //---------------------------------
        // iMetadataWayPoint < iWayPointUD
        //---------------------------------
        if(!bAllTimeStamps) {                    // use distances
            fTotalDistance = 0;
            fRadiusDistance = 0;
            for(int i = iCurrentWayPoint; i < iFarthestUsefulWaypointID - 1; i ++) {
                fTotalDistance += pFDistances[i - iCurrentWayPoint];
                if(i >= iMetadataWayPoint) {
                    fRadiusDistance += pFDistances[i - iCurrentWayPoint];
                }
            }
            if(fTotalDistance > 0) {
                fRank = fRadiusDistance * (9.0 - fRawRank) / fTotalDistance;
            }
            fRank += fRawRank;
            checkAndLogMsg ("MatchMakingPolicies::rankingByExpirationTime", Logger::L_HighDetailDebug,
                            "USE DISTANCES, rank = %f\n", fRank);
        }
        else {                                  // use timeStamps
            uint64 ui64TotalTime = 0, ui64MetadataTime = 0;
            for(int i = iCurrentWayPoint + 1; i < iFarthestUsefulWaypointID; i ++) {
                ui64TotalTime += pUi64TimeStamps[i - iCurrentWayPoint] - pUi64TimeStamps[0];
                if(i >= iMetadataWayPoint) {
                    ui64MetadataTime += pUi64TimeStamps[i - iCurrentWayPoint] - pUi64TimeStamps[0];
                }
            }
            if(ui64TotalTime > 0) {
                fRank = ui64MetadataTime * (9.0 - fRawRank) / ui64TotalTime;
            }
            fRank += fRawRank;
            checkAndLogMsg ("MatchMakingPolicies::rankingByExpirationTime", Logger::L_HighDetailDebug,
                            "USE TIMESTAMPS, rank = %f\n", fRank);
        }
    }
    else {
        //---------------------------------
        // iMetadataWayPoint == iWayPointUD
        //---------------------------------
        fRank = fRawRank;
    }

    free(pUi64TimeStamps);
    pUi64TimeStamps = NULL;
    free(pFDistances);
    pFDistances = NULL;
    return fRank;
} */

Match MatchMakingPolicies::rankByExpirationTime (uint64 ui64SourceTimeStamp, uint64 ui64ExpirationTimeStamp)
{
    if ((ui64ExpirationTimeStamp == 0) || (ui64ExpirationTimeStamp == 1)) {
        return Match (Match::NOT_SURE);
    }
    if (ui64SourceTimeStamp == (uint64) MetaData::SOURCE_TIME_STAMP_UNSET) {
        checkAndLogMsg ("MatchMakingPolicies::rankingByExpirationTime", Logger::L_MildError,
                        "The source time stamp in the given MetaData is an unknown value. "
                        "Could not rank the metadata.\n");
        return Match (Match::NOT_SURE);
    }
    uint64 actualTime = ui64ExpirationTimeStamp - getTimeInMilliseconds() + ui64SourceTimeStamp;
    float fRank = actualTime * MetadataRankerLocalConfiguration::MAX_RANK / ui64ExpirationTimeStamp;
    if (fRank > MetadataRankerLocalConfiguration::MIN_RANK) {
        return Match (Match::YES, fRank);
    }
    return Match (Match::NO);
}

Match MatchMakingPolicies::rankByImportance (double dImportance)
{
    if ((dImportance > 5.0) || (dImportance < 0.0)) {
        // dImportance is out of range (look at the comments in the method declaration)
        return Match (Match::NOT_SURE);
    }
    if (dImportance == 0.0) {
        return Match (Match::NO);
    }
    return Match (Match::YES, (float) (dImportance * 2));
}

Match MatchMakingPolicies::rankByTarget (const char *pMetaDataTarget,
                                         const char *pMetaDataTargetRole,
                                         const char *pMetaDataTargetTeam,
                                         const char *pMetaDataTargetMission,
                                         NodeContext *pNodeContext)
{
    if (pNodeContext == NULL) {
        Match match (Match::NOT_SURE);
        return match;
    }

    // If pMetaDataTarget is set, it superseeds all the other targets
    if (!MetadataInterface::isFieldValueUnknown (pMetaDataTarget)) {
        const String nodeId (pNodeContext->getNodeId());
        if ((nodeId == pMetaDataTarget) || (pNodeContext->hasUserId (pMetaDataTarget))) {
            return Match (Match::YES, MetadataRankerLocalConfiguration::MAX_RANK);
        }
        else {
            return Match (Match::NO);
        }
    }

    float fMaximumAchievableMatch = 0.0f;
    float fAchievedMatch = 0.0f;

    rankByTarget (pNodeContext->getRole(), pMetaDataTargetRole, fAchievedMatch, fMaximumAchievableMatch);
    rankByTarget (pNodeContext->getTeamId(), pMetaDataTargetTeam, fAchievedMatch, fMaximumAchievableMatch);
    rankByTarget (pNodeContext->getMissionId(), pMetaDataTargetMission, fAchievedMatch, fMaximumAchievableMatch);

    if (fMaximumAchievableMatch > 0.0f) {
        // Returns the "average match"
        bool bError = false;
        float fRank = scale (fAchievedMatch, 0.0, fMaximumAchievableMatch, MetadataRankerLocalConfiguration::MIN_RANK,
                             MetadataRankerLocalConfiguration::MAX_RANK, bError);
        if (bError) {
            // There was an error, return the average rank
            checkAndLogMsg ("MatchMakingPolicies::rankingByTarget", Logger::L_SevereError,
                            "scale returned an error for arguments %f, 0.0, %f, %f, %f",
                            fAchievedMatch, fMaximumAchievableMatch, MetadataRankerLocalConfiguration::MIN_RANK,
                            MetadataRankerLocalConfiguration::MAX_RANK);
            return Match (Match::NOT_SURE);
        }

        if (fRank > MetadataRankerLocalConfiguration::MIN_RANK) {
            return Match (Match::YES, fRank);
        }
        else {
            return Match (Match::NO);
        }
    }
    else {
        // pMetaDataTarget, pMetaDataTargetRole, pMetaDataTargetTeam and
        // pMetaDataTargetMission are not set, return a "neuter" value
        return Match (Match::NOT_SURE);
    }
}

void MatchMakingPolicies::rankByTarget (const char *pszNodeTargetValue, const char *pMetaDataTargetValue,
                                        float &fCurrentRank, float &fMaximumMatch)
{
    if (!MetadataInterface::isFieldValueUnknown (pMetaDataTargetValue)) {
        if ((pszNodeTargetValue != NULL) && (0 == stricmp (pMetaDataTargetValue, pszNodeTargetValue))) {
            fCurrentRank += MetadataRankerLocalConfiguration::MAX_RANK;
        }
        fMaximumMatch += MetadataRankerLocalConfiguration::MAX_RANK;
    }
}

bool MatchMakingPolicies::isNodeOrUserTarget (const char *pMetaDataTarget, NodeContext *pNodeContext)
{
    if (pMetaDataTarget == NULL) {
        return true;
    }
    if (pNodeContext == NULL) {
        return true;
    }
    String metadataTarget (pMetaDataTarget);
    if (metadataTarget.length() <= 0) {
        return true;
    }
    if (metadataTarget == MetadataInterface::UNKNOWN) {
        return true;
    }

    return ((metadataTarget == pNodeContext->getNodeId()) ||
            (pNodeContext->hasUserId (pMetaDataTarget)));
}

Match MatchMakingPolicies::rankByPrediction (IHMC_C45::Prediction *pPrediction)
{
    if (pPrediction == NULL) {
        Match match (Match::NOT_SURE);
        return match;
    }
    String prediction (pPrediction->getPrediction());
    if (prediction == PredictionClass::USEFUL) {
        // direct ranking
        const float fProb = pPrediction->getProbability();
        if (fProb > 0.0f) {
            Match match (Match::YES, fProb * MetadataRankerLocalConfiguration::MAX_RANK);
            return match;
        }
        else if (fProb < 0.0f) {
            Match match (Match::NOT_SURE);
            return match;
        }
    } 
    Match match (Match::NO);
    return match;
    // inverse ranking
    //return ((1 - pPrediction->getProbability()) * MetaDataRankerLocalConfiguration::MAX_RANK);
}

Match MatchMakingPolicies::toRank (float fValue, float fOldMin, float fOldMax)
{
    bool bError = false;
    const float fRank = scale (fValue, fOldMin, fOldMax,
            MetadataRankerLocalConfiguration::MIN_RANK,
            MetadataRankerLocalConfiguration::MAX_RANK, bError);
    if (bError) {
        return Match (Match::NOT_SURE);
    }
    else if (fRank > 0.0f) {
        return Match (Match::YES, fRank);
    }
    return Match (Match::NO);
}

bool MatchMakingPolicies::futureSegmentAreaOverlap (float fMetaLatC, float fMetaLongC,
                                                    float fMetaLatE, float fMetaLongE,
                                                    Path *pPath, int iCurrentWayPoint,
                                                    int *pIMetadataWayPoint, String &comment)
{
    for (int i = iCurrentWayPoint; i < pPath->getPathLength() - 1; i ++) {
        if (segmentAreaOverlap (pPath->getLatitude (i),        // fWayPointLatA
                                pPath->getLongitude (i),       // fWayPointLongA
                                pPath->getLatitude (i + 1),    // fWayPointLatB
                                pPath->getLongitude (i + 1),   // fWayPointLongB
                                fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE,
                                i, iCurrentWayPoint, comment)) {
            (*pIMetadataWayPoint) = i;
            return true;
        }
    }
    return false;
}

bool MatchMakingPolicies::pastSegmentAreaOverlap (float fMetaLatC, float fMetaLongC,
                                                  float fMetaLatE, float fMetaLongE,
                                                  Path *pPath, int iCurrentWayPoint,
                                                  int *pIMetadataWayPoint, String &comment)
{
    for (int i = iCurrentWayPoint; i > 0; i--) {
        if (segmentAreaOverlap (pPath->getLatitude (i - 1),  // fWayPointLatA
                                pPath->getLongitude (i - 1), // fWayPointLongA
                                pPath->getLatitude (i),      // fWayPointLatB
                                pPath->getLongitude (i),     // fWayPointLongB
                                fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE,
                                i, iCurrentWayPoint, comment)) {
            (*pIMetadataWayPoint) = i;
            return true;
        }
    }
    return false;
}

bool MatchMakingPolicies::segmentAreaOverlap (float fWayPointLatA, float fWayPointLongA,
                                              float fWayPointLatB, float fWayPointLongB,
                                              float fMetaLatC, float fMetaLongC,
                                              float fMetaLatE, float fMetaLongE,
                                              int i, int iCurrentWayPoint,
                                              String &comment)
{
    // Metadata's bounding box:        |          // Path Segment:
    //                                 |
    //  C                 D            |                 B (currentWayPoint+1)
    //   +---------------+             |               /
    //   |               |             |              /
    //   +---------------+             |             /
    // F                  E                         A

    // Check if the path segment is fully contained
    if (segmentContained (fWayPointLatA, fWayPointLongA, fWayPointLatB, fWayPointLongB,
                          fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE)) {
        char szComment[COMMENT_LEN];
        szComment[0] = '\0';
        sprintf (szComment, "%s segment %d-%d of coordinates (%f, %f)(%f, %f) in contained in "
                 "the area defined by the upper-left and bottom-right corners (%f, %f)(%f, %f)",
                 (i < iCurrentWayPoint ? "Past" : "Future"), i, (i+1),
                 fWayPointLatA, fWayPointLongA, fWayPointLatB, fWayPointLongB,
                 fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE);
        comment = szComment;
        return true;
    }

    // Check if the path segment intersects with any of the sides of the
    // metadata bounding box
    if (segmentAreaIntersection (fWayPointLatA, fWayPointLongA,
                                 fWayPointLatB, fWayPointLongB,
                                 fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE)) {

        char szComment[COMMENT_LEN];
        szComment[0] = '\0';
        sprintf (szComment, "%s segment %d-%d of coordinates (%f, %f)(%f, %f) intersects the "
                 "area defined by the upper-left and bottom-right corners (%f, %f)(%f, %f)",
                 (i < iCurrentWayPoint ? "Past" : "Future"), i, (i+1),
                 fWayPointLatA, fWayPointLongA, fWayPointLatB, fWayPointLongB,
                 fMetaLatC, fMetaLongC, fMetaLatE, fMetaLongE);
        comment = szComment;
        return true;
    }

    return false;
}

