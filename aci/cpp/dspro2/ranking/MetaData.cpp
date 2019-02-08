/*
 * MetaData.cpp
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

#include "MetaData.h"

#include "DSSFLib.h"
#include "Logger.h"
#include "NLFLib.h"

#include "Writer.h"

#include "Json.h"
#include "StrClass.h"

#include "BufferReader.h"
#include "CompressedWriter.h"
#include "CompressedReader.h"
#include "StringHashset.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

MetaData::MetaData (void)
{
}

MetaData::~MetaData (void)
{
}

MetaData * MetaData::clone (void)
{
    MetaData *pCopy = new MetaData();
    if (pCopy != nullptr) {
        JsonObject *pJson = _impl.toJson();
        if (pJson != nullptr) {
            pCopy->_impl.fromJson (pJson);
            delete pJson;
        }
    }
    return pCopy;
}

NOMADSUtil::BoundingBox  MetaData::getLocation (float fRange) const
{
    return _impl.getLocation (fRange);
}

int MetaData::setFieldValue (const char *pszAttribute, int64 i64Value)
{
    return _impl.setFieldValue (pszAttribute, i64Value);
}

int MetaData::setFieldValue (const char *pszAttribute, const char *pszValue)
{
    return _impl.setFieldValue (pszAttribute, pszValue);
}

int MetaData::resetFieldValue (const char *pszFieldName)
{
    return _impl.resetFieldValue (pszFieldName);
}

int MetaData::getReferredDataMsgId (NOMADSUtil::String &refersTo) const
{
    return _impl.getReferredDataMsgId (refersTo);
}

int MetaData::fromString (const char *pszJson)
{
    if (pszJson == nullptr) {
        return -1;
    }
    int iLen = strlen (pszJson);
    if (iLen <= 0) {
        return - 2;
    }
    JsonObject json (pszJson);
    return fromJson (&json);
}

int MetaData::fromJson (const JsonObject *pJson)
{
    return _impl.fromJson (pJson);
}

JsonObject * MetaData::toJson (void) const
{
    return _impl.toJson();
}

int MetaData::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == nullptr) {
        return -1;
    }
    JsonObject json;
    if (json.read (pReader, true) < 0) {
        return -2;
    }
    int rc = _impl.fromJson (&json);
    return (rc == 0 ? 0 : -3);
}

int MetaData::write (Writer *pWriter, uint32 ui32MaxSize, StringHashset *pFilters)
{
    if (pWriter == nullptr) {
        return -1;
    }
    JsonObject *pObject = _impl.toJson();
    if (pObject != nullptr) {
        if (pFilters != nullptr) {
            StringHashset::Iterator iter = pFilters->getAllElements();
            for (; !iter.end(); iter.nextElement()) {
                pObject->removeValue (iter.getKey());
            }
        }
        int rc = pObject->write (pWriter, true);
        delete pObject;
        return rc == 0 ? 0 : -2;
    }
    return -3;
}

int MetaData::findFieldValue (const char *pszFieldName, String &value) const
{
    return _impl.findFieldValue (pszFieldName, value);
}

