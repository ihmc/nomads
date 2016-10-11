/* 
 * Path.cpp
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 */

#include "Path.h"

#include "Defs.h"

#include "NodeContext.h"
#include "NodePath.h"

#include "GeoUtils.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

Path::Path()
{
}

Path::~Path()
{
}

AdjustedPathWrapper::AdjustedPathWrapper()
{
    configure (NULL); 
}

AdjustedPathWrapper::AdjustedPathWrapper (NodeContext *pNodeContext)
{
    configure (pNodeContext);
}

AdjustedPathWrapper::~AdjustedPathWrapper()
{
}

int AdjustedPathWrapper::configure (NodeContext *pNodeContext)
{
    _pNodeContext = pNodeContext;
    if (_pNodeContext == NULL) {
        _pPath = NULL;
        _iPathLenght = 0;
        _iDetourLength = 0;
        _iDetourStartIndex = 0;
        return -1;
    }

    _pPath = pNodeContext->getPath();
    if (_pPath == NULL) {
        _pPath = NULL;
        _iPathLenght = 0;
        _iDetourLength = 0;
        _iDetourStartIndex = 0;
        return -2;
    }

    NodeContext::PositionApproximationMode approxMode = pNodeContext->getPathAdjustingMode();
    switch (pNodeContext->getStatus()) {
        case NodeContext::PATH_UNSET:
            _iDetourStartIndex = pNodeContext->getCurrentWayPointInPath();
        case NodeContext::ON_WAY_POINT:
            _iPathLenght = _pPath->getPathLength();
            _iDetourLength = 0;
            _iDetourStartIndex = pNodeContext->getCurrentWayPointInPath();
            break;

        case NodeContext::PATH_DETOURED:
            _iPathLenght = _pPath->getPathLength();
            if (approxMode == NodeContext::GO_TO_NEXT_WAY_POINT) {
                _iDetourLength = 1;
            }
            else if (approxMode == NodeContext::GO_TO_PROJECTION) {
                _iDetourLength = 2;
            }
            _iDetourStartIndex = pNodeContext->getCurrentWayPointInPath();
            break;

        case NodeContext::PATH_IN_TRANSIT:
            _iPathLenght = _pPath->getPathLength();
            _iDetourLength = 1;
            _iDetourStartIndex = pNodeContext->getCurrentWayPointInPath();
            break;

        case NodeContext::TOO_FAR_FROM_PATH:
            _iPathLenght = 0;
            _iDetourLength = 1;
            _iDetourStartIndex = 0;
            break;
    }

    return 0;
}

float AdjustedPathWrapper::getLatitude (int iWaypointIndex)
{
    if ((_pNodeContext->getStatus() == NodeContext::TOO_FAR_FROM_PATH) ||
        (_pNodeContext->getStatus() == NodeContext::PATH_UNSET)) {
        // If the node is too far from the path, or if the path has not been set,
        // it is interested only in the data in the area around the current
        // actual position
        if (iWaypointIndex == 0) {
            float latitude;
            if (_pNodeContext->getCurrentLatitude (latitude) == 0) {
                return latitude;
            }
        }
        return MAX_LATITUDE + 1;
    }
    if (isInDetour (iWaypointIndex)) {
        float latitude;
        if (iWaypointIndex == _iDetourStartIndex) {
            if (0 == _pNodeContext->getCurrentLatitude (latitude)) {
                return latitude;
            }
        }
        else if (iWaypointIndex == (_iDetourStartIndex+1)) {
            return _pNodeContext->getClosestPointOnPathLatitude();
        }
        checkAndLogMsg ("DetouredPathWrapper::getLatitude", Logger::L_MildError,
                        "could not find current actual latitude\n");
        return MAX_LATITUDE + 1;
    }
    return _pPath->getLatitude (getWayPoint (iWaypointIndex));
}

float AdjustedPathWrapper::getLongitude (int iWaypointIndex)
{
     if ((_pNodeContext->getStatus() == NodeContext::TOO_FAR_FROM_PATH) ||
         (_pNodeContext->getStatus() == NodeContext::PATH_UNSET)) {
        // If the node is too far from the path, or if the path has not been set,
        // it is interested only in the data in the area around the current
        // actual position
        if (iWaypointIndex == 0) {
            float longitude;
            if (_pNodeContext->getCurrentLongitude (longitude) == 0) {
                return longitude;
            }
        }
        return MAX_LONGITUDE + 1;
    }
    if (isInDetour (iWaypointIndex)) {
        float longitude;
        if (iWaypointIndex == _iDetourStartIndex) {
            if (0 == _pNodeContext->getCurrentLongitude (longitude)) {
                return longitude;
            }
        }
        else if (iWaypointIndex == (_iDetourStartIndex+1)) {
            return _pNodeContext->getClosestPointOnPathLongitude();
        }
        checkAndLogMsg ("DetouredPathWrapper::getLongitude", Logger::L_MildError,
                        "could not find current actual longitude\n");
        return 0;
    }
    return _pPath->getLongitude (getWayPoint (iWaypointIndex));
}

uint64 AdjustedPathWrapper::getTimeStamp (int iWaypointIndex)
{
     if ((_pNodeContext->getStatus() == NodeContext::TOO_FAR_FROM_PATH) ||
         (_pNodeContext->getStatus() == NodeContext::PATH_UNSET)) {
        // If the node is too far from the path, or if the path has not been set,
        // it is interested only in the data in the area around the current
        // actual position
        if (iWaypointIndex == 0) {
            uint64 timestamp;
            if (_pNodeContext->getCurrentTimestamp (timestamp) == 0) {
                return timestamp;
            }
        }
        return -1;
    }
    if (isInDetour (iWaypointIndex)) {
        uint64 timestamp;
        if (iWaypointIndex == _iDetourStartIndex) {
            if (0 == _pNodeContext->getCurrentTimestamp (timestamp)) {
                return timestamp;
            }
        }
        else if (iWaypointIndex == (_iDetourStartIndex+1)) {
            // TODO: estimate this
            if (0 == _pNodeContext->getCurrentTimestamp (timestamp)) {
                return timestamp;
            }
        }
        checkAndLogMsg ("DetouredPathWrapper::getTimeStamp", Logger::L_MildError,
                        "could not find current actual timestamp\n");
        return 0;
    }
    return _pPath->getTimeStamp (getWayPoint (iWaypointIndex));
}

int AdjustedPathWrapper::getPathLength()
{
    switch (_pNodeContext->getStatus()) {
        case NodeContext::PATH_UNSET: return 1;
        case NodeContext::TOO_FAR_FROM_PATH: return 1;
        default: return (_pPath == NULL ? -1 : _iPathLenght + _iDetourLength);
    }
}

int AdjustedPathWrapper::getWayPoint (int iWaypointIndex)
{
    switch (_pNodeContext->getStatus()) {
        case NodeContext::PATH_UNSET: return 0;
        case NodeContext::ON_WAY_POINT: return iWaypointIndex;
        case NodeContext::PATH_DETOURED:
        case NodeContext::PATH_IN_TRANSIT:
            if (isBeforeDetour (iWaypointIndex)) {
                return iWaypointIndex;
            }
            if (isAfterDetour (iWaypointIndex)) {
                return iWaypointIndex - _iDetourLength;
            }
            else {
                // In detour
                checkAndLogMsg ("AdjustedPathWrapper::getWayPoint", caseUnhandled);
                return iWaypointIndex;
            }
        case NodeContext::TOO_FAR_FROM_PATH: return 0;
        default: return -1;  // this case should never happen!
    }
}

