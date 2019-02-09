/*
 * AreasOfIntInfo.cpp
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on September 27, 2011, 2:00 PM
 */

#include "AreasOfIntInfo.h"
#include "Json.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;
using namespace IHMC_VOI;

const char * AreasOfInterestsInfo::AREAS_OF_INTEREST_OBJ = "areasOfInterest";

namespace AREAS_OF_INTEREST
{
    int getCoordinates (const JsonObject *pJson, double &lat, double &lon)
    {
        if ((pJson->getNumber ("lat", lat) == 0) && (pJson->getNumber ("lon", lon) == 0)) {
            return 0;
        }
        return -1;
    }

    BoundingBox getBoundingBox (const JsonObject *pJson)
    {
        double ulLat, ullon, lrLat, lrLon;
        JsonObject *pCorner = pJson->getObject ("upperLeftCorner");
        getCoordinates (pCorner, ulLat, ullon);
        delete pCorner;
        pCorner = pJson->getObject ("lowerRightCorner");
        getCoordinates (pCorner, lrLat, lrLon);
        delete pCorner;
        return BoundingBox (ulLat, ullon, lrLat, lrLon);
    }
}

using namespace AREAS_OF_INTEREST;

AreasOfInterestsInfo::AreasOfInterestsInfo (void)
    : _pJson (new JsonObject())
{
    JsonArray areas;
    _pJson->setObject ("areas", &areas);
}

AreasOfInterestsInfo::~AreasOfInterestsInfo (void)
{
    if (_pJson != nullptr) {
        delete _pJson;
    }
}

bool AreasOfInterestsInfo::add (const char *pszAreaName, BoundingBox &bb, int64 i64StatTime, int64 i64EndTime)
{
    if (pszAreaName == nullptr) {
        return false;
    }
    if (i64StatTime < 0) {
        i64StatTime = 0;
    }
    if (i64EndTime < 0) {
        i64EndTime = 0x7FFFFFFFFFFFFFFF;
    }

    JsonArray *pAreas = _pJson->getArrayReference ("areas");
    if (pAreas == nullptr) {
        JsonArray *ptmp = new JsonArray();
        _pJson->setObject ("areas", ptmp);
        pAreas = _pJson->getArrayReference (AREAS_OF_INTEREST_OBJ);
    }
    const int iCount = pAreas->getSize() < 0 ? 0 : pAreas->getSize();
    for (int i = 0; i < iCount; i++) {
        JsonObject *pArea = pAreas->getObject (i);
        if (pArea != nullptr) {
            String sName;
            if ((pArea->getAsString ("name", sName) == 0) && (sName == pszAreaName)) {
                pAreas->removeObject (i);
                delete pArea;
                break;
            }
            delete pArea;
        }
    }

    JsonObject newArea;
    newArea.setString ("name", pszAreaName);
    newArea.setNumber ("startTime", i64StatTime);
    newArea.setNumber ("endTime", i64EndTime);
    JsonObject ulCorner;
    ulCorner.setNumber ("lat", bb._leftUpperLatitude);
    ulCorner.setNumber ("lon", bb._leftUpperLongitude);
    JsonObject lrCorner;
    lrCorner.setNumber ("lat", bb._rightLowerLatitude);
    lrCorner.setNumber ("lon", bb._rightLowerLongitude);
    JsonObject boundingBox;
    boundingBox.setObject ("upperLeftCorner", &ulCorner);
    boundingBox.setObject ("lowerRightCorner", &lrCorner);

    newArea.setObject ("boundingBox", &boundingBox);

    int rc = pAreas->addObject (&newArea);
    if (rc < 0) {
        return false;
    }
    return true;
}

AreaOfInterestList * AreasOfInterestsInfo::getAreasOfInterest (void) const
{
    AreaOfInterestList *pRet = new AreaOfInterestList();
    if ((pRet == nullptr) || (_pJson == nullptr)) {
        return nullptr;
    }
    const JsonArray *pAreas = _pJson->getArray ("areas");
    if (pAreas == nullptr) {
        return nullptr;
    }
    const int iCount = pAreas->getSize() < 0 ? 0 : pAreas->getSize();
    for (int i = 0; i < iCount; i++) {
        JsonObject *pArea = pAreas->getObject (i);
        if (pArea != nullptr) {
            String areaName;
            pArea->getString ("name", areaName);
            JsonObject *pBoundingBox = pArea->getObject ("boundingBox");
            BoundingBox bb (getBoundingBox (pBoundingBox));
            AreaOfInterest *ptmp = new AreaOfInterest (areaName, bb);
            delete pBoundingBox;
            if (ptmp != nullptr) {
                pRet->prepend (ptmp);
            }
            double dStartTime, dEndTime;
            pArea->getNumber ("startTime", dStartTime);
            pArea->getNumber ("endTime", dEndTime);
            ptmp->setEndTime ((int64) dStartTime);
            ptmp->setEndTime ((int64) dEndTime);
        }
    }
    return pRet;
}

int AreasOfInterestsInfo::fromJson (const JsonObject *pJson)
{
    if (pJson == nullptr) {
        return -1;
    }
    delete _pJson;
    _pJson = (JsonObject*) pJson->clone();
    if (_pJson == nullptr) {
        return -2;
    }
    return 0;
}

JsonObject * AreasOfInterestsInfo::toJson (void) const
{
    return (JsonObject*) _pJson->clone();
}

void AreasOfInterestsInfo::reset (void)
{
    delete _pJson;
    _pJson = new JsonObject();
    JsonArray areas;
    _pJson->setObject ("areas", &areas);
    _ui16Version = 0;
}

