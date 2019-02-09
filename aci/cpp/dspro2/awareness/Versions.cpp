/**
 * Versions.h
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
 * Created on December 22, 2016, 12:45 PM
 */

#include "Versions.h"

#include "Defs.h"

#include "Logger.h"
#include "Writer.h"

#include "Json.h"
#include <stdlib.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const char * Versions::VERSIONS_OBJECT_NAME = "versions";
const char * Versions::STARTING_TIME = "startingTime";
const char * Versions::INFO_VERSION = "nodeInfo";
const char * Versions::PATH_VERSION = "path";
const char * Versions::WAYPOINT_VERSION = "waypoint";
const char * Versions::CLASSIFIER_VERSION = "classifier";
const char * Versions::MATCHMAKING_VERSION = "matchmaking";

Versions::Versions (void)
    : _i64StartingTime (0), _ui16InfoVersion (0), _ui16PathVersion (0),
      _ui16WaypointVersion (0), _ui16ClassifierVersion (0), _ui16MatchmakerQualifierVersion (0),
      _ui16AreasOfInterestVersion (0)
{
}

Versions::Versions (int64 i64StartingTime, uint16 ui16InfoVersion, uint16 ui16PathVersion,
                    uint16 ui16WaypointVersion, uint16 ui16ClassifierVersion, uint16 ui16MatchmakerQualifierVersion,
                    uint16 ui16AreasOfInterestVersion)
    : _i64StartingTime (i64StartingTime), _ui16InfoVersion (ui16InfoVersion),
      _ui16PathVersion (ui16PathVersion), _ui16WaypointVersion (ui16WaypointVersion),
      _ui16ClassifierVersion (ui16ClassifierVersion), _ui16MatchmakerQualifierVersion (ui16MatchmakerQualifierVersion),
      _ui16AreasOfInterestVersion (ui16AreasOfInterestVersion)
{
}

Versions::Versions (const Versions &version)
    : _i64StartingTime (version._i64StartingTime), _ui16InfoVersion (version._ui16InfoVersion),
      _ui16PathVersion (version._ui16PathVersion), _ui16WaypointVersion (version._ui16WaypointVersion),
      _ui16ClassifierVersion (version._ui16ClassifierVersion), _ui16MatchmakerQualifierVersion (version._ui16MatchmakerQualifierVersion),
      _ui16AreasOfInterestVersion (version._ui16AreasOfInterestVersion)
{
}

Versions::~Versions (void)
{
}

void Versions::reset (void)
{
    _i64StartingTime = (int64)0;
    _ui16InfoVersion = (uint16)0;
    _ui16PathVersion = (uint16)0;
    _ui16WaypointVersion = (uint16)0;
    _ui16ClassifierVersion = (uint16)0;
    _ui16MatchmakerQualifierVersion = (uint16)0;
    _ui16AreasOfInterestVersion = (uint16)0;
}

void Versions::set (int64 i64StartingTime, uint16 ui16InfoVersion, uint16 ui16PathVersion,
                    uint16 ui16WaypointVersion, uint16 ui16ClassifierVersion, uint16 ui16MatchmakerQualifierVersion,
                    uint16 ui16AreasOfInterestVersion)
{
    _i64StartingTime = i64StartingTime;
    _ui16InfoVersion = ui16InfoVersion;
    _ui16PathVersion = ui16PathVersion;
    _ui16WaypointVersion = ui16WaypointVersion;
    _ui16ClassifierVersion = ui16ClassifierVersion;
    _ui16MatchmakerQualifierVersion = ui16MatchmakerQualifierVersion;
    _ui16AreasOfInterestVersion = ui16AreasOfInterestVersion;
}

bool Versions::greaterThan (const Versions &versions, bool bExcludeWayPointVersion) const
{
    if ((_ui16InfoVersion > versions._ui16InfoVersion) ||
        (_ui16PathVersion > versions._ui16PathVersion) ||
        (_ui16ClassifierVersion > versions._ui16ClassifierVersion) ||
        (_ui16MatchmakerQualifierVersion > versions._ui16MatchmakerQualifierVersion) ||
        (_ui16AreasOfInterestVersion > versions._ui16AreasOfInterestVersion)) {
        return true;
    }
    if (!bExcludeWayPointVersion) {
        return (_ui16WaypointVersion > versions._ui16WaypointVersion);
    }
    return false;
}

bool Versions::lessThan (const Versions &versions, bool bExcludeWayPointVersion) const
{
    if ((_ui16InfoVersion < versions._ui16InfoVersion) ||
        (_ui16PathVersion < versions._ui16PathVersion) ||
        (_ui16ClassifierVersion < versions._ui16ClassifierVersion) ||
        (_ui16MatchmakerQualifierVersion < versions._ui16MatchmakerQualifierVersion) ||
        (_ui16AreasOfInterestVersion < versions._ui16AreasOfInterestVersion)) {
        return true;
    }
    if (!bExcludeWayPointVersion) {
        return (_ui16WaypointVersion < versions._ui16WaypointVersion);
    }
    return false;
}

bool Versions::pathGreater (const Versions &version) const
{
    if (_i64StartingTime > version._i64StartingTime) {
        return true;
    }
    return (_ui16PathVersion > version._ui16PathVersion);
}

namespace VERSIONS_JSON_NAMES
{
    const char * INFO = "nodeInfo";
    const char * PATH = "path";
    const char * WAYPOINT = "waypoint";
    const char * CLASSIFIER = "classifier";
    const char * MATCHMAKER = "matchmaker";
    const char * AREAS_OF_INTEREST = "areasOfInterest";
    const char * STARTING_TIME = "startingTime";
}

int Versions::fromJson (const JsonObject *pJson)
{
    const char *pszMethodName = "Versions::fromJson";
    if (pJson == nullptr) {
        return -1;
    }
    int64 i64StartingTime = _i64StartingTime;
    uint16 ui16InfoVersion = _ui16InfoVersion;
    uint16 ui16PathVersion = _ui16PathVersion;
    uint16 ui16WaypointVersion = _ui16WaypointVersion;
    uint16 ui16ClassifierVersion = _ui16ClassifierVersion;
    uint16 ui16MatchmakerQualifierVersion = _ui16MatchmakerQualifierVersion;
    uint16 ui16AreasOfInterestVersion = _ui16AreasOfInterestVersion;
    int rc = pJson->getNumber (VERSIONS_JSON_NAMES::INFO, ui16InfoVersion);
    rc += pJson->getNumber (VERSIONS_JSON_NAMES::PATH, ui16PathVersion);
    rc += pJson->getNumber (VERSIONS_JSON_NAMES::WAYPOINT, ui16WaypointVersion);
    rc += pJson->getNumber (VERSIONS_JSON_NAMES::CLASSIFIER, ui16ClassifierVersion);
    rc += pJson->getNumber (VERSIONS_JSON_NAMES::MATCHMAKER, ui16MatchmakerQualifierVersion);
    rc += pJson->getNumber (VERSIONS_JSON_NAMES::AREAS_OF_INTEREST, ui16AreasOfInterestVersion);
    rc += pJson->getNumber (VERSIONS_JSON_NAMES::STARTING_TIME, i64StartingTime);
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "version message protocol version mismatch.\n");
    }
    set (i64StartingTime, ui16InfoVersion, ui16PathVersion, ui16WaypointVersion,
         ui16ClassifierVersion, ui16MatchmakerQualifierVersion, ui16AreasOfInterestVersion);
    return 0;
}

JsonObject * Versions::toJson (void) const
{
    JsonObject *pJson = new JsonObject();
    if (pJson == nullptr) {
        return nullptr;
    }
    int rc = pJson->setNumber (VERSIONS_JSON_NAMES::INFO, _ui16InfoVersion);
    rc += pJson->setNumber (VERSIONS_JSON_NAMES::PATH, _ui16PathVersion);
    rc += pJson->setNumber (VERSIONS_JSON_NAMES::WAYPOINT, _ui16WaypointVersion);
    rc += pJson->setNumber (VERSIONS_JSON_NAMES::CLASSIFIER, _ui16ClassifierVersion);
    rc += pJson->setNumber (VERSIONS_JSON_NAMES::MATCHMAKER, _ui16MatchmakerQualifierVersion);
    rc += pJson->setNumber (VERSIONS_JSON_NAMES::AREAS_OF_INTEREST, _ui16AreasOfInterestVersion);
    rc += pJson->setNumber (VERSIONS_JSON_NAMES::STARTING_TIME, _i64StartingTime);
    if (rc != 0) {
        delete pJson;
        return nullptr;
    }
    return pJson;
}

int Versions::read (Reader *pReader)
{
    if (pReader == nullptr) {
        return -1;
    }
    char *pszVersions = nullptr;
    if (pReader->readString (&pszVersions) < 0) {
        if (pReader != nullptr) {
            free (pszVersions);
        }
    }
    if (pszVersions == nullptr) {
        return -2;
    }
    JsonObject json (pszVersions);
    int rc = fromJson (&json);
    free (pszVersions);
    return rc < 0 ? -3 : 0;
}

int Versions::write (Writer *pWriter)
{
    if (pWriter == nullptr) {
        return -1;
    }
    const JsonObject *pVersions = toJson();
    String json (pVersions->toString());
    if (pWriter->writeString (json) < 0) {
        return -2;
    }
    return 0;
}

