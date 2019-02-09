/*
 * MetadataImpl.cpp
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
 * Created on July 5, 2013, 11:52 AM
 */

#include "MetadataImpl.h"

#include "GeoUtils.h"
#include "InstrumentedWriter.h"
#include "Json.h"
#include "NLFLib.h"
#include "StringHashset.h"
#include "Writer.h"

#include <vector>

using namespace IHMC_VOI;
using namespace NOMADSUtil;

MetadataImpl::MetadataImpl (void)
    : _pJson (new JsonObject())
{
}

MetadataImpl::~MetadataImpl (void)
{
    if (_pJson != NULL) {
        delete _pJson;
        _pJson = NULL;
    }
}

int MetadataImpl::resetFieldValue (const char *pszFieldName)
{
    if (_pJson == NULL) {
        return -1;
    }
    return _pJson->removeValue (pszFieldName);
}

int MetadataImpl::setFieldValue (const char *pszAttribute, int64 i64Value)
{
    if (pszAttribute == NULL) {
        return -1;
    }
    if (_pJson->setNumber (pszAttribute, i64Value) < 0) {
        return -2;
    }
    return 0;
}

int MetadataImpl::setFieldValue (const char *pszAttribute, const char *pszValue)
{
    if ((pszAttribute == NULL) || (pszValue == NULL)) {
        return -1;
    }
    if (_pJson->setString (pszAttribute, pszValue) < 0) {
        return -2;
    }
    return 0;
}

int MetadataImpl::findFieldValue (const char *pszFieldName, String &value) const
{
    if (_pJson == NULL) {
        return -1;
    }
    if (_pJson->getAsString (pszFieldName, value) < 0) {
        char *pszValue = value.r_str();
        if (pszValue != NULL) {
            free (pszValue);
        }
    }
    return 0;
}

int MetadataImpl::fromJson (const JsonObject *pJson)
{
    if (pJson == NULL) {
        return -1;
    }
    if (_pJson != NULL) {
        delete _pJson;
    }
    _pJson = (JsonObject*) pJson->clone();
    return (_pJson == NULL ? -2 : 0);
}

JsonObject * MetadataImpl::toJson (void) const
{
    if (_pJson == NULL) {
        return NULL;
    }
    return (JsonObject*) _pJson->clone();
}

BoundingBox MetadataImpl::getLocation (float fRange) const
{
    JsonObject *pJson = _pJson;
    if ((_pJson != NULL) && _pJson->hasObject (MetadataInterface::LOCATION)) {
        // Lingua Franca
        pJson = _pJson->getObject (MetadataInterface::LOCATION);
    }
    if (pJson == NULL) {
        return BoundingBox();
    }
    // Lingua Franca or old Attribute-Value
    double fluLat, fluLon, frlLat, flrLon;
    if ((0 == pJson->getNumber (MetadataInterface::LEFT_UPPER_LATITUDE, fluLat)) &&
        (0 == pJson->getNumber (MetadataInterface::LEFT_UPPER_LONGITUDE, fluLon)) &&
        (0 == pJson->getNumber (MetadataInterface::RIGHT_LOWER_LATITUDE, frlLat)) &&
        (0 == pJson->getNumber (MetadataInterface::RIGHT_LOWER_LONGITUDE, flrLon))) {

        BoundingBox bbox (fluLat, fluLon, frlLat, flrLon);
        if (fRange > bbox.getRadius()) {
            // Lingua Franca
            Point baricenter (bbox.getBaricenter());
            return BoundingBox::getBoundingBox (baricenter, fRange);
        }
        return bbox;
    }
    double fLat, fLon;
    if ((0 == pJson->getNumber (MetadataInterface::LATITUDE, fLat)) &&
        (0 == pJson->getNumber (MetadataInterface::LONGITUDE, fLon))) {
        double padding;
        if ((0 != pJson->getNumber (MetadataInterface::RADIUS, padding)) || (padding < fRange)) {
            // Lingua Franca
            padding = fRange;
        }
        return BoundingBox::getBoundingBox (fLat, fLon, padding);
    }
    return BoundingBox();
}

int MetadataImpl::getReferredDataMsgId (NOMADSUtil::String &refersTo) const
{
    if (_pJson == nullptr) {
        return -1;
    }
    if (_pJson->hasObject (MetadataInterface::RESOURCES)) {
        // Lingua Franca
        const JsonArray *pResources = _pJson->getArray (MetadataInterface::LOCATION);
        if (pResources != nullptr) {
            JsonObject *pResource;
            std::vector<String> referredObjectIds;
            for (int i = 0; (pResource = pResources->getObject (i)) != nullptr; i++) {
                JsonObject *pLocator = pResource->getObject ("locator");
                if (pLocator != nullptr) {
                    String type;
                    if ((0 == pLocator->getString ("type", type)) && (type == "DSPro")) {
                        String pointer;
                        if (0 == pLocator->getString ("pointer", pointer)) {
                            referredObjectIds.push_back (pointer);
                        }
                    }
                }
            }
            if (referredObjectIds.size() == 1) {
                refersTo = referredObjectIds[0];
                return 0;
            }
        }
    }
    else if (0 == _pJson->getString (MetadataInterface::REFERS_TO, refersTo)) {
        // Old Attribute-Value
        return 0;
    }

    return -2;
}

int MetadataImpl::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }
    char *pszJson = NULL;
    if (pReader->readString (&pszJson) < 0) {
        return -2;
    }
    JsonObject *pJson = new JsonObject (pszJson);
    delete pszJson;
    if (pJson == NULL) {
        return -3;
    }
    if (fromJson (pJson) < 0) {
        return -4;
    }
    return 0;
}

int MetadataImpl::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, NOMADSUtil::StringHashset *pFilters)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (_pJson == NULL) {
        return -2;
    }
    String json;
    if (pFilters == NULL) {
        json = _pJson->toString();
    }
    else {
        JsonObject *pCpy = (JsonObject*) _pJson->clone();
        if (pCpy != NULL) {
            StringHashset::Iterator iter = pFilters->getAllElements();
            for (; !iter.end(); iter.nextElement()) {
                pCpy->removeValue (iter.getKey());
            }
        }
        json = pCpy->toString();
        delete pCpy;
    }
    if (json.length() <= 0) {
        return -3;
    }
    InstrumentedWriter iw (pWriter);
    if (iw.writeString (json) < 0) {
        return -4;
    }
    if ((ui32MaxSize > 0) && (iw.getBytesWritten() > ui32MaxSize)) {
        return -5;
    }
    return 0;
}

