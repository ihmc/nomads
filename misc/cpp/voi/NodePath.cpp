/*
 * NodePath.cpp
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

#include "NodePath.h"

#include "VoiDefs.h"

#include "ConfigManager.h"
#include "GeoUtils.h"
#include "Json.h"
#include "LineOrientedReader.h"
#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"
#include "StringTokenizer.h"
#include "Writer.h"

#include <string.h>
#include <time.h>

#if defined (UNIX)
    #define stricmp strcasecmp
#endif

using namespace NOMADSUtil;
using namespace IHMC_VOI;

const char * NodePath::PAST_PATH = "Node Past Path";
const int NodePath::MAIN_PATH_TO_OBJECTIVE = 1;
const int NodePath::ALTERNATIVE_PATH_TO_OBJECTIVE = 2;
const int NodePath::MAIN_PATH_TO_BASE = 3;
const int NodePath::ALTERNATIVE_PATH_TO_BASE = 4;
const int NodePath::FIXED_LOCATION = 5;

const float NodePath::LATITUDE_UNSET = MAX_LATITUDE + 1;
const float NodePath::LONGITUDE_UNSET = MAX_LONGITUDE + 1;

namespace NODE_PATH
{
    static const char * PATH_ID = "pathId";
    static const char * PATH_TYPE = "pathType";
    static const char * PATH_PROBABILITY = "pathProbability";
    static const char * WAYPOINTS = "waypoints";
}

namespace WAYPOINT
{
    static const char * LATITUDE = "lat";
    static const char * LONGITUDE = "lon";
    static const char * ALTITUDE = "alt";
    static const char * TIMESTAMP = "timestmp";
    static const char * LOCATION = "loc";
    static const char * NOTE = "note";

    JsonObject * getWaypoint (JsonObject *pPath, int wayPointIndex)
    {
        if (pPath == NULL) {
            return NULL;
        }
        const JsonArray *pWaypoints = pPath->getArray (NODE_PATH::WAYPOINTS);
        if (pWaypoints == NULL) {
            return NULL;
        }
        const int iLen = pWaypoints->getSize();
        if ((wayPointIndex < 0) || (wayPointIndex >= iLen)) {
            return NULL;
        }
        JsonObject *pWaypoint = pWaypoints->getObject (wayPointIndex);
        delete pWaypoints;
        return pWaypoint;
    }

    float getWaypointFloatValue (JsonObject *pPath, int wayPointIndex, const char *pszName)
    {
        JsonObject *pWaypoint = getWaypoint (pPath, wayPointIndex);
        if (pWaypoint == NULL) {
            return 0.0f;
        }
        double dCoordinate = 0.0;
        if (pWaypoint->getNumber (pszName, dCoordinate) < 0) {
            delete pWaypoint;
            return 0.0f;
        }
        delete pWaypoint;
        return (float) dCoordinate;
    }

    String getWaypointStringValue (JsonObject *pPath, int wayPointIndex, const char *pszName)
    {
        JsonObject *pWaypoint = getWaypoint (pPath, wayPointIndex);
        if (pWaypoint == NULL) {
            return String();
        }
        String s;
        if (pWaypoint->getString (pszName, s) < 0) {
            delete pWaypoint;
            return String();
        }
        delete pWaypoint;
        return s;
    }
}

NodePath::NodePath (void)
    : _pPath (NULL),
      _maxLat (LATITUDE_UNSET),
      _minLat (LATITUDE_UNSET),
      _maxLong (LONGITUDE_UNSET),
      _minLong (LONGITUDE_UNSET)
{
}

NodePath::NodePath (const char *pathId, int pathType, float fPathProbability)
    : _pPath (new JsonObject()),
      _maxLat (LATITUDE_UNSET),
      _minLat (LATITUDE_UNSET),
      _maxLong (LONGITUDE_UNSET),
      _minLong (LONGITUDE_UNSET)
{
    _pPath->setString (NODE_PATH::PATH_ID, pathId);
    _pPath->setNumber (NODE_PATH::PATH_TYPE, pathType);
    if ((fPathProbability <= 1) && (fPathProbability >= 0)) {
        _pPath->setNumber (NODE_PATH::PATH_PROBABILITY, fPathProbability);
    }
    else {
        _pPath->setNumber (NODE_PATH::PATH_PROBABILITY, 0.0f);
    }
}

NodePath::~NodePath (void)
{
    if (_pPath != NULL) {
        delete _pPath;
        _pPath = NULL;
    }
}

int NodePath::appendWayPoint (int8 azimuthDeg, uint8 azimuthMin, uint8 azimuthSec, uint16 distanceInMeters,
                              float altitude, const char *pszLocation, const char *pszNote,
                              int hour, int min, int sec, int day, int mon, int year)
{
    if (_pPath == NULL) {
        return -1;
    }
    const JsonArray *pWaypoints = _pPath->getArray (NODE_PATH::WAYPOINTS);
    if (pWaypoints == NULL) {
        return -2;
    }
    const int iLen = pWaypoints->getSize();
    if (iLen <= 0) {
        return -3;
    }
    JsonObject *pPrevWaypoint = pWaypoints->getObject (iLen - 1);
    if (pPrevWaypoint == NULL) {
        return -4;
    }

    double prevLat, prevLong;
    pPrevWaypoint->getNumber (WAYPOINT::LATITUDE, prevLat);
    pPrevWaypoint->getNumber (WAYPOINT::LONGITUDE, prevLong);
    delete pPrevWaypoint;
    float azimuth = abs(azimuthDeg) + azimuthMin/60.0f + azimuthSec/3600.0f;
    if (azimuthDeg < 0.0f) {
        azimuth = - azimuth;
    }
    float latitude = prevLat + distanceInMeters * cos (azimuth);
    float longitude = prevLong + distanceInMeters * sin (azimuth);
    return appendWayPoint (latitude, longitude, altitude, pszLocation, pszNote, hour, min, sec, day, mon, year);
}

int NodePath::appendWayPoint (int8 latitudeDeg, uint8 latitudeMin, uint8 latitudeSec, int8 longitudeDeg, uint8 longitudeMin,
                              uint8 longitudeSec, float altitude, const char *pszLocation, const char *pszNote,
                              int hour, int min, int sec, int day, int mon, int year)
{
    float latitude = abs(latitudeDeg) + latitudeMin/60.0f + latitudeSec/3600.0f;
    if (latitudeDeg < 0.0f) {
        latitude = - latitude;
    }
    float longitude = abs(longitudeDeg) + longitudeMin/60.0f + longitudeSec/3600.0f;
    if (longitudeDeg < 0.0f) {
        longitude = - longitude;
    }
    return appendWayPoint (latitude, longitude, altitude, pszLocation, pszNote, hour, min, sec, day, mon, year);
}

int NodePath::appendWayPoint (float latitude, float longitude, float altitude,
                              const char *pszLocation, const char *pszNote,
                              int hour, int min, int sec, int day, int mon, int year)
{
    if ((hour < 0) || (min < 0) || (sec < 0) || (day < 0) || (mon < 0) || (year < 0)) {
        return appendWayPoint(latitude, longitude, altitude, pszLocation, pszNote, 0);
    }

    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);

    timeinfo->tm_sec = sec;
    timeinfo->tm_min = min;
    timeinfo->tm_hour = hour;
    timeinfo->tm_mday = day;
    timeinfo->tm_mon = mon - 1;
    timeinfo->tm_year = year - 1900;

    int64 i64Timestamp = mktime (timeinfo);
    if (i64Timestamp == -1) {
        i64Timestamp = 0;
    }
    free (timeinfo);
    return appendWayPoint (latitude, longitude, altitude, pszLocation,
                           pszNote, (uint64) i64Timestamp);
}

int NodePath::appendWayPoint (int8 latitudeDeg, uint8 latitudeMin, uint8 latitudeSec, int8 longitudeDeg, uint8 longitudeMin,
                              uint8 longitudeSec, float altitude, const char *pszLocation, const char *pszNote, uint64 timeStamp)
{
    float latitude, longitude;
    latitude = abs (latitudeDeg) + latitudeMin/60.0f + latitudeSec/3600.0f;
    if (latitudeDeg < 0.0f) {
        latitude = 0 - latitude;
    }
    longitude = abs (longitudeDeg) + longitudeMin/60.0f + longitudeSec/3600.0f;
    if (longitudeDeg < 0.0f) {
        longitude = 0.0f - longitude;
    }
    return appendWayPoint (latitude, longitude, altitude, pszLocation, pszNote, timeStamp);
}

int NodePath::appendWayPoint (float latitude, float longitude, float altitude, const char *pszLocation, const char *pszNote, uint64 timeStamp)
{
    if (_pPath == NULL) {
        return -1;
    }
    JsonArray *pWaypoints = _pPath->getArrayReference (NODE_PATH::WAYPOINTS);
    if (pWaypoints == NULL) {
        pWaypoints = new JsonArray();
        _pPath->setObject (NODE_PATH::WAYPOINTS, pWaypoints);
        delete pWaypoints;
        pWaypoints = _pPath->getArrayReference (NODE_PATH::WAYPOINTS);
    }
    if (pWaypoints == NULL) {
        // memory exhausted
        return -2;
    }
    int iType;
    _pPath->getNumber (NODE_PATH::PATH_TYPE, iType);
    if ((pWaypoints->getSize() > 0) && (iType == NodePath::FIXED_LOCATION)) {
        delete pWaypoints;
        return -3;
    }
    JsonObject waypoint;
    waypoint.setNumber (WAYPOINT::LATITUDE, latitude);
    waypoint.setNumber (WAYPOINT::LONGITUDE, longitude);
    waypoint.setNumber (WAYPOINT::ALTITUDE, altitude);
    waypoint.setNumber (WAYPOINT::TIMESTAMP, (double) timeStamp);
    if (pszLocation != NULL) {
        waypoint.setString (WAYPOINT::LOCATION, pszLocation);
    }
    if (pszNote != NULL) {
        waypoint.setString (WAYPOINT::NOTE, pszNote);
    }
    int rc = pWaypoints->setObject (&waypoint);
    if (rc == 0) {
        updatePathBoundingBox (latitude, longitude);
    }
    return rc;
}

int NodePath::setProbability (float fProbability)
{
    if (_pPath == NULL) {
        return -1;
    }
    if ((fProbability > 1.0f) || (fProbability < 0.0f)) {
        return -2;
    }
    return _pPath->setNumber (NODE_PATH::PATH_PROBABILITY, fProbability);
}

int NodePath::getBoundingBox (float &maxLat, float &minLat, float &maxLong, float &minLong)
{
    if ((_maxLat < MIN_LATITUDE || _maxLat > MAX_LATITUDE) ||
        (_minLat < MIN_LATITUDE || _minLat > MAX_LATITUDE) ||
        (_maxLong < MIN_LONGITUDE || _maxLong > MAX_LONGITUDE) ||
        (_maxLong < MIN_LONGITUDE || _maxLong > MAX_LONGITUDE)) {
        return -1;
    }
    maxLat = _maxLat;
    minLat = _minLat;
    maxLong = _maxLong;
    minLong = _minLong;
    return 0;
}

int NodePath::getBoundingBox (float &maxLat, float &minLat, float &maxLong, float &minLong, float fPadding)
{
    float tmpMaxLat, tmpMinLat, tmpMaxLong, tmpMinLong;
    if (getBoundingBox (tmpMaxLat, tmpMinLat, tmpMaxLong, tmpMinLong) != 0) {
        return -1;
    }
    addPaddingToBoudingBox (tmpMaxLat, tmpMinLat, tmpMaxLong, tmpMinLong,
                            fPadding, maxLat, minLat, maxLong, minLong);

    return 0;
}

BoundingBox NodePath::getBoundingBox (float fPadding)
{
    float tmpMaxLat, tmpMinLat, tmpMaxLong, tmpMinLong;
    if (getBoundingBox (tmpMaxLat, tmpMinLat, tmpMaxLong, tmpMinLong, fPadding) < 0) {
        return BoundingBox();
    }
    return BoundingBox (tmpMaxLat, tmpMinLong, tmpMinLat, tmpMaxLong);
}

float NodePath::getLatitude (int wayPointIndex)
{
    return WAYPOINT::getWaypointFloatValue (_pPath, wayPointIndex, WAYPOINT::LATITUDE);
}

float NodePath::getLongitude (int wayPointIndex)
{
    return WAYPOINT::getWaypointFloatValue (_pPath, wayPointIndex, WAYPOINT::LONGITUDE);
}

float NodePath::getAltitude (int wayPointIndex)
{
    return WAYPOINT::getWaypointFloatValue (_pPath, wayPointIndex, WAYPOINT::ALTITUDE);
}

String NodePath::getLocation (int wayPointIndex)
{
    return WAYPOINT::getWaypointStringValue (_pPath, wayPointIndex, WAYPOINT::LOCATION);
}

String NodePath::getNote (int wayPointIndex)
{
    return WAYPOINT::getWaypointStringValue (_pPath, wayPointIndex, WAYPOINT::NOTE);
}

int NodePath::getPathLength (void)
{
    if (_pPath == NULL) {
        return -1;
    }
    const JsonArray *pWaypoints = _pPath->getArray (NODE_PATH::WAYPOINTS);
    if (pWaypoints == NULL) {
        return 0;
    }
    return pWaypoints->getSize();
}

int NodePath::getPathType (void)
{
    if (_pPath == NULL) {
        return -1;
    }
    int iType = 0;
    _pPath->getNumber (NODE_PATH::PATH_TYPE, iType);
    return iType;
}

float NodePath::getPathProbability (void)
{
    if (_pPath == NULL) {
        return -1.0f;
    }
    double dProb = -1.0;
    _pPath->getNumber (NODE_PATH::PATH_PROBABILITY, dProb);
    return (float) dProb;
}

String NodePath::getPathId (void)
{
    if (_pPath == NULL) {
        return String();
    }
    String id;
    if (_pPath->getString (NODE_PATH::PATH_ID, id) < 0) {
        return String ();
    }
    return id;
}

uint64 NodePath::getTimeStamp (int wayPointIndex)
{
    return (uint64)WAYPOINT::getWaypointFloatValue (_pPath, wayPointIndex, WAYPOINT::TIMESTAMP);
}

int64  NodePath::read (ConfigManager *pConfMgr, uint32 ui32MaxSize)
{
    if (pConfMgr == NULL) {
        return -1;
    }

    // Delete previous data
    if (_pPath != NULL) {
        delete _pPath;
        _pPath = new JsonObject();
    }
    if (_pPath == NULL) {
        return -2;
    }

    if (pConfMgr->hasValue ("aci.dspro.nodePath.pszPathID") &&
        pConfMgr->hasValue ("aci.dspro.nodePath.pathType") &&
        pConfMgr->hasValue ("aci.dspro.nodePath.probability")) {
        // Read and set values for _pszPathID, _pathType and _pathProbability
        _pPath->setString (NODE_PATH::PATH_ID, pConfMgr->getValue ("aci.dspro.nodePath.pszPathID"));
        _pPath->setNumber (NODE_PATH::PATH_TYPE, pConfMgr->getValueAsInt ("aci.dspro.nodePath.pathType"));
        _pPath->setNumber (NODE_PATH::PATH_PROBABILITY, (float)atof (pConfMgr->getValue ("aci.dspro.nodePath.probability")));

        // Read and append Way Points
        char wayPointCounter[12];
        NOMADSUtil::String wayPointPropName = "aci.dspro.nodePath.waypoint.";
        int iWayPointCounter = 0;
        for (; true; iWayPointCounter++) {
            itoa (wayPointCounter, iWayPointCounter);
            if (!pConfMgr->hasValue (wayPointPropName + wayPointCounter + ".type")) {
                break;
            }
            switch (pConfMgr->getValueAsInt (wayPointPropName + wayPointCounter + ".type")) {
                case DECDEG_TIMESTP: {
                    float latitude = (pConfMgr->hasValue (wayPointPropName + wayPointCounter + ".latitude") ?
                                      (float) atof (pConfMgr->getValue (wayPointPropName + wayPointCounter + ".latitude")) :
                                      0);
                    float longitude = (pConfMgr->hasValue (wayPointPropName + wayPointCounter + ".longitude") ?
                                       (float) atof (pConfMgr->getValue (wayPointPropName + wayPointCounter + ".longitude")) :
                                       0);
                    float altitude = (pConfMgr->hasValue (wayPointPropName + wayPointCounter + ".altitude") ?
                                      (float) atof (pConfMgr->getValue (wayPointPropName + wayPointCounter + ".altitude")) :
                                      0);
                    const char *pszLocation = (pConfMgr->hasValue (wayPointPropName + wayPointCounter + ".location") ?
                                               pConfMgr->getValue (wayPointPropName + wayPointCounter + ".location") :
                                               NULL);
                    const char *pszNote = (pConfMgr->hasValue (wayPointPropName + wayPointCounter + ".note") ?
                                           pConfMgr->getValue (wayPointPropName + wayPointCounter + ".note") :
                                           NULL);
                    uint64 timeStamp =  (pConfMgr->hasValue (wayPointPropName + wayPointCounter + ".timeStamp") ?
                                         atoi64(pConfMgr->getValue (wayPointPropName + wayPointCounter + ".timeStamp")) :
                                         0);
                    int rc = appendWayPoint (latitude, longitude, altitude, pszLocation, pszNote, timeStamp);
                    if (rc < 0) {
                        // Error!
                        return rc;
                    }
                    break;
                }
                case DEG_TIMESTP: {
                    break;
                }
                case DECDEG_DATE: {
                    break;
                }
                case DEG_DATE: {
                    break;
                }
                case AZM_DATE: {
                    break;
                }
                default: {
                    // Error!!!
                }
            }
        }
        // If no way point was set, return an error
        return (iWayPointCounter == 0 ? -1 : iWayPointCounter);
    }
    // If it reaches here, it means that at least one of the necessary properties
    // is missing, thus return an error
    return -1;
}

int NodePath::readDPRFile (NOMADSUtil::Reader *pReader)
{
    // Delete previous data
    if (_pPath != NULL) {
        delete _pPath;
    }
    _pPath = new JsonObject();
    if (_pPath == NULL) {
        return -2;
    }
    bool bIsFirst = true;
    bool bIsRelTime = false;
    bool bPathTerminated = false;
    int64 i64Time = getTimeInMilliseconds();
    const int BUF_LEN = 1024;
    char buf[BUF_LEN];
    memset (buf, 0, BUF_LEN);
    StringTokenizer tokenizer;
    LineOrientedReader reader (pReader);
    for (int rc; (rc = reader.readLine (buf, BUF_LEN)) >= 0;) {
        if (rc == 0) {
            // Skip the empty line
            continue;
        }
        String line (buf);
        line.trim();
        if (line.startsWith ("-1")) {
            // End of the path
            bPathTerminated = true;
            break;
        }

        tokenizer.init (line, ',', ',');
        if (bIsFirst) {
            String token;
            int beginIndex, endIndex;
            for (int iToken = 0; (token = tokenizer.getNextToken()) != NULL; iToken++) {
                token.trim();
                beginIndex = (token.startsWith(",") ? 1 : 0);
                endIndex = (token.endsWith(",") ? token.length()-1 : token.length());
                if (beginIndex == endIndex) {
                    continue;
                }
                token = token.substring (beginIndex, endIndex);

                switch (iToken) {
                    case 0:
                        // Path Name
                        _pPath->setString (NODE_PATH::PATH_ID, token);
                        break;
                    case 1:
                        // Time Mode rel/abs
                        bIsRelTime = (0 == stricmp ("RelTime", (const char *) token));
                        break;
                    case 2:
                        // Path Time
                        _pPath->setNumber (NODE_PATH::PATH_TYPE, atoi (token));
                        break;
                    case 3:
                        // Path Prob
                        _pPath->setNumber (NODE_PATH::PATH_PROBABILITY, (float)atof ((const char *)token));
                        break;
                }
            }
            bIsFirst = false;
        }
        else {
            float fLat, fLong, fAlt;
            fLat = fLong = fAlt = 0.0f;
            String location, note;
            const char *pszWaypointToken = tokenizer.getNextToken();
            if (pszWaypointToken != NULL) {
                // Time
                if (bIsRelTime) {
                    i64Time += atoi64 (pszWaypointToken);
                }
                else {
                    i64Time = atoi64 (pszWaypointToken);
                }
            }
            pszWaypointToken = tokenizer.getNextToken();
            if (pszWaypointToken != NULL) {
                fLat = (float)atof (pszWaypointToken);
            }
            pszWaypointToken = tokenizer.getNextToken();
            if (pszWaypointToken != NULL) {
                fLong = (float)atof (pszWaypointToken);
            }
            pszWaypointToken = tokenizer.getNextToken();
            if (pszWaypointToken != NULL) {
                fAlt = (float)atof (pszWaypointToken);
            }
            pszWaypointToken = tokenizer.getNextToken();
            if (pszWaypointToken != NULL) {
                location = pszWaypointToken;
            }
            pszWaypointToken = tokenizer.getNextToken();
            if (pszWaypointToken != NULL) {
                note = pszWaypointToken;
            }
            if (appendWayPoint (fLat, fLong, fAlt, location, note, i64Time) < 0) {
                // Error!
                return -3;
            }
        }
        memset (buf, 0, BUF_LEN);
    }

    if (!bPathTerminated || bIsFirst) {
        return -1;
    }
    return 0;
}

void NodePath::updatePathBoundingBox (float latitude, float longitude)
{
    if (_maxLat == LATITUDE_UNSET || _maxLat < latitude) {
        _maxLat = latitude;
    }
    if (_minLat == LATITUDE_UNSET || _minLat > latitude) {
        _minLat = latitude;
    }
    if (_maxLong == LONGITUDE_UNSET || _maxLong < longitude) {
        _maxLong = longitude;
    }
    if (_minLong == LONGITUDE_UNSET || _minLong > longitude) {
        _minLong = longitude;
    }
}

int NodePath::fromJson (const JsonObject *pJson)
{
    if (_pPath != NULL) {
        delete _pPath;
    }
    _pPath = (JsonObject*) pJson->clone();
    return 0;
}

JsonObject * NodePath::toJson (void) const
{
    return (JsonObject *) _pPath->clone();
}

void NodePath::display (void)
{
    const char *pszMethodName = "NodePath::display";
    if (_pPath == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "no path set.\n");
    }
    else {
        String s (_pPath->toString());
        checkAndLogMsg (pszMethodName, Logger::L_Info, "%s\n", s.c_str());
    }
}


