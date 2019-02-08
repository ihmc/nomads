/*
 * LocalNodeContext.cpp
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

#include "LocalNodeContext.h"

#include "C45LocalNodeContext.h"
#include "Defs.h"
#include "MetadataConfigurationImpl.h"
#include "MetadataRankerConfiguration.h"
#include "NodePath.h"
#include "NonClassifyingLocalNodeContext.h"

#include "C45DecisionTree.h"
#include "C45AVList.h"

#include "Logger.h"
#include "NLFLib.h"
#include "ConfigManager.h"
#include "StrClass.h"

using namespace NOMADSUtil;
using namespace IHMC_C45;
using namespace IHMC_VOI;
using namespace IHMC_ACI;
using namespace IHMC_MISC_MIL_STD_2525;

const char * LocalNodeContext::TEAM_ID_PROPERTY = "aci.dspro.localNodeContext.teamId";
const char * LocalNodeContext::MISSION_ID_PROPERTY = "aci.dspro.localNodeContext.missionId";
const char * LocalNodeContext::ROLE_PROPERTY = "aci.dspro.localNodeContext.role";
const char * LocalNodeContext::NODE_TYPE_PROPERTY = "aci.dspro.localNodeContext.nodeType";
const char * LocalNodeContext::LIMIT_PRESTAGING_TO_LOCAL_DATA_PROPERTY = "aci.dspro.localNodeContext.limitPrestagingToLocalData";
const char * LocalNodeContext::MATCHMAKING_QUALIFIERS = "aci.dspro.localNodeContext.mathcmakingQualifiers";

LocalNodeContext::LocalNodeContext (const char *pszNodeId, Classifier *pClassifier,
                                    double dTooFarCoeff, double dApproxCoeff)
    : NodeContextImpl (pszNodeId, dTooFarCoeff, dApproxCoeff)
{
    _i64StartingTime = getTimeInMilliseconds();
}

LocalNodeContext::~LocalNodeContext (void)
{
}

LocalNodeContext * LocalNodeContext::getInstance (const char *pszNodeId, ConfigManager *pCfgMgr,
                                                  MetadataConfigurationImpl *pMetadataConf)
{
    if (pCfgMgr == nullptr) {
        return nullptr;
    }

    const char *pszMethodName = "LocalNodeContext::getInstance";

    LocalNodeContext *_pLocalNodeContext = nullptr;
    NOMADSUtil::String classifier = pCfgMgr->getValue ("aci.dspro.localNodeContext.classifier.type",
                                                       NonClassifyingLocalNodeContext::TYPE);

    if (classifier == C45LocalNodeContext::TYPE) {
        // Instantiate classifier
        C45LocalNodeContext *pC45Class = new C45LocalNodeContext (pszNodeId,
                                                                  NodeContext::TOO_FAR_FROM_PATH_COEFF,
                                                                  NodeContext::APPROXIMABLE_TO_POINT_COEFF);

        // Configure classifier
        int rc = 0;
        if (pCfgMgr != nullptr) {
            NOMADSUtil::String algorithm = pCfgMgr->getValue ("aci.dspro.localNodeContext.classification.algorithm",
                                                              C45LocalNodeContext::WINDOW_ALGORITHM);
            int iInitWinSize = pCfgMgr->getValueAsInt ("aci.dspro.localNodeContext.classification.algorithm.win.size.init", 30);
            int iMaxWinSize = pCfgMgr->getValueAsInt ("aci.dspro.localNodeContext.classification.algorithm.win.size.max", 60);
            int iIncrement = pCfgMgr->getValueAsInt ("aci.dspro.localNodeContext.classification.algorithm.win.size.incr", 5);
            if (algorithm == C45LocalNodeContext::CYCLE_ALGORITHM) {
                float fPercCycleErr = (float) atof (pCfgMgr->getValue ("aci.dspro.localNodeContext.classification.algorithm.error.pcycle", "0.0"));
                float fPercCycleWin = (float) atof (pCfgMgr->getValue ("aci.dspro.localNodeContext.classification.algorithm.win.size.pcycle", "0.0"));

                rc = pC45Class->configureCycleModeClassifier (pMetadataConf->getMetadataAsStructure(), iInitWinSize,
                                                             iMaxWinSize, iIncrement, fPercCycleErr, fPercCycleWin);
            }
            else if (algorithm == C45LocalNodeContext::WINDOW_ALGORITHM) {
                rc = pC45Class->configureWindowModeClassifier (pMetadataConf->getMetadataAsStructure(),
                                                               iInitWinSize, iMaxWinSize, iIncrement);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "C4.5 Classifier configuration failed. Algorithm of unknown type (%s)\n",
                                algorithm.c_str());
                return nullptr;
            }
        }
        else {
            // By default, set window mode classifier
            rc = pC45Class->configureWindowModeClassifier (pMetadataConf->getMetadataAsStructure());
        }

        if (rc != 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "Classifier configuration failed. The classifier won't work.\n");
        }

        _pLocalNodeContext = pC45Class;
    }
    else if (classifier == NonClassifyingLocalNodeContext::TYPE) {
        // Instantiate classifier
        _pLocalNodeContext = new NonClassifyingLocalNodeContext (pszNodeId,
                                                                 NodeContext::TOO_FAR_FROM_PATH_COEFF,
                                                                 NodeContext::APPROXIMABLE_TO_POINT_COEFF);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Classifier configuration failed. "
                        "The classifier of unknown type (%s)\n", classifier.c_str());
    }

    if (pCfgMgr->hasValue (UsefulDistance::USEFUL_DISTANCE_PROPERTY)) {
        _pLocalNodeContext->setDefaultUsefulDistance (atoui32(pCfgMgr->getValue (UsefulDistance::USEFUL_DISTANCE_PROPERTY)));
    }
    if (pCfgMgr->hasValue (UsefulDistance::USEFUL_DISTANCE_BY_TYPE_PROPERTY)) {
        _pLocalNodeContext->parseAndSetUsefulDistanceByType (pCfgMgr->getValue (UsefulDistance::USEFUL_DISTANCE_BY_TYPE_PROPERTY));
    }

    if (pCfgMgr->hasValue (LocalNodeContext::MATCHMAKING_QUALIFIERS)) {
        if (MatchmakingInfoHelper::parseAndAddQualifiers (&(_pLocalNodeContext->_matchmakingInfo), pCfgMgr->getValue (LocalNodeContext::MATCHMAKING_QUALIFIERS)) == 0) {
            _pLocalNodeContext->_matchmakingInfo.incrementVersion();
        }
    }

    bool bLimitPrestagingToLocalData = false;
    if (pCfgMgr->hasValue (LIMIT_PRESTAGING_TO_LOCAL_DATA_PROPERTY)) {
        bLimitPrestagingToLocalData = pCfgMgr->getValueAsBool (LIMIT_PRESTAGING_TO_LOCAL_DATA_PROPERTY);
    }
    else if (pCfgMgr->hasValue ("aci.dspro.controller.limitPrestagingToLocalData")) {
        // obsolete property name, keep it only for retro-compatibility
        bLimitPrestagingToLocalData = pCfgMgr->getValueAsBool ("aci.dspro.informationPush.limitPrestagingToLocalData");
    }
    else if (pCfgMgr->hasValue ("aci.dspro.informationPush.limitPrestagingToLocalData")) {
        // obsolete property name, keep it only for retro-compatibility
        bLimitPrestagingToLocalData = pCfgMgr->getValueAsBool ("aci.dspro.informationPush.limitPrestagingToLocalData");
    }
    _pLocalNodeContext->_matchmakingInfo.setLimitToLocalMatchmakingOnly (bLimitPrestagingToLocalData);

    float fMatchmakingThreshold = 6.0f;
    if (pCfgMgr->hasValue ("aci.dspro.localNodeContext.rankThreshold")) {
        fMatchmakingThreshold = static_cast<float>(atof (pCfgMgr->getValue ("aci.dspro.localNodeContext.rankThreshold")));
    }
    else if (pCfgMgr->hasValue ("aci.dspro.informationPush.rankThreshold")) {
        // obsolete property name, keep it only for retro-compatibility
        fMatchmakingThreshold = static_cast<float>(atof (pCfgMgr->getValue ("aci.dspro.informationPush.rankThreshold")));
    }
    _pLocalNodeContext->setMatchmakingThreshold (fMatchmakingThreshold);

    if (_pLocalNodeContext->_matchmakingInfo.init (pCfgMgr) < 0) {
        return nullptr;
    }

    return _pLocalNodeContext;
}

int LocalNodeContext::addCustomPolicies (const char **ppszCustomPoliciesXML)
{
    if (ppszCustomPoliciesXML == nullptr) {
        return 0;
    }
    for (unsigned int i = 0; ppszCustomPoliciesXML[i] != nullptr; i++) {
        if (!_matchmakingInfo.setCustomPolicy (ppszCustomPoliciesXML[i])) {
            checkAndLogMsg ("LocalNodeContext::addCustomPolicies", Logger::L_Warning,
                            "could not add policy: %s.\n", ppszCustomPoliciesXML[i]);
            return -1;
        }
    }
    _matchmakingInfo.incrementVersion();
    return 0;
}

int LocalNodeContext::addCustomPolicy (CustomPolicyImpl *pPolicy)
{
    if (pPolicy == nullptr) {
        return 0;
    }
    if (_matchmakingInfo.setCustomPolicy (pPolicy)) {
        _matchmakingInfo.incrementVersion();
    }
    return 0;
}

int LocalNodeContext::configure (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == nullptr) {
        return -1;
    }

    // Init node info
    bool bAnyNodeInfo = false;
    if (pCfgMgr->hasValue (TEAM_ID_PROPERTY)) {
        const char *pszValue = pCfgMgr->getValue (TEAM_ID_PROPERTY);
        if (!_nodeInfo.setTeamId (pszValue)) {
            return -2;
        }
        bAnyNodeInfo = true;
        checkAndLogMsg ("LocalNodeContext::configure", Logger::L_Warning,
                        "%s set to %s\n", TEAM_ID_PROPERTY, (pszValue == nullptr ? "NULL" : pszValue));
    }
    if (pCfgMgr->hasValue (MISSION_ID_PROPERTY)) {
        const char *pszValue = pCfgMgr->getValue (MISSION_ID_PROPERTY);
        if (!_nodeInfo.setMisionId (pszValue)) {
            return -3;
        }
        bAnyNodeInfo = true;
        checkAndLogMsg ("LocalNodeContext::configure", Logger::L_Warning,
                        "%s set to %s\n", MISSION_ID_PROPERTY, (pszValue == nullptr ? "NULL" : pszValue));
    }
    if (pCfgMgr->hasValue (ROLE_PROPERTY)) {
        const char *pszValue = pCfgMgr->getValue (ROLE_PROPERTY);
        if (!_nodeInfo.setRole (pszValue)) {
            return -4;
        }
        bAnyNodeInfo = true;
        checkAndLogMsg ("LocalNodeContext::configure", Logger::L_Warning,
                        "%s set to %s\n", ROLE_PROPERTY, (pszValue == nullptr ? "NULL" : pszValue));
    }
    if (pCfgMgr->hasValue (NODE_TYPE_PROPERTY)) {
        const char *pszValue = pCfgMgr->getValue (NODE_TYPE_PROPERTY);
        if (!_nodeInfo.setNodeType (pszValue)) {
            return -5;
        }
        bAnyNodeInfo = true;
        checkAndLogMsg ("LocalNodeContext::configure", Logger::L_Warning,
            "%s set to %s\n", NODE_TYPE_PROPERTY, (pszValue == nullptr ? "NULL" : pszValue));
    }
    if (bAnyNodeInfo) {
        _nodeInfo.incrementVersion();
    }

    // Init Useful distance
    bool bAnyLocationInfo = false;
    if (MatchmakingInfoHelper::parseAndSetUsefulDistanceByType (&_matchmakingInfo, pCfgMgr->getValue (UsefulDistance::USEFUL_DISTANCE_BY_TYPE_PROPERTY))) {
        bAnyLocationInfo = true;
    }
    if (MatchmakingInfoHelper::parseAndSetRangesOfInfluence (&_matchmakingInfo, pCfgMgr->getValue (RangeOfInfluence::RANGE_OF_INFLUENCE_BY_MILSTD2525_SYMBOL_CODE))) {
        bAnyLocationInfo = true;
    }
    if (_matchmakingInfo.setMetadataRankerParameters (pCfgMgr)) {
        bAnyLocationInfo = true;
    }
    if (bAnyLocationInfo) {
        _matchmakingInfo.incrementVersion();
    }

    return 0;
}

int LocalNodeContext::configureMetadataRanker (float coordRankWeight, float timeRankWeight,
                                               float expirationRankWeight, float impRankWeight,
                                               float sourceReliabilityRankWeigth, float informationContentRankWeigth,
                                               float predRankWeight, float targetWeight, bool bStrictTarget,
                                               bool bConsiderFuturePathSegmentForMatchmacking)
{
    if (_matchmakingInfo.setMetadataRankerParameters (coordRankWeight, timeRankWeight, expirationRankWeight,
                                                      impRankWeight, sourceReliabilityRankWeigth, informationContentRankWeigth,
                                                      predRankWeight, targetWeight, bStrictTarget, bConsiderFuturePathSegmentForMatchmacking)) {
        _matchmakingInfo.incrementVersion();
    }
    return 0;
}

int LocalNodeContext::parseAndSetUsefulDistanceByType (const char *pszUsefulDistanceValues)
{
    if (MatchmakingInfoHelper::parseAndSetUsefulDistanceByType (&_matchmakingInfo, pszUsefulDistanceValues)) {
        _matchmakingInfo.incrementVersion();
    }
    return 0;
}

void LocalNodeContext::configureNodeContext (const char *pszTeamId, const char *pszMissionId, const char *pszRole)
{
    const char *pszMethodName = "LocalNodeContext::configureNodeContext";
    if (_nodeInfo.setTeamId (pszTeamId) || _nodeInfo.setMisionId (pszMissionId) || _nodeInfo.setRole (pszRole)) {
        _nodeInfo.incrementVersion();
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "local node context configured: team ID = <%s>; mission ID = <%s>; "
                    "role = <%s>\n", pszTeamId != nullptr ? pszTeamId : "NULL", pszMissionId != nullptr ? pszMissionId : "NULL",
                    pszRole != nullptr ? pszRole : "NULL");
}

NodePath * LocalNodeContext::getPath (const char *pszPathId)
{
    return _pathInfo.getPath (pszPathId);
}

NodePath * LocalNodeContext::getPath (void)
{
    return _pathInfo.getPath();
}

uint16 LocalNodeContext::getClassifierVersion (void)
{
    return _pClassifier->getVersion();
}

int64 LocalNodeContext::getStartTime (void) const
{
    return _i64StartingTime;
}

int LocalNodeContext::getCurrentLatitude (float &latitude)
{
    float longitude, altitude;
    const char *pszLocation, *pszNote;
    uint64 timestamp;
    return _locationInfo.getCurrentPosition (latitude, longitude, altitude,
                                             pszLocation, pszNote, timestamp);
}

int LocalNodeContext::getCurrentLongitude (float &longitude)
{
    float latitude, altitude;
    const char *pszLocation, *pszNote;
    uint64 timestamp;
    return _locationInfo.getCurrentPosition (latitude, longitude, altitude,
                                             pszLocation, pszNote, timestamp);
}

int LocalNodeContext::getCurrentTimestamp (uint64 &timestamp)
{
    float latitude, longitude, altitude;
    const char *pszLocation, *pszNote;
    return _locationInfo.getCurrentPosition (latitude, longitude, altitude,
                                             pszLocation, pszNote, timestamp);
}

int LocalNodeContext::getCurrentPosition (float &latitude, float &longitude, float &altitude,
                                          const char *&pszLocation, const char *&pszNote,
                                          uint64 &timeStamp)
{
    return _locationInfo.getCurrentPosition (latitude, longitude, altitude,
                                             pszLocation, pszNote, timeStamp);
}

LocationInfo * LocalNodeContext::getLocationInfo (void)
{
    return &_locationInfo;
}

bool LocalNodeContext::isPeerActive (void)
{
    return true;
}

int LocalNodeContext::setPathProbability (const char *pszPathId, float probability)
{
    if (pszPathId == nullptr) {
        return -1;
    }
    if (_pathInfo.setPathProbability (pszPathId, probability)) {
        return 0;
    }
    return -2;
}

int LocalNodeContext::setCurrentPath (const char *pszPathId)
{
    if (pszPathId == nullptr) {
        return -1;
    }
    if (_pathInfo.setCurrentPath (pszPathId)) {
        _pathInfo.incrementVersion();

        // Also set first position if the path is a fixed location
        NodePath *pPath = _pathInfo.getPath (pszPathId);
        if (pPath == nullptr) {
            return -2;
        }
        if (pPath->getPathType() == NodePath::FIXED_LOCATION) {
            _locationInfo.setCurrentPosition (pPath, pPath->getLatitude (0),
                                              pPath->getLongitude (0), pPath->getAltitude (0),
                                              _matchmakingInfo.getMaximumUsefulDistance());
        }

        _locationInfo.setCurrentPath (pPath->getLatitude (0), pPath->getLongitude (0));
        _locationInfo.incrementVersion();
        return 0;
    }
    return 0;
}

int LocalNodeContext::addPath (NodePath *pNodePath)
{
    if (pNodePath == nullptr) {
        return -1;
    }
    if (_pathInfo.addPath (pNodePath)) {
        return 0;
    }
    return -1;
}

int LocalNodeContext::deletePath (const char *pszPathId)
{
    if (_pathInfo.deletePath (pszPathId)) {
        return 0;
    }
    return -1;
}

bool LocalNodeContext::setCurrentPosition (float latitude, float longitude, float altitude,
                                           const char *pszLocation, const char *pszNote, uint64 timeStamp)
{
    if (_locationInfo.setCurrentPosition (_pathInfo.getPath(), latitude, longitude, altitude,
                                          _matchmakingInfo.getMaximumUsefulDistance())) {
        _locationInfo.incrementVersion();
        return true;
    }
    return false;
}

bool LocalNodeContext::setMatchmakingThreshold (float fMatchmakingThreshold)
{
    const char *pszMethodName = "LocalNodeContext::setMatchmakingThreshold";
    checkAndLogMsg (pszMethodName, Logger::L_Info, "setting matchmaking threshould %f\n", fMatchmakingThreshold);
    if (_matchmakingInfo.setMatchmakingThreshold (fMatchmakingThreshold)) {
        _matchmakingInfo.incrementVersion();
        return true;
    }
    return false;
}

void LocalNodeContext::setBatteryLevel (uint8 ui8BatteryLevel)
{
    if (_nodeInfo.setBatteryLevel (ui8BatteryLevel)) {
        _nodeInfo.incrementVersion();
    }
}

void LocalNodeContext::setMemoryAvailable (uint8 ui8MemoryAvailable)
{
    if (_nodeInfo.setMemoryAvailable (ui8MemoryAvailable)) {
        _nodeInfo.incrementVersion();
    }
}

void LocalNodeContext::setMissionId (const char *pszMissionId)
{
    if (_nodeInfo.setMisionId (pszMissionId)) {
        _nodeInfo.incrementVersion();
    }
}

void LocalNodeContext::setTeam (const char *pszTeam)
{
    if (_nodeInfo.setTeamId (pszTeam)) {
        _nodeInfo.incrementVersion();
    }
}

void LocalNodeContext::setRole (const char *pszRole)
{
    if (_nodeInfo.setRole (pszRole)) {
        _nodeInfo.incrementVersion();
    }
}

void LocalNodeContext::setNodeType (const char *pszType)
{
    if (_nodeInfo.setNodeType (pszType)) {
        _nodeInfo.incrementVersion();
    }
}

void LocalNodeContext::setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters)
{
    const char *pszMethodName = "LocalNodeContext::setDefaultUsefulDistance";
    checkAndLogMsg (pszMethodName, Logger::L_Info, "setting default useful distance to %u\n", ui32UsefulDistanceInMeters);
    if (_matchmakingInfo.setDefaultUsefulDistance (ui32UsefulDistanceInMeters)) {
        _matchmakingInfo.incrementVersion();
    }
}

void LocalNodeContext::setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters)
{
    const char *pszMethodName = "LocalNodeContext::setDefaultUsefulDistance";
    checkAndLogMsg (pszMethodName, Logger::L_Info, "setting useful distance for %s to %u\n", pszDataMIMEType, ui32UsefulDistanceInMeters);
    if (_matchmakingInfo.setUsefulDistance (pszDataMIMEType, ui32UsefulDistanceInMeters)) {
        _matchmakingInfo.incrementVersion();
    }
}

void LocalNodeContext::setRangeOfInfluence (const char *pszNodeType, uint32 ui32RangeOfInfluenceInMeters)
{
    const char *pszMethodName = "LocalNodeContext::setRangeOfInfluence";
    checkAndLogMsg (pszMethodName, Logger::L_Info, "setting range of influence for %s to %u\n", pszNodeType, ui32RangeOfInfluenceInMeters);
    if (_matchmakingInfo.setRangeOfInfluence (pszNodeType, ui32RangeOfInfluenceInMeters)) {
        _matchmakingInfo.incrementVersion();
    }
}

int LocalNodeContext::updateClassifier (C45AVList *pDataset)
{
    int retValue = _pClassifier->addNewData (pDataset);
    if (retValue != 0) {
        return (0 - retValue);
    }
    return _pClassifier->getVersion();
}

