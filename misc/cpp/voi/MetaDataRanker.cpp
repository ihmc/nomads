/*
 * MetaDataRanker.cpp
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
 */

#include "MetaDataRanker.h"

#include "BoundingBox.h"
#include "C45Utils.h"
#include "MatchMakingPolicies.h"
#include "MetadataConfiguration.h"
#include "MetadataRankerConfiguration.h"
#include "MetadataRankerLocalConfiguration.h"
#include "MatchMakingFilters.h"
#include "NodePath.h"
#include "Rank.h"
#include "RankFactory.h"
#include "VoiDefs.h"

#include "Classifier.h"
#include "Prediction.h"

#include "Logger.h"
#include "Path.h"

#ifdef WIN32
    #define snprintf _snprintf
#endif

#define nullMetadata "The given MetaData is a NULL pointer. Could not rank it"
#define nullNodeContext "The given NodeContext is a NULL pointer. Could not rank it"
#define nullParameters "NULL parameter or empty list. Nothing to matchmake"

using namespace IHMC_C45;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

const float MetaDataRanker::DEFAULT_COORD_WEIGHT = 1.0f;
const float MetaDataRanker::DEFAULT_TIME_WEIGHT = 1.0f;
const float MetaDataRanker::DEFAULT_EXP_WEIGHT = 1.0f;
const float MetaDataRanker::DEFAULT_IMP_WEIGHT = 1.0f;
const float MetaDataRanker::DEFAULT_SRC_REL_WEIGHT = 1.0f;
const float MetaDataRanker::DEFAULT_INFO_CONTENT_WEIGHT = 1.0f;
const float MetaDataRanker::DEFAULT_PRED_WEIGHT = 1.0f;
const float MetaDataRanker::DEFAULT_TARGET_WEIGHT = 1.0f;

const char * MetaDataRanker::COORDINATES_RANK_DESCRIPTOR = "Coordinates";
const char * MetaDataRanker::TIME_RANK_DESCRIPTOR = "Time";
const char * MetaDataRanker::EXPIRATION_RANK_DESCRIPTOR = "Expiration";
const char * MetaDataRanker::IMPORTANCE_RANK_DESCRIPTOR = "Importance";
const char * MetaDataRanker::SOURCE_RELIABILITY_RANK_DESCRIPTOR = "Source_Reliability";
const char * MetaDataRanker::INFORMATION_CONTENT_RANK_DESCRIPTOR = "Information_Content";
const char * MetaDataRanker::PREDICTION_RANK_DESCRIPTOR = "Prediction";
const char * MetaDataRanker::TARGET_RANK_DESCRIPTOR = "Target";

const bool MetaDataRanker::DEFAULT_STRICT_TARGET = false;
const bool MetaDataRanker::DEFAULT_CONSIDER_FUTURE_PATH_SEGMENTS = true;

const char * MetaDataRanker::COORDINATES_RANK_PROPERTY = "aci.dspro.metadataRanker.coordRankWeight";
const char * MetaDataRanker::TIME_RANK_PROPERTY = "aci.dspro.metadataRanker.timeRankWeight";
const char * MetaDataRanker::EXPIRATION_RANK_PROPERTY = "aci.dspro.metadataRanker.expirationRankWeight";
const char * MetaDataRanker::IMPORTANCE_RANK_PROPERTY = "aci.dspro.metadataRanker.impRankWeight";
const char * MetaDataRanker::SOURCE_RELIABILITY_RANK_PROPERTY = "aci.dspro.metadataRanker.srcRelRankWeight";
const char * MetaDataRanker::INFORMATION_CONTENT_RANK_PROPERTY = "aci.dspro.metadataRanker.infoContentRankWeight";
const char * MetaDataRanker::PREDICTION_RANK_PROPERTY = "aci.dspro.metadataRanker.predRankWeight";
const char * MetaDataRanker::TARGET_RANK_PROPERTY = "aci.dspro.metadataRanker.targetRankWeight";
const char * MetaDataRanker::TIME_RANK_CONSIDER_FUTURE_SEG_PROPERTY = "aci.dspro.metadataRanker.timeRank.considerFutureSegments";
const char * MetaDataRanker::TARGET_RANK_STRICT_PROPERTY = "aci.dspro.metadataRanker.targetStrict";

const float MetadataRankerLocalConfiguration::MAX_RANK = 10.0f;
const float MetadataRankerLocalConfiguration::MIN_RANK = 0.0f;
const float MetadataRankerLocalConfiguration::DEF_RANK = 5.0f;

namespace METADATA_RANKER
{
    static const float COORD_RANK_THRESHOLD = 0.000001f;

    Path * getPath (NodeContext *pNodeContext, int &iAdjustedCurrWayPointInPath)
    {
        iAdjustedCurrWayPointInPath = 0;
        const NodeContext::NodeStatus status = pNodeContext->getStatus();
        Path *pPath = NULL;
        switch (status) {
        case NodeContext::ON_WAY_POINT:
            pPath = pNodeContext->getPath();
            iAdjustedCurrWayPointInPath = pNodeContext->getCurrentWayPointInPath();
            break;

        default:
            AdjustedPathWrapper *pAdjPath = pNodeContext->getAdjustedPath();
            pPath = pAdjPath;
            iAdjustedCurrWayPointInPath = pAdjPath->getAdjustedCurrentWayPoint();
        }

        if (pLogger != NULL) {
            const char *pszMethodName = "MetaDataRanker::getPath";
            float latitude, longitude;
            if (pNodeContext->getCurrentLatitude (latitude) == 0 &&
                pNodeContext->getCurrentLongitude (longitude) == 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "path status %s.  "
                    "The current way point in path is %d (%d adjusted). "
                    "The current actual coordinates are lat: %f lon: %f.\n",
                    NodeContext::getStatusAsString (status),
                    pNodeContext->getCurrentWayPointInPath(),
                    iAdjustedCurrWayPointInPath, latitude, longitude);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "path status %s.  "
                    "The current way point in path is %d (%d adjusted). "
                    "The current actual coordinates have not been set.\n",
                    NodeContext::getStatusAsString (status),
                    pNodeContext->getCurrentWayPointInPath(),
                    iAdjustedCurrWayPointInPath);
            }
        }

        return pPath;
    }

    BoundingBox getBoundingBox (NodeContext *pNodeContext, MetadataInterface *pMetadata, MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
    {
        uint32 ui32RangeOfInfluence = 0U;
        if ((pMetadataRankerLocalConf != NULL) && (pNodeContext != NULL)) {
            const String rangeOfInflAttributeName (pMetadataRankerLocalConf->getRangeOfInfluenceAttributeName());
            String description;
            pMetadata->getFieldValue (rangeOfInflAttributeName, description);
            if (description.length() <= 0) {
                pMetadata->getFieldValue ("Node_Type", description);
            }
            if (description.length() > 0) {
                ui32RangeOfInfluence = pNodeContext->getRangeOfInfluence (description);
            }
        }
        return pMetadata->getLocation ((float)ui32RangeOfInfluence);
    }

    Match rankByTarget (MetadataInterface *pMetadata, NodeContext *pNodeContext, String &metaDataTarget)
    {
        String metaDataTargetRole, metaDataTargetTeam, metaDataTargetMission;
        if (0 != pMetadata->getFieldValue (MetadataInterface::TARGET_ID, metaDataTarget)) {
            metaDataTarget = NULL;
        }
        if (0 != pMetadata->getFieldValue (MetadataInterface::TARGET_ROLE, metaDataTargetRole)) {
            metaDataTargetRole = NULL;
        }
        if (0 != pMetadata->getFieldValue (MetadataInterface::TARGET_TEAM, metaDataTargetTeam)) {
            metaDataTargetTeam = NULL;
        }
        if (0 != pMetadata->getFieldValue (MetadataInterface::RELEVANT_MISSION, metaDataTargetMission)) {
            metaDataTargetMission = NULL;
        }

        return MatchMakingPolicies::rankByTarget (metaDataTarget, metaDataTargetRole, metaDataTargetTeam,
                                                  metaDataTargetMission, pNodeContext);
    }

    Rank * checkRankByTarget (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                              MetadataRankerLocalConfiguration *pMetadataRankerLocalConf,
                              const Match &targetMatch, const String &metaDataTarget)
    {
        const MetadataRankerConfiguration *pNodeRankerCfg = pNodeContext->getMetaDataRankerConfiguration();
        if (pNodeRankerCfg->_bStrictTarget) {
            const String nodeId (pNodeContext->getNodeId());
            switch (targetMatch._match) {
                case Match::YES: {
                    Rank *pRank = RankFactory::getRank (pMetadata, nodeId, false, targetMatch._fMatchConfidence,
                                                        targetMatch._fMatchConfidence, pMetadataRankerLocalConf->_bInstrumented);
                    if (pRank != NULL) {
                        char szComment[256];
                        snprintf (szComment, 256, "Metadata's target is <%s>. It matches with peer <%s>, _bStrictTarget is set, therefore the metadata gets maximum match",
                                  metaDataTarget.c_str(), nodeId.c_str());
                        pRank->_loggingInfo._comment = szComment;
                    }
                    return pRank;
                }
                case Match::NO: {
                    Rank *pRank = RankFactory::getZeroRank (pMetadata, nodeId, pMetadataRankerLocalConf->_bInstrumented);
                    if (pRank != NULL) {
                        char szComment[256];
                        snprintf (szComment, 256, "Metadata's target is <%s>. It does not match with peer <%s>",
                                  metaDataTarget.c_str(), nodeId.c_str());
                        pRank->_loggingInfo._comment = szComment;
                    }
                    return pRank;
                }
                default:
                    // just continue
                    break;
            }
        }
        return NULL;
    }

    Match rankByImportance (MetadataInterface *pMetadata, NodeContext *pNodeContext)
    {
        double dImportance = MetadataInterface::IMPORTANCE_UNSET;
        if (0 != pMetadata->getFieldValue (MetadataInterface::IMPORTANCE, &dImportance)) {
            dImportance = MetadataInterface::IMPORTANCE_UNSET;
        }
        return MatchMakingPolicies::rankByImportance (dImportance);
    }

    Rank * checkRankByImportance (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                                  MetadataRankerLocalConfiguration *pMetadataRankerLocalConf,
                                  const Match &importanceMatch)
    {
        switch (importanceMatch._match) {
        case Match::YES: {
            if (importanceMatch._fMatchConfidence < MetadataRankerLocalConfiguration::MAX_RANK) {
                // just continue
                return NULL;
            }
            const String nodeId (pNodeContext->getNodeId());
            Rank *pRank = RankFactory::getRank (pMetadata, nodeId, false, importanceMatch._fMatchConfidence,
                importanceMatch._fMatchConfidence, pMetadataRankerLocalConf->_bInstrumented);
            if (pRank != NULL) {
                char szComment[256];
                snprintf (szComment, 256, "Metadata's imporance was set to MAX_RANK, therefore the metadata gets maximum match");
                pRank->_loggingInfo._comment = szComment;
            }
            return pRank;
        }
        default:
            // just continue
            return NULL;
        }
    }

    Match rankByCoord (MetadataInterface *pMetadata, NodeContext *pNodeContext, MetadataRankerLocalConfiguration *pMetadataRankerLocalConf,
                       Path *pPath, uint32 ui32UsefulDistance, const String &dataFormat, const BoundingBox &metadataBBox,
                       int &metadataWayPoint, int &iAdjustedCurrWayPointInPath, String &rankCoordComment)
    {
        const char *pszMethodName = "MetaDataRanker::rankByCoord";

        Match coordinateMatch (Match::NOT_SURE);
        const MetadataRankerConfiguration *pNodeRankerCfg = pNodeContext->getMetaDataRankerConfiguration();
        if (pNodeRankerCfg->_fCoordRankWeight > COORD_RANK_THRESHOLD) {
            coordinateMatch = MatchMakingPolicies::rankByPathCoordinates (metadataBBox, pPath, iAdjustedCurrWayPointInPath,
                                                                          ui32UsefulDistance, &metadataWayPoint,
                                                                          rankCoordComment);
            if (coordinateMatch._fMatchConfidence < 1.0f) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "data format %s; rank %f; useful distance = %d\n",
                                dataFormat.c_str(), coordinateMatch._fMatchConfidence, (int)ui32UsefulDistance);
            }
            if (coordinateMatch._match == Match::YES &&
                (coordinateMatch._fMatchConfidence < (MetadataRankerLocalConfiguration::MIN_RANK + 0.000001f))) {
                coordinateMatch._match = Match::NO;
                coordinateMatch._fMatchConfidence = MetadataRankerLocalConfiguration::MIN_RANK;
            }
        }

        return coordinateMatch;
    }

    Rank * checkRankByCoord (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                             MetadataRankerLocalConfiguration *pMetadataRankerLocalConf,
                             const BoundingBox &metadataBBox, const Match &coordinateMatch)
    {
        if (coordinateMatch._match == Match::NO) {
            const String nodeId (pNodeContext->getNodeId());
            Rank *pRank = RankFactory::getZeroRank (pMetadata, nodeId, pMetadataRankerLocalConf->_bInstrumented);
            if (pRank != NULL) {
                const MetadataRankerConfiguration *pNodeRankerCfg = pNodeContext->getMetaDataRankerConfiguration();
                pRank->addRank (MetaDataRanker::COORDINATES_RANK_DESCRIPTOR, coordinateMatch._fMatchConfidence, pNodeRankerCfg->_fCoordRankWeight);
                char szComment[256];
                snprintf (szComment, 256, "Coordinate rank is 0 for area (%f, %f) - (%f, %f), "
                          "the message is not sent, regardless of the value of the other ranks",
                          metadataBBox._leftUpperLatitude, metadataBBox._leftUpperLongitude,
                          metadataBBox._rightLowerLatitude, metadataBBox._rightLowerLongitude);
                pRank->_loggingInfo._comment = szComment;
            }
            return pRank;
        }
        return NULL;
    }

    Match rankByExpTime (MetadataInterface *pMetadata)
    {
        const char *pszMethodName = "MetaDataRanker::rankByExpTime";

        uint64 sourceTimeStamp, expirationTimeStamp;
        if (0 != pMetadata->getFieldValue (MetadataInterface::SOURCE_TIME_STAMP, &sourceTimeStamp)) {
            sourceTimeStamp = MetadataInterface::SOURCE_TIME_STAMP_UNSET;
        }
        if (0 != pMetadata->getFieldValue (MetadataInterface::EXPIRATION_TIME, &expirationTimeStamp)) {
            expirationTimeStamp = MetadataInterface::EXPIRATION_TIME_UNSET;
        }

        Match expirationMatch = MatchMakingPolicies::rankByExpirationTime (sourceTimeStamp, expirationTimeStamp);
        if (expirationMatch._match == Match::NOT_SURE) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "It is not possible to "
                            "rank the given MetaData by expiration time.\n");
        }

        return expirationMatch;
    }

    Match rankByPred (NodeContext *pNodeContext, Prediction *pPrediction)
    {
        const char *pszMethodName = "MetaDataRanker::rankByPred";

        Match predictionMatch (Match::NOT_SURE);
        const MetadataRankerConfiguration *pNodeRankerCfg = pNodeContext->getMetaDataRankerConfiguration();
        if (pNodeRankerCfg->_fPredRankWeight > 0.0f) {
            predictionMatch = MatchMakingPolicies::rankByPrediction (pPrediction);
            if (predictionMatch._match == Match::NOT_SURE) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "It is not possible "
                                "to rank the given metadata using the prediction.\n");
            }
        }
        return predictionMatch;
    }

    /*
     * Returns a Rank if the matchmaking for the metadata should be skipped, NULL otherwise
     */
    Rank * filter (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                   MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
    {
        Rank *pRank = filterByDataType (pMetadata, pNodeContext, pMetadataRankerLocalConf);
        if (pRank != NULL) {
            return pRank;
        }
        pRank = filterByPedigree (pMetadata, pNodeContext, pMetadataRankerLocalConf);
        return pRank;
    }

    /*
     * Compute the rank for a single metadata for the push mode.
     */
    Rank * rankInternal (MetadataInterface *pMetadata, NodeContext *pNodeContext, Prediction *pPrediction,
                         MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
    {
        const char *pszMethodName = "MetaDataRanker::rankInternal";

        // Rank by Target (if _bStrictTarget is set, and the target rank matches, then it supersedes all
        // the other ranks)
        String metaDataTarget;
        const Match targetMatch = METADATA_RANKER::rankByTarget (pMetadata, pNodeContext, metaDataTarget);
        Rank *pRank = METADATA_RANKER::checkRankByTarget (pMetadata, pNodeContext, pMetadataRankerLocalConf, targetMatch, metaDataTarget);
        if (pRank != NULL) {
            return pRank;
        }

        // Rank by importance (if the application specified value of importance is set to MAX_RANK,
        // then it supersedes all the other ranks)
        const Match importanceMatch = METADATA_RANKER::rankByImportance (pMetadata, pNodeContext);
        pRank = METADATA_RANKER::checkRankByImportance (pMetadata, pNodeContext, pMetadataRankerLocalConf, importanceMatch);
        if (pRank != NULL) {
            return pRank;
        }

        // Rank by coordinates (if coordinate do not match, this rank supersedes all the others)
        int metadataWayPoint = 0;
        int iAdjustedCurrWayPointInPath = 0;
        Path *pPath = METADATA_RANKER::getPath (pNodeContext, iAdjustedCurrWayPointInPath);
        String dataFormat;
        if (0 != pMetadata->getFieldValue (MetadataInterface::DATA_FORMAT, dataFormat)) {
            dataFormat = NULL;
        }
        String rankCoordComment;
        const uint32 ui32UsefulDistance = pNodeContext->getUsefulDistance (dataFormat);
        const BoundingBox metadataBBox (METADATA_RANKER::getBoundingBox (pNodeContext, pMetadata, pMetadataRankerLocalConf));
        Match coordinateMatch = METADATA_RANKER::rankByCoord (pMetadata, pNodeContext, pMetadataRankerLocalConf,
                                                              pPath, ui32UsefulDistance, dataFormat, metadataBBox,
                                                              metadataWayPoint, iAdjustedCurrWayPointInPath, rankCoordComment);
        Match areaOfInterestMatch (Match::NO);
        AreaOfInterestList *pAreasOfInterest = pNodeContext->getAreasOfInterest();
        if ((pAreasOfInterest != NULL) && (!pAreasOfInterest->isEmpty())) {
            const BoundingBox metadataBB (pMetadata->getLocation (0.0f));
            areaOfInterestMatch = MatchMakingPolicies::rankByAreasOfInterest (metadataBB, pAreasOfInterest);
            pAreasOfInterest->removeAll (true);
            delete pAreasOfInterest;
        }
        pRank = METADATA_RANKER::checkRankByCoord (pMetadata, pNodeContext, pMetadataRankerLocalConf,
                                                   metadataBBox, coordinateMatch);
        if (pRank != NULL) {
            return pRank;
        }

        const MetadataRankerConfiguration *pNodeRankerCfg = pNodeContext->getMetaDataRankerConfiguration();

        // Rank by time
        Match timeMatch (Match::NOT_SURE);
        if (coordinateMatch._match == Match::YES) {
            timeMatch = MatchMakingPolicies::rankByPathTimeStamps (pNodeContext->getPath(), iAdjustedCurrWayPointInPath,
                                                                   metadataWayPoint, ui32UsefulDistance,
                                                                   pNodeRankerCfg->_bConsiderFuturePathSegmentForMatchmacking);
            if (timeMatch._match == Match::NOT_SURE) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "It is not possible to rank "
                                "the given MetaData by time stamps in path.\n");
            }
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "It is not possible to rank the given MetaData time stamps in path.\n");
        }

        // Adjust coordinate and time ranks if the area of interest matches
        if (areaOfInterestMatch > coordinateMatch) {
            coordinateMatch = areaOfInterestMatch;
        }
        if (areaOfInterestMatch._match == Match::YES) {
            timeMatch._fMatchConfidence = 10.0f;
        }

        double dSourceReliability = 0.0f;
        if (0 != pMetadata->getFieldValue (MetadataInterface::SOURCE_RELIABILITY, &dSourceReliability)) {
            dSourceReliability = 0.0f;
        }
        const Match sourceReliabilityMatch = MatchMakingPolicies::toRank ((float)dSourceReliability, 0.0f, 10.0f);

        double dInformationContent = 0.0f;
        if (0 != pMetadata->getFieldValue (MetadataInterface::INFORMATION_CONTENT, &dInformationContent)) {
            dInformationContent = 0.0f;
        }
        const Match informationContentMatch = MatchMakingPolicies::toRank ((float)dSourceReliability, 0.0f, 10.0f);

        // Rank by expiration time
        const Match expirationMatch = METADATA_RANKER::rankByExpTime (pMetadata);

        // Rank by prediction
        const Match predictionMatch = METADATA_RANKER::rankByPred (pNodeContext, pPrediction);

        float fRank = 0.0f;
        float fWeightSum = 0.0f;

        if (!pNodeRankerCfg->_bStrictTarget) {
            // If it is configured as _bStrictTarget, rankTarget is always either 0 or
            // 10, so it does not have to be taken into account when computing the final
            // rank
            if (targetMatch._match != Match::NOT_SURE) {
                fRank += targetMatch._fMatchConfidence * pNodeRankerCfg->_fTargetRankWeight;
                fWeightSum += pNodeRankerCfg->_fTargetRankWeight;
            }
        }
        if (importanceMatch._match != Match::NOT_SURE) {
            fRank += importanceMatch._fMatchConfidence * pNodeRankerCfg->_fImpRankWeight;
            fWeightSum += pNodeRankerCfg->_fImpRankWeight;
        }
        if (sourceReliabilityMatch._match != Match::NOT_SURE) {
            fRank += sourceReliabilityMatch._fMatchConfidence * pNodeRankerCfg->_fSourceReliabilityRankWeigth;
            fWeightSum += pNodeRankerCfg->_fSourceReliabilityRankWeigth;
        }
        if (informationContentMatch._match != Match::NOT_SURE) {
            fRank += informationContentMatch._fMatchConfidence * pNodeRankerCfg->_fInformationContentRankWeigth;
            fWeightSum += pNodeRankerCfg->_fInformationContentRankWeigth;
        }
        if (pNodeRankerCfg->_fCoordRankWeight > METADATA_RANKER::COORD_RANK_THRESHOLD) {
            fRank += coordinateMatch._fMatchConfidence * pNodeRankerCfg->_fCoordRankWeight;
            fWeightSum += pNodeRankerCfg->_fCoordRankWeight;
        }
        if (timeMatch._match != Match::NOT_SURE) {
            fRank += timeMatch._fMatchConfidence * pNodeRankerCfg->_fTimeRankWeight;
            fWeightSum += pNodeRankerCfg->_fTimeRankWeight;
        }
        if (expirationMatch._match != Match::NOT_SURE) {
            fRank += expirationMatch._fMatchConfidence * pNodeRankerCfg->_fExpirationRankWeight;
            fWeightSum += pNodeRankerCfg->_fExpirationRankWeight;
        }
        if (predictionMatch._match != Match::NOT_SURE) {
            fRank += predictionMatch._fMatchConfidence * pNodeRankerCfg->_fPredRankWeight;
            fWeightSum += pNodeRankerCfg->_fPredRankWeight;
        }

        // Use custom policies
        PtrLList<CustomPolicy> *pPolicies = pNodeContext->getCustomPolicies();
        if (pPolicies != NULL) {
            for (CustomPolicy *pPolicy = pPolicies->getFirst(); pPolicy != NULL; pPolicy = pPolicies->getNext()) {
                const Match match (pPolicy->rank (pMetadata));
                if (match._match != Match::NOT_SURE) {
                    fRank += match._fMatchConfidence * pPolicy->getRankWeight();
                    fWeightSum += pPolicy->getRankWeight();
                }
            }
        }

        if (fWeightSum > 0.0f) {
            // Compute weighted average
            fRank = (fRank / fWeightSum);
        }
        else {
            fRank = 0.0f;
        }

        const String nodeId (pNodeContext->getNodeId());
        const float fRankTime = (timeMatch._match == Match::NOT_SURE ? MetadataRankerLocalConfiguration::DEF_RANK : timeMatch._fMatchConfidence);
        pRank = RankFactory::getRank (pMetadata, nodeId, false, fRank, fRankTime, pMetadataRankerLocalConf->_bInstrumented);
        if (pRank != NULL) {
            pRank->addRank (MetaDataRanker::COORDINATES_RANK_DESCRIPTOR, coordinateMatch._fMatchConfidence, pNodeRankerCfg->_fCoordRankWeight);
            pRank->addRank (MetaDataRanker::TIME_RANK_DESCRIPTOR, timeMatch._fMatchConfidence, pNodeRankerCfg->_fTimeRankWeight);
            pRank->addRank (MetaDataRanker::EXPIRATION_RANK_DESCRIPTOR, expirationMatch._fMatchConfidence, pNodeRankerCfg->_fExpirationRankWeight);
            pRank->addRank (MetaDataRanker::IMPORTANCE_RANK_DESCRIPTOR, importanceMatch._fMatchConfidence, pNodeRankerCfg->_fImpRankWeight);
            pRank->addRank (MetaDataRanker::SOURCE_RELIABILITY_RANK_DESCRIPTOR, sourceReliabilityMatch._fMatchConfidence, pNodeRankerCfg->_fSourceReliabilityRankWeigth);
            pRank->addRank (MetaDataRanker::INFORMATION_CONTENT_RANK_DESCRIPTOR, informationContentMatch._fMatchConfidence, pNodeRankerCfg->_fInformationContentRankWeigth);
            pRank->addRank (MetaDataRanker::PREDICTION_RANK_DESCRIPTOR, predictionMatch._fMatchConfidence, pNodeRankerCfg->_fPredRankWeight);
            pRank->addRank (MetaDataRanker::TARGET_RANK_DESCRIPTOR, targetMatch._fMatchConfidence, pNodeRankerCfg->_fTargetRankWeight);
            pRank->_loggingInfo._comment = rankCoordComment;
        }

        return pRank;
    }
}

Rank * MetaDataRanker::rank (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                             MetadataConfiguration *pMetadataConf,
                             MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
{
    const char *pszMethodName = "MetaDataRanker::rank (1)";
    if (pMetadata == NULL) {
        String msg = nullMetadata;
        Rank *pRank = RankFactory::getZeroRank (NULL, NULL, pMetadataRankerLocalConf->_bInstrumented);
        if (pRank != NULL) {
            pRank->_loggingInfo._comment = msg;
        }
        msg += "\n";
        checkAndLogMsg (pszMethodName, Logger::L_Warning, msg);
        return pRank;
    }
    if (pNodeContext == NULL) {
        String msg = nullNodeContext;
        Rank *pRank = RankFactory::getZeroRank (pMetadata, NULL, pMetadataRankerLocalConf->_bInstrumented);
        if (pRank != NULL) {
            pRank->_loggingInfo._comment = msg;
        }
        msg += "\n";
        checkAndLogMsg (pszMethodName, Logger::L_Warning, msg);
        return pRank;
    }

    // Check whether the data should be filtered for the peer
    Rank *pRank = METADATA_RANKER::filter (pMetadata, pNodeContext, pMetadataRankerLocalConf);
    if (pRank != NULL) {
        return pRank;
    }

    // Retrieve the prediction
    Prediction *pPrediction = C45Utils::getPrediction (pMetadata, pNodeContext, pMetadataConf);

    pRank = METADATA_RANKER::rankInternal (pMetadata, pNodeContext, pPrediction, pMetadataRankerLocalConf);
    if (pRank != NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "CALCULATED RANK IS = %f\n", pRank->_fTotalRank);
    }

    if (pPrediction != NULL) {
        delete pPrediction;
    }

    return pRank;
}

Ranks * MetaDataRanker::rank (MetadataList *pMetadataList, NodeContext *pNodeContext,
                              MetadataConfiguration *pMetadataConf,
                              MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
{
    const char *pszMethodName = "MetaDataRanker::rank (2)";
    if (pMetadataList == NULL || pMetadataList->isEmpty() || pNodeContext == NULL ||
        pMetadataConf == NULL || pMetadataRankerLocalConf == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, nullParameters);
        return NULL;
    }

    Ranks *pRanks = NULL;
    for (MetadataInterface *pMetadata = pMetadataList->getFirst(); pMetadata != NULL;
         pMetadata = pMetadataList->getNext()) {
        Rank *pRank = rank (pMetadata, pNodeContext, pMetadataConf, pMetadataRankerLocalConf);
        if (pRank != NULL) {
            if (pRanks == NULL) {
                pRanks = new Ranks();
            }
            if (pRanks != NULL) {
                pRanks->insert (pRank);
            }
        }
    }

    return pRanks;
}

Ranks * MetaDataRanker::rank (MetadataList *pMetadataList, NodeContextList *pNodeCtxtList,
                              MetadataConfiguration *pMetadataCfg,
                              MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg)
{
    const char *pszMethodName = "MetaDataRanker::rank (3)";
    if (pMetadataList == NULL || pMetadataList->isEmpty() ||
        pNodeCtxtList == NULL || pNodeCtxtList->isEmpty() ||
        pMetadataCfg == NULL || pMetadataCfg == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, nullParameters);
        return NULL;
    }

    Ranks *pRanks = NULL;
    for (NodeContext *pNodeContext = pNodeCtxtList->getFirst(); pNodeContext != NULL; pNodeContext = pNodeCtxtList->getNext()) {
        Ranks *pPartialRanks = rank (pMetadataList, pNodeContext, pMetadataCfg, pMetadataRankerLocalCfg);
        if (pPartialRanks != NULL) {
            if (pRanks == NULL) {
                pRanks = new Ranks();
            }
            if (pRanks != NULL) {
                pRanks->insertAll (pPartialRanks);
            }
        }
    }

    return pRanks;
}

