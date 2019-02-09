/*
 * NodeContextImpl.cpp
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

#include "NodeContextImpl.h"

#include "Defs.h"
#include "Path.h"

#include "Classifier.h"

#include "Logger.h"

#include "Json.h"

#include <stdlib.h>

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

#ifdef DUMP_REMOTELY_SEARCHED_IDS
    #include <stdio.h>
#endif
#include <InstrumentedReader.h>

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

NodeContextImpl::NodeContextImpl (const char *pszNodeId, double dTooFarCoeff, double dApproxCoeff)
    : _i64StartingTime (0),
      _nodeInfo (pszNodeId),
      _locationInfo (dTooFarCoeff, dApproxCoeff),
      _pClassifier (nullptr)
{
}

NodeContextImpl::~NodeContextImpl (void)
{
    if (_pClassifier != nullptr) {
        delete _pClassifier;
        _pClassifier = nullptr;
    }
}

Versions NodeContextImpl::getVersions (void)
{
    return Versions (_i64StartingTime, _nodeInfo.getVersion(), _pathInfo.getVersion(),
                     _locationInfo.getVersion(), 0, _matchmakingInfo.getVersion(),
                     _areasOfInterestInfo.getVersion());
}

uint32 NodeContextImpl::getRangeOfInfluence (const char *pszNodeType)
{
    return _matchmakingInfo.getRangeOfInfluence (pszNodeType);
}

uint32 NodeContextImpl::getMaximumRangeOfInfluence (void)
{
    return _matchmakingInfo.getMaximumRangeOfInfluence();
}

int NodeContextImpl::addUserId (const char *pszUserId)
{
    if (pszUserId == nullptr) {
        return -1;
    }
    if (_nodeInfo.addUserId (pszUserId)) {
        _nodeInfo.incrementVersion();
        return 0;
    }
    return -3;
}

int NodeContextImpl::addAreaOfInterest (const char *pszAreaName, BoundingBox &bb, int64 i64StatTime, int64 i64EndTime)
{
    if (pszAreaName == nullptr) {
        return -1;
    }
    if (_areasOfInterestInfo.add (pszAreaName, bb, i64StatTime, i64EndTime)) {
        _areasOfInterestInfo.incrementVersion();
        return 0;
    }
    return -3;
}

void NodeContextImpl::display (void)
{
    const char *pszMethodName = "NodeContextImpl::display";
    const JsonObject *pJson = toJson (nullptr);
    String json;
    if (pJson != nullptr) {
        json = pJson->toString();
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "%s\n", json.c_str());
}

AdjustedPathWrapper * NodeContextImpl::getAdjustedPath (void)
{
    return _locationInfo.getAdjustedPath (this);
}

PtrLList<CustomPolicy> * NodeContextImpl::getCustomPolicies (void)
{
    PtrLList<CustomPolicy> *pPolicies = new PtrLList<CustomPolicy>();
    CustomPolicies *pPoliciesImpl = _matchmakingInfo.getCustomPolicies();
    if ((pPolicies != nullptr) && (pPoliciesImpl != nullptr)) {
        CustomPolicyImpl *pimpl = pPoliciesImpl->getFirst();
        for (; pimpl != nullptr; pimpl = pPoliciesImpl->getNext()) {
            pPolicies->prepend (pimpl);
        }
    }
    return pPolicies;
}

IHMC_VOI::AreaOfInterestList * NodeContextImpl::getAreasOfInterest (void) const
{
    return _areasOfInterestInfo.getAreasOfInterest();
}

bool NodeContextImpl::hasUserId (const char *pszUserId)
{
    return _nodeInfo.hasUserId (pszUserId);
}

namespace NODE_CONTEX_JSON_NAMES
{
    const char * VERSIONS = "nodeContext";
}

JsonObject * NodeContextImpl::toJson (const Versions *pIncomingVersions, bool bCurrwaypointOnly)
{
    JsonObject *pJson = new JsonObject();
    if (pJson == nullptr) {
        return nullptr;
    }
    const Versions currVersions (getVersions());
    const bool allParts = (pIncomingVersions == nullptr) ||
        (pIncomingVersions->_i64StartingTime != currVersions._i64StartingTime);

    JsonObject *pJsonVersions = currVersions.toJson();
    if (pJsonVersions == nullptr) {
        return nullptr;
    }
    pJson->setObject (Versions::VERSIONS_OBJECT_NAME, pJsonVersions);
    delete pJsonVersions;

    JsonObject *pJsonPart = nullptr;
    if ((!bCurrwaypointOnly) && (allParts || currVersions._ui16InfoVersion > pIncomingVersions->_ui16InfoVersion)) {
        pJsonPart = _nodeInfo.toJson();
        pJson->setObject (NodeGenInfo::NODE_INFO_OBJECT_NAME, pJsonPart);
        delete pJsonPart;
    }
    if ((!bCurrwaypointOnly) && (allParts || currVersions._ui16ClassifierVersion > pIncomingVersions->_ui16ClassifierVersion)) {
        //pJson->setObject (NodeInfo::NODE_INFO_OBJECT_NAME, _nodeInfo.toJson());
    }
    if ((!bCurrwaypointOnly) && (allParts || currVersions._ui16MatchmakerQualifierVersion > pIncomingVersions->_ui16MatchmakerQualifierVersion)) {
        pJsonPart = _matchmakingInfo.toJson();
        pJson->setObject (MatchmakingInfo::MATCHMAKING_INFO_OBJECT_NAME, pJsonPart);
        delete pJsonPart;
    }
    if ((!bCurrwaypointOnly) && (allParts || currVersions._ui16PathVersion > pIncomingVersions->_ui16PathVersion)) {
        pJsonPart = _pathInfo.toJson();
        pJson->setObject (PathInfo::PATH_INFO_OBJECT_NAME, pJsonPart);
        delete pJsonPart;
    }
    if ((!bCurrwaypointOnly) && (allParts || currVersions._ui16AreasOfInterestVersion > pIncomingVersions->_ui16AreasOfInterestVersion)) {
        pJsonPart = _areasOfInterestInfo.toJson();
        pJson->setObject (AreasOfInterestsInfo::AREAS_OF_INTEREST_OBJ, pJsonPart);
        delete pJsonPart;
    }
    if (allParts || currVersions._ui16WaypointVersion > pIncomingVersions->_ui16WaypointVersion) {
        pJsonPart = _locationInfo.toJson ();
        pJson->setObject (LocationInfo::LOCATION_INFO_OBJECT_NAME, pJsonPart);
        delete pJsonPart;
    }
    return pJson;
}

void NodeContextImpl::reset (void)
{
    _nodeInfo.reset();
    _matchmakingInfo.reset();
    _pathInfo.reset();
    _locationInfo.reset();
    _i64StartingTime = 0;
}

