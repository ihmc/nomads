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
#include "MetadataConfiguration.h"
#include "MetadataRankerConfiguration.h"
#include "NodePath.h"
#include "NonClassifyingLocalNodeContext.h"

#include "C45DecisionTree.h"
#include "C45AVList.h"

#include "Logger.h"
#include "NLFLib.h"
#include "ConfigManager.h"
#include "StringTokenizer.h"
#include "MetaDataRanker.h"
#include "MatchmakingQualifier.h"

#include <string.h>

using namespace NOMADSUtil;
using namespace IHMC_C45;
using namespace IHMC_ACI;

const unsigned short int LocalNodeContext::ACTUAL_COVERED_PATH_INDEX = 0;

const char * LocalNodeContext::TEAM_ID_PROPERTY = "aci.dspro.localNodeContext.pszTeamID";
const char * LocalNodeContext::MISSION_ID_PROPERTY = "aci.dspro.localNodeContext.pszMissionID";
const char * LocalNodeContext::ROLE_PROPERTY = "aci.dspro.localNodeContext.pszRole";
const char * LocalNodeContext::USEFUL_DISTANCE_PROPERTY = "aci.dspro.localNodeContext.usefulDistance";
const char * LocalNodeContext::USEFUL_DISTANCE_BY_TYPE_PROPERTY = "aci.dspro.localNodeContext.usefulDistanceByType";
const char * LocalNodeContext::LIMIT_PRESTAGING_TO_LOCAL_DATA_PROPERTY = "aci.dspro.localNodeContext.limitPrestagingToLocalData";
const char * LocalNodeContext::MATCHMAKING_QUALIFIERS = "aci.dspro.localNodeContext.mathcmakingQualifiers";

LocalNodeContext::LocalNodeContext (const char *pszNodeID, Classifier *pClassifier,
                                    double dTooFarCoeff, double dApproxCoeff)
    : NodeContext (pszNodeID, dTooFarCoeff, dApproxCoeff)
{
    _ui16PathsNumber = 1;

    int64 i64Time = getTimeInMilliseconds(); 
    _ui32StartingTime = (i64Time > 0 ? (uint32) i64Time : 0); 

    _pPaths = new DArray2<NodePath *>(_ui16PathsNumber);
    _pPaths->setDefIncr (1);

    NodePath *pPastPath = new NodePath (NodePath::PAST_PATH, 0, 0);
    (*_pPaths)[_ui16PathsNumber - 1] = pPastPath;

    _iCurrPath = -1;

    _pClassifier = pClassifier;
    assert (_pClassifier != NULL);
    if (_pClassifier == NULL) {
        checkAndLogMsg ("LocalNodeContext::LocalNodeContext", Logger::L_SevereError,
                        "Could not initialize classifier.");
    }
}

LocalNodeContext::~LocalNodeContext (void)
{
    if (_pPaths != NULL) {
        for (int i = 0; i < _ui16PathsNumber; i ++) {
            delete (*_pPaths)[i];
            (*_pPaths)[i] = NULL;
        }
        delete _pPaths;
        _pPaths = NULL;
    }
}

LocalNodeContext * LocalNodeContext::getInstance (const char *pszNodeId, ConfigManager *pCfgMgr,
                                                  MetadataConfiguration *pMetadataConf)
{
    if (pCfgMgr == NULL) {
        return NULL;
    }

    const char *pszMethodName = "LocalNodeContext::getInstance";

    LocalNodeContext *_pLocalNodeContext = NULL;
    NOMADSUtil::String classifier = pCfgMgr->getValue ("aci.dspro.localNodeContext.classifier.type",
                                                       NonClassifyingLocalNodeContext::TYPE);

    if (classifier == C45LocalNodeContext::TYPE) {
        // Instantiate classifier
        C45LocalNodeContext *pC45Class = new C45LocalNodeContext (pszNodeId,
                                                                  NodeContext::TOO_FAR_FROM_PATH_COEFF,
                                                                  NodeContext::APPROXIMABLE_TO_POINT_COEFF);

        // Configure classifier
        int rc = 0;
        if (pCfgMgr != NULL) {
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
                                (const char *) algorithm);
                return NULL;
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
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Classifier configuration failed. The classifier of unknown type (%s)\n",
                        (const char *) classifier);
    }

    _pLocalNodeContext->setDefaultUsefulDistance (pCfgMgr->hasValue (LocalNodeContext::USEFUL_DISTANCE_PROPERTY) ?
                                                  pCfgMgr->getValueAsUInt32 (LocalNodeContext::USEFUL_DISTANCE_PROPERTY) :
                                                  NodeContext::DEFAULT_USEFUL_DISTANCE);
    if (pCfgMgr->hasValue (LocalNodeContext::USEFUL_DISTANCE_BY_TYPE_PROPERTY)) {
        _pLocalNodeContext->parseAndSetUsefulDistanceByType (pCfgMgr->getValue (LocalNodeContext::USEFUL_DISTANCE_BY_TYPE_PROPERTY));
    }

    if (pCfgMgr->hasValue (LocalNodeContext::MATCHMAKING_QUALIFIERS)) {
        if (_pLocalNodeContext->_qualifiers.parseAndAddQualifiers (pCfgMgr->getValue (LocalNodeContext::MATCHMAKING_QUALIFIERS)) == 0) {
            _pLocalNodeContext->_ui16CurrMatchmakerQualifierVersion++;
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
    _pLocalNodeContext->setLimitToLocalMatchmakingOnly (bLimitPrestagingToLocalData);

    float fMatchmakingThreshold = 6.0f;
    if (pCfgMgr->hasValue ("aci.dspro.localNodeContext.rankThreshold")) {
        fMatchmakingThreshold = (float) atof (pCfgMgr->getValue ("aci.dspro.localNodeContext.rankThreshold"));
    }
    else if (pCfgMgr->hasValue ("aci.dspro.informationPush.rankThreshold")) {
        // obsolete property name, keep it only for retro-compatibility
        fMatchmakingThreshold = (float) atof (pCfgMgr->getValue ("aci.dspro.informationPush.rankThreshold"));
    }
    _pLocalNodeContext->setMatchmakingThreshold (fMatchmakingThreshold);

    if (_pLocalNodeContext->_customPolicies.init (pCfgMgr) < 0) {
        return NULL;
    }

    return _pLocalNodeContext;
}

int LocalNodeContext::addCustomPolicies (const char **ppszCustomPoliciesXML)
{
    if (ppszCustomPoliciesXML == NULL) {
        return 0;
    }
    for (unsigned int i = 0; ppszCustomPoliciesXML[i] != NULL; i++) {
        int rc = _customPolicies.add (ppszCustomPoliciesXML[i]);
        if (rc < 0) {
            checkAndLogMsg ("LocalNodeContext::addCustomPolicies", Logger::L_Warning,
                            "could not add policy: %s. The returned code was: %d.\n",
                            ppszCustomPoliciesXML[i], rc);
            return -1;
        }
    }
    _ui16CurrInfoVersion++;
    return 0;
}

int LocalNodeContext::configure (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return -1;
    }
    if (pCfgMgr->hasValue (TEAM_ID_PROPERTY)) {
        const char *pszValue = pCfgMgr->getValue (TEAM_ID_PROPERTY);
        free (_pszTeamID);
        checkAndLogMsg ("LocalNodeContext::configure", Logger::L_Warning,
                        "%s set to %s\n", TEAM_ID_PROPERTY, (pszValue == NULL ? "NULL" : pszValue));
        _pszTeamID = (pszValue != NULL ? strDup (pszValue) : NULL);
        if (pszValue != NULL && _pszTeamID == NULL) {
            checkAndLogMsg ("LocalNodeContext::configure", memoryExhausted);
            return -2;
        }
    }
    if (pCfgMgr->hasValue (MISSION_ID_PROPERTY)) {
        const char *pszValue = pCfgMgr->getValue (MISSION_ID_PROPERTY);
        free (_pszMissionID);
        checkAndLogMsg ("LocalNodeContext::configure", Logger::L_Warning,
                        "%s set to %s\n", MISSION_ID_PROPERTY, (pszValue == NULL ? "NULL" : pszValue));
        _pszMissionID = (pszValue != NULL ? strDup (pszValue) : NULL);
        if (pszValue != NULL && _pszMissionID == NULL) {
            checkAndLogMsg ("LocalNodeContext::configure", memoryExhausted);
            return -3;
        }
    }
    if (pCfgMgr->hasValue (ROLE_PROPERTY)) {
        const char *pszValue = pCfgMgr->getValue (ROLE_PROPERTY);
        free (_pszRole);
        checkAndLogMsg ("LocalNodeContext::configure", Logger::L_Warning,
                        "%s set to %s\n", ROLE_PROPERTY, (pszValue == NULL ? "NULL" : pszValue));
        _pszRole = (pszValue != NULL ? strDup (pszValue) : NULL);
        if (pszValue != NULL && _pszRole == NULL) {
            checkAndLogMsg ("LocalNodeContext::configure", memoryExhausted);
            return -4;
        }
    }

    if (_pszTeamID != NULL || _pszMissionID != NULL || _pszRole != NULL) {
        _ui16CurrInfoVersion++;
    }

    if (pCfgMgr->hasValue (USEFUL_DISTANCE_PROPERTY)) {
        _ui32DefaultUsefulDistance = pCfgMgr->getValueAsUInt32 (USEFUL_DISTANCE_PROPERTY);
        checkAndLogMsg ("LocalNodeContext::configure", Logger::L_Info,
                        "ui16UsefulDistance is set to %u\n", _ui32DefaultUsefulDistance);
    }
    if (pCfgMgr->hasValue (USEFUL_DISTANCE_BY_TYPE_PROPERTY)) {
        parseAndSetUsefulDistanceByType (pCfgMgr->getValue (USEFUL_DISTANCE_BY_TYPE_PROPERTY));
    }

    int rc = configureMetadataRanker (pCfgMgr);
    if (rc < 0) {
        checkAndLogMsg ("LocalNodeContext::configure", Logger::L_Warning,
                        "could not configure MetadataRanker. Return code %d.\n", rc);
        return -5;
    }

    return 0;
}

int LocalNodeContext::configureMetadataRanker (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return -1;
    }
    if (_pMetaDataRankerConf == NULL) {
        return -2;
    }
    return _pMetaDataRankerConf->configure (pCfgMgr);
}

int LocalNodeContext::configureMetadataRanker (float coordRankWeight, float timeRankWeight,
                                               float expirationRankWeight, float impRankWeight,
                                               float sourceReliabilityRankWeigth, float informationContentRankWeigth,
                                               float predRankWeight, float targetWeight, bool bStrictTarget,
                                               bool bConsiderFuturePathSegmentForMatchmacking)
{
    if (_pMetaDataRankerConf == NULL) {
        return -1;
    }
    return _pMetaDataRankerConf->configure (coordRankWeight, timeRankWeight, expirationRankWeight,
                                            impRankWeight, sourceReliabilityRankWeigth, informationContentRankWeigth,
                                            predRankWeight, targetWeight, bStrictTarget, bConsiderFuturePathSegmentForMatchmacking);
}

int LocalNodeContext::parseAndSetUsefulDistanceByType (const char *pszUsefulDistanceValues)
{
    if (pszUsefulDistanceValues == NULL) {
        return 0;
    }
    int rc = 0;
    StringTokenizer tokenizer (pszUsefulDistanceValues, ';', ';');
    StringTokenizer innerTokenizer;
    for (const char *pszUsefulDistanceByType = tokenizer.getNextToken();
         pszUsefulDistanceByType != NULL;
         pszUsefulDistanceByType = tokenizer.getNextToken()) {
        innerTokenizer.init (pszUsefulDistanceByType, ',', ',');
        const char *pszType = innerTokenizer.getNextToken();
        const char *pszUsefulDistance = innerTokenizer.getNextToken();
        if (pszType != NULL && pszUsefulDistance != NULL) {
            setUsefulDistance (pszType, atoui32 (pszUsefulDistance));
        }
        else {
            checkAndLogMsg ("LocalNodeContext::parseUsefulDistanceByType", Logger::L_Warning,
                            "misconfiguration: type <%s> useful distance <%s>",
                            pszType != NULL ? pszType : "NULL",
                            pszUsefulDistance != NULL ? pszUsefulDistance : "NULL");
            rc--;
        }
    }
    return rc;
}

void LocalNodeContext::configureNodeContext (const char *pszTeamID, const char *pszMissionID, const char *pszRole)
{
    free (_pszTeamID);
    _pszTeamID = (pszTeamID != NULL ? strDup (pszTeamID) : NULL);

    free (_pszMissionID);
    _pszMissionID = (pszMissionID != NULL ? strDup (pszMissionID) : NULL);

    free (_pszRole);
    _pszRole = (pszRole != NULL ? strDup (pszRole) : NULL);

    _ui16CurrInfoVersion++;
    checkAndLogMsg ("LocalNodeContext::configureNodeContext", Logger::L_Info,
                    "local node context configured: team ID = <%s>; mission ID = <%s>; role = <%s>\n",
                    _pszTeamID != NULL ? _pszTeamID : "null",
                    _pszMissionID != NULL ? _pszMissionID : "null",
                    _pszRole != NULL ? _pszRole : "null");
}

NodePath * LocalNodeContext::getPath (const char *pszPathID)
{
    for (int i = 0; i < _ui16PathsNumber; i ++) {
        if (0 == strcmp (pszPathID, (*_pPaths)[i]->getPathID())) {
            return (*_pPaths)[i];
        }
    }
    return NULL;
}

NodePath * LocalNodeContext::getPath (void)
{
    if (_iCurrPath == -1) {
        return NULL;
    }
    return (*_pPaths)[_iCurrPath];
}

uint16 LocalNodeContext::getClassifierVersion (void)
{
    return _pClassifier->getVersion();
}

int LocalNodeContext::getCurrentLatitude (float &latitude)
{
    if (!_pPaths->used (ACTUAL_COVERED_PATH_INDEX)) {
        return -1;
    }
    NodePath *pCoveredPath = (*_pPaths)[ACTUAL_COVERED_PATH_INDEX];
    if (pCoveredPath == NULL) {
        return -2;
    }
    
    // The current way point in the covered path, is always the last one
    int iCurrWayPointInPath = pCoveredPath->getPathLength() - 1;
    if (iCurrWayPointInPath < 0) {
        return -3;
    }

    latitude = pCoveredPath->getLatitude (iCurrWayPointInPath);
    return 0;
}

int LocalNodeContext::getCurrentLongitude (float &longitude)
{
    if (!_pPaths->used (ACTUAL_COVERED_PATH_INDEX)) {
        return -1;
    }
    NodePath *pCoveredPath = (*_pPaths)[ACTUAL_COVERED_PATH_INDEX];
    if (pCoveredPath == NULL) {
        return -2;
    }
    
    // The current way point in the covered path, is always the last one
    int iCurrWayPointInPath = pCoveredPath->getPathLength() - 1;
    if (iCurrWayPointInPath < 0) {
        return -3;
    }

    longitude = pCoveredPath->getLongitude (iCurrWayPointInPath);
    return 0;
}

int LocalNodeContext::getCurrentTimestamp (uint64 &timestamp)
{
    if (!_pPaths->used (ACTUAL_COVERED_PATH_INDEX)) {
        return -1;
    }
    NodePath *pCoveredPath = (*_pPaths)[ACTUAL_COVERED_PATH_INDEX];
    if (pCoveredPath == NULL) {
        return -2;
    }
    
    // The current way point in the covered path, is always the last one
    int iCurrWayPointInPath = pCoveredPath->getPathLength() - 1;
    if (iCurrWayPointInPath < 0) {
        return -3;
    }

    timestamp = pCoveredPath->getTimeStamp (iCurrWayPointInPath);
    return 0;
}

int LocalNodeContext::getCurrentPosition (float &latitude, float &longitude, float &altitude,
                                          const char *&pszLocation, const char *&pszNote,
                                          uint64 &timeStamp)
{
    if (!_pPaths->used (ACTUAL_COVERED_PATH_INDEX)) {
        return -1;
    }
    NodePath *pCoveredPath = (*_pPaths)[ACTUAL_COVERED_PATH_INDEX];
    if (pCoveredPath == NULL) {
        return -2;
    }
    
    // The current way point in the covered path, is always the last one
    int iCurrWayPointInPath = pCoveredPath->getPathLength() - 1;
    if (iCurrWayPointInPath < 0) {
        return -3;
    }

    latitude = pCoveredPath->getLatitude (iCurrWayPointInPath);
    longitude = pCoveredPath->getLongitude (iCurrWayPointInPath);
    altitude = pCoveredPath->getAltitude (iCurrWayPointInPath);
    pszLocation = pCoveredPath->getLocation (iCurrWayPointInPath);
    pszNote = pCoveredPath->getNote (iCurrWayPointInPath);
    timeStamp = pCoveredPath->getTimeStamp (iCurrWayPointInPath);

    return 0;
}

bool LocalNodeContext::isPeerActive()
{
    return true;
}

int LocalNodeContext::setPathProbability (const char *pszPathID, float probability)
{
    if (pszPathID == NULL) {
        return -1;
    }
    for (int i = 1; i < _ui16PathsNumber; i++) {
        if (0 == strcmp (pszPathID, (*_pPaths)[i]->getPathID())) {
            return (*_pPaths)[i]->setProbability (probability);
        }
    }
    return -1;
}

int LocalNodeContext::setCurrentPath (const char *pszPathID)
{
    if (pszPathID == NULL) {
        return -1;
    }
checkAndLogMsg ("LocalNodeContext::setCurrentPath", Logger::L_Info,
                "pszPathID = <%s>\n", pszPathID);
    for (int i = 1; i < _ui16PathsNumber; i ++) {
        if (0 == strcmp (pszPathID, (*_pPaths)[i]->getPathID())) {
            _iCurrPath = i;
checkAndLogMsg ("LocalNodeContext::setCurrentPath", Logger::L_Info,
                "setting _icurrPath to %d\n", _iCurrPath);
            _ui16CurrPathVersion ++;
            _ui16CurrWaypointVersion ++;
            NodePath * pCurrPath = (*_pPaths)[_iCurrPath];
            if ((*_pPaths)[_iCurrPath]->getPathType() == NodePath::FIXED_LOCATION) {
                _iCurrWayPointInPath = 0;
                (*_pPaths)[ACTUAL_COVERED_PATH_INDEX]->appendWayPoint (pCurrPath->getLatitude (0),
                                                                       pCurrPath->getLongitude (0),
                                                                       pCurrPath->getAltitude (0),
                                                                       pCurrPath->getLocation (0),
                                                                       pCurrPath->getNote (0),
                                                                       pCurrPath->getTimeStamp (0));
            }
            else {
                /*NodePath * pCoveredPath = (*_pPaths)[ACTUAL_COVERED_PATH_INDEX];
                // The current way point in the covered path, is always the last one
                int iCurrWayPointInPath = (*_pPaths)[ACTUAL_COVERED_PATH_INDEX]->getPathLength() - 1;
                _iCurrWayPointInPath = calculateCurrentWayPointInPath(pCoveredPath->getLatitude(iCurrWayPointInPath),
                                                                      pCoveredPath->getLongitude(iCurrWayPointInPath),
                                                                      pCoveredPath->getAltitude(iCurrWayPointInPath));*/
                _iCurrWayPointInPath = 0;
            }
            _closestPointOnPathLat = pCurrPath->getLatitude (0);
            _closestPointOnPathLong = pCurrPath->getLongitude (0);
            _status = ON_WAY_POINT;

            return 0;
        }
    }
    return -1;
}
            
int LocalNodeContext::addPath (NodePath *pNodePath)
{
    if (pNodePath == NULL) {
        return -1;
    }
    _ui16PathsNumber++;
    (*_pPaths)[_ui16PathsNumber - 1] = pNodePath;
    return 0;
}
            
int LocalNodeContext::deletePath (const char *pszPathID)
{
    for (int i = 1; i < _ui16PathsNumber; i++) {
        if (0 == strcmp (pszPathID, (*_pPaths)[i]->getPathID())) {
            if(_iCurrPath == i) {
                return -1;
            }
            for (int j = i; j < _ui16PathsNumber - 1; j++) {
                (*_pPaths)[j] = (*_pPaths)[j + 1];
            }
            _pPaths->clear (_ui16PathsNumber - 1);
            _ui16PathsNumber --;
            return 0;
        }
    }
    return -1;
}

bool LocalNodeContext::setCurrentPosition (float latitude, float longitude, float altitude,
                                           const char *pszLocation, const char *pszNote, uint64 timeStamp)
{
    bool bRet = NodeContext::setCurrentPosition (latitude, longitude, altitude);
    if (!bRet && _pPaths->used (ACTUAL_COVERED_PATH_INDEX)) {
        int iLen = (*_pPaths)[ACTUAL_COVERED_PATH_INDEX]->getPathLength();
        if (iLen > 0) {
            if (!isApproximableToPoint ((*_pPaths)[ACTUAL_COVERED_PATH_INDEX]->getLatitude (iLen-1),
                                        (*_pPaths)[ACTUAL_COVERED_PATH_INDEX]->getLongitude (iLen-1),
                                        latitude, longitude)) {
                bRet = true;
            }
            (*_pPaths)[ACTUAL_COVERED_PATH_INDEX]->getLatitude (iLen);
        }
        else {
            // if iLen <= 0, it means it is the first time that the position is
            // set, return true
            bRet = true;
        }
    }
    (*_pPaths)[ACTUAL_COVERED_PATH_INDEX]->appendWayPoint (latitude, longitude, altitude, pszLocation, pszNote, timeStamp);
    return bRet;
}

void LocalNodeContext::setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters)
{
    checkAndLogMsg ("LocalNodeContext::setDefaultUsefulDistance", Logger::L_Info,
                    "setting default useful distance to %u\n", ui32UsefulDistanceInMeters);
    _ui32DefaultUsefulDistance = ui32UsefulDistanceInMeters;
}

void LocalNodeContext::setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters)
{
    if (pszDataMIMEType == NULL) {
        return;
    }
    uint32 *pui32 = (uint32*) calloc (1, sizeof (uint32));
    *pui32 = ui32UsefulDistanceInMeters;
    checkAndLogMsg ("LocalNodeContext::setUsefulDistance", Logger::L_Info,
                    "setting %s useful distance to %u\n",
                    pszDataMIMEType, ui32UsefulDistanceInMeters);
    uint32 *pui32Old = _usefulDistanceByMimeType.put (pszDataMIMEType, pui32);
    if (pui32Old != NULL) {
        free (pui32Old);
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

