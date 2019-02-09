/**
 * UsefulDistance.cpp
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
 */

#include "UsefulDistance.h"

#include "Defs.h"

#include "ConfigManager.h"
#include "Json.h"
#include "Logger.h"
#include "StringTokenizer.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const unsigned int UsefulDistance::DEFAULT_USEFUL_DISTANCE = 1000;
const char * UsefulDistance::USEFUL_DISTANCE_PROPERTY = "aci.dspro.localNodeContext.usefulDistance";
const char * UsefulDistance::USEFUL_DISTANCE_BY_TYPE_PROPERTY = "aci.dspro.localNodeContext.usefulDistanceByType";

namespace USEFUL_DISTANCE
{
    static const char * DEFAULT_USEFUL_DISTANCE = "defaultUsefulDistance";
    static const char * USEFUL_DISTANCE_BY_TYPE = "usefulDistanceByType";
    static const char * TYPE = "type";
    static const char * DISTANCE = "distance";
}

UsefulDistance::UsefulDistance (void)
    : _ui32UsefulDistanceInMeters (DEFAULT_USEFUL_DISTANCE),
      _usefulDistanceByMimeType (false,         // bCaseSensitiveKeys
                                 true,          // bCloneKeys
                                 true,          // bDeleteKeys
                                 true)          // bDeleteValues
{
}

UsefulDistance::~UsefulDistance (void)
{
}

/*
int UsefulDistance::init (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == nullptr) {
        return -1;
    }
    if (pCfgMgr->hasValue (USEFUL_DISTANCE_PROPERTY)) {
        setDefaultUsefulDistance (pCfgMgr->getValueAsUInt32 (USEFUL_DISTANCE_PROPERTY));
    }
    if (pCfgMgr->hasValue (USEFUL_DISTANCE_BY_TYPE_PROPERTY)) {
        parseAndSetUsefulDistanceByType (pCfgMgr->getValue (USEFUL_DISTANCE_BY_TYPE_PROPERTY));
    }
    return 0;
}*/

uint32 UsefulDistance::getMaximumUsefulDistance (void)
{
    if (_usefulDistanceByMimeType.getCount() == 0) {
        return _ui32UsefulDistanceInMeters;
    }
    uint32 ui32Max = _ui32UsefulDistanceInMeters;
    StringHashtable<uint32>::Iterator i = _usefulDistanceByMimeType.getAllElements();
    for (; !i.end(); i.nextElement()) {
        uint32 ui32Val = *(i.getValue());
        if (ui32Val > ui32Max) {
            ui32Max = ui32Val;
        }
    }
    return ui32Max;
}

uint32 UsefulDistance::getUsefulDistance (const char *pszDataMIMEType) const
{
    if ((pszDataMIMEType != nullptr) && (_usefulDistanceByMimeType.containsKey (pszDataMIMEType))) {
        return *(_usefulDistanceByMimeType.get (pszDataMIMEType));
    }
    return _ui32UsefulDistanceInMeters;
}

bool UsefulDistance::setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters)
{
    if (_ui32UsefulDistanceInMeters != ui32UsefulDistanceInMeters) {
        _ui32UsefulDistanceInMeters = ui32UsefulDistanceInMeters;
        return true;
    }
    return false;
}

bool UsefulDistance::setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters)
{
    const char *pszMethodName = "UsefulDistance::setUsefulDistance";
    if (pszDataMIMEType == nullptr) {
        return false;
    }
    uint32 *pui32 = static_cast<uint32*>(calloc (1, sizeof (uint32)));
    if (pui32 == nullptr) {
        return false;
    }
    *pui32 = ui32UsefulDistanceInMeters;
    checkAndLogMsg (pszMethodName, Logger::L_Info, "setting %s useful distance to %u\n",
                    pszDataMIMEType, ui32UsefulDistanceInMeters);
    uint32 *pui32Old = _usefulDistanceByMimeType.put (pszDataMIMEType, pui32);
    if (pui32Old != nullptr) {
        const bool bUpdated = *pui32Old != ui32UsefulDistanceInMeters;
        free (pui32Old);
        return bUpdated;
    }
    return true;
}

void UsefulDistance::reset (void)
{
    _ui32UsefulDistanceInMeters = DEFAULT_USEFUL_DISTANCE;
    _usefulDistanceByMimeType.removeAll();
}

int UsefulDistance::fromJson (const JsonObject *pRoot)
{
    if (pRoot == nullptr) {
        return -1;
    }
    _usefulDistanceByMimeType.removeAll();
    uint32 ui32UsefulDistance = 0U;
    if (pRoot->getNumber (USEFUL_DISTANCE::DEFAULT_USEFUL_DISTANCE, ui32UsefulDistance) == 0) {
        setDefaultUsefulDistance (ui32UsefulDistance);
    }
    const JsonArray *pUsefulDistances = pRoot->getArray (USEFUL_DISTANCE::USEFUL_DISTANCE_BY_TYPE);
    if (pUsefulDistances == nullptr) {
        return 0;
    }
    const int iLen = pUsefulDistances->getSize();
    for (int i = 0; i < iLen; i++) {
        JsonObject *pUsefulDistance = pUsefulDistances->getObject (i);
        if (pUsefulDistance != nullptr) {
            String type;
            uint32 ui32UsefulDistance = 0U;
            if ((pUsefulDistance->getString (USEFUL_DISTANCE::TYPE, type) == 0) &&
                (pUsefulDistance->getNumber (USEFUL_DISTANCE::DISTANCE, ui32UsefulDistance) == 0)) {
                setUsefulDistance (type, ui32UsefulDistance);
            }
            delete pUsefulDistance;
        }
    }
    return 0;
}

JsonObject * UsefulDistance::toJson (void) const
{
    JsonObject *pJson = new JsonObject();
    if (pJson == nullptr) {
        return nullptr;
    }
    pJson->setNumber (USEFUL_DISTANCE::DEFAULT_USEFUL_DISTANCE, (double) _ui32UsefulDistanceInMeters);
    if (_usefulDistanceByMimeType.getCount() <= 0) {
        return pJson;
    }
    JsonArray *pArray = new JsonArray();
    if (pArray == nullptr) {
        return pJson;
    }
    for (StringHashtable<uint32>::Iterator i = _usefulDistanceByMimeType.getAllElements(); !i.end(); i.nextElement()) {
        const char *pType = i.getKey();
        uint32 *pUsefulDistance = i.getValue();
        JsonObject *pDistanceByType = new JsonObject();
        if (pDistanceByType == nullptr) {
            break;
        }
        pDistanceByType->setString (USEFUL_DISTANCE::TYPE, pType);
        pDistanceByType->setNumber (USEFUL_DISTANCE::DISTANCE, *pUsefulDistance);
        pArray->addObject (pDistanceByType);
    }
    if (pArray->getSize() > 0) {
        pJson->setObject (USEFUL_DISTANCE::USEFUL_DISTANCE_BY_TYPE, pArray);
    }
    return pJson;
}

