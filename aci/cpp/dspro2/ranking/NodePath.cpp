/* 
 * NodePath.cpp
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

#include "NodePath.h"

#include "Defs.h"

#include "ConfigManager.h"
#include "GeoUtils.h"
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
using namespace IHMC_ACI;

const char * NodePath::PAST_PATH = "Node Past Path";
const int NodePath::MAIN_PATH_TO_OBJECTIVE = 1;
const int NodePath::ALTERNATIVE_PATH_TO_OBJECTIVE = 2;
const int NodePath::MAIN_PATH_TO_BASE = 3;
const int NodePath::ALTERNATIVE_PATH_TO_BASE = 4;
const int NodePath::FIXED_LOCATION = 5;

const float NodePath::LATITUDE_UNSET = MAX_LATITUDE + 1;
const float NodePath::LONGITUDE_UNSET = MAX_LONGITUDE + 1;

NodePath::NodePath()
{
    _pszPathID = NULL;
    _pWayPoint = NULL;
    _pathLength = 0;
    _pathType = FIXED_LOCATION;
    _pathProbability = 0;

    _maxLat = _minLat = LATITUDE_UNSET;
    _maxLong = _minLong = LONGITUDE_UNSET;
}

NodePath::NodePath (const char *pathID, int pathType, float fPathProbability)
{
    _pWayPoint = NULL;
    _pathLength = 0;
    _pathType = pathType;

    _maxLat = _minLat = LATITUDE_UNSET;
    _maxLong = _minLong = LONGITUDE_UNSET;

    if ((fPathProbability <= 1) && (fPathProbability >= 0)) {
        _pathProbability = fPathProbability;
    }
    else {
        _pathProbability = 0;
    }
    _pszPathID = (pathID != NULL ? strDup (pathID) : NULL);
}

NodePath::~NodePath()
{
    if (_pszPathID != NULL) {
        free(_pszPathID);
        _pszPathID = NULL;
    }
    if (_pWayPoint != NULL) {
        for (int i = 0; i < _pathLength; i ++) {
            if (_pWayPoint[i]._pszLocation != NULL) {
                free(_pWayPoint[i]._pszLocation);
                _pWayPoint[i]._pszLocation = NULL;
            }
            if (_pWayPoint[i]._pszNote != NULL) {
                free(_pWayPoint[i]._pszNote);
                _pWayPoint[i]._pszNote = NULL;
            }
        }
        free (_pWayPoint);
        _pWayPoint = NULL;
    }
}

int NodePath::appendWayPoint (int8 azimuthDeg, uint8 azimuthMin, uint8 azimuthSec, uint16 distanceInMeters,
                              float altitude, const char *pszLocation, const char *pszNote,
                              int hour, int min, int sec, int day, int mon, int year)
{
    if (_pathLength == 0) {
        return -1;
    }
    float prevLat = _pWayPoint[_pathLength - 1]._latitude;
    float prevLong = _pWayPoint[_pathLength - 1]._longitude;
    float azimuth = abs(azimuthDeg) + azimuthMin/60.0f + azimuthSec/3600.0f;
    if (azimuthDeg < 0) {
        azimuth = - azimuth;
    }
    float latitude = prevLat + distanceInMeters * cos (azimuth);
    float longitude = prevLong + distanceInMeters * sin (azimuth);
    return appendWayPoint(latitude, longitude, altitude, pszLocation, pszNote, hour, min, sec, day, mon, year);
}

int NodePath::appendWayPoint (int8 latitudeDeg, uint8 latitudeMin, uint8 latitudeSec, int8 longitudeDeg, uint8 longitudeMin,
                              uint8 longitudeSec, float altitude, const char *pszLocation, const char *pszNote,
                              int hour, int min, int sec, int day, int mon, int year)
{
    float latitude = abs(latitudeDeg) + latitudeMin/60.0f + latitudeSec/3600.0f;
    if(latitudeDeg < 0) {
        latitude = - latitude;
    }
    float longitude = abs(longitudeDeg) + longitudeMin/60.0f + longitudeSec/3600.0f;
    if(longitudeDeg < 0) {
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
    if (latitudeDeg < 0) {
        latitude = 0 - latitude;
    }
    longitude = abs (longitudeDeg) + longitudeMin/60.0f + longitudeSec/3600.0f;
    if (longitudeDeg < 0) {
        longitude = 0.0f - longitude;
    }
    return appendWayPoint (latitude, longitude, altitude, pszLocation, pszNote, timeStamp);
}

int NodePath::appendWayPoint (float latitude, float longitude, float altitude, const char *pszLocation, const char *pszNote, uint64 timeStamp)
{
    if(_pathType != NodePath::FIXED_LOCATION) {
        _pathLength ++;
        _pWayPoint = (_wayPoint *) realloc(_pWayPoint, _pathLength * sizeof(_wayPoint));
        if(pszLocation != NULL) {
            _pWayPoint[_pathLength - 1]._pszLocation = strDup (pszLocation);
        }
        else {
            _pWayPoint[_pathLength - 1]._pszLocation = NULL;
        }
        if(pszNote != NULL) {
            _pWayPoint[_pathLength - 1]._pszNote = strDup (pszNote);
        }
        else {
            _pWayPoint[_pathLength - 1]._pszNote = NULL;
        }
        _pWayPoint[_pathLength - 1]._latitude = latitude;
        _pWayPoint[_pathLength - 1]._longitude = longitude;
        _pWayPoint[_pathLength - 1]._altitude = altitude;
        _pWayPoint[_pathLength - 1]._timeStamp = timeStamp;
    }
    else {
        if(_pathLength == 0) {
            _pathLength ++;
            _pWayPoint = (_wayPoint *) calloc(_pathLength, sizeof(_wayPoint));
        }
        if (_pWayPoint != NULL ) {
            if(_pWayPoint[_pathLength - 1]._pszLocation != NULL) {
                free(_pWayPoint[_pathLength - 1]._pszLocation);
            }
            if(pszLocation != NULL) {
                _pWayPoint[_pathLength - 1]._pszLocation = strDup (pszLocation);
            }
            else {
                _pWayPoint[_pathLength - 1]._pszLocation = NULL;
            }
            if(_pWayPoint[_pathLength - 1]._pszNote != NULL) {
                free(_pWayPoint[_pathLength - 1]._pszNote);
            }
            if(pszNote != NULL) {
                _pWayPoint[_pathLength - 1]._pszNote = strDup (pszNote);
            }
            else {
                _pWayPoint[_pathLength - 1]._pszNote = NULL;
            }
            _pWayPoint[_pathLength - 1]._latitude = latitude;
            _pWayPoint[_pathLength - 1]._longitude = longitude;
            _pWayPoint[_pathLength - 1]._altitude = altitude;
            _pWayPoint[_pathLength - 1]._timeStamp = timeStamp;
        }
    }

    updatePathBoundingBox (latitude, longitude);

    return 0;
}

int NodePath::setProbability (float probability)
{
    if((probability <= 1) && (probability >= 0)) {
        _pathProbability = probability;
        return 0;
    }
    return -1;
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

float NodePath::getLatitude(int wayPointIndex)
{
    if ((_pWayPoint != NULL) && (wayPointIndex > -1) && (wayPointIndex < _pathLength)) {
        return _pWayPoint[wayPointIndex]._latitude;
    }
    return 0;
}

float NodePath::getLongitude (int wayPointIndex)
{
    if ((_pWayPoint != NULL) && (wayPointIndex > -1) && (wayPointIndex < _pathLength)) {
        return _pWayPoint[wayPointIndex]._longitude;
    }
    return 0;
}

float NodePath::getAltitude (int wayPointIndex)
{
    if ((_pWayPoint != NULL) && (wayPointIndex > -1) && (wayPointIndex < _pathLength)) {
        return _pWayPoint[wayPointIndex]._altitude;
    }
    return 0;
}

const char * NodePath::getLocation (int wayPointIndex)
{
    if ((_pWayPoint != NULL) && (wayPointIndex > -1) && (wayPointIndex < _pathLength)) {
        return _pWayPoint[wayPointIndex]._pszLocation;
    }
    return NULL;
}

const char * NodePath::getNote (int wayPointIndex)
{
    if ((_pWayPoint != NULL) && (wayPointIndex > -1) && (wayPointIndex < _pathLength)) {
        return _pWayPoint[wayPointIndex]._pszNote;
    }
    return NULL;
}

uint64 NodePath::getTimeStamp (int wayPointIndex)
{
    if ((_pWayPoint != NULL) && (wayPointIndex > -1) && (wayPointIndex < _pathLength)) {
        return _pWayPoint[wayPointIndex]._timeStamp;
    }
    return 0;
}

int64 NodePath::read (Reader *pReader, uint32 ui32MaxSize, bool bSkip)
{
    if (pReader == NULL) {
        return -1;
    }
    uint16 length;
    uint32 totLength = 0;
    int8 rc;

    // Delete previous data
    if (!bSkip && _pWayPoint != NULL) {
        for (int i = 0; i < _pathLength; i ++) {
            if (_pWayPoint[i]._pszLocation != NULL) {
                free (_pWayPoint[i]._pszLocation);
            }
        }
        free (_pWayPoint);
        _pWayPoint = NULL;
        _pathLength = 0;
        _maxLat = _minLat = LATITUDE_UNSET;
        _maxLong = _minLong = LONGITUDE_UNSET;
    }
    if (_pszPathID != NULL) {
        free (_pszPathID);
        _pszPathID = NULL;
    }

    // Read the new path
    if (ui32MaxSize < (totLength + 2)) {
        return -2;
    }
    if (pReader->read16 (&length) < 0) {
        return -3;
    }
    totLength += 2;
    if (length == 0) {
        if (!bSkip) {
            _pszPathID = NULL;
        }
    }
    else {
        if (ui32MaxSize < (totLength + length)) {
            checkAndLogMsg ("NodePath::read", Logger::L_MildError,
                            "length of path id (%d) plus totLength (%d) exceeds max size (%d)\n",
                            (int) length, (int) totLength, (int) ui32MaxSize);
            return -4;
        }
        char *pszNodeId = (char *) calloc (length + 1, sizeof(char));
        rc = pReader->readBytes (pszNodeId, length);
        if (rc < 0) {
            free (pszNodeId);
            pszNodeId = NULL;
            return -5;
        }
        if (!bSkip) {
            if (_pszPathID != NULL) {
                free (_pszPathID);
            }
            _pszPathID = pszNodeId;
        }
        totLength += length;
    }
    if (ui32MaxSize < (totLength + sizeof(float))) {
        return -6;
    }

    float pathProbability;
    rc = pReader->read32 (&pathProbability);
    if (rc < 0) {
        return -7;
    }
    if (!bSkip) {
        _pathProbability = pathProbability;
    }

    totLength += sizeof (float);
    if (ui32MaxSize < (totLength + sizeof(int))) {
        return -8;
    }

    int pathType;
    rc = pReader->read32 (&pathType);
    if (rc < 0) {
        return -9;
    }
    if (!bSkip) {
        _pathType = pathType;
    }

    totLength += sizeof (int);
    if (ui32MaxSize < (totLength + sizeof(int))) {
        return -10;
    }
    int pathLength;
    rc = pReader->read32 (&pathLength);
    if (rc < 0) {
        return -11;
    }
    if (!bSkip) {
        _pathLength = pathLength;
    }

    totLength += sizeof (int);
    if (_pathLength == 0) {
        checkAndLogMsg ("NodePath::read", Logger::L_SevereError, "Read path of length 0.");
        return totLength;
    }

    float altitude, latitude, longitude;
    char *pszLocation;
    uint64 ui64Timestamp;

    for (int i = 0; i < pathLength; i ++) {
        if (ui32MaxSize < (totLength + sizeof(float))) {
            return -12;
        }
        rc = pReader->read32 (&altitude);
        if (rc < 0) {
            return -13;
        }
        totLength += sizeof (float);
        if (ui32MaxSize < (totLength + sizeof(float))) {
            return -14;
        }
        rc = pReader->read32 (&latitude);
        if (rc < 0) {
            return -15;
        }
        totLength += sizeof (float);
        if (ui32MaxSize < (totLength + sizeof(float))) {
            return -16;
        }
        rc = pReader->read32 (&longitude);
        if (rc < 0) {
            return -17;
        }
        totLength += sizeof (float);
        if(ui32MaxSize < (totLength + 2)) {
            return -18;
        }

        rc = pReader->read16 (&length);
        if (rc < 0) {
            return -19;
        }
        totLength += 2;
        if (length == 0) {
            pszLocation = NULL;
        }
        else {
            if (ui32MaxSize < (totLength + length)) {
                return -20;
            }
            pszLocation = (char *) calloc (length + 1, sizeof(char));
            rc = pReader->readBytes (pszLocation, length);
            if (rc < 0) {
                free (pszLocation);
                pszLocation = NULL;
                return -21;
            }
            totLength += length;
        }
        if (ui32MaxSize < (totLength + 8)) {
            return -22;
        }
        rc = pReader->read64 (&ui64Timestamp);
        if (rc < 0) {
            return -23;
        }
        totLength += 8;
        
        if (!bSkip) {
            if (_pWayPoint == NULL) {
                _pWayPoint = (_wayPoint *) calloc (_pathLength, sizeof (_wayPoint));
            }
            _pWayPoint[i]._altitude = altitude;
            _pWayPoint[i]._latitude = latitude;
            _pWayPoint[i]._longitude = longitude;
            updatePathBoundingBox (_pWayPoint[i]._latitude, _pWayPoint[i]._longitude);
            _pWayPoint[i]._pszLocation = pszLocation;
            _pWayPoint[i]._timeStamp = ui64Timestamp;
        }
    }

    return totLength;
}

int64  NodePath::read (ConfigManager *pConfMgr, uint32 ui32MaxSize)
{
    if (pConfMgr == NULL) {
        return -1;
    }

    // Delete previous data
    if (_pWayPoint != NULL) {
        for (int i = 0; i < _pathLength; i ++) {
            if (_pWayPoint[i]._pszLocation != NULL) {
                free (_pWayPoint[i]._pszLocation);
            }
        }
        free (_pWayPoint);
        _pWayPoint = NULL;
    }
    if (_pszPathID != NULL) {
        free (_pszPathID);
        _pszPathID = NULL;
    }

    if (pConfMgr->hasValue ("aci.dspro.nodePath.pszPathID") &&
        pConfMgr->hasValue ("aci.dspro.nodePath.pathType") &&
        pConfMgr->hasValue ("aci.dspro.nodePath.probability")) {
        // Read and set values for _pszPathID, _pathType and _pathProbability
        _pszPathID = (char *) pConfMgr->getValue ("aci.dspro.nodePath.pszPathID");
        _pathType = pConfMgr->getValueAsInt ("aci.dspro.nodePath.pathType");
        _pathProbability = (float) atof (pConfMgr->getValue ("aci.dspro.nodePath.probability"));

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
    LineOrientedReader reader (pReader);

    const int BUF_LEN = 1024;
    char buf[BUF_LEN];
    memset (buf, 0, BUF_LEN);

    String line, token;

    StringTokenizer tokenizer;
    tokenizer.setLeftDelimiter (',');
    tokenizer.setRightDelimiter (',');
    int beginIndex, endIndex;

    bool bIsFirst = true;
    bool bIsRelTime = false;
    bool bPathTerminated = false;

    float fLat, fLong, fAlt;
    int64 i64Time = getTimeInMilliseconds();
    const char *pszLocation, *pszNote;

    int rc;
    while (true) {
        rc = reader.readLine (buf, BUF_LEN);
        if (rc == 0) {
            // Skip the empty line
            continue;
        }
        if (rc < 0) {
            // EOF!
            break;
        }

        line = buf;
        line.trim();

        if (line.startsWith ("-1")) {
            // End of the path
            bPathTerminated = true;
            break;
        }

        tokenizer.init(line);

        if (bIsFirst) {
            for (int iToken = 0; (token = tokenizer.getNextToken()) != NULL; iToken++) {
                token.trim();
                beginIndex = (token.startsWith(",") ? 1 : 0);
                endIndex = (token.endsWith(",") ? token.length()-1 : token.length());
                if (beginIndex == endIndex) {
                    continue;
                }
                token = token.substring(beginIndex, endIndex);

                switch (iToken) {
                    case 0:
                        // Path Name
                        _pszPathID = strDup (token);
                        break;
                    case 1:
                        // Time Mode rel/abs
                        bIsRelTime = (0 == stricmp ("RelTime", (const char *) token));
                        break;
                    case 2:
                        // Path Time
                        _pathType = atoi (token);
                        break;
                    case 3:
                        // Path Prob
                        _pathProbability = (float) atof ((const char *) token);
                        break;
                }
            }
            bIsFirst = false;
        }
        else {
            fLat = fLong = fAlt = 0.0f;
            pszLocation = pszNote = NULL;

            for (int iToken = 0; (token = tokenizer.getNextToken()) != NULL; iToken++) {
                token.trim();
                beginIndex = (token.startsWith(",") ? 1 : 0);
                endIndex = (token.endsWith(",") ? token.length()-1 : token.length());
                if (beginIndex == endIndex) {
                    continue;
                }
                token = token.substring(beginIndex, endIndex);
                switch (iToken) {
                    case 0:
                        // Time
                        if (bIsRelTime) {
                            i64Time += atoi64 (token);
                        }
                        else {
                            i64Time = atoi64 (token);
                        }
                    case 1:
                        // Latitude
                        fLat = (float) atof (token);
                        break;
                    case 2:
                        // Longitude
                        fLong = (float) atof (token);
                        break;
                    case 3:
                        // Altitude
                        fAlt = (float) atof (token);
                        break;
                    case 4:
			// Location
                        pszLocation = strDup ((const char *) token);
                        break;
                    case 5:
			// Note
                        pszNote = strDup ((const char *) token);
                        break;
                }
            }
            if (appendWayPoint (fLat, fLong, fAlt, pszLocation, pszNote, i64Time) < 0) {
                // Error!
                return -3;
            }
            if (pszLocation != NULL) {
                delete pszLocation;
            }
            if (pszNote != NULL) {
                delete pszNote;
            }
        }
        memset (buf, 0, BUF_LEN);
    }

    if (!bPathTerminated || bIsFirst) {
        return -1;
    }
    return 0;
}

int64 NodePath::write (Writer *pWriter)
{
    return write(pWriter, 0, true);
}

int64 NodePath::write (Writer *pWriter, uint32 ui32MaxSize)
{
    return write(pWriter, ui32MaxSize, false);
}

int64 NodePath::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, bool bIgnoreMaxSizes)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (_pWayPoint == NULL) {
        return -3;
    }
    uint16 length;
    uint32 totLength = 0;
    int8 retValue;
    if (_pszPathID == NULL) {
        length = 0;
        if (!bIgnoreMaxSizes && ui32MaxSize < totLength + 2) {
            return -2;
        }
        retValue = pWriter->write16 (&length);
        if (retValue < 0) {
            return retValue;
        }
        totLength += 2;
    }
    else {
        length = strlen (_pszPathID);
        if (!bIgnoreMaxSizes && ui32MaxSize < (uint32) (length + 2)) {
            return -2;
        }
        retValue = pWriter->write16 (&length);
        if (retValue < 0) {
            return retValue;
        }
        totLength += 2;
        retValue = pWriter->writeBytes ((const char *) _pszPathID, length);
        if (retValue < 0) {
            return retValue;
        }
        totLength += length;
    }
    if (!bIgnoreMaxSizes && ui32MaxSize < (totLength + sizeof(float))) {
        return -2;
    }
    retValue = pWriter->write32 (&_pathProbability);
    if (retValue < 0) {
        return retValue;
    }
    totLength += sizeof(float);
    if (!bIgnoreMaxSizes && ui32MaxSize < (totLength + sizeof(int))) {
        return -2;
    }
    retValue = pWriter->write32(&_pathType);
    if (retValue < 0) {
        return retValue;
    }
    totLength += sizeof(int);
    if (!bIgnoreMaxSizes && ui32MaxSize < (totLength + sizeof(int))) {
        return -2;
    }
    retValue = pWriter->write32 (&_pathLength);
    if (retValue < 0) {
        return retValue;
    }
    totLength += sizeof(int);
    for (int i = 0; i < _pathLength; i ++) {
        if (!bIgnoreMaxSizes && ui32MaxSize < (totLength + sizeof(float))) {
            return -2;
        }
        retValue = pWriter->write32 (&(_pWayPoint[i]._altitude));
        if (retValue < 0) {
            return retValue;
        }
        totLength += sizeof(float);
        if (!bIgnoreMaxSizes && ui32MaxSize < (totLength + sizeof(float))) {
            return -2;
        }
        retValue = pWriter->write32 (&(_pWayPoint[i]._latitude));
        if (retValue < 0) {
            return retValue;
        }
        totLength += sizeof(float);
        if (!bIgnoreMaxSizes && ui32MaxSize < (totLength + sizeof(float))) {
            return -2;
        }
        retValue = pWriter->write32 (&(_pWayPoint[i]._longitude));
        if (retValue < 0) {
            return retValue;
        }
        totLength += sizeof(float);
        if (_pWayPoint[i]._pszLocation == NULL) {
            length = 0;
            if(!bIgnoreMaxSizes && ui32MaxSize < totLength + 2) return -2;
            retValue = pWriter->write16 (&length);
            if(retValue < 0) return retValue;
            totLength += 2;
        }
        else {
            length = strlen(_pWayPoint[i]._pszLocation);
            if (!bIgnoreMaxSizes && ui32MaxSize < (uint32) (length + 2)) {
                return -2;
            }
            retValue = pWriter->write16(&length);
            if (retValue < 0) {
                return retValue;
            }
            totLength += 2;
            retValue = pWriter->writeBytes ((const char *) _pWayPoint[i]._pszLocation, length);
            if (retValue < 0) {
                return retValue;
            }
            totLength += length;
        }
        if (!bIgnoreMaxSizes && ui32MaxSize < (totLength + 8)) {
            return -2;
        }
        retValue = pWriter->write64 (&(_pWayPoint[i]._timeStamp));
        if (retValue < 0) {
            return retValue;
        }
        totLength += 8;
    }
    return totLength;
}

int64 NodePath::getWriteLength (void)
{
    if (_pWayPoint == NULL) {
        return 0;
    }
    uint32 totLength = 0;
    totLength += 2; // For path length
    if (_pszPathID != NULL) {
        totLength += strlen(_pszPathID);
    }
    totLength += 2 * sizeof (int);
    totLength += sizeof (float);
    for (int i = 0; i < _pathLength; i ++) {
        totLength += 3 * sizeof(float) + 8;
        if (_pWayPoint[i]._pszLocation == NULL) {
            totLength += 2;
        }
        else {
            totLength += 2 + strlen(_pWayPoint[i]._pszLocation);
        }
    }
    return totLength;
}

int64 NodePath::writeFrom (Writer *pWriter, uint32 ui32MaxSize, int wayPointIndex)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (_pWayPoint == NULL) {
        return -3;
    }
    if ((wayPointIndex < 0) || (wayPointIndex >= _pathLength)) {
        return -3;
    }
    uint16 length;
    uint32 totLength = 0;
    int8 retValue;
    if(_pszPathID == NULL) {
        length = 0;
        if (ui32MaxSize < totLength + 2) {
            return -2;
        }
        retValue = pWriter->write16(&length);
        if (retValue < 0) {
            return retValue;
        }
        totLength += 2;
    }
    else {
        length = strlen(_pszPathID);
        if (ui32MaxSize < (uint32) (length + 2)) {
            return -2;
        }
        retValue = pWriter->write16 (&length);
        if (retValue < 0) {
            return retValue;
        }
        totLength += 2;
        retValue = pWriter->writeBytes ((const char *) _pszPathID, length);
        if (retValue < 0) {
            return retValue;
        }
        totLength += length;
    }
    if (ui32MaxSize < (totLength + sizeof(float))) {
        return -2;
    }
    retValue = pWriter->write32 (&_pathProbability);
    if (retValue < 0) {
        return retValue;
    }
    totLength += sizeof(float);
    if (ui32MaxSize < (totLength + sizeof(int))) {
        return -2;
    }
    retValue = pWriter->write32 (&_pathType);
    if (retValue < 0) {
        return retValue;
    }
    totLength += sizeof(int);
    if (ui32MaxSize < (totLength + sizeof(int))) {
        return -2;
    }
    int len = _pathLength - wayPointIndex + 1;
    retValue = pWriter->write32 (&len);
    if (retValue < 0) {
        return retValue;
    }
    totLength += sizeof(int);
    for(int i = wayPointIndex; i < _pathLength; i ++) {
        if(ui32MaxSize < (totLength + sizeof(float))) return -2;
        retValue = pWriter->write32 (&(_pWayPoint[i]._altitude));
        if(retValue < 0) {
            return retValue;
        }
        totLength += sizeof(float);
        if (ui32MaxSize < (totLength + sizeof(float))) {
            return -2;
        }
        retValue = pWriter->write32 (&(_pWayPoint[i]._latitude));
        if (retValue < 0) {
            return retValue;
        }
        totLength += sizeof(float);
        if (ui32MaxSize < (totLength + sizeof(float))) {
            return -2;
        }
        retValue = pWriter->write32 (&(_pWayPoint[i]._longitude));
        if (retValue < 0) {
            return retValue;
        }
        totLength += sizeof(float);
        if(_pWayPoint[i]._pszLocation == NULL) {
            length = 0;
            if (ui32MaxSize < totLength + 2) {
                return -2;
            }
            retValue = pWriter->write16(&length);
            if (retValue < 0) {
                return retValue;
            }
            totLength += 2;
        }
        else {
            length = strlen (_pWayPoint[i]._pszLocation);
            if(ui32MaxSize < (uint32) (length + 2)) return -2;
            retValue = pWriter->write16 (&length);
            if (retValue < 0) {
                return retValue;
            }
            totLength += 2;
            retValue = pWriter->writeBytes ((const char *) _pWayPoint[i]._pszLocation, length);
            if (retValue < 0) {
                return retValue;
            }
            totLength += length;
        }
        if (ui32MaxSize < (totLength + 8)) {
            return -2;
        }
        retValue = pWriter->write64 (&(_pWayPoint[i]._timeStamp));
        if (retValue < 0) {
            return retValue;
        }
        totLength += 8;
    }
    return totLength;
}
                                                                       
int64 NodePath::getWriteFromLength (int wayPointIndex)
{
    if (_pWayPoint == NULL) {
        return -1;
    }
    if ((wayPointIndex < 0) || (wayPointIndex >= _pathLength)) {
        return -1;
    }
    uint32 totLength = 0;
    if (_pszPathID != NULL) {
        totLength += strlen(_pszPathID);
    }
    totLength += 2;
    totLength += sizeof (int);
    totLength += sizeof (float);
    for (int i = wayPointIndex; i < _pathLength; i ++) {
        totLength += 3 * sizeof(float) + 8;
        if (_pWayPoint[i]._pszLocation == NULL) {
            totLength += 2;
        }
        else {
            totLength += 2 + strlen (_pWayPoint[i]._pszLocation);
        }
    }
    return totLength;
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

void NodePath::display()
{
    const char *pszMethodName = "NodePath::display";
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Path ID: %s\n", (_pszPathID ? _pszPathID : "NULL"));
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Path Type: %d\n", _pathType);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Path Probability: %f\n", _pathProbability);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Path Length: %d\n", _pathLength);
    for(int i = 0; i < _pathLength; i ++) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Way Point n %d: latitude %f / longitude %f "
                        "/ altitude %f / location %s / note %s / timestamp %u\n",
                        i, _pWayPoint[i]._latitude, _pWayPoint[i]._longitude, _pWayPoint[i]._altitude,
                        (_pWayPoint[i]._pszLocation ? _pWayPoint[i]._pszLocation : "NULL"),
                        (_pWayPoint[i]._pszNote ? _pWayPoint[i]._pszNote : "NULL"),
                        _pWayPoint[i]._timeStamp);
    }
}

