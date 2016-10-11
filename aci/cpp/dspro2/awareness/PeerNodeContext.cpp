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
#include "DSLib.h"

#include "Logger.h"
#include "NLFLib.h"
#include "Reader.h"
#include "Writer.h"
#include "InstrumentedReader.h"
#include <stdlib.h>
#include <string.h>

using namespace IHMC_ACI;
using namespace IHMC_C45;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    bool updateNodeContext (bool bForceRead, uint16 ui16LocalVersion, uint16 ui16RemoteVersion)
    {
        return (bForceRead || (ui16RemoteVersion > ui16LocalVersion));
    }
}

PeerNodeContext::PeerNodeContext (const char *pszNodeID, IHMC_C45::C45AVList *pClassifierConfiguration,
                                  double dTooFarCoeff, double dApproxCoeff)
    : NodeContext (pszNodeID, dTooFarCoeff, dApproxCoeff),
      _reacheableThrough (false)
{
    _pClassifier = new IHMC_C45::C45DecisionTree();
    _pClassifier->configureClassifier (pClassifierConfiguration);
    _pCurrPath = NULL;
    _pPastPath = NULL;
    _ui16ClassifierVersion = 0;
    _pCurrActualPosition = NULL;
}

PeerNodeContext::~PeerNodeContext()
{
    if (_pCurrPath != NULL) {
        delete _pCurrPath;
        _pCurrPath = NULL;
    }
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

int64 PeerNodeContext::readUpdates (Reader *pReader, uint32 ui32MaxSize, bool &bContextUnsynchronized)
{
    return readUpdatesInternal (pReader, ui32MaxSize, false, bContextUnsynchronized);
}
                                                         
int64 PeerNodeContext::readAll (Reader *pReader, uint32 ui32MaxSize)
{
    bool bContextUnsynchronized = false;
    return readUpdatesInternal (pReader, ui32MaxSize, true, bContextUnsynchronized);
}

int64 PeerNodeContext::readUpdatesInternal (Reader *pReader, uint32 ui32MaxSize,
                                            bool bForceReadAll, bool &bContextUnsynchronized)
{
    const char *pszMethodName = "PeerNodeContext::readUpdatesInternal";
    bContextUnsynchronized = false;

    uint32 ui32TotLength = 0;
    int8 rc;
    uint32 ui32StartingTime;
    uint16 ui16Version;
    if (pReader == NULL) {
        return -1;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "max size = %d\n", (int) ui32MaxSize);
    // Read information
    if (ui32MaxSize < (ui32TotLength + 4)) {
        return -2;
    }
    rc = pReader->read32 (&ui32StartingTime);
    if (rc < 0) {
        return -3;
    }
    ui32TotLength += 4;
    if (ui32StartingTime > _ui32StartingTime) {
        if (!bForceReadAll) {
            // if the ui32StartingTime has changed the node expects to receive a
            // whole new context, if this is not the case there may be parts of
            // the context that may be unsynchronized
            bContextUnsynchronized = true;
        }
        _ui32StartingTime = ui32StartingTime;
    }
    else if (bForceReadAll && (ui32StartingTime <= _ui32StartingTime)) {
        // It's an old whole message - just drop it!
        return 1;
    }

    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -5;
    }
    rc = pReader->read16 (&ui16Version);
    if (rc < 0) {
        return -6;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + 1)) {
        return -7;
    }

    bool bReadTeamMissionRole;
    if (DSLib::readBool (pReader, ui32MaxSize - ui32TotLength, bReadTeamMissionRole) < 0) {
        return -8;
    }
    ui32TotLength += 1;

    int64 i64;
    if (bReadTeamMissionRole) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "reading team, mission and role\n");
        const bool bSkip = !updateNodeContext (bForceReadAll, _ui16CurrInfoVersion, ui16Version);
        i64 = readLocalInformation (pReader, ui32MaxSize - ui32TotLength, bSkip);
        if (i64 < 0) {
            return i64;
        }
        if (!bSkip) {
            _customPolicies.removeAll();
        }
        const int iCustumPoliciesBytes = bSkip ?
                                         _customPolicies.skip (pReader, ui32MaxSize - ui32TotLength) :
                                         _customPolicies.read (pReader, ui32MaxSize - ui32TotLength);
        if (iCustumPoliciesBytes < 0) {
            return iCustumPoliciesBytes;
        }
        ui32TotLength += (uint32) iCustumPoliciesBytes;

        if (ui16Version > _ui16CurrInfoVersion) {
            _ui16CurrInfoVersion = ui16Version;
        }
        ui32TotLength += (uint32) i64;
    }

    // Read path info
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -9;
    }
    if ((rc = pReader->read16 (&ui16Version)) < 0) {
        return rc;
    }
    ui32TotLength += 2;

    i64 = readPathInformation (pReader, ui32MaxSize - ui32TotLength,
                               !updateNodeContext (bForceReadAll, _ui16CurrPathVersion, ui16Version));
    if (i64 < 0) {
        return i64;
    }
    ui32TotLength += (uint32) i64;
    checkAndLogMsg (pszMethodName, Logger::L_Info, "usefulDistance = %u\n", _ui32DefaultUsefulDistance);

    bool bReadPath;
    bool bPathHasBeenRead = false;
    if (DSLib::readBool (pReader, ui32MaxSize - ui32TotLength, bReadPath) < 0) {
        return -10;
    }
    ui32TotLength += 1;
    if ((ui16Version > 0) && updateNodeContext (bForceReadAll, _ui16CurrPathVersion, ui16Version)) {
        if (bReadPath) {
            _ui16CurrPathVersion = ui16Version;
            if (_pCurrPath == NULL) {
                _pCurrPath = new NodePath (NULL, 0, 0);
            }
            int64 pathLength = _pCurrPath->read (pReader, ui32MaxSize - ui32TotLength, false);
            if (pathLength < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to read path; rc = %d\n", (int) pathLength);
                return -22;
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "read path of length %d\n", _pCurrPath->getPathLength());
            }
            ui32TotLength += (uint32) pathLength;
            bPathHasBeenRead = true;
        }
        else if (!bContextUnsynchronized) {
            // The node expected to read the path, but it was not sent,
            // which means it it was sent in a previous update message, that
            // therefore must have been missed by the local node
            bContextUnsynchronized = true;
        }
    }
    else if (bReadPath) {
        NodePath path;  // create fake path
        int64 pathLength = path.read (pReader, ui32MaxSize - ui32TotLength, true);
        if (pathLength < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to read path; rc = %d\n", (int) pathLength);
            return -23;
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "read path of length %d\n", _pCurrPath->getPathLength());
        }
        ui32TotLength += (uint32) pathLength;
    }

    // Read Matchmaker Qualifiers
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -11;
    }
    if ((rc = pReader->read16 (&ui16Version)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    bool bReadCurrMatchmakerQualifier;
    if (DSLib::readBool (pReader, ui32MaxSize - ui32TotLength, bReadCurrMatchmakerQualifier) < 0) {
        return -12;
    }
    if (updateNodeContext (bForceReadAll, _ui16CurrMatchmakerQualifierVersion, ui16Version) && (ui16Version > 0)) {
        if (bReadCurrMatchmakerQualifier) {
            InstrumentedReader ir (pReader);
            _qualifiers.read (&ir, ui32MaxSize - ui32TotLength);
            ui32TotLength += ir.getBytesRead();
        }
        else if (!bContextUnsynchronized) {
            // The node expected to read the matchmaker qualifiers, but they were
            // not sent, which means they were sent in a previous update message,
            // that therefore must have been missed by the local node
            bContextUnsynchronized = true;
        }
    }
    else if (bReadCurrMatchmakerQualifier) {
        MatchmakingQualifiers qualifiers; // create fake path
        InstrumentedReader ir (pReader);
        qualifiers.read (&ir, ui32MaxSize - ui32TotLength);
        ui32TotLength += ir.getBytesRead();
    }

    // Read decision tree
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -23;
    }
    rc = pReader->read16 (&ui16Version);
    if (rc < 0) {
        return -24;
    }
    ui32TotLength += 2;
    bool bReadClassifier;
    bool bClassifierHasBeenRead = false;
    if (DSLib::readBool (pReader, ui32MaxSize - ui32TotLength, bReadClassifier) < 0) {
        return -6;
    }
    ui32TotLength += 1;
    if (updateNodeContext (bForceReadAll, _ui16ClassifierVersion, ui16Version) && (ui16Version > 0)) {
        if (bReadClassifier) {
            _ui16ClassifierVersion = ui16Version;
            int64 treeLength = _pClassifier->read (pReader, ui32MaxSize - ui32TotLength);
            if (treeLength < 0) {
                return -25;
            }
            ui32TotLength += (uint32) treeLength;
            bClassifierHasBeenRead = true;
        }
        else if (!bContextUnsynchronized) {
            // The node expected to read the classifier, but it was not sent,
            // which means it it was sent in a previous update message, that
            // therefore must have been missed by the local node 
            bContextUnsynchronized = true;
        }
    }
    else if (bReadClassifier) {
        int64 treeLength = _pClassifier->skip (pReader, ui32MaxSize - ui32TotLength);
        if (treeLength < 0) {
            return -26;
        }
        ui32TotLength += (uint32) treeLength;
    }

    if (bContextUnsynchronized && bReadTeamMissionRole &&
        bPathHasBeenRead && bClassifierHasBeenRead) {
        // The whole context was read anyway, the context can't be unsynchronized
        bContextUnsynchronized = false;
    }

    if (bForceReadAll) {
        rc = _pMetaDataRankerConf->read (pReader, ui32MaxSize - ui32TotLength); 
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "failed reading "
                            "the metadata ranker configuration %d\n", rc);
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "read "
                            "metadata ranker configuration.\n");
        }
        ui32TotLength += _pMetaDataRankerConf->getLength();
    }

    return ui32TotLength;
}

uint16 PeerNodeContext::getClassifierVersion (void)
{
    return _ui16ClassifierVersion;
}

int PeerNodeContext::getCurrentLatitude (float &latitude)
{
    if (_pCurrActualPosition == NULL) {
        return -1;
    }
    latitude = _pCurrActualPosition->latitude;
    return 0;
}

int PeerNodeContext::getCurrentLongitude (float &longitude)
{
    if (_pCurrActualPosition == NULL) {
        return -1;
    }
    longitude = _pCurrActualPosition->longitude;
    return 0;
}

int PeerNodeContext::getCurrentTimestamp (uint64 &timestamp)
{
    if (_pCurrActualPosition == NULL) {
        return -1;
    }
    timestamp = _pCurrActualPosition->timeStamp;
    return 0;
}

int PeerNodeContext::getCurrentPosition (float &latitude, float &longitude, float &altitude)
{
    if (_pCurrActualPosition == NULL) {
        return -1;
    }

    latitude = _pCurrActualPosition->latitude;
    longitude = _pCurrActualPosition->longitude;
    altitude = _pCurrActualPosition->altitude;

    return 0;
}

int PeerNodeContext::getCurrentPosition (float &latitude, float &longitude, float &altitude,
                                         const char *&pszLocation, const char *&pszNote,
                                         uint64 &timeStamp)
{
    if (_pCurrActualPosition == NULL) {
        return -1;
    }
    if (getCurrentPosition (latitude, longitude, altitude) < 0) {
        return -2;
    }
    pszLocation = _pCurrActualPosition->pszLocation;
    pszNote = _pCurrActualPosition->pszNote;
    timeStamp = _pCurrActualPosition->timeStamp;

    return 0;
}

bool PeerNodeContext::setCurrentPosition (float latitude, float longitude, float altitude,
                                          const char *pszLocation, const char *pszNote,
                                          uint64 timeStamp)
{
    bool rc = NodeContext::setCurrentPosition (latitude, longitude, altitude);
    if (_pCurrActualPosition != NULL) {
        if (!rc) {
            if (!isApproximableToPoint (_pCurrActualPosition->latitude,
                                        _pCurrActualPosition->longitude,
                                        latitude, longitude)) {
                rc = true;
            }
        }
    }
    else {
        _pCurrActualPosition = new CurrentActualPosition;
        if (_pCurrActualPosition == NULL) {
            checkAndLogMsg ("PeerNodeContext::setCurrentPosition", memoryExhausted);
            return rc;
        }
        // if _pCurrActualPosition, it is the first time that the position is
        // set, return true
        rc = true;
    }

    _pCurrActualPosition->latitude = latitude;
    _pCurrActualPosition->longitude = longitude;
    _pCurrActualPosition->altitude = altitude;

    free ((char *)_pCurrActualPosition->pszLocation);
    _pCurrActualPosition->pszLocation = (pszLocation != NULL ? strDup (pszLocation) : NULL);

    free ((char *)_pCurrActualPosition->pszNote);
    _pCurrActualPosition->pszNote = (pszNote != NULL ? strDup (pszNote) : NULL);

    _pCurrActualPosition->timeStamp = timeStamp;

    return rc;
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
    return _pCurrPath;
}

bool PeerNodeContext::operator == (PeerNodeContext &rhsPeerNodeContext)
{
    return (0 == strcmp (_pszNodeId, rhsPeerNodeContext.getNodeId()));
}

// CurrentActualPosition

PeerNodeContext::CurrentActualPosition::CurrentActualPosition()
{
    pszLocation = NULL;
    pszNote = NULL;
}

PeerNodeContext::CurrentActualPosition::~CurrentActualPosition()
{
    free ((char *)pszLocation);
    pszLocation = NULL;
    free ((char *)pszNote);
    pszNote = NULL;
}

