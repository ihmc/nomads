/**
 * LocationInfo.cpp
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
 * Created on December 23, 2016, 10:20 PM
 */

#include "LocationInfo.h"

#include "Defs.h"
#include "NodeContext.h"
#include "NodePath.h"

#include "GeoUtils.h"
#include "Json.h"
#include "Logger.h"
#include "StringTokenizer.h"
#include "StringStringHashtable.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

namespace LOCATION_INFO
{
    struct Localization
    {
        Localization (double dUsefulDistance)
            : _dApproxCoeff (10.0),
              _dTooFarCoeff (10.0),
              _dUsefulDistance (dUsefulDistance)
        {
        }

        /*
         * Given the point at (latitude, longitude), returns the closest point
         * on the path.  The coordinates of the closest points are stored in
         * (closestPointLat, closestPointLong).
         * Returns the index of the way-point that ends the path segment
         * that contains the closest way-point.
         *
         * Returns a negative number in case of error (if the path is empty)
         */
        NodeContext::NodeStatus calculateClosestCurrentPointInPath (NodePath *pCurrPath,
                                                                    float latitude, float longitude, float altitude,
                                                                    float &closestPointLat, float &closestPointLong,
                                                                    int &segmentStartIndex)
        {
            const char *pszMethodName = "NodeContextImpl::calculateCurrentWayPointInPath";
            if (pCurrPath == nullptr) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "pCurrPath is nullptr, probably because the "
                                "current path is not set. It is not possible to calculate the current position\n");
                return NodeContext::PATH_UNSET;
            }

            int iPathLength = pCurrPath->getPathLength();
            if (iPathLength < 1) {
                segmentStartIndex = -1;
                return NodeContext::PATH_UNSET;
            }
            if (iPathLength == 1) {
                closestPointLat = pCurrPath->getLatitude (0);
                closestPointLong = pCurrPath->getLongitude (0);
                if (isApproximableToPoint (closestPointLat, closestPointLong, latitude, longitude)) {
                    segmentStartIndex = 0;
                    return NodeContext::ON_WAY_POINT;
                }
                if (isTooFarFromPath (closestPointLat, closestPointLong, latitude, longitude)) {
                    segmentStartIndex = -1;
                    return NodeContext::TOO_FAR_FROM_PATH;
                }
                segmentStartIndex = -1;
                return NodeContext::PATH_DETOURED;
            }

            // iPathLength > 1

            // TODO : now "altitude" is not considered
            float minDistFromSegment, fDist;
            minDistFromSegment = -1.0f;
            float segLatA, segLongA, segLatB, segLongB, projectionLat, projectionLong;
            for (int i = 0; i < (pCurrPath->getPathLength() - 1); i++) {
                segLatA = pCurrPath->getLatitude (i);
                segLongA = pCurrPath->getLongitude (i);
                segLatB = pCurrPath->getLatitude (i + 1);
                segLongB = pCurrPath->getLongitude (i + 1);

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
            segLatB = pCurrPath->getLatitude (segmentStartIndex + 1);
            segLongB = pCurrPath->getLongitude (segmentStartIndex + 1);

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
                        return NodeContext::ON_WAY_POINT;
                    }
                }
                else if (isApproximableToPoint (fDistB)) {
                    // the node is relatively close to B, I can approximate and use
                    // B as current waypoint
                    segmentStartIndex += 1;
                    return NodeContext::ON_WAY_POINT;
                }
                // The node is relatively close to the path but far from the waypoints,
                // it is probably in transit from waypoint "i-1" to waypoint "i"
                return NodeContext::PATH_IN_TRANSIT;
            }
            if (isTooFarFromPath (minDistFromSegment)) {
                // too far!
                segmentStartIndex = -1;
                return NodeContext::TOO_FAR_FROM_PATH;
            }
            // the node is far from the path, it detoured!!!
            return NodeContext::PATH_DETOURED;
        }

        /*
         * Evaluate whether (latitude1, longitude1) is close enough to
         * (latitude2, longitude2) that it can be approximated to it.
         * If (latitude2, longitude2) is not specified, (latitude1, longitude1)
         * is compared with the latest position that was set.
         */
        bool isApproximableToPoint (float latitude1, float longitude1, float latitude2, float longitude2)
        {
            return isApproximableToPoint (greatCircleDistance (latitude1, longitude1, latitude2, longitude2));
        }

        bool isApproximableToPreviousPosition (NodePath &history, float latitude, float longitude)
        {
            const int iLen = history.getPathLength();
            if (iLen <= 0) {
                return false;
            }
            const float fOldActualLat = history.getLatitude (iLen - 1);
            const float fOldActualLong = history.getLongitude (iLen - 1);
            return isApproximableToPoint (fOldActualLat, fOldActualLong, latitude, longitude);
        }

        bool isApproximableToPoint (double dDistance)
        {
            // TODO: Replace 10 with _dApproxCoeff
            return (dDistance < (_dUsefulDistance / _dApproxCoeff));
        }

        bool isTooFarFromPath (float latitude1, float longitude1, float latitude2, float longitude2)
        {
            return isTooFarFromPath (greatCircleDistance (latitude1, longitude1, latitude2, longitude2));
        }

        bool isTooFarFromPath (double dDistance)
        {
            // TODO: Replace 10 with _dTooFarCoeff
            return (dDistance > (_dUsefulDistance * _dTooFarCoeff));
        }

        private:
            double _dApproxCoeff;
            double _dTooFarCoeff;
            double _dUsefulDistance;
    };
}

const char * LocationInfo::LOCATION_INFO_OBJECT_NAME = "locationInfo";

LocationInfo::LocationInfo (double dTooFarCoeff, double dApproxCoeff)
    : _status (NodeContext::PATH_UNSET),
      _iCurrWayPointInPath (NodeContext::WAYPOINT_UNSET),
      _closestPointOnPathLat (MAX_LATITUDE + 1),
      _closestPointOnPathLong (MAX_LONGITUDE + 1),
      _pathAdjustingMode (NodeContext::DEFAULT_PATH_ADJUSTING_MODE),
      _pPathWrapper (new AdjustedPathWrapper()),
      _pCurrLocation (new JsonObject()),
      _history (NodePath::PAST_PATH, NodePath::MAIN_PATH_TO_OBJECTIVE, 1.0f)
{
}

LocationInfo::~LocationInfo (void)
{
    delete _pPathWrapper;
    delete _pCurrLocation;
}

AdjustedPathWrapper * LocationInfo::getAdjustedPath (NodeContext *pNodeContext)
{
    _pPathWrapper->configure (pNodeContext);
    return _pPathWrapper;
}

float LocationInfo::getClosestPointOnPathLatitude (void) const
{
    return _closestPointOnPathLat;
}

float LocationInfo::getClosestPointOnPathLongitude (void) const
{
    return _closestPointOnPathLong;
}

int LocationInfo::getCurrentWayPointInPath (void) const
{
    return _iCurrWayPointInPath;
}

int LocationInfo::getCurrentPosition (float &latitude, float &longitude, float &altitude,
                                      const char *&pszLocation, const char *&pszNote,
                                      uint64 &timeStamp)
{
    if (_pCurrLocation != nullptr) {
        double d;
        if (_pCurrLocation->hasObject ("lat")) {
            _pCurrLocation->getNumber ("lat", d);
            latitude = d;
        }
        else {
            latitude = MAX_LATITUDE + 1;
        }
        if (_pCurrLocation->hasObject ("lon")) {
            _pCurrLocation->getNumber ("lon", d);
            longitude = d;
        }
        else {
            latitude = MAX_LONGITUDE + 1;
        }
        if (_pCurrLocation->hasObject ("alt")) {
            _pCurrLocation->getNumber ("alt", d);
            altitude = d;
        }
        else {
            altitude = 0.0f;
        }
        if (_pCurrLocation->hasObject ("time")) {
            _pCurrLocation->getNumber ("time", timeStamp);
        }
        else {
            timeStamp = getTimeInMilliseconds();
        }
    }

    /*latitude = _history.getLatitude (iLatestLocationIdx);
    longitude = _history.getLongitude (iLatestLocationIdx);
    altitude = _history.getAltitude (iLatestLocationIdx);
    // pszLocation = _history.getLocation (iLatestLocationIdx);
    // pszNote = _history.getNote (iLatestLocationIdx);
    timeStamp = _history.getTimeStamp (iLatestLocationIdx);*/
    return 0;
}

NodeContext::PositionApproximationMode LocationInfo::getPathAdjustingMode (void) const
{
    return _pathAdjustingMode;
}

NodeContext::NodeStatus LocationInfo::getStatus (void) const
{
    return _status;
}

bool LocationInfo::setCurrentPosition (NodePath *pPath, float latitude, float longitude, float altitude, uint32 ui32MaxUsefulDistance)
{
    const char *pszMethodName = "PeerNodeContextImpl::setCurrentPosition";
    float closestPointLat, closestPointLong;
    int currentSegmentStartIndex;
    LOCATION_INFO::Localization loc (ui32MaxUsefulDistance);
    NodeContext::NodeStatus status = loc.calculateClosestCurrentPointInPath (pPath, latitude, longitude, altitude,
                                                                             closestPointLat, closestPointLong,
                                                                             currentSegmentStartIndex);
    bool bRet = (_status != status);
    const bool bIsAppoxToPrevPosition = loc.isApproximableToPreviousPosition (_history, latitude, longitude);
    if (!bRet && !bIsAppoxToPrevPosition) {
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

    if (!bIsAppoxToPrevPosition) {
        _history.appendWayPoint (latitude, longitude, altitude, nullptr, nullptr, getTimeInMilliseconds());
    }
    if (_pCurrLocation != nullptr) {
        _pCurrLocation->setNumber ("lat", latitude);
        _pCurrLocation->setNumber ("lon", longitude);
        _pCurrLocation->setNumber ("alt", altitude);
        _pCurrLocation->setNumber ("time", (uint64) getTimeInMilliseconds());
    }

    switch (status) {
        case NodeContext::PATH_UNSET:
            // There is not anything to do
            break;

        case NodeContext::ON_WAY_POINT:
        case NodeContext::PATH_DETOURED:
        case NodeContext::PATH_IN_TRANSIT:
            if ((currentSegmentStartIndex >= 0) && (currentSegmentStartIndex != _iCurrWayPointInPath)) {
                _iCurrWayPointInPath = currentSegmentStartIndex;
                // Way point has changed, return true
                bRet = true;
                checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "updated current waypoint to %d\n",
                                _iCurrWayPointInPath);
            }
            // NOTE: there is no missing "break", it is supposed to execute the
            // code in the TOO_FAR_FROM_PATH case as well!

        case NodeContext::TOO_FAR_FROM_PATH:
            _closestPointOnPathLat = closestPointLat;
            _closestPointOnPathLong = closestPointLong;
            checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "current position updated to %f,%f\n",
                            _closestPointOnPathLat, _closestPointOnPathLong);
            break;

        default:
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "unknown path state\n");
    }
    _status = status;
    return bRet;
}

void LocationInfo::setCurrentPath (float closestPointOnPathLat, float closestPointOnPathLong)
{
    _iCurrWayPointInPath = 0;
    _closestPointOnPathLat = closestPointOnPathLat;
    _closestPointOnPathLong = closestPointOnPathLong;
    _status = NodeContext::ON_WAY_POINT;
}

void LocationInfo::reset (void)
{
    _status = NodeContext::PATH_UNSET;
    _iCurrWayPointInPath = NodeContext::WAYPOINT_UNSET;
    _closestPointOnPathLat = MAX_LATITUDE + 1;
    _closestPointOnPathLong = MAX_LONGITUDE + 1;
    if (_pCurrLocation != nullptr) {
        delete _pCurrLocation;
    }
    _pCurrLocation = new JsonObject();
    _ui16Version = 0;
}

namespace LOCATION_INFO_JSON
{
    static const char * STATUS = "status";
    static const char * CURRENT_WAY_POINT = "currWaypoint";
    static const char * CLOSEST_POINT_ON_PATH_LAT = "closestPointLat";
    static const char * CLOSEST_POINT_ON_PATH_LON = "closestPointLon";
    static const char * CURRENT_LOCATION = "currLocation";
}

int LocationInfo::fromJson (const JsonObject *pJson)
{
    if (pJson == nullptr) {
        return -1;
    }
    int rc = 0;
    if (pJson->hasObject (LOCATION_INFO_JSON::STATUS)) {
        uint32 ui32 = 0U;
        rc = pJson->getNumber (LOCATION_INFO_JSON::STATUS, ui32);
        _status = (NodeContext::NodeStatus) ui32;
    }
    if (pJson->hasObject (LOCATION_INFO_JSON::CURRENT_WAY_POINT)) {
        rc = pJson->getNumber (LOCATION_INFO_JSON::CURRENT_WAY_POINT, _iCurrWayPointInPath);
    }
    if (pJson->hasObject (LOCATION_INFO_JSON::CLOSEST_POINT_ON_PATH_LAT)) {
        rc = pJson->getNumber (LOCATION_INFO_JSON::CLOSEST_POINT_ON_PATH_LAT, _closestPointOnPathLat);
    }
    if (pJson->hasObject (LOCATION_INFO_JSON::CLOSEST_POINT_ON_PATH_LON)) {
        rc = pJson->getNumber (LOCATION_INFO_JSON::CLOSEST_POINT_ON_PATH_LON, _closestPointOnPathLong);
    }
    if (pJson->hasObject (LOCATION_INFO_JSON::CURRENT_LOCATION)) {
        JsonObject *pPart = pJson->getObject (LOCATION_INFO_JSON::CURRENT_LOCATION);
        if (pPart != nullptr) {
            delete _pCurrLocation;
            _pCurrLocation = (JsonObject*) pPart->clone();
            delete pPart;
            /*if (_pCurrLocation != nullptr) {
                double latitude, longitude, altitude;
                _pCurrLocation->getNumber ("lat", latitude);
                _pCurrLocation->getNumber ("lon", latitude);
                _pCurrLocation->getNumber ("alt", latitude);
            }*/
        }
        else {
            rc = -4;
        }
    }
    return (rc < 0 ? -2 : 0);
}

JsonObject * LocationInfo::toJson (void) const
{
    JsonObject *pJson = new JsonObject();
    if (pJson == nullptr) {
        return nullptr;
    }
    pJson->setNumber (LOCATION_INFO_JSON::STATUS, _status);
    pJson->setNumber (LOCATION_INFO_JSON::CURRENT_WAY_POINT, _iCurrWayPointInPath);
    pJson->setNumber (LOCATION_INFO_JSON::CLOSEST_POINT_ON_PATH_LAT, _closestPointOnPathLat);
    pJson->setNumber (LOCATION_INFO_JSON::CLOSEST_POINT_ON_PATH_LON, _closestPointOnPathLong);
    JsonObject *pPart = (JsonObject *)_pCurrLocation->clone();
    if (pPart != nullptr) {
        pJson->setObject (LOCATION_INFO_JSON::CURRENT_LOCATION, pPart);
        delete pPart;
    }
    return pJson;
}

