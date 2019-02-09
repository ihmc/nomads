/**
 * MatchmakingInfo.cpp
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
 * Created on December 24, 2016, 8:23 PM
 */

#include "MatchmakingInfo.h"

#include "Defs.h"
#include "MetadataRankerConfiguration.h"
#include "NLFLib.h"

#include "Json.h"
#include "StringTokenizer.h"
#include "StringStringHashtable.h"
#include "Logger.h"
#include "ConfigManager.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

const char * MatchmakingInfo::MATCHMAKING_INFO_OBJECT_NAME = "matchmakingInfo";

MatchmakingInfo::MatchmakingInfo (void)
    : _bLimitPrestagingToLocalData (false),
      _fMatchmakingThreshold (6.0f),
      _pMetaDataRankerConf (new MetadataRankerConfiguration())
{
}

MatchmakingInfo::~MatchmakingInfo (void)
{
}

int MatchmakingInfo::init (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == nullptr) {
        return -1;
    }
    if (_customPolicies.init (pCfgMgr) < 0) {
        return -2;
    }
    return 0;
}

CustomPolicies * MatchmakingInfo::getCustomPolicies (void)
{
    return &_customPolicies;
}

bool MatchmakingInfo::getLimitToLocalMatchmakingOnly (void) const
{
    return _bLimitPrestagingToLocalData;
}

float MatchmakingInfo::getMatchmakingThreshold (void) const
{
    return _fMatchmakingThreshold;
}

MatchmakingQualifiers * MatchmakingInfo::getMatchmakingQualifiers (void)
{
    return &_qualifiers;
}

IHMC_VOI::MetadataRankerConfiguration * MatchmakingInfo::getMetaDataRankerConfiguration (void) const
{
    return _pMetaDataRankerConf;
}

uint32 MatchmakingInfo::getUsefulDistance (const char *pszInformationMIMEType) const
{
    return _usefulDistance.getUsefulDistance (pszInformationMIMEType);
}

uint32 MatchmakingInfo::getRangeOfInfluence (const char *pszNodeType)
{
    return _rangeOfInfluence.getRangeOfInfluence (pszNodeType);
}

uint32 MatchmakingInfo::getMaximumUsefulDistance (void)
{
    return _usefulDistance.getMaximumUsefulDistance ();
}

uint32 MatchmakingInfo::getMaximumRangeOfInfluence (void)
{
    return _rangeOfInfluence.getMaximumRangeOfInfluence ();
}

bool MatchmakingInfo::setCustomPolicy (const char *pszCustomPoliciesXML)
{
    return _customPolicies.add (pszCustomPoliciesXML);
}

bool MatchmakingInfo::setCustomPolicy (CustomPolicyImpl *pPolicy)
{
    return _customPolicies.add (pPolicy);
}

bool MatchmakingInfo::setLimitToLocalMatchmakingOnly (bool bLimitPrestagingToLocalData)
{
    if (_bLimitPrestagingToLocalData == bLimitPrestagingToLocalData) {
        return false;
    }
    _bLimitPrestagingToLocalData = bLimitPrestagingToLocalData;
    return true;
}

bool MatchmakingInfo::setMatchmakingThreshold (float fMatchmakingThreshold)
{
    if (fEquals (_fMatchmakingThreshold, fMatchmakingThreshold, 0.0001)) {
        return false;
    }
    _fMatchmakingThreshold = fMatchmakingThreshold;
    return true;
}

bool MatchmakingInfo::setMetadataRankerParameters (ConfigManager *pCfgMgr)
{
    return _pMetaDataRankerConf->configure (pCfgMgr);
}

bool MatchmakingInfo::setMetadataRankerParameters (float coordRankWeight, float timeRankWeight,
                                                   float expirationRankWeight, float impRankWeight,
                                                   float sourceReliabilityRankWeigth, float informationContentRankWeigth,
                                                   float predRankWeight, float targetWeight, bool bStrictTarget,
                                                   bool bConsiderFuturePathSegmentForMatchmacking)
{
    return _pMetaDataRankerConf->configure (coordRankWeight, timeRankWeight, expirationRankWeight,
                                            impRankWeight, sourceReliabilityRankWeigth, informationContentRankWeigth,
                                            predRankWeight, targetWeight, bStrictTarget, bConsiderFuturePathSegmentForMatchmacking);
}

bool MatchmakingInfo::setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters)
{
    return _usefulDistance.setDefaultUsefulDistance (ui32UsefulDistanceInMeters);
}

bool MatchmakingInfo::setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters)
{
    return _usefulDistance.setUsefulDistance (pszDataMIMEType, ui32UsefulDistanceInMeters);
}

bool MatchmakingInfo::setRangeOfInfluence (const char *pszNodeType, uint32 ui32RangeOfInfluenceInMeters)
{
    return _rangeOfInfluence.setRangeOfInfluence (pszNodeType, ui32RangeOfInfluenceInMeters);
}

void MatchmakingInfo::reset (void)
{
    _bLimitPrestagingToLocalData = false;
    _fMatchmakingThreshold = 6.0f;
    if (_pMetaDataRankerConf != nullptr) {
        delete _pMetaDataRankerConf;
    }
    _pMetaDataRankerConf = new MetadataRankerConfiguration();
    _usefulDistance.reset();
    _rangeOfInfluence.reset();
    _ui16Version = 0;
}

namespace MATCHMAKER_INFO_JSON
{
    static const char * MATCHMAKING_THRESHOLD = "matchmakingThreshold";
    static const char * LIMIT_PRESTAGING_TO_LOCAL_DATA = "limitPrestagingToLocalData";
    static const char * CUSTOM_POLICIES = "customPolicies";
    static const char * METADATA_RANKER_CONF = "metadatRankerConf";
    static const char * USEFUL_DISTANCE = "usefulDistance";
    static const char * RANGE_OF_INFLUENCE = "rangeOfInfluence";
}

int MatchmakingInfo::fromJson (const JsonObject *pJson)
{
    if (pJson == nullptr) {
        return -1;
    }
    if (pJson->hasObject (MATCHMAKER_INFO_JSON::LIMIT_PRESTAGING_TO_LOCAL_DATA)) {
        pJson->getBoolean (MATCHMAKER_INFO_JSON::LIMIT_PRESTAGING_TO_LOCAL_DATA, _bLimitPrestagingToLocalData);
    }
    if (pJson->hasObject (MATCHMAKER_INFO_JSON::MATCHMAKING_THRESHOLD)) {
        double dVal = 0.0;
        pJson->getNumber (MATCHMAKER_INFO_JSON::MATCHMAKING_THRESHOLD, dVal);
        _fMatchmakingThreshold = (float)dVal;
    }
    int rc = 0;
    if (pJson->hasObject (MATCHMAKER_INFO_JSON::METADATA_RANKER_CONF)) {
        JsonObject *pMetadataRankerConfJson = pJson->getObject (MATCHMAKER_INFO_JSON::METADATA_RANKER_CONF);
        rc += _pMetaDataRankerConf->fromJson (pMetadataRankerConfJson);
        delete pMetadataRankerConfJson;
    }
    if (pJson->hasObject (MATCHMAKER_INFO_JSON::USEFUL_DISTANCE)) {
        JsonObject *pPart = pJson->getObject (MATCHMAKER_INFO_JSON::USEFUL_DISTANCE);
        if (pPart != nullptr) {
            rc += _usefulDistance.fromJson (pPart);
            delete pPart;
        }
    }
    if (pJson->hasObject (MATCHMAKER_INFO_JSON::RANGE_OF_INFLUENCE)) {
        JsonObject *pPart = pJson->getObject (MATCHMAKER_INFO_JSON::RANGE_OF_INFLUENCE);
        if (pPart != nullptr) {
            rc += _rangeOfInfluence.fromJson (pPart);
            delete pPart;
        }
    }
    if (pJson->hasObject (MATCHMAKER_INFO_JSON::CUSTOM_POLICIES)) {
        JsonObject *pPart = pJson->getObject (MATCHMAKER_INFO_JSON::CUSTOM_POLICIES);
        if (pPart != nullptr) {
            rc += _customPolicies.fromJson (pPart);
            delete pPart;
        }
    }
    return rc;
}

JsonObject * MatchmakingInfo::toJson (void) const
{
    JsonObject *pJson = new JsonObject();
    if (pJson == nullptr) {
        return nullptr;
    }
    pJson->setBoolean (MATCHMAKER_INFO_JSON::LIMIT_PRESTAGING_TO_LOCAL_DATA, _bLimitPrestagingToLocalData);
    pJson->setNumber (MATCHMAKER_INFO_JSON::MATCHMAKING_THRESHOLD, _fMatchmakingThreshold);

    JsonObject *pMetadataRankerConfJson = _pMetaDataRankerConf->toJson();
    if (pMetadataRankerConfJson != nullptr) {
        pJson->setObject (MATCHMAKER_INFO_JSON::METADATA_RANKER_CONF, pMetadataRankerConfJson);
        delete pMetadataRankerConfJson;
    }
    JsonObject *pPart = _usefulDistance.toJson();
    if (pPart != nullptr) {
        pJson->setObject (MATCHMAKER_INFO_JSON::USEFUL_DISTANCE, pPart);
        delete pPart;
    }
    pPart = _rangeOfInfluence.toJson();
    if (pPart != nullptr) {
        pJson->setObject (MATCHMAKER_INFO_JSON::RANGE_OF_INFLUENCE, pPart);
        delete pPart;
    }
    pPart = _customPolicies.toJson();
    if (pPart != nullptr) {
        pJson->setObject (MATCHMAKER_INFO_JSON::CUSTOM_POLICIES, pPart);
        delete pPart;
    }

    return pJson;
}

//---------------------------------------------------------

bool MatchmakingInfoHelper::parseAndSetUsefulDistanceByType (MatchmakingInfo *pLocationInfo, const char *pszUsefulDistanceValues)
{
    if (pLocationInfo == nullptr) {
        return false;
    }
    const char *pszMethodName = "UsefulDistance::parseAndSetUsefulDistanceByType";
    if (pszUsefulDistanceValues == nullptr) {
        return false;
    }
    StringTokenizer tokenizer (pszUsefulDistanceValues, ';', ';');
    StringTokenizer innerTokenizer;
    bool bAtLeastOneUpdate = false;
    for (const char *pszUsefulDistanceByType = tokenizer.getNextToken ();
        pszUsefulDistanceByType != nullptr;
        pszUsefulDistanceByType = tokenizer.getNextToken ()) {
        innerTokenizer.init (pszUsefulDistanceByType, ',', ',');
        const char *pszType = innerTokenizer.getNextToken ();
        const char *pszUsefulDistance = innerTokenizer.getNextToken ();
        if (pszType != nullptr && pszUsefulDistance != nullptr) {
            bAtLeastOneUpdate |= pLocationInfo->setUsefulDistance (pszType, atoui32 (pszUsefulDistance));
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "misconfiguration: type <%s> "
                            "useful distance <%s>", pszType != nullptr ? pszType : "NULL",
                            pszUsefulDistance != nullptr ? pszUsefulDistance : "NULL");
        }
    }
    return bAtLeastOneUpdate;
}

bool MatchmakingInfoHelper::parseAndSetRangesOfInfluence (MatchmakingInfo *pLocationInfo, const char *pszRangeOfInflByMilSTD)
{
    if ((pLocationInfo == nullptr) || (pszRangeOfInflByMilSTD == nullptr)) {
        return false;
    }
    bool bUpdated = false;
    if (pszRangeOfInflByMilSTD != nullptr) {
        StringStringHashtable *pRangeOfInflByMilSTD = StringStringHashtable::parseStringStringHashtable (pszRangeOfInflByMilSTD);
        if (pRangeOfInflByMilSTD != nullptr) {
            StringStringHashtable::Iterator iter = pRangeOfInflByMilSTD->getAllElements ();
            for (; !iter.end (); iter.nextElement ()) {
                bUpdated |= pLocationInfo->setRangeOfInfluence (iter.getKey (), atoui32 (iter.getValue ()));
            }
        }
    }
    return bUpdated;
}

int MatchmakingInfoHelper::parseAndAddQualifiers (MatchmakingInfo *pQualifiers, const char *pszLine)
{
    if ((pQualifiers == nullptr) || (pszLine == nullptr)) {
        return -1;
    }
    return pQualifiers->_qualifiers.parseAndAddQualifiers (pszLine);
}

