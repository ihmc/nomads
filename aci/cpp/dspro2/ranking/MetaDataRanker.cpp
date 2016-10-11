/* 
 * MetaDataRanker.cpp
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
 */

#include "MetaDataRanker.h"

#include "CustomPolicies.h"
#include "Defs.h"
#include "MatchMakingPolicies.h"
#include "MetaData.h"
#include "MetadataConfiguration.h"
#include "MetadataRankerConfiguration.h"
#include "NodePath.h"
#include "NodeContext.h"
#include "Pedigree.h"
#include "SQLAVList.h"
#include "Rank.h"
#include "RankFactory.h"

#include "Classifier.h"
#include "C45AVList.h"
#include "Prediction.h"

#include "SymbolCode.h"

#include "Logger.h"
#include "NLFLib.h"
#include "BoundingBox.h"

#include <string.h>

#define nullMetadata "The given MetaData is a NULL pointer. Could not rank it"
#define nullMetadataConf Logger::L_Warning, "MetadataConfiguration is not properly initialzied. Can't use the classifier\n"
#define nullNodeContext "The given NodeContext is a NULL pointer. Could not rank it"

using namespace IHMC_C45;
using namespace IHMC_MISC_MIL_STD_2525;
using namespace IHMC_ACI;
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

Rank * MetaDataRanker::rank (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                             MetadataConfiguration *pMetadataConf,
                             MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
{
    if (pMetadata == NULL) {
        String msg = nullMetadata
                     "\n";
        checkAndLogMsg ("MetaDataRanker::rank", Logger::L_Warning, msg);
        return NULL;
    }
    if (pNodeContext == NULL) {
        String msg = nullNodeContext;
        Rank *pRank = RankFactory::getZeroRank (pMetadata, pNodeContext->getNodeId(), pMetadataRankerLocalConf->_bInstrumented);
        if (pRank != NULL) {
            pRank->_loggingInfo._comment = msg;
        }
        msg += "\n";
        checkAndLogMsg ("MetaDataRanker::rank", Logger::L_Warning, msg);
        return pRank;
    }

    // Check whether the data should be filtered for the peer
    Rank *pRank = filter (pMetadata, pNodeContext, pMetadataRankerLocalConf);
    if (pRank != NULL) {
        return pRank;
    }

    // Retrieve the prediction
    Prediction *pPrediction = NULL;
    C45AVList *pRecord = NULL;
    if (pMetadataConf != NULL) {
        MetadataRankerConfiguration *pRankerConf = pNodeContext->getMetaDataRankerConfiguration();
        if (pRankerConf->_fPredRankWeight > 0.0f) {
            pRecord = pMetadataConf->getMetadataAsC45List (pMetadata);
            if (pRecord != NULL) {
                Classifier *pClassifier = pNodeContext->getClassifier();
                if (pClassifier != NULL) {
                    pPrediction = (IHMC_C45::Prediction *) pClassifier->consultClassifier (pRecord);
                }
            }
            else {
                checkAndLogMsg ("MetaDataRanker::pushRank", Logger::L_Warning,
                                "Can not retrieve metadata as C45 list from information store\n");
            }
        }
    }
    else {
        checkAndLogMsg ("MetaDataRanker::pushRank", nullMetadataConf);
    }

    pRank = rankInternal (pMetadata, pNodeContext, pPrediction, pMetadataRankerLocalConf);
    if ((pRank != NULL) && (pRank != NULL)) {
        checkAndLogMsg ("MetaDataRanker::rank", Logger::L_Info,
                        "CALCULATED RANK IS = %f\n", pRank->_fTotalRank);
    }

    if (pPrediction != NULL) {
        delete pPrediction;
        pPrediction = NULL;
    }
    if (pRecord != NULL) {
        delete pRecord;
        pRecord = NULL;
    }

    return pRank;
}

Ranks * MetaDataRanker::rank (MetadataList *pMetadataList, NodeContext *pNodeContext,
                              MetadataConfiguration *pMetadataConf,
                              MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
{
    if (pMetadataList == NULL || pMetadataList->getFirst() == NULL || pNodeContext == NULL ||
        pMetadataConf == NULL || pMetadataRankerLocalConf == NULL) {
        checkAndLogMsg ("MetaDataRanker::rank", Logger::L_MildError,
                        "NULL parameter or pFields contains 0 fields. Could not rank them.\n");
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

Rank * MetaDataRanker::filter (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                               MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
{
    String dataFormat;
    if (0 != pMetadata->getFieldValue (MetadataInterface::DATA_FORMAT, dataFormat)) {
        dataFormat = NULL;
    }

    if (pMetadataRankerLocalConf->_dataTypesToFilter.containsKey (dataFormat)) {
        Rank *pRank = RankFactory::getFilteredRank (pMetadata, pNodeContext->getNodeId(), pMetadataRankerLocalConf->_bInstrumented);
        if (pRank != NULL) {
            char szComment[256];
            sprintf (szComment, "Metadata of type %s is configured not to match\n", dataFormat.c_str());
            pRank->_loggingInfo._comment = szComment;
            return pRank;
        }
    }

    if ((pMetadataRankerLocalConf != NULL) && (dataFormat.length() > 0) && pMetadataRankerLocalConf->_dataTypesNotToMatchByNodeId.hasKeyValue (pNodeContext->getNodeId(), dataFormat)) {
        Rank *pRank = RankFactory::getFilteredRank (pMetadata, pNodeContext->getNodeId(), pMetadataRankerLocalConf->_bInstrumented);
        if (pRank != NULL) {
            char szComment[256];
            sprintf (szComment, "Metadata of type %s is configured not to match for peer %s.\n", dataFormat.c_str(), pNodeContext->getNodeId());
            pRank->_loggingInfo._comment = szComment;
            return pRank;
        }
    }

    char *pszSource = NULL;
    if (0 != pMetadata->getFieldValue (MetaData::SOURCE, &pszSource)) {
        pszSource = NULL;
    }

    if (pszSource != NULL) {
        if (strcmp (pszSource, pNodeContext->getNodeId()) == 0) {
            Rank *pRank = RankFactory::getFilteredRank (pMetadata, pNodeContext->getNodeId(), pMetadataRankerLocalConf->_bInstrumented);
            if (pRank != NULL) {
                char szComment[256];
                sprintf (szComment, "Node %s is the source of the metadata\n", pszSource);
                pRank->_loggingInfo._comment = szComment;
            }
            free (pszSource);
            return pRank;
        }
        else if ((pMetadataRankerLocalConf != NULL) && (pMetadataRankerLocalConf->getLimitToLocalMatchmakingOnly()) &&
                 (pMetadataRankerLocalConf->_nodeId != pszSource)) {
            // The node is configured to pre-stage only the messages for which it is the source
            Rank *pRank = RankFactory::getFilteredRank (pMetadata, pNodeContext->getNodeId(),
                                                        pMetadataRankerLocalConf->_bInstrumented);
            if (pRank != NULL) {
                char szComment[256];
                sprintf (szComment, "Node configured to only replicate its own data\n");
                pRank->_loggingInfo._comment = szComment;
            }
            free (pszSource);
            return pRank;
        }
    }

    char *pszPedigree = NULL;
    if (0 != pMetadata->getFieldValue (MetaData::PEDIGREE, &pszPedigree)) {
        pszPedigree = NULL;
    }

    if (pszPedigree != NULL && (0 != strcmp (pszPedigree, "")) &&
            (0 != strcmp (pszPedigree, SQLAVList::UNKNOWN))) {
        if ((pMetadataRankerLocalConf != NULL) && (pMetadataRankerLocalConf->getLimitToLocalMatchmakingOnly())) {
            // The node is configured to pre-stage only the nodes for which it is the source
            Rank *pRank = RankFactory::getFilteredRank (pMetadata, pNodeContext->getNodeId(),
                                                        pMetadataRankerLocalConf->_bInstrumented);
            if (pRank != NULL) {
                char szComment[256];
                sprintf (szComment, "Node configured to only replicate its own "
                         "data contains pedigree (%s)\n", pszPedigree);
                pRank->_loggingInfo._comment = szComment;
            }
            free (pszPedigree);
            if (pszSource != NULL) {
                free (pszSource);
            }
            return pRank;
        }
        Pedigree pedigree (pszSource, pszPedigree);
        if (pedigree.containsNodeID (pNodeContext->getNodeId(), true)) {
            Rank *pRank = RankFactory::getFilteredRank (pMetadata, pNodeContext->getNodeId(),
                                                        pMetadataRankerLocalConf->_bInstrumented);
            if (pRank != NULL) {
                char szComment[256];
                sprintf (szComment, "Node %s is the pedigree %s\n",
                         pNodeContext->getNodeId(), pszPedigree);
                pRank->_loggingInfo._comment = szComment;
            }
            if (pszSource != NULL) {
                free (pszSource);
            }
            free (pszPedigree);
            return pRank;
        }
    }

    if (pszSource != NULL) {
        free (pszSource);
    }
    if (pszPedigree != NULL) {
        free (pszPedigree);
    }

    return NULL;
}

Rank * MetaDataRanker::rankInternal (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                                     Prediction *pPrediction,
                                     MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
{
    const char *pszMethodName = "MetaDataRanker::rankInternal";

    // Rank by Target (if _bStrictTarget is set, and the target rank matches, then it supersedes all
    // the other ranks)
    String metaDataTarget, metaDataTargetRole, metaDataTargetTeam, metaDataTargetMission, dataFormat;
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
    if (0 != pMetadata->getFieldValue (MetadataInterface::DATA_FORMAT, dataFormat)) {
        dataFormat = NULL;
    }

    const MetadataRankerConfiguration *pNodeRankerCfg = pNodeContext->getMetaDataRankerConfiguration();
    Match targetMatch = MatchMakingPolicies::rankByTarget (metaDataTarget, metaDataTargetRole,
                                                           metaDataTargetTeam, metaDataTargetMission,
                                                           pNodeContext);
    if (pNodeRankerCfg->_bStrictTarget) {
        switch (targetMatch._match) {
            case Match::YES: {
                Rank *pRank = RankFactory::getRank (pMetadata, pNodeContext->getNodeId(), false,
                                                    targetMatch._fMatchConfidence, targetMatch._fMatchConfidence,
                                                    pMetadataRankerLocalConf->_bInstrumented);
                if (pRank != NULL) {
                    char szComment[256];
                    sprintf (szComment, "Metadata's target is <%s>. It matches with peer <%s>, _bStrictTarget is set, therefore the metadata gets maximum match",
                             metaDataTarget.c_str(), pNodeContext->getNodeId());
                    pRank->_loggingInfo._comment = szComment;
                }
                return pRank;
            }
            case Match::NO: {
                Rank *pRank = RankFactory::getZeroRank (pMetadata, pNodeContext->getNodeId(),
                                                        pMetadataRankerLocalConf->_bInstrumented);
                if (pRank != NULL) {
                    char szComment[256];
                    sprintf (szComment, "Metadata's target is <%s>. It does not match with peer <%s>",
                             metaDataTarget.c_str(), pNodeContext->getNodeId());
                    pRank->_loggingInfo._comment = szComment;
                }
                return pRank;
            }
            default:
                // just continue
                break;
                
        }
    }

    // Rank by importance (if the application specified value of importance is set to MAX_RANK,
    // then it supersedes all the other ranks)
    double dImportance = MetaData::IMPORTANCE_UNSET;
    if (0 != pMetadata->getFieldValue (MetaData::IMPORTANCE, &dImportance)) {
        dImportance = MetaData::IMPORTANCE_UNSET;
    }
    Match importanceMatch = MatchMakingPolicies::rankByImportance (dImportance);
    switch (importanceMatch._match) {
        case Match::YES: {
            if (importanceMatch._fMatchConfidence < MetadataRankerLocalConfiguration::MAX_RANK) {
                // just continue
                break;
            }
            else {
                Rank *pRank = RankFactory::getRank (pMetadata, pNodeContext->getNodeId(), false,
                                                    importanceMatch._fMatchConfidence, importanceMatch._fMatchConfidence,
                                                    pMetadataRankerLocalConf->_bInstrumented);
                if (pRank != NULL) {
                    char szComment[256];
                    sprintf (szComment, "Metadata's imporance was set to MAX_RANK, therefore the metadata gets maximum match");
                    pRank->_loggingInfo._comment = szComment;
                }
                return pRank;
            }
        }
        default:
            // just continue
            break;
    }

    double dSourceReliability = 0.0f;
    if (0 != pMetadata->getFieldValue (MetaData::SOURCE_RELIABILITY, &dSourceReliability)) {
        dSourceReliability = 0.0f;
    }
    Match sourceReliabilityMatch = MatchMakingPolicies::toRank ((float) dSourceReliability, 0.0f, 10.0f);

    double dInformationContent = 0.0f;
    if (0 != pMetadata->getFieldValue (MetaData::INFORMATION_CONTENT, &dInformationContent)) {
        dInformationContent = 0.0f;
    }
    Match informationContentMatch = MatchMakingPolicies::toRank ((float) dSourceReliability, 0.0f, 10.0f);

    // Rank by coordinate (if coordinate do not match, this rank supersedes all the others)
    NodeContext::NodeStatus status = pNodeContext->getStatus();
    Path *pPath = NULL;
    int iAdjustedCurrWayPointInPath = 0;
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

    String description;
    pMetadata->getFieldValue (MetaData::DESCRIPTION, description);
    SymbolCode symbolCode (description);
    uint32 ui32RangeOfInfluence = 0U;
    if (symbolCode.isValid()) {
        ui32RangeOfInfluence = pMetadataRankerLocalConf->getRangeOfInfluence (symbolCode);
        checkAndLogMsg (pszMethodName, Logger::L_Info, "range of influence for %s is %u.\n",
                        description.c_str(), ui32RangeOfInfluence);
    }
    else if (pMetadata->getFieldValue ("Node_Type", description) == 0) {
        SymbolCode symbolCode2 (description);
        if (symbolCode2.isValid()) {
            ui32RangeOfInfluence = pMetadataRankerLocalConf->getRangeOfInfluence (symbolCode2);
            checkAndLogMsg (pszMethodName, Logger::L_Info, "range of influence for %s is %u.\n",
                            description.c_str(), ui32RangeOfInfluence);
        }
    }
    const BoundingBox metadataBBox = BoundingBox::getBoundingBox (*pMetadata, ui32RangeOfInfluence);

    int metadataWayPoint;
    const uint32 ui32UsefulDistance = pNodeContext->getUsefulDistance (dataFormat);
    const float fCoordRankThreshold = 0.000001f;
    Match coordinateMatch (Match::NOT_SURE);
    String rankCoordComment;
    if (pNodeRankerCfg->_fCoordRankWeight > fCoordRankThreshold) {
        coordinateMatch = MatchMakingPolicies::rankByPathCoordinates (metadataBBox, pPath, iAdjustedCurrWayPointInPath,
                                                                      ui32UsefulDistance, &metadataWayPoint,
                                                                      rankCoordComment);
        if (coordinateMatch._fMatchConfidence < 1.0f) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "data format %s; rank %f; useful distance = %d\n",
                            dataFormat.c_str(), coordinateMatch._fMatchConfidence, (int) ui32UsefulDistance);
        }
        if (coordinateMatch._match == Match::YES &&
            (coordinateMatch._fMatchConfidence < (MetadataRankerLocalConfiguration::MIN_RANK + 0.000001f))) {
            coordinateMatch._match = Match::NO;
            coordinateMatch._fMatchConfidence = MetadataRankerLocalConfiguration::MIN_RANK;
        }
    }

    if (coordinateMatch._match == Match::NO) {
        Rank *pRank = RankFactory::getZeroRank (pMetadata, pNodeContext->getNodeId(),
                                                pMetadataRankerLocalConf->_bInstrumented);
        if (pRank != NULL) {
            pRank->addRank (COORDINATES_RANK_DESCRIPTOR, coordinateMatch._fMatchConfidence, pNodeRankerCfg->_fCoordRankWeight);
            char szComment[256];
            sprintf (szComment, "Coordinate rank is 0 for area (%f, %f) - (%f, %f), "
                     "the message is not sent, regardless of the value of the other ranks",
                     metadataBBox._leftUpperLatitude, metadataBBox._leftUpperLongitude,
                     metadataBBox._rightLowerLatitude, metadataBBox._rightLowerLongitude);
            pRank->_loggingInfo._comment = szComment;
        }
        return pRank;
    }

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

    // Rank by expiration time
    uint64 sourceTimeStamp, expirationTimeStamp;
    if (0 != pMetadata->getFieldValue (MetaData::SOURCE_TIME_STAMP, &sourceTimeStamp)) {
        sourceTimeStamp = MetaData::SOURCE_TIME_STAMP_UNSET;
    }
    if (0 != pMetadata->getFieldValue (MetaData::EXPIRATION_TIME, &expirationTimeStamp)) {
        expirationTimeStamp = MetaData::EXPIRATION_TIME_UNSET;
    }

    Match expirationMatch = MatchMakingPolicies::rankByExpirationTime (sourceTimeStamp, expirationTimeStamp);
    if (expirationMatch._match == Match::NOT_SURE) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,  "It is not possible to "
                        "rank the given MetaData by expiration time.\n");
    }

    // Rank by prediction
    Match predictionMatch (Match::NOT_SURE);
    if (pNodeRankerCfg->_fPredRankWeight > 0.0f) {
        predictionMatch = MatchMakingPolicies::rankByPrediction (pPrediction);
        if (predictionMatch._match == Match::NOT_SURE) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "It is not possible "
                            "to rank the given metadata using the prediction.\n");
        }
    }

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
    if (pNodeRankerCfg->_fCoordRankWeight > fCoordRankThreshold) {
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
    CustomPolicies *pPolicies = pNodeContext->getCustomPolicies();
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

    const float fRankTime = (timeMatch._match == Match::NOT_SURE ? MetadataRankerLocalConfiguration::DEF_RANK : timeMatch._fMatchConfidence);
    Rank *pRank = RankFactory::getRank (pMetadata, pNodeContext->getNodeId(), false,
                                        fRank, fRankTime, pMetadataRankerLocalConf->_bInstrumented);
    if (pRank != NULL) {
        pRank->addRank (COORDINATES_RANK_DESCRIPTOR, coordinateMatch._fMatchConfidence, pNodeRankerCfg->_fCoordRankWeight);
        pRank->addRank (TIME_RANK_DESCRIPTOR, timeMatch._fMatchConfidence, pNodeRankerCfg->_fTimeRankWeight);
        pRank->addRank (EXPIRATION_RANK_DESCRIPTOR, expirationMatch._fMatchConfidence, pNodeRankerCfg->_fExpirationRankWeight);
        pRank->addRank (IMPORTANCE_RANK_DESCRIPTOR, importanceMatch._fMatchConfidence, pNodeRankerCfg->_fImpRankWeight);
        pRank->addRank (SOURCE_RELIABILITY_RANK_DESCRIPTOR, sourceReliabilityMatch._fMatchConfidence, pNodeRankerCfg->_fSourceReliabilityRankWeigth);
        pRank->addRank (INFORMATION_CONTENT_RANK_DESCRIPTOR, informationContentMatch._fMatchConfidence, pNodeRankerCfg->_fInformationContentRankWeigth);
        pRank->addRank (PREDICTION_RANK_DESCRIPTOR, predictionMatch._fMatchConfidence, pNodeRankerCfg->_fPredRankWeight);
        pRank->addRank (TARGET_RANK_DESCRIPTOR, targetMatch._fMatchConfidence, pNodeRankerCfg->_fTargetRankWeight);
        pRank->_loggingInfo._comment = rankCoordComment;
    }

    return pRank;
}
