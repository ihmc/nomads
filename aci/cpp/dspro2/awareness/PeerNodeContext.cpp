/*
 * PeerNodeContext.cpp
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

#include "PeerNodeContext.h"

#include "Defs.h"
#include "MetadataRankerConfiguration.h"
#include "NodePath.h"

#include "C45DecisionTree.h"
#include "C45AVList.h"

#include "Json.h"
#include "Logger.h"
#include <stdlib.h>

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace IHMC_C45;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    bool updateNodeContext (bool bForceRead, uint16 ui16LocalVersion, uint16 ui16RemoteVersion)
    {
        return (bForceRead || (ui16RemoteVersion > ui16LocalVersion));
    }

    void updatePart (Part &part, const JsonObject *pJson, bool readAll, uint16 &incomingVersion)
    {
        if (readAll) {
            part.reset();
        }
        if (pJson != nullptr) {
            if (readAll || (incomingVersion > part.getVersion())) {
                if (part.fromJson (pJson) == 0) {
                    part.setVersion (incomingVersion);
                }
            }
            delete pJson;
        }
    }
}

PeerNodeContext::PeerNodeContext (const char *pszNodeID, IHMC_C45::C45AVList *pClassifierConfiguration,
                                  double dTooFarCoeff, double dApproxCoeff)
    : NodeContextImpl (pszNodeID, dTooFarCoeff, dApproxCoeff),
      _reacheableThrough (false)
{
    _pClassifier = new IHMC_C45::C45DecisionTree();
    _pClassifier->configureClassifier (pClassifierConfiguration);
    _ui16ClassifierVersion = 0;
}

PeerNodeContext::~PeerNodeContext (void)
{
}

int PeerNodeContext::addAdaptor (AdaptorId adaptorId)
{
    if (adaptorId < _reacheableThrough.size() && _reacheableThrough[adaptorId]) {
        // The adaptor had already been set
        return 1;
    }
    _reacheableThrough[adaptorId] = true;
    return 0;
}

int PeerNodeContext::removeAdaptor (AdaptorId adaptorId)
{
    if (adaptorId < _reacheableThrough.size() && !_reacheableThrough[adaptorId]) {
        // The adaptor had already been removed or never set
        return 1;
    }
    _reacheableThrough[adaptorId] = false;
    return 0;
}

bool PeerNodeContext::isReacheableThrough (AdaptorId adaptorId)
{
    if (adaptorId < _reacheableThrough.size() && !_reacheableThrough[adaptorId]) {
        // The adaptor had already been removed or never set
        return false;
    }
    return _reacheableThrough[adaptorId];
}

uint16 PeerNodeContext::getClassifierVersion (void)
{
    return _ui16ClassifierVersion;
}

int PeerNodeContext::getCurrentLatitude (float &latitude)
{
    float longitude, altitude;
    const char *pszLocation, *pszNote;
    uint64 timeStamp;
    _locationInfo.getCurrentPosition (latitude, longitude, altitude, pszLocation, pszNote, timeStamp);
    return 0;
}

int PeerNodeContext::getCurrentLongitude (float &longitude)
{
    float latitude, altitude;
    const char *pszLocation, *pszNote;
    uint64 timeStamp;
    _locationInfo.getCurrentPosition (latitude, longitude, altitude, pszLocation, pszNote, timeStamp);
    return 0;
}

int PeerNodeContext::getCurrentTimestamp (uint64 &timeStamp)
{
    float latitude, longitude, altitude;
    const char *pszLocation, *pszNote;
    _locationInfo.getCurrentPosition (latitude, longitude, altitude, pszLocation, pszNote, timeStamp);
    return 0;
}

int PeerNodeContext::getCurrentPosition (float &latitude, float &longitude, float &altitude)
{
    const char *pszLocation, *pszNote;
    uint64 timeStamp;
    _locationInfo.getCurrentPosition (latitude, longitude, altitude, pszLocation, pszNote, timeStamp);
    return 0;
}

int PeerNodeContext::getCurrentPosition (float &latitude, float &longitude, float &altitude,
                                         const char *&pszLocation, const char *&pszNote,
                                         uint64 &timeStamp)
{
    _locationInfo.getCurrentPosition (latitude, longitude, altitude, pszLocation, pszNote, timeStamp);
    return 0;
}

void PeerNodeContext::setPeerPresence (bool active)
{
    _bIsPeerActive = active;
}

bool PeerNodeContext::isPeerActive (void)
{
    return _bIsPeerActive;
}

NodePath * PeerNodeContext::getPath (void)
{
    return _pathInfo.getPath();
}

bool PeerNodeContext::operator == (PeerNodeContext &rhsPeerNodeContext)
{
    const NOMADSUtil::String nodeId (getNodeId());
    const NOMADSUtil::String rhsNodeId (rhsPeerNodeContext.getNodeId());
    return (nodeId == rhsNodeId);
}

int PeerNodeContext::fromJson (const NOMADSUtil::JsonObject *pJson)
{
    if (pJson == nullptr) {
        return -1;
    }
    Versions inVersions;
    JsonObject *pVersions = pJson->getObject (Versions::VERSIONS_OBJECT_NAME);
    if (pVersions == nullptr) {
        // Versions are expected in all the messages
        return -2;
    }
    if (inVersions.fromJson (pVersions) < 0) {
        delete pVersions;
        return -3;
    }
    if (inVersions._i64StartingTime < _i64StartingTime) {
        // The incoming versions are stale
        delete pVersions;
        return -4;
    }
    const bool bReadAll = (inVersions._i64StartingTime > _i64StartingTime);

    updatePart (_nodeInfo, pJson->getObject (NodeGenInfo::NODE_INFO_OBJECT_NAME), bReadAll,
                inVersions._ui16InfoVersion);
    updatePart (_pathInfo, pJson->getObject (PathInfo::PATH_INFO_OBJECT_NAME), bReadAll,
                inVersions._ui16PathVersion);
    updatePart (_matchmakingInfo, pJson->getObject (MatchmakingInfo::MATCHMAKING_INFO_OBJECT_NAME), bReadAll,
                inVersions._ui16MatchmakerQualifierVersion);
    updatePart (_areasOfInterestInfo, pJson->getObject (AreasOfInterestsInfo::AREAS_OF_INTEREST_OBJ),
                bReadAll, inVersions._ui16AreasOfInterestVersion);
    updatePart (_locationInfo, pJson->getObject (LocationInfo::LOCATION_INFO_OBJECT_NAME),
                bReadAll, inVersions._ui16WaypointVersion);
    if (bReadAll) {
        _i64StartingTime = inVersions._i64StartingTime;
    }
    return 0;
}

// CurrentActualPosition

PeerNodeContext::CurrentActualPosition::CurrentActualPosition (void)
{
    pszLocation = nullptr;
    pszNote = nullptr;
}

PeerNodeContext::CurrentActualPosition::~CurrentActualPosition (void)
{
    free ((char *)pszLocation);
    pszLocation = nullptr;
    free ((char *)pszNote);
    pszNote = nullptr;
}

