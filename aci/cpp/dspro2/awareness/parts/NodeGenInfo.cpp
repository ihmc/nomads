/**
 * NodeGenInfo.cpp
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
 * Created on November 12, 2010, 4:56 PM
 */

#include "NodeGenInfo.h"

#include "Json.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace NODE_INFO_JSON_NAMES
{
    const char * NODE_ID = "nodeId";
    const char * BATTERY_LEVEL = "batteryLevel";
    const char * STORAGE_USAGE = "storageUsage";
    const char * MISSION_ID = "mission";
    const char * TEAM_ID = "team";
    const char * ROLE = "role";
    const char * TYPE = "type";
    const char * USER_IDS = "usersIds";
}

namespace NODE_INFO
{
    String getString (const JsonObject *pJson, const char *pszName)
    {
        String s;
        if (pJson->getString (pszName, s) < 0) {
            return String();
        }
        return s;
    }

    // Return true whether the value has changed, false otherwise
    bool set (JsonObject *pJson, const char *pszName, const char *pszValue)
    {
        const String newValue (pszValue);
        String oldValue;
        pJson->getString (pszName, oldValue);
        if (newValue.length() <= 0) {
            if (oldValue.length() <= 0) {
                // nothing to do
                return false;
            }
            pJson->removeValue (pszName);
            return true;
        }
        return pJson->setString (pszName, pszValue) == 0;
    }

    void reset (JsonObject *pJson, const char *pszNodeId)
    {
        if (pJson != nullptr) {
            if (pszNodeId != nullptr) {
                pJson->setString (NODE_INFO_JSON_NAMES::NODE_ID, pszNodeId);  // _json makes a copy of pszNodeId
            }
            pJson->setNumber (NODE_INFO_JSON_NAMES::BATTERY_LEVEL, (uint8)10);
            pJson->setNumber (NODE_INFO_JSON_NAMES::STORAGE_USAGE, (uint8)0);
        }
    }
}

const char * NodeGenInfo::NODE_INFO_OBJECT_NAME = "nodeInfo";

NodeGenInfo::NodeGenInfo (const char *pszNodeId)
    : _pJson (new JsonObject())
{
    NODE_INFO::reset (_pJson, pszNodeId);
}

NodeGenInfo::~NodeGenInfo (void)
{
    delete _pJson;
}

bool NodeGenInfo::setBatteryLevel (uint8 ui8BatteryLevel)
{
    if (ui8BatteryLevel > 10) {
        return false;
    }
    if (!_pJson->hasObject (NODE_INFO_JSON_NAMES::BATTERY_LEVEL)) {
        return (0 == _pJson->setNumber (NODE_INFO_JSON_NAMES::BATTERY_LEVEL, ui8BatteryLevel));
    }
    uint16 iVal = 0;
    if (_pJson->getNumber (NODE_INFO_JSON_NAMES::BATTERY_LEVEL, iVal) < 0) {
        return false;
    }
    if (ui8BatteryLevel != iVal) {
        return (0 == _pJson->setNumber (NODE_INFO_JSON_NAMES::BATTERY_LEVEL, ui8BatteryLevel));
    }
    return false;
}

bool NodeGenInfo::setMemoryAvailable (uint8 ui8MemoryAvailable)
{
    if (ui8MemoryAvailable > 10) {
        return false;
    }
    if (!_pJson->hasObject (NODE_INFO_JSON_NAMES::STORAGE_USAGE)) {
        return (0 == _pJson->setNumber (NODE_INFO_JSON_NAMES::STORAGE_USAGE, ui8MemoryAvailable));
    }
    uint16 iVal = 0;
    if (_pJson->getNumber (NODE_INFO_JSON_NAMES::STORAGE_USAGE, iVal) < 0) {
        return false;
    }
    if (ui8MemoryAvailable != iVal) {
        return (0 == _pJson->setNumber (NODE_INFO_JSON_NAMES::STORAGE_USAGE, ui8MemoryAvailable));
    }
    return false;
}

bool NodeGenInfo::addUserId (const char *pszUserId)
{
    JsonArray *pArray = _pJson->getArrayReference (NODE_INFO_JSON_NAMES::USER_IDS);
    if (pArray == nullptr) {
        const char *ids[1] = { pszUserId };
        _pJson->setStrings (NODE_INFO_JSON_NAMES::USER_IDS, ids, 1);
        return true;
    }
    if (hasUserId (pszUserId)) {
        return false;
    }
    return pArray->addString (pszUserId) == 0;
}

unsigned int NodeGenInfo::getBatteryLevel (void) const
{
    int iVal = (unsigned int) 10;
    if (!_pJson->hasObject (NODE_INFO_JSON_NAMES::BATTERY_LEVEL) ||
        (_pJson->getNumber (NODE_INFO_JSON_NAMES::BATTERY_LEVEL, iVal) < 0)) {
        return (unsigned int) 10;
    }
    return (unsigned int)iVal;
}

unsigned int NodeGenInfo::getMemoryAvailable (void) const
{
    int iVal = 0;
    if (!_pJson->hasObject (NODE_INFO_JSON_NAMES::STORAGE_USAGE) ||
        (_pJson->getNumber (NODE_INFO_JSON_NAMES::STORAGE_USAGE, iVal) < 0)) {
        return (unsigned int) 0;
    }
    return (unsigned int)iVal;
}

String NodeGenInfo::getNodeId (void) const
{
    return NODE_INFO::getString (_pJson, NODE_INFO_JSON_NAMES::NODE_ID);
}

String NodeGenInfo::getMisionId (void) const
{
    return NODE_INFO::getString (_pJson, NODE_INFO_JSON_NAMES::MISSION_ID);
}

String NodeGenInfo::getRole (void) const
{
    return NODE_INFO::getString (_pJson,NODE_INFO_JSON_NAMES::ROLE);
}

String NodeGenInfo::getTeamId (void) const
{
    return NODE_INFO::getString (_pJson, NODE_INFO_JSON_NAMES::TEAM_ID);
}

bool NodeGenInfo::hasUserId (const char *pszUserId) const
{
    const JsonArray *pArray = _pJson->getArray (NODE_INFO_JSON_NAMES::USER_IDS);
    if (pArray == nullptr) {
        return false;
    }
    const int iLen = pArray->getSize();
    for (int i = 0; i < iLen; i++) {
        String userId;
        if (pArray->getString (i, userId) == 0) {
            if (userId == pszUserId) {
                return true;
            }
        }
    }
    return false;
}

bool NodeGenInfo::setMisionId (const char *pszMissionId)
{
    return NODE_INFO::set (_pJson, NODE_INFO_JSON_NAMES::MISSION_ID, pszMissionId);
}

bool NodeGenInfo::setRole (const char *pszRole)
{
    return NODE_INFO::set (_pJson, NODE_INFO_JSON_NAMES::ROLE, pszRole);
}

bool NodeGenInfo::setTeamId (const char *pszMissionId)
{
    return NODE_INFO::set (_pJson, NODE_INFO_JSON_NAMES::TEAM_ID, pszMissionId);
}

bool NodeGenInfo::setNodeType (const char *pszType)
{
    return NODE_INFO::set (_pJson, NODE_INFO_JSON_NAMES::TYPE, pszType);
}

#include <stdio.h>

void NodeGenInfo::reset (void)
{
    if (_pJson != nullptr) {
        const String nodeId (getNodeId());
        delete _pJson;
        _pJson = new JsonObject();
        NODE_INFO::reset (_pJson, nodeId);
    }
    _ui16Version = 0;
}

int NodeGenInfo::fromJson (const JsonObject *pJson)
{
    if (pJson == nullptr) {
        return -1;
    }
    String oldNodeId;
    if (_pJson->getString (NODE_INFO_JSON_NAMES::NODE_ID, oldNodeId) < 0) {
        return -2;
    }
    String newNodeId;
    if (pJson->getString (NODE_INFO_JSON_NAMES::NODE_ID, newNodeId) < 0) {
        return -3;
    }
    if (oldNodeId != newNodeId) {
        return -3;
    }
    JsonObject *pNewJson = (JsonObject *) pJson->clone();
    if (pNewJson == nullptr) {
        return -4;
    }
    delete _pJson;
    _pJson = pNewJson;
    return 0;
}

JsonObject * NodeGenInfo::toJson (void) const
{
    if (_pJson != nullptr) {
        return (JsonObject *) _pJson->clone();
    }
    return nullptr;
}

