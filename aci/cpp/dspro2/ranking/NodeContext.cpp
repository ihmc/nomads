/* 
 * NodeContext.cpp
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

#include "NodeContext.h"

#include "Defs.h"
#include "MetadataRankerConfiguration.h"
#include "NodePath.h"
#include "Path.h"
#include "DSLib.h"

#include "Classifier.h"

#include "GeoUtils.h"
#include "InstrumentedWriter.h"
#include "Logger.h"
#include "NLFLib.h"
#include "NullWriter.h"
#include "Writer.h"
#include "CustomPolicies.h"

#include <stdlib.h>
#include <string.h>

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

#ifdef DUMP_REMOTELY_SEARCHED_IDS
    #include <stdio.h>
#endif

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const unsigned int NodeContext::DEFAULT_USEFUL_DISTANCE = 1000;
const NodeContext::PositionApproximationMode NodeContext::DEFAULT_PATH_ADJUSTING_MODE = GO_TO_NEXT_WAY_POINT;
const unsigned int NodeContext::WAYPOINT_UNSET = -1;

const char * NodeContext::PATH_UNSET_DESCRIPTOR = "PATH_UNSET";
const char * NodeContext::ON_WAY_POINT_DESCRIPTOR = "ON_WAY_POINT";
const char * NodeContext::PATH_DETOURED_DESCRIPTOR = "PATH_DETOURED";
const char * NodeContext::PATH_IN_TRANSIT_DESCRIPTOR = "PATH_IN_TRANSIT";
const char * NodeContext::TOO_FAR_FROM_PATH_DESCRIPTOR = "TOO_FAR_FROM_PATH";

const double NodeContext::TOO_FAR_FROM_PATH_COEFF = 10.0;
const double NodeContext::APPROXIMABLE_TO_POINT_COEFF = 10.0;

NodeContext::NodeContext (const char *pszNodeID, double dTooFarCoeff, double dApproxCoeff)
    : _bLimitPrestagingToLocalData (false),
      _ui8BatteryLevel (0),
      _ui8MemoryAvailable (0),
      _ui16CurrInfoVersion (0),
      _ui16CurrPathVersion (0),    
      _ui16CurrWaypointVersion (0),
      _ui16CurrMatchmakerQualifierVersion (0),
      _status (PATH_UNSET),
      _ui32StartingTime (0),
      _ui32DefaultUsefulDistance (DEFAULT_USEFUL_DISTANCE),
      _iCurrWayPointInPath (WAYPOINT_UNSET),
      _closestPointOnPathLat (MAX_LATITUDE + 1),
      _closestPointOnPathLong (MAX_LONGITUDE + 1),
      _fMatchmakingThreshold (6.0f),
      _dTooFarCoeff (dTooFarCoeff),
      _dApproxCoeff (dApproxCoeff),
      _pszNodeId ((pszNodeID != NULL) ? strDup (pszNodeID) : NULL),
      _pszTeamID (NULL),
      _pszMissionID (NULL),
      _pszRole (NULL),
      _pClassifier (NULL),
      _pMetaDataRankerConf (NULL),
      _usefulDistanceByMimeType (false,         // bCaseSensitiveKeys
                                 true,          // bCloneKeys
                                 true,          // bDeleteKeys
                                 true),         // bDeleteValues
      _pathAdjustingMode (DEFAULT_PATH_ADJUSTING_MODE),
      _pPathWrapper (NULL)
      
{
    _pPathWrapper = new AdjustedPathWrapper();
    if (_pPathWrapper == NULL) {
        checkAndLogMsg ("NodeContext::NodeContext", memoryExhausted);
        exit (-1);
    }
    _pMetaDataRankerConf = new MetadataRankerConfiguration();
}

NodeContext::~NodeContext()
{
    if (_pszNodeId != NULL) {
        free((char *)_pszNodeId);
    }
    if (_pszTeamID != NULL) {
        free(_pszTeamID);
        _pszTeamID = NULL;
    }
    if (_pszMissionID != NULL) {
        free(_pszMissionID);
        _pszMissionID = NULL;
    }
    if (_pszRole != NULL) {
        free(_pszRole);
        _pszRole = NULL;
    }
    if (_pClassifier != NULL) {
        delete _pClassifier;
        _pClassifier = NULL;
    }
}

int NodeContext::addUserId (const char *pszUserId)
{
    if (pszUserId == NULL) {
        return -1;
    }
    NOMADSUtil::String userId (pszUserId);
    if (userId.length() < 1) {
        return -2;
    }
    if (_usersIds.search (userId) == 0) {
        _usersIds.add (userId);
        _ui16CurrInfoVersion++;
        return 0;
    }
    return -3;
}

int NodeContext::setMissionId (const char *pszMissionName)
{
    if (pszMissionName == NULL) {
        return -1;
    }
    NOMADSUtil::String missionId (pszMissionName);
    if (missionId.length() < 1) {
        return -2;
    }
    if (_pszMissionID != NULL) {
        String currId (_pszMissionID);
        if ((missionId == currId) == 1) {
            return 1;
        }
        free (_pszMissionID);
    }
    _pszMissionID = missionId.r_str();
    _ui16CurrInfoVersion++;
    return 0;
}

void NodeContext::display (void)
{
    const char *pszMethodName = "NodeContext::display";
    checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "Node ID <%s> | Team ID <%s> | Mission ID <%s> | Role <%s>\n",
                    (_pszNodeId ? _pszNodeId : "NULL"), (_pszTeamID ? _pszTeamID : "NULL"),
                    (_pszMissionID ? _pszMissionID : "NULL"), (_pszRole ? _pszRole : "NULL"));
    checkAndLogMsg (pszMethodName, Logger::L_Info, "_ui16CurrInfoVersion %d | _ui16CurrPathVersion %d | _ui16CurrWayPointVersion %d | _iCurrWayPointInPath %d | _status %d\n"
                    "_closestPointOnPathLat %f | _closestPointOnPathLong %f\n_ui32UsefulDistance %u | _pathAdjustingMode %d", _ui16CurrInfoVersion,
                    _ui16CurrPathVersion, _ui16CurrWaypointVersion, _iCurrWayPointInPath, _status, _closestPointOnPathLat, _closestPointOnPathLong,
                    _ui32DefaultUsefulDistance, _pathAdjustingMode);
    _pMetaDataRankerConf->display();
}

AdjustedPathWrapper * NodeContext::getAdjustedPath (void)
{
    _pPathWrapper->configure (this);
    return _pPathWrapper;
}

const char * NodeContext::getStatusAsString (NodeStatus status)
{
    switch (status) {
        case PATH_UNSET:
            return PATH_UNSET_DESCRIPTOR;

        case ON_WAY_POINT:
            return ON_WAY_POINT_DESCRIPTOR;

        case PATH_DETOURED:
            return PATH_DETOURED_DESCRIPTOR;

        case PATH_IN_TRANSIT:
            return PATH_IN_TRANSIT_DESCRIPTOR;

        case TOO_FAR_FROM_PATH:
            return TOO_FAR_FROM_PATH_DESCRIPTOR;

        default:
            return NULL;
    }
}

bool NodeContext::hasUserId (const char *pszUserId)
{
    if (pszUserId != NULL) {
        NOMADSUtil::String userId;
        for (int rc = _usersIds.getFirst (userId); rc == 1; rc = _usersIds.getNext (userId)) {
            if (stricmp (userId.c_str(), pszUserId) == 0) {
                return true;
            }
        }
    }
    return false;
}

//------------------------------------------------------------------------------
// VERSIONS
//------------------------------------------------------------------------------

int64 NodeContext::readVersions (Reader *pReader, uint32 ui32MaxSize, bool &pbContainsVersions,
                                 Versions &versions)
{
    if (pReader == NULL) {
        return -1;
    }
    uint32 ui32TotLength = 0;
    int8 rc;
    uint8 ui8ContainsVersions = 0;
    
    uint32 ui32StartingTime;
    uint16 ui16InformationVersion, ui16PathVersion, ui16WayPointVersion,
           ui16CurrMatchmakerQualifierVersion, ui16ClassifierVersion;
    
    if (ui32MaxSize < (ui32TotLength + 1)) {
        return -2;
    }
    if ((rc = pReader->read8 (&ui8ContainsVersions)) < 0) {
        return rc;
    }
    ui32TotLength += 1;
    pbContainsVersions = (ui8ContainsVersions == 1);
    if (!pbContainsVersions) {
        return ui32TotLength;
    }
    if (ui32MaxSize < (ui32TotLength + 4)) {
        return -2;
    }
    if ((rc = pReader->read32 (&ui32StartingTime)) < 0) {
        return rc;
    }
    ui32TotLength += 4;
    if ((rc = pReader->read16 (&ui16InformationVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -3;
    }
    if ((rc = pReader->read16 (&ui16PathVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -4;
    }
    if ((rc = pReader->read16 (&ui16WayPointVersion)) < 0) {
        return rc;
    }
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -5;
    }
    if ((rc = pReader->read16 (&ui16CurrMatchmakerQualifierVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -6;
    }
    if ((rc = pReader->read16 (&ui16ClassifierVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;

    versions.set (ui32StartingTime, ui16InformationVersion, ui16PathVersion,
                  ui16WayPointVersion, ui16ClassifierVersion, ui16CurrMatchmakerQualifierVersion);

    return ui32TotLength;
}

int64 NodeContext::writeEmptyVersions (NOMADSUtil::Writer *pWriter)
{
    if (pWriter == NULL) {
        return -1;
    }
    uint8 ui8ContainsVersion = 0;
    if (pWriter->write8 (&ui8ContainsVersion) < 0) {
        return -2;
    }

    return 1;
}

int64 NodeContext::writeVersions (Writer *pWriter)
{
    return writeVersionsInternal (pWriter, 0, true);
}

int64 NodeContext::writeVersions (Writer *pWriter, uint32 ui32MaxSize)
{
    return writeVersionsInternal (pWriter, ui32MaxSize, false);
}

int64 NodeContext::writeVersionsInternal (Writer *pWriter, uint32 ui32MaxSize, bool bIgnoreMaxSizes)
{
    if (pWriter == NULL) {
        return -1;
    }
    uint32 ui32TotLength = 0;
    int8 rc;
    if ((!bIgnoreMaxSizes) && ui32MaxSize < (ui32TotLength + 2)) {
        return -2;
    }
    uint8 ui8ContainsVersion = 1;
    if (pWriter->write8 (&ui8ContainsVersion) < 0) {
        return -2;
    }
    ui32TotLength += 1;
    if ((rc = pWriter->write32 (&_ui32StartingTime)) < 0) {
        return rc;
    }
    if ((rc = pWriter->write16 (&_ui16CurrInfoVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if ((!bIgnoreMaxSizes) && ui32MaxSize < (ui32TotLength + 2)) {
        return -3;
    }
    if ((rc = pWriter->write16 (&_ui16CurrPathVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if ((!bIgnoreMaxSizes) && ui32MaxSize < (ui32TotLength + 2)) {
        return -4;
    }
    if ((rc = pWriter->write16 (&_ui16CurrWaypointVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if ((!bIgnoreMaxSizes) && ui32MaxSize < (ui32TotLength + 2)) {
        return -5;
    }
    if ((rc = pWriter->write16 (&_ui16CurrMatchmakerQualifierVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if ((!bIgnoreMaxSizes) && ui32MaxSize < (ui32TotLength + 2)) {
        return -6;
    }

    uint16 ui16ClassifierVersion = getClassifierVersion();
    if ((rc = pWriter->write16 (&ui16ClassifierVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    return ui32TotLength;
}

int NodeContext::getWriteVersionsLength (void)
{
    return 11;
}

//------------------------------------------------------------------------------
// ALL
//------------------------------------------------------------------------------

int64 NodeContext::writeAll (Writer *pWriter, uint32 ui32MaxSize)
{
    Versions versions;
    return writeUpdatesInternal (pWriter, ui32MaxSize, versions, true);
}

int64 NodeContext::getWriteAllLength()
{
    Versions versions;
    versions._ui16ClassifierVersion = (uint16) getClassifierVersion();
    return getWriteUpdatesLengthInternal (versions , true);
}

//------------------------------------------------------------------------------
// UPDATE
//------------------------------------------------------------------------------

int64 NodeContext::writeUpdates (Writer *pWriter, uint32 ui32MaxSize, const Versions &versions)
{
    return writeUpdatesInternal (pWriter, ui32MaxSize, versions, false);
}

int64 NodeContext::getWriteUpdatesLength (const Versions &versions)
{
    return getWriteUpdatesLengthInternal (versions, false);
}

int64 NodeContext::writeUpdatesInternal (Writer *pWriter, uint32 ui32MaxSize,
                                         const Versions &versions, bool bForceWriteAll)
{
    if (pWriter == NULL) {
        return -1;
    }

    uint32 ui32TotLength = 0;
    int8 rc;

    // Write information
    if (ui32MaxSize < (ui32TotLength + 4)) {
        return -2;
    }
    if ((rc = pWriter->write32 (&_ui32StartingTime)) < 0) {
        return -3;
    }
    ui32TotLength += 4;

    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -2;
    }
    if ((rc = pWriter->write16 (&_ui16CurrInfoVersion)) < 0) {
        return -4;
    }
    ui32TotLength += 2;

    if (((versions._ui16InfoVersion < _ui16CurrInfoVersion) || bForceWriteAll) &&
        (_ui16CurrInfoVersion > 0)) {

        int8 i8WriteTeamMissionRole = 1;
        if ((rc = pWriter->write8 (&i8WriteTeamMissionRole)) < 0) {
            return -5;
        }
        ui32TotLength += 1;

        // Write local information
        int64 i64 = writeLocalInformation (pWriter, ui32MaxSize - ui32TotLength);
        if (i64 < 0) {
            return i64;
        }
        ui32TotLength += (uint32) i64;

        // Write custom policies
        const int iCustumPoliciesBytes = _customPolicies.write (pWriter, ui32MaxSize - ui32TotLength);
        if (iCustumPoliciesBytes < 0) {
            return iCustumPoliciesBytes;
        }
        ui32TotLength += (uint32) iCustumPoliciesBytes;
    }
    else {
        int8 i8WriteTeamMissionRole = 0;
        if ((rc = pWriter->write8 (&i8WriteTeamMissionRole)) < 0) {
            return -20;
        }
    }

    // Write path information
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -2;
    }
    if ((rc = pWriter->write16 (&_ui16CurrPathVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    int64 i64 = writePathInformation (pWriter, ui32MaxSize - ui32TotLength);
    if (i64 < 0) {
        return i64;
    }
    ui32TotLength += (uint32) i64;
    checkAndLogMsg ("NodeContext::writeUpdatesInternal", Logger::L_Info,
                    "usefulDistance = %u\n", _ui32DefaultUsefulDistance);

    // Write current path
    if (((versions._ui16PathVersion < _ui16CurrPathVersion) || bForceWriteAll) &&
        (_ui16CurrPathVersion > 0)) {
        if (DSLib::writeBool (pWriter, ui32MaxSize - ui32TotLength, true) < 0) {
            return -5;
        }
        ui32TotLength += 1;

        NodePath *pCurrPath = getPath();
        if (pCurrPath != NULL) {
            int64 pathLength = pCurrPath->write (pWriter, ui32MaxSize - ui32TotLength);
            if (pathLength < 0) {
                return -29;
            }
            ui32TotLength += (uint32) pathLength;
        }
    }
    else {
        if (DSLib::writeBool (pWriter, ui32MaxSize - ui32TotLength, false) < 0) {
            return -5;
        }
        ui32TotLength += 1;
    }

    if ((rc = pWriter->write16 (&_ui16CurrMatchmakerQualifierVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if (versions._ui16MatchmakerQualifierVersion > _ui16CurrMatchmakerQualifierVersion || bForceWriteAll) {
        if (DSLib::writeBool (pWriter, ui32MaxSize - ui32TotLength, true) < 0) {
            return -5;
        }
        ui32TotLength += 1;
        InstrumentedWriter iw (pWriter);
        _qualifiers.write (&iw, ui32MaxSize - ui32TotLength);
        ui32TotLength += iw.getBytesWritten();
    }
    else {
        if (DSLib::writeBool (pWriter, ui32MaxSize - ui32TotLength, false) < 0) {
            return -5;
        }
        ui32TotLength += 1;
    }

    // Write the decision tree
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -30;
    }
    uint16 trees = (uint16) getClassifierVersion();
    if ((rc = pWriter->write16 (&trees)) < 0) {
        return -31;
    }
    ui32TotLength += 2;
    if (((versions._ui16ClassifierVersion < trees) || bForceWriteAll) &&
        (trees > 0)) {

        if (DSLib::writeBool (pWriter, ui32MaxSize - ui32TotLength, true) < 0) {
            return -5;
        }
        ui32TotLength += 1;

        int64 treeLength = _pClassifier->write (pWriter, ui32MaxSize - ui32TotLength);
        if (treeLength < 0) {
            return -32;
        }
        ui32TotLength += (uint32) treeLength;
    }
    else {
        if (DSLib::writeBool (pWriter, ui32MaxSize - ui32TotLength, false) < 0) {
            return -5;
        }
        ui32TotLength += 1; 
    }

    if (bForceWriteAll) {
        _pMetaDataRankerConf->write (pWriter, ui32MaxSize - ui32TotLength);
        ui32TotLength += _pMetaDataRankerConf->getLength();
    }

    checkAndLogMsg ("NodeContext::writeUpdatesInternal", Logger::L_Info,
                    "total length = %d\n", (int) ui32TotLength);
    return ui32TotLength;
}

int64 NodeContext::writeForDSProListener (Writer *pWriter)
{
    if (pWriter == NULL) {
        return -1;
    }
    InstrumentedWriter iw (pWriter);
    uint32 len = (_pszNodeId != NULL ? strlen (_pszNodeId) : 0);
    iw.write32 (&len);
    if (len > 0) {
        iw.writeBytes (_pszNodeId, len);
    }
    len = (_pszTeamID != NULL ? strlen (_pszTeamID) : 0);
    iw.write32 (&len);
    if (len > 0) {
        iw.writeBytes (_pszTeamID, len);
    }
    len = (_pszMissionID != NULL ? strlen (_pszMissionID) : 0);
    iw.write32 (&len);
    if (len > 0) {
        iw.writeBytes (_pszMissionID, len);
    }
    len = (_pszRole != NULL ? strlen (_pszRole) : 0);
    iw.write32 (&len);
    if (len > 0) {
        iw.writeBytes (_pszRole, len);
    }
    uint8 ui8 = (isPeerActive() ? 1 : 0);
    iw.write8 (&ui8);
    iw.write32 (&_ui32DefaultUsefulDistance);
    iw.write32 (&_iCurrWayPointInPath);
    char *pszQualifiers = _qualifiers.getAsString();
    len = (pszQualifiers == NULL ? 0 : strlen (pszQualifiers));
    iw.write32 (&len);
    if (len > 0) {
        iw.writeBytes (pszQualifiers, len);
    }
    if (pszQualifiers != NULL) {
        free (pszQualifiers);
    }
    int64 ret = iw.getBytesWritten();
    ret += writeVersions (pWriter);
    return ret;
}

int64 NodeContext::getWriteUpdatesLengthInternal (const Versions &versions, bool bForceWriteAll)
{
    const uint32 NO_LIMIT = 0xFFFFFFFFU;
    NullWriter writer;
    return writeUpdatesInternal (&writer, NO_LIMIT, versions, bForceWriteAll);
}

//------------------------------------------------------------------------------
// WAYPOINT
//------------------------------------------------------------------------------

int NodeContext::readCurrentWaypoint (Reader *pReader, uint32 ui32MaxSize,
                                      Versions &versions, bool &bPositionHasChanged)
{
    if (pReader == NULL) {
        return -1;
    }

    uint32 ui32StartingTime;
    uint16 ui16InformationVersion, ui16PathVersion, ui16CurrMatchmakerQualifierVersion,
           ui16ClassifierVersion, ui16WayPointVersion;

    uint32 totLength = 0;
    int8 rc;
    if (ui32MaxSize < (totLength + 2)) {
        return -2;
    }
    if ((rc = pReader->read32 (&ui32StartingTime)) < 0) {
        return rc;
    }
    totLength += 4;
    if ((rc = pReader->read16 (&ui16InformationVersion)) < 0) {
        return rc;
    }
    totLength += 2;
    if (ui32MaxSize < (totLength + 2)) {
        return -2;
    }
    if ((rc = pReader->read16 (&ui16PathVersion)) < 0) {
        return rc;
    }
    totLength += 2;
    if (ui32MaxSize < (totLength + 2)) {
        return -2;
    }
    if ((rc = pReader->read16 (&ui16CurrMatchmakerQualifierVersion)) < 0) {
        return rc;
    }
    totLength += 2;
    if (ui32MaxSize < (totLength + 2)) {
        return -2;
    }
    if ((rc = pReader->read16 (&ui16ClassifierVersion)) < 0) {
        return rc;
    }
    totLength += 2;
    if (ui32MaxSize < (totLength + 2)) {
        return -2;
    }
    if ((rc = pReader->read16 (&ui16WayPointVersion)) < 0) {
        return rc;
    }
    totLength += 2;
    if (ui32MaxSize < (totLength + sizeof(int))) {
        return -2;
    }

    versions.set (ui32StartingTime, ui16InformationVersion, ui16PathVersion,
            ui16WayPointVersion, ui16ClassifierVersion, ui16CurrMatchmakerQualifierVersion);

    int iNewCurrentWayPoint;
    if ((rc = pReader->read32 (&iNewCurrentWayPoint)) < 0) {
        return rc;
    }
    totLength += 4;

    if (ui32StartingTime < _ui32StartingTime ||
        ui16InformationVersion < _ui16CurrInfoVersion ||
        ui16PathVersion < _ui16CurrPathVersion) {
        //checkAndLogMsg ("TEST", Logger::L_Info,"waypoint for old node context - it is not worth it to read the rest of the message");
        bPositionHasChanged = false;
        return 0;
    }
    if (ui32StartingTime == _ui32StartingTime &&
        ui16InformationVersion == _ui16CurrInfoVersion &&
        ui16PathVersion == _ui16CurrPathVersion &&
        ui16WayPointVersion <= _ui16CurrWaypointVersion) {
        //checkAndLogMsg ("TEST", Logger::L_Info, "old waypoint for the current node context - it is not worth it to read the rest of the message");
        bPositionHasChanged = false;
        return 0;
    }

    // Either new waypoint for the current node context, or waypoint for a newer
    // node context - I should read it, because the most updated position of the
    // node

    uint8 ui8ReadActualPosition;
    pReader->read8 (&ui8ReadActualPosition);
    if (ui8ReadActualPosition == 1) {
        float fLatitude, fLongitude, fAltitude;
        int64 i64TimeStamp;
        if (ui32MaxSize < (totLength + 4 + 4 + 4 + 8)) {
            return -3;
        }
        if ((rc = pReader->read32 (&fLatitude)) < 0) {
            return rc;
        }
        if ((rc = pReader->read32 (&fLongitude)) < 0) {
            return rc;
        }
        if ((rc = pReader->read32 (&fAltitude)) < 0) {
            return rc;
        }
        if ((rc = pReader->read64 (&i64TimeStamp)) < 0) {
            return rc;
        }
        if (!bPositionHasChanged) {

        }
        totLength += 4 + 4 + 4 + 8;
        bool bPositionHasChangedTmp = setCurrentPosition (fLatitude, fLongitude, fAltitude, NULL, NULL, i64TimeStamp);
        if (bPositionHasChanged != bPositionHasChangedTmp) {
            checkAndLogMsg ("NodeContext::readCurrentWayPoint", Logger::L_Info,
                            "the position has changed and the version message has not, or the other way around\n");
        }
        bPositionHasChanged = bPositionHasChangedTmp;
    }
    else {
        if (_iCurrWayPointInPath != iNewCurrentWayPoint) {
            checkAndLogMsg ("NodeContext::readCurrentWayPoint", Logger::L_Info,
                            "The position has changed: %d -> %d\n",
                            _iCurrWayPointInPath, iNewCurrentWayPoint);
            _iCurrWayPointInPath = iNewCurrentWayPoint; // Update the current way point
            bPositionHasChanged = true;
        }
        else {
            checkAndLogMsg ("NodeContext::readCurrentWayPoint", Logger::L_Info,
                            "The position has not changed: %d == %d\n",
                             _iCurrWayPointInPath, iNewCurrentWayPoint);
            bPositionHasChanged = false;
        }
        _ui16CurrWaypointVersion = ui16WayPointVersion;
    }

    #ifdef DUMP_REMOTELY_SEARCHED_IDS
        static FILE *_currentWayPointsDumpFile = fopen ("currentWayPointsDumpFile.csv", "w");
        if (_currentWayPointsDumpFile != NULL) {
            fprintf (_currentWayPointsDumpFile, "%d,%u,%lld\n", _iCurrWayPointInPath, (*ui16WayPointVersion), getTimeInMilliseconds());
            fflush (_currentWayPointsDumpFile);
        }
    #endif

    return totLength;
}

void NodeContext::setBatteryLevel (unsigned int uiBatteryLevel)
{
    if (uiBatteryLevel <= 10) {
        _ui8BatteryLevel = uiBatteryLevel;
    }
    else {
        _ui8BatteryLevel = 10;
    }
}

void NodeContext::setMemoryAvailable (unsigned int uiMemoryAvailable)
{
    if (uiMemoryAvailable <= 10) {
        _ui8MemoryAvailable = uiMemoryAvailable;
    }
    else {
        _ui8MemoryAvailable = 10;
    }
}

void NodeContext::setLimitToLocalMatchmakingOnly (bool bLimitPrestagingToLocalData)
{
    _bLimitPrestagingToLocalData = bLimitPrestagingToLocalData;
    _ui16CurrInfoVersion++;
    checkAndLogMsg ("NodeContext::setLimitToLocalMatchmakingOnly", Logger::L_Info,
                    "LimitPrestagingToLocalData: %s\n", _bLimitPrestagingToLocalData ?
                    "true" : "false");
}

void NodeContext::setMatchmakingThreshold (float fMatchmakingThreshold)
{
    _fMatchmakingThreshold = fMatchmakingThreshold;
    _ui16CurrInfoVersion++;
    checkAndLogMsg ("NodeContext::setMatchmakingThreshold", Logger::L_Info,
                    "MatchmakingThreshold: %f\n", _fMatchmakingThreshold);
}

int NodeContext::writeCurrentWaypoint (Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return -1;
    }
    uint32 ui32TotLength = 0;
    int8 rc;
    if (ui32MaxSize < (ui32TotLength + 4)) {
        return -2;
    }
    if ((rc = pWriter->write32 (&_ui32StartingTime)) < 0) {
        return rc;
    }
    ui32TotLength += 4;
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -3;
    }
    if ((rc = pWriter->write16 (&_ui16CurrInfoVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -4;
    }
    if ((rc = pWriter->write16 (&_ui16CurrPathVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -5;
    }
    if ((rc = pWriter->write16 (&_ui16CurrMatchmakerQualifierVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -5;
    }    
    uint16 trees = (uint16) getClassifierVersion();
    if ((rc = pWriter->write16 (&trees)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -6;
    }
    if ((rc = pWriter->write16 (&_ui16CurrWaypointVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + sizeof(int))) {
        return -7;
    }
    if ((rc = pWriter->write32 (&_iCurrWayPointInPath)) < 0) {
        return rc;
    }
    ui32TotLength += 4;

    float fLatitude, fLongitude, fAltitude;
    const char *pszLocation;
    const char *pszNote;
    uint64 ui64TimeStamp;
    uint8 ui8WriteActualPosition;
    if (0 == getCurrentPosition (fLatitude, fLongitude, fAltitude, pszLocation, pszNote, ui64TimeStamp)) {
        ui8WriteActualPosition = 1;
        pWriter->write8 (&ui8WriteActualPosition);

        if (ui32MaxSize < (ui32TotLength + 4 + 4 + 4 + 8)) {
            return -3;
        }
        if ((rc = pWriter->write32 (&fLatitude)) < 0) {
            return rc;
        }
        if ((rc = pWriter->write32 (&fLongitude)) < 0) {
            return rc;
        }
        if ((rc = pWriter->write32 (&fAltitude)) < 0) {
            return rc;
        }
        if ((rc = pWriter->write64 (&ui64TimeStamp)) < 0) {
            return rc;
        }
        ui32TotLength += 4 + 4 + 4 + 8;
    }
    else {
        ui8WriteActualPosition = 0;
        pWriter->write8 (&ui8WriteActualPosition);
    }

    return ui32TotLength;
}

int NodeContext::getWriteWaypointLength (void)
{
    int iRet = 10 + sizeof(int) + 1;

    float fLatitude, fLongitude, fAltitude;
    const char *pszLocation;
    const char *pszNote;
    uint64 ui64TimeStamp;
    if (0 == getCurrentPosition (fLatitude, fLongitude, fAltitude, pszLocation, pszNote, ui64TimeStamp)) {
        iRet += (4 + 4 + 4 + 8);
    }
    return 8 + iRet;
}

// LOCAL INFORMATION

int64 NodeContext::readLocalInformation (Reader *pReader, uint32 ui32MaxSize, bool bSkip)
{
    if (pReader == NULL) {
        return -1;
    }
    uint32 ui32TotLength = 0;
    uint32 ui32BytesRead = 0;

    char *pszTmp = NULL;

    if (!bSkip && (_pszTeamID != NULL)) {
        free (_pszTeamID);
        _pszTeamID = NULL;
    }
    
    if (DSLib::readString (pReader, ui32MaxSize - ui32TotLength, pszTmp, ui32BytesRead) < 0) {
        return -2;
    }
    if (!bSkip) {
       _pszTeamID = pszTmp;
    }

    ui32TotLength += ui32BytesRead;

    ui32BytesRead = 0;
    if (!bSkip && (_pszMissionID != NULL)) {
        free (_pszMissionID);
        _pszMissionID = NULL;
    }
    if (DSLib::readString (pReader, ui32MaxSize - ui32TotLength, pszTmp, ui32BytesRead) < 0) {
        return -3;
    }
    if (!bSkip) {
       _pszMissionID = pszTmp;
    }
    ui32TotLength += ui32BytesRead;

    ui32BytesRead = 0;
    if (!bSkip && (_pszRole != NULL)) {
        free (_pszRole);
        _pszRole = NULL;
    }
    if (DSLib::readString (pReader, ui32MaxSize - ui32TotLength, pszTmp, ui32BytesRead) < 0) {
        return -4;
    }
    if (!bSkip) {
       _pszRole = pszTmp;
    }
    ui32TotLength += ui32BytesRead;

    bool bLimitPrestagingToLocalData = false;
    if (pReader->readBool (&bLimitPrestagingToLocalData) < 0) {
        return -5;
    }
    if (!bSkip) {
        _bLimitPrestagingToLocalData = bLimitPrestagingToLocalData;
    }
    ui32TotLength += 1;

    float fMatchmakingThreshold;
    if (pReader->readFloat (&fMatchmakingThreshold) < 0) {
        return -5;
    }
    if (!bSkip) {
        _fMatchmakingThreshold = fMatchmakingThreshold;
    }
    ui32TotLength += 4;

    uint16 ui16NUsersIds = 0;
    if (pReader->read16 (&ui16NUsersIds) < 0) {
        return -6;
    }
    if (!bSkip) {
        _usersIds.removeAll();
    }
    ui32TotLength += 2;

    pszTmp = NULL;
    for (uint16 i = 0; i < ui16NUsersIds; i++) {
        ui32BytesRead = 0;
        if (DSLib::readString (pReader, ui32MaxSize - ui32TotLength, pszTmp, ui32BytesRead) < 0) {
            return -7;
        }
        if (!bSkip && (pszTmp != NULL)) {
            String userId (pszTmp);
            if (userId.length() > 0) {
                _usersIds.add (userId);
            }
        }
        if (pszTmp != NULL) {
            free (pszTmp);
            pszTmp = NULL;
        }
        ui32TotLength += ui32BytesRead;
    }

    return ui32TotLength;
}

int64 NodeContext::writeLocalInformation (Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return -1;
    }
    uint32 ui32TotLength = 0;
    uint32 ui32BytesWritten = 0;

    if (DSLib::writeString (_pszTeamID, pWriter, ui32MaxSize, ui32BytesWritten) < 0) {
        return -2;  
    }
    ui32TotLength += ui32BytesWritten;

    ui32BytesWritten = 0;
    if (DSLib::writeString (_pszMissionID, pWriter, ui32MaxSize - ui32TotLength, ui32BytesWritten) < 0) {
        return -3;
    }
    ui32TotLength += ui32BytesWritten;

    ui32BytesWritten = 0;
    if (DSLib::writeString (_pszRole, pWriter, ui32MaxSize - ui32TotLength, ui32BytesWritten) < 0) {
        return -4;
    }
    ui32TotLength += ui32BytesWritten;

    if (pWriter->writeBool (&_bLimitPrestagingToLocalData) < 0) {
        return -5;
    }
    ui32TotLength += 1;

    if (pWriter->writeFloat (&_fMatchmakingThreshold) < 0) {
        return -6;
    }
    ui32TotLength += 4;

    const int iNUsersIds = _usersIds.getCount();
    assert (iNUsersIds < 0xFFFF);
    uint16 ui16 = (iNUsersIds <= 0 ? 0U : (uint16) iNUsersIds);
    if (pWriter->write16 (&ui16) < 0) {
        return -7;
    }
    ui32TotLength += 2;
    if (ui16 > 0) {
         String userId;
         for (int rc = _usersIds.getFirst (userId); rc == 1; rc = _usersIds.getNext (userId)) {
             ui32BytesWritten = 0;
             if (DSLib::writeString (userId, pWriter, ui32MaxSize - ui32TotLength, ui32BytesWritten) < 0) {
                 return -8;
             }
             else {
                 ui32TotLength += ui32BytesWritten;
             }
         }
    }

    return ui32TotLength;
}

int NodeContext::getWriteLocalInformationLength()
{
    uint32 totLength = 0;
    if (_ui16CurrInfoVersion > 0) {
        totLength += 2;
        if (_pszTeamID != NULL) {
            totLength += strlen (_pszTeamID);
        }

        totLength += 2;
        if (_pszMissionID != NULL) {
            totLength += strlen (_pszMissionID);
        }

        totLength += 2;
        if (_pszRole != NULL) {
            totLength += strlen (_pszRole);
        }

        totLength += 4; // _fMatchmakingThreshold
        totLength += 2; // IDs length
        String userId;
        for (int rc = _usersIds.getFirst (userId); rc == 1; rc = _usersIds.getNext (userId)) {
            totLength += (2 + userId.length());
        }
    }
    return totLength;
}

// PATH INFORMATION

int64 NodeContext::readPathInformation (Reader *pReader, uint32 ui32MaxSize, bool bSkip)
{
    int8 rc;
    uint32 ui32TotLength = 0;
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -2;
    }

    uint16 ui16CurrWayPointVersion;
    rc = pReader->read16 (&ui16CurrWayPointVersion);
    if (rc < 0) {
        return rc;
    }
    else if (!bSkip) {
        _ui16CurrWaypointVersion = ui16CurrWayPointVersion;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + sizeof(int))) {
        return -2;
    }

    int iCurrWayPointInPath;
    rc = pReader->read32 (&iCurrWayPointInPath);
    if (rc < 0) {
        return rc;
    }
    else if (!bSkip) {
        _iCurrWayPointInPath = iCurrWayPointInPath;
        _status = ON_WAY_POINT; // Force to be on waypoint
    }
    ui32TotLength += 4;
    if(ui32MaxSize < (ui32TotLength + 2)) {
        return -2;
    }

    // Read default useful distance
    uint32 ui32DefaultUsefulDistance;
    rc = pReader->read32 (&ui32DefaultUsefulDistance);
    if (rc < 0) {
        return rc;
    }
    else if (!bSkip) {
        _ui32DefaultUsefulDistance = ui32DefaultUsefulDistance;
    }
    ui32TotLength += 4;

    // Read custom useful distance values
    uint32 ui32UsefulDistances = 0;
    if ((rc = pReader->read32 (&ui32UsefulDistances)) < 0) {
        return rc;
    }
    ui32TotLength += 4;
    for (uint32 i = 0; i < ui32UsefulDistances; i++) {
        uint16 ui16 = 0;
        if ((rc = pReader->read16 (&ui16)) < 0) {
            return rc;
        }
        if (ui16 == 0) {
            continue;
        }

        char *pMIMEType = (char *) calloc (ui16+1, sizeof (char));
        if (pMIMEType == NULL) {
            checkAndLogMsg ("NodeContext::readPathInformation", memoryExhausted);
        }
        if ((rc = pReader->readBytes (pMIMEType, ui16)) < 0) {
            free (pMIMEType);
            return rc;
        }

        uint32 *pUsefulDistance = (uint32*) calloc (1, sizeof (uint32));
        if (pUsefulDistance != NULL) {
            if ((rc = pReader->read32 (pUsefulDistance)) < 0) {
                free (pMIMEType);
                free (pUsefulDistance);
                return rc;
            }
        }

        if (!bSkip && pUsefulDistance != NULL) {
            free (_usefulDistanceByMimeType.put (pMIMEType, pUsefulDistance));
        }
        free (pMIMEType);    // _usefulDistanceByMimeType makes a copy
        ui32TotLength += (2 + ui16 + 4);
    }

    return ui32TotLength;
}

int64 NodeContext::writePathInformation (Writer *pWriter, uint32 ui32MaxSize)
{
    int8 rc;
    uint32 ui32TotLength = 0;
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -2;
    }
    if ((rc = pWriter->write16 (&_ui16CurrWaypointVersion)) < 0) {
        return rc;
    }
    ui32TotLength += 2;
    if (ui32MaxSize < (ui32TotLength + sizeof(int))) {
        return -2;
    }
    if ((rc = pWriter->write32 (&_iCurrWayPointInPath)) < 0) {
        return rc;
    }
    ui32TotLength += 4;

    // Write default useful distance
    if (ui32MaxSize < (ui32TotLength + 2)) {
        return -2;
    }
    if ((rc = pWriter->write32 (&_ui32DefaultUsefulDistance)) < 0) {
        return rc;
    }
    ui32TotLength += 4;

    // Write custom useful distance values - if any
    uint32 ui32UsefulDistances = _usefulDistanceByMimeType.getCount();
    if ((rc = pWriter->write32 (&ui32UsefulDistances)) < 0) {
        return rc;
    }
    ui32TotLength += 4;
    if (ui32UsefulDistances == 0) {
        return ui32TotLength;
    }

    for (StringHashtable<uint32>::Iterator i = _usefulDistanceByMimeType.getAllElements();
         !i.end(); i.nextElement()) {
        const char *pMIMEType = i.getKey();
        uint32 *pUsefulDistance = i.getValue();
        uint16 ui16 = strlen (pMIMEType);

        if ((rc = pWriter->write16 (&ui16)) < 0 ||
            (rc = pWriter->writeBytes (pMIMEType, ui16)) < 0 ||
            (rc = pWriter->write32 (pUsefulDistance)) < 0) {
            return rc;
        }
        ui32TotLength += (2 + ui16 + 4);
    }

    return ui32TotLength;
}

int NodeContext::getWritePathInformationLength()
{
    uint32 ui32TotLength = 10;
    
    ui32TotLength += 4;         // number of "custom" useful distances

    if (_usefulDistanceByMimeType.getCount() == 0) {
        return ui32TotLength;
    }

    for (StringHashtable<uint32>::Iterator i = _usefulDistanceByMimeType.getAllElements();
         !i.end(); i.nextElement()) {
        ui32TotLength += 2;     // MIME type length
        uint16 ui16 = strlen (i.getKey());
        ui32TotLength += ui16;  // MIME type
        ui32TotLength += 4;     // value of useful distance for the particular MIME type
    }

    return ui32TotLength;
}

bool NodeContext::setCurrentPosition (float latitude, float longitude, float altitude)
{
    float closestPointLat, closestPointLong;
    int currentSegmentStartIndex;

    NodeStatus status = calculateClosestCurrentPointInPath (latitude, longitude, altitude,
                                                            closestPointLat, closestPointLong,
                                                            currentSegmentStartIndex);
    bool bRet = (_status != status);
    if (!bRet && !isApproximableToPreviousPosition (latitude, longitude)) {
        // The node has moved far enough from the previous point
        // If the rate with which the actual path is updated is high enough, and
        // the node does not have time to move much,
        // isApproximableToPreviousPosition() will always return true, therefore
        // this option will never be called, therefore, since the return code of
        // this method is used to decide whether a position update should be sent,
        // the position update may never be sent. This case would happen only for
        // nodes that are moving far away from the path, so it is an unlikely
        // condition, however it has to be kept in mind.
        // Even in the case of this unlikely condition, the current position will
        // be periodically sent anyway by the NodeContextManager, so the only
        // effect would be some delay in the notification of the updated position.
        bRet = true;
    }

    switch (status) {
        case PATH_UNSET:
            // There is not anything to do 
            break;

        case ON_WAY_POINT:
        case PATH_DETOURED:
        case PATH_IN_TRANSIT:
            if ((currentSegmentStartIndex >= 0) && (currentSegmentStartIndex != _iCurrWayPointInPath)) {
                _iCurrWayPointInPath = currentSegmentStartIndex;
                _ui16CurrWaypointVersion++;
                // Way point has changed, return true
                bRet = true;
                checkAndLogMsg ("PeerNodeContext::setCurrentPosition", Logger::L_LowDetailDebug,
                                "updated current waypoint to %d\n", _iCurrWayPointInPath);
            }
            // NOTE: there is no missing "break", it is supposed to execute the
            // code in the TOO_FAR_FROM_PATH case as well!

        case TOO_FAR_FROM_PATH:
            _closestPointOnPathLat = closestPointLat;
            _closestPointOnPathLong = closestPointLong;
            checkAndLogMsg ("PeerNodeContext::setCurrentPosition", Logger::L_LowDetailDebug,
                            "current position updated to %f,%f\n",
                            _closestPointOnPathLat, _closestPointOnPathLong);
            break;

        default:
            checkAndLogMsg ("PeerNodeContext::setCurrentPosition", Logger::L_Warning,
                            "unknown path state\n");
    }

    _status = status;
    return bRet;
}

NodeContext::NodeStatus NodeContext::calculateClosestCurrentPointInPath (float latitude, float longitude, float altitude,
                                                                         float &closestPointLat, float &closestPointLong,
                                                                         int &segmentStartIndex)
{
    NodePath *pCurrPath = getPath();
    if (pCurrPath == NULL) {
        checkAndLogMsg ("NodeContext::calculateCurrentWayPointInPath", Logger::L_Warning,
                        "pCurrPath is NULL, probably because the current path is not set. "
                        "It is not possible to calculate the current position\n");
        return PATH_UNSET;
    }

    int iPathLength = pCurrPath->getPathLength();
    if (iPathLength < 1) {
        segmentStartIndex = -1;
        return PATH_UNSET;
    }
    if (iPathLength == 1) {
        closestPointLat = pCurrPath->getLatitude (0);
        closestPointLong = pCurrPath->getLongitude (0);
        if (isApproximableToPoint (closestPointLat, closestPointLong, latitude, longitude)) {
            segmentStartIndex = 0;
            return ON_WAY_POINT;
        }
        if (isTooFarFromPath (closestPointLat, closestPointLong, latitude, longitude)) {
            segmentStartIndex = -1;
            return TOO_FAR_FROM_PATH;
        }
        else {
            segmentStartIndex = -1;
            return PATH_DETOURED;
        }
    }

    // iPathLength > 1

    // TODO : now "altitude" is not considered
    float minDistFromSegment, fDist;
    minDistFromSegment = -1.0f;
    float segLatA, segLongA, segLatB, segLongB, projectionLat, projectionLong;
    for (int i = 0; i < (pCurrPath->getPathLength() - 1); i++) {
        segLatA = pCurrPath->getLatitude (i);
        segLongA = pCurrPath->getLongitude (i);
        segLatB = pCurrPath->getLatitude (i+1);
        segLongB = pCurrPath->getLongitude (i+1);

        fDist = distancePointSegment (segLatA, segLongA, segLatB, segLongB,
                                      latitude, longitude,
                                      projectionLat, projectionLong);
        if (fDist >= 0.0f) {
            if ((minDistFromSegment < 0.0f) || (fDist < minDistFromSegment)) {
                minDistFromSegment = fDist;
                closestPointLat = projectionLat;
                closestPointLong = projectionLong;
                segmentStartIndex = i;
            }
        }
    }

    // Ends of the segment which is the closest to the current actual position
    segLatA = pCurrPath->getLatitude (segmentStartIndex);
    segLongA = pCurrPath->getLongitude (segmentStartIndex);
    segLatB = pCurrPath->getLatitude (segmentStartIndex+1);
    segLongB = pCurrPath->getLongitude (segmentStartIndex+1);

    // See if (latitude, longitude) is approximable to
    // (closestPointLat, closestPointLong)
    if (isApproximableToPoint (minDistFromSegment)) {
        // The node is relatively close to the path
        float fDistA = greatCircleDistance (latitude, longitude, segLatA, segLongA);
        float fDistB = greatCircleDistance (latitude, longitude, segLatB, segLongB);
        if (fDistA < fDistB) {       
            if (isApproximableToPoint (fDistA)) {
                // the node is relatively close to A, I can approximate and use
                // A as current waypoint
                return ON_WAY_POINT;
            }
        }
        else {
            if (isApproximableToPoint (fDistB)) {
                // the node is relatively close to B, I can approximate and use
                // B as current waypoint
                segmentStartIndex += 1;
                return ON_WAY_POINT;
            }
        }
        // The node is relatively close to the path but far from the waypoints,
        // it is probably in transit from waypoint "i-1" to waypoint "i"
        return PATH_IN_TRANSIT;
    }
    if (isTooFarFromPath (minDistFromSegment)) {
        // too far!
        segmentStartIndex = -1;
        return TOO_FAR_FROM_PATH;
    }
    // the node is far from the path, it detoured!!!
    return PATH_DETOURED;
}

bool NodeContext::isApproximableToPoint (float latitude1, float longitude1, float latitude2, float longitude2)
{
    return isApproximableToPoint (greatCircleDistance (latitude1, longitude1, latitude2, longitude2));
}

bool NodeContext::isApproximableToPreviousPosition (float latitude, float longitude)
{
    float fOldActualLat, fOldActualLong;
    if (0 == getCurrentLatitude (fOldActualLat) &&
        0 == getCurrentLongitude (fOldActualLong)) {
        // The actual current position was set
        if (isApproximableToPoint (fOldActualLat, fOldActualLong, latitude, longitude)) {
            return true;
        }
    }
    else {
        checkAndLogMsg ("NodeContext::isApproximableToPoint", Logger::L_MediumDetailDebug,
                        "Current actual latitude or longitude is not set.\n");
    }
    return false;
}

bool NodeContext::isApproximableToPoint (double dDistance)
{
    // TODO: Replace 10 with _dApproxCoeff
    return (dDistance < (_ui32DefaultUsefulDistance/10));
}

bool NodeContext::isTooFarFromPath (float latitude1, float longitude1, float latitude2, float longitude2)
{
    return isTooFarFromPath (greatCircleDistance (latitude1, longitude1, latitude2, longitude2));
}

bool NodeContext::isTooFarFromPath (double dDistance)
{
    // TODO: Replace 10 with _dTooFarCoeff
    return (dDistance > (_ui32DefaultUsefulDistance*10));
}

