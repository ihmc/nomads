/**
 * PathInfo.cpp
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

#include "PathInfo.h"

#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

const char * PathInfo::PATH_INFO_OBJECT_NAME = "pathInfo";

PathInfo::PathInfo (void)
{
}

PathInfo::~PathInfo (void)
{
}

NodePath * PathInfo::getPath (const char *pszPathId)
{
    return _paths.get (pszPathId);
}

NodePath * PathInfo::getPath (void)
{
    if (_currPathId.length() <= 0) {
        return nullptr;
    }
    return _paths.get (_currPathId);
}

bool PathInfo::setPathProbability (const char *pszPathId, float probability)
{
    NodePath *pPath = _paths.get (pszPathId);
    if (pPath == nullptr) {
        return false;
    }
    const float fProb = pPath->getPathProbability();
    if (fEquals (fProb, probability, 0.001f)) {
        return false;
    }
    return (0 == pPath->setProbability (probability));
}

bool PathInfo::setCurrentPath (const char *pszPathId)
{
    if (_currPathId == pszPathId) {
        return false;
    }
    if (!_paths.containsKey (pszPathId)) {
        return false;
    }
    _currPathId = pszPathId;
    return true;
}

bool PathInfo::addPath (NodePath *pNodePath)
{
    delete _paths.put (pNodePath->getPathId(), pNodePath);
    return true;
}

bool PathInfo::deletePath (const char *pszPathId)
{
    if (!_paths.containsKey (pszPathId)) {
        return false;
    }
    delete _paths.get (pszPathId);
    if (_currPathId == pszPathId) {
        delete _currPathId.r_str();
    }
    return true;
}

void PathInfo::reset (void)
{
    _paths.removeAll();
    delete _currPathId.r_str();
    _ui16Version = 0;
}

int PathInfo::fromJson (const NOMADSUtil::JsonObject *pJson)
{
    if (pJson == nullptr) {
        return -1;
    }
    NodePath *pPath = new NodePath();
    if (pPath == nullptr) {
        return -2;
    }
    if (pPath->fromJson (pJson) < 0) {
        return -3;
    }
    delete _paths.put (pPath->getPathId(), pPath);
    _currPathId = pPath->getPathId();
    return 0;
}

NOMADSUtil::JsonObject * PathInfo::toJson (void) const
{
    NodePath *pPath = _paths.get (_currPathId);
    if (pPath == nullptr) {
        return nullptr;
    }
    return pPath->toJson();
}

