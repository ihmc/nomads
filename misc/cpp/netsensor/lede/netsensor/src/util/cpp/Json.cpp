/*
 * Json.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#include "Json.h"

#include "CompressedReader.h"
#include "CompressedWriter.h"
#include "cJSON.h"
#include "NLFLib.h"

#include "Writer.h"

#include <stdlib.h>

using namespace NOMADSUtil;

namespace JSON
{
    cJSON * fromString (const char *pszJson, int type)
    {
        if (pszJson == NULL) {
            switch (type) {
                case cJSON_Array:
                    return cJSON_CreateArray();
                case cJSON_Object:
                    return cJSON_CreateObject();
                default:
                    return NULL;
            }
        }

        cJSON *pRoot = cJSON_Parse (pszJson);
        if (pRoot == NULL) {
            return NULL;
        }
        switch (type) {
            case cJSON_Array:
            case cJSON_Object:
                if (pRoot->type == type)
                    break;
            default:
                delete pRoot;
                pRoot = NULL;
        }
        return pRoot;
    }

    int inputSanityCheck (cJSON *pRoot, const char *pszName)
    {
        if ((pRoot == NULL) || (pszName == NULL) || (pRoot->type != cJSON_Object)) {
            return -1;
        }
        return 0;
    }

    int valueSanityCheck (cJSON *pBody, int iExpectedType1, int iExpectedType2)
    {
        if ((pBody == NULL) || ((pBody->type != iExpectedType1) && (pBody->type != iExpectedType2))) {
            return -3;
        }
        return 0;
    }

    int valueSanityCheck (cJSON *pBody, int iExpectedType)
    {
        return valueSanityCheck (pBody, iExpectedType, iExpectedType);
    }

    template<typename T>
    int getIntegralNumber (cJSON *pRoot, const char *pszName, T &val, bool bSigned)
    {
        if (inputSanityCheck (pRoot, pszName) < 0) {
            return -1;
        }
        cJSON *pBody = cJSON_GetObjectItem (pRoot, pszName);
        if (valueSanityCheck (pBody, cJSON_Number) < 0) {
            return -2;
        }
        int exponent = sizeof (T) * 8;
        if (bSigned) {
            exponent = exponent - 1;
        }
        const double largestNumber = pow (2, exponent) - 1;
        if (pBody->valuedouble > largestNumber) {
            return -3;
        }
        val = pBody->valuedouble;
        if (pBody->valuedouble - val >= 0.5) {
            // Properly round up instead of blindly truncate
            val = val + 1;
        }
        return 0;
    }

    int readCompressedString (String &s, Reader *pReader)
    {
        if (pReader == NULL) {
            return -1;
        }
        uint32 ui32 = 0U;
        if (pReader->read32 (&ui32) < 0) {
            return -2;
        }
        if (ui32 == 0) {
            return 0;
        }
        char *pszJson = static_cast<char *>(calloc (ui32 + 1, sizeof (char)));
        if (pszJson == NULL) {
            return -3;
        }
        CompressedReader cr (pReader, false, false);
        if (cr.readBytes (pszJson, ui32) < 0) {
            free (pszJson);
            return -4;
        }
        s = pszJson;
        free (pszJson);
        return 0;
    }

    int writeCompressedString (const String &s, Writer *pWriter)
    {
        if (pWriter == NULL) {
            return -1;
        }
        uint32 ui32 = s.length();
        // Write length of the uncompressed string
        if (pWriter->write32 (&ui32) < 0) {
            return -2;
        }
        CompressedWriter cw (pWriter);
        if ((cw.writeBytes (s, s.length()) < 0) || (cw.flush() < 0)) {
            return -3;
        }
        return 0;
    }
}

//---------------------------------------------------------

Json::Json (cJSON *pRoot, bool bDeallocate)
    : _bDeallocate (bDeallocate),
      _pRoot (pRoot)
{
}

Json::~Json (void)
{
    if ((_pRoot != NULL) && _bDeallocate) {
        cJSON_Delete (_pRoot);
        _pRoot = NULL;
    }
}

int Json::init (const char *pszJson)
{
    if (pszJson == NULL) {
        return -1;
    }
    if (_pRoot != NULL) {
        cJSON_Delete (_pRoot);
    }
    _pRoot = cJSON_Parse (pszJson);
    if (_pRoot == NULL) {
        return -2;
    }
    return 0;
}

Json * Json::clone (void) const
{
    static const int RECURSE = 1;
    cJSON *pRoot = cJSON_Duplicate (_pRoot, RECURSE);
    if (pRoot == NULL) {
        return NULL;
    }
    switch (pRoot->type) {
        case cJSON_Array:
            return new JsonArray (pRoot, true);
        case cJSON_Object:
            return new JsonObject (pRoot, true);
        default:
            cJSON_Delete (pRoot);
            return NULL;
    }
}

const JsonArray * Json::getArray (const char *pszName) const
{
    return getArrayReference (pszName);
}

JsonArray * Json::getArrayReference (const char *pszName) const
{
    if (_pRoot == NULL) {
        return NULL;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (pBody == NULL) {
        return NULL;
    }
    if (pBody->type != cJSON_Array) {
        return NULL;
    }
    return new JsonArray (pBody, false);
}

JsonObject * Json::getObject (const char *pszName) const
{
    if (_pRoot == NULL) {
        return NULL;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (pBody == NULL) {
        return NULL;
    }
    if (pBody->type != cJSON_Object) {
        return NULL;
    }
    JsonObject *pRet = new JsonObject (pBody, false);
    if (pRet == NULL) {
        return NULL;
    }
    return pRet;
}

int Json::getSize (void) const
{
    if (_pRoot == NULL) {
        return 0;
    }
    return cJSON_GetArraySize (_pRoot);
}

cJSON * Json::relinquishCJson (void)
{
    cJSON *proot = _pRoot;
    _pRoot = NULL;
    return proot;
}

//---------------------------------------------------------

JsonArray::JsonArray (const char *pszJson)
    : Json (JSON::fromString (pszJson, cJSON_Array), true)
{
}

JsonArray::JsonArray (cJSON *pRoot, bool bDeallocate)
    : Json (pRoot, bDeallocate)
{
}

JsonArray::~JsonArray (void)
{
}

int JsonArray::setObject (JsonObject *pValue)
{
    if (pValue == NULL) {
        return -1;
    }
    cJSON_AddItemToArray (_pRoot, pValue->relinquishCJson());
    return 0;
}

int JsonArray::addObject (JsonObject *pValue)
{
    if (_pRoot == NULL) {
        return -1;
    }
    if (pValue == NULL) {
        return -2;
    }
    cJSON_AddItemToArray (_pRoot, pValue->relinquishCJson());
    return 0;
}

int JsonArray::addString (const char *pszValue)
{
    if (_pRoot == NULL) {
        return -1;
    }
    if (pszValue == NULL) {
        return -2;
    }
    cJSON *pElement = cJSON_CreateString (pszValue);
    if (pElement == NULL) {
        return -3;
    }
    cJSON_AddItemToArray (_pRoot, pElement);
    return 0;
}

int JsonArray::addNumber (uint32 ui32Value)
{
    if (_pRoot == NULL) {
        return -1;
    }
    cJSON *pElement = cJSON_CreateNumber (ui32Value);
    if (pElement == NULL) {
        return -2;
    }
    cJSON_AddItemToArray (_pRoot, pElement);
    return 0;
}

JsonObject * JsonArray::getObject (int iIdx) const
{
    if (_pRoot == NULL) {
        return NULL;
    }
    cJSON *pBody = cJSON_GetArrayItem (_pRoot, iIdx);
    if (pBody == NULL) {
        return NULL;
    }
    if (pBody->type != cJSON_Object) {
        return NULL;
    }
    JsonObject *pRet = new JsonObject (pBody, false);
    if (pRet == NULL) {
        return NULL;
    }
    return pRet;
}

int JsonArray::getString (int iIdx, NOMADSUtil::String &sVal) const
{
    if (_pRoot == NULL) {
        return -1;
    }
    cJSON *pBody = cJSON_GetArrayItem (_pRoot, iIdx);
    if (pBody == NULL) {
        return -2;
    }
    if (pBody->type != cJSON_String) {
        return -3;
    }
    sVal = pBody->valuestring;
    return 0;
}

//---------------------------------------------------------

JsonObject::JsonObject (const char *pszJson)
    : Json (JSON::fromString (pszJson, cJSON_Object), true)
{
}

JsonObject::JsonObject (cJSON *pRoot, bool bDeallocate)
    : Json (pRoot, bDeallocate)
{
}

JsonObject::~JsonObject (void)
{
}

int JsonObject::init (const char *pszJson)
{
    if (_pRoot != NULL) {
        cJSON_Delete (_pRoot);
    }
    _pRoot = JSON::fromString (pszJson, cJSON_Object);
    return 0;
}

void JsonObject::clear (void)
{
    if (_pRoot != NULL) {
        String objectName (_pRoot->string);
        cJSON_Delete (_pRoot);
        //_pRoot = JSON::fromString (objectName, cJSON_Object);
    }
}

int JsonObject::setObject (const char *pszName, Json *pValue)
{
    if ((pszName == NULL) || (pValue == NULL)) {
        return -1;
    }
    cJSON_AddItemToObject (_pRoot, pszName, pValue->relinquishCJson());
    return 0;
}

int JsonObject::setObject (const char *pszName, const Json *pValue)
{
    if ((pszName == NULL) || (pValue == NULL)) {
        return -1;
    }
    Json *pCpy = pValue->clone();
    if (pCpy == NULL) {
        return -2;
    }
    int rc = setObject (pszName, pCpy);
    delete pCpy;
    return rc;
}

int JsonObject::setString (const char *pszName, const char *pszValue)
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (pBody == NULL) {
        cJSON_AddStringToObject (_pRoot, pszName, pszValue);
        return 0;
    }
    if (JSON::valueSanityCheck (pBody, cJSON_String) < 0) {
        return -2;
    }
    if (pBody->valuestring != NULL) {
        free (pBody->valuestring);
    }
    pBody->valuestring = strDup (pszValue);
    if (pBody->valuestring == NULL) {
        return -3;
    }
    return 0;
}

int JsonObject::setStrings (const char *pszName, const char **ppszValues, int i)
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON_DeleteItemFromObject (_pRoot, pszName);
    cJSON *pBody = cJSON_CreateStringArray (ppszValues, i);
    if (pBody == NULL) {
        return -2;
    }
    if (pBody->type != cJSON_Array) {
        return -3;
    }
    cJSON_AddItemToObject (_pRoot, pszName, pBody);
    return 0;
}

int JsonObject::setNumber (const char *pszName, int iValue)
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (pBody == NULL) {
        cJSON_AddNumberToObject (_pRoot, pszName, iValue);
        return 0;
    }
    if (JSON::valueSanityCheck (pBody, cJSON_Number) < 0) {
        return -2;
    }
    cJSON_SetNumberValue (pBody, iValue);
    return 0;
}

int JsonObject::setNumber (const char *pszName, uint32 ui32Value)
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (pBody == NULL) {
        cJSON_AddNumberToObject (_pRoot, pszName, ui32Value);
        return 0;
    }
    if (JSON::valueSanityCheck (pBody, cJSON_Number) < 0) {
        return -2;
    }
    cJSON_SetNumberValue (pBody, ui32Value);
    return 0;
}

int JsonObject::setNumber (const char *pszName, uint64 ui64Value)
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (pBody == NULL) {
        cJSON_AddNumberToObject (_pRoot, pszName, ui64Value);
        return 0;
    }
    if (JSON::valueSanityCheck (pBody, cJSON_Number) < 0) {
        return -2;
    }
    cJSON_SetNumberValue (pBody, ui64Value);
    return 0;
}

int JsonObject::setNumber (const char *pszName, int64 i64Value)
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (pBody == NULL) {
        cJSON_AddNumberToObject (_pRoot, pszName, i64Value);
        return 0;
    }
    if (JSON::valueSanityCheck (pBody, cJSON_Number) < 0) {
        return -2;
    }
    cJSON_SetNumberValue (pBody, i64Value);
    return 0;
}

int JsonObject::setNumber (const char *pszName, double dValue)
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (pBody == NULL) {
        cJSON_AddNumberToObject (_pRoot, pszName, dValue);
        return 0;
    }
    if (JSON::valueSanityCheck (pBody, cJSON_Number) < 0) {
        return -2;
    }
    cJSON_SetNumberValue (pBody, dValue);
    return 0;
}

int JsonObject::setBoolean (const char *pszName, bool bValue)
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (pBody == NULL) {
        bValue ? cJSON_AddTrueToObject (_pRoot, pszName) :
            cJSON_AddFalseToObject (_pRoot, pszName);
        return 0;
    }
    if (JSON::valueSanityCheck (pBody, cJSON_True, cJSON_False) < 0) {
        return -2;
    }
    pBody->type = bValue ? cJSON_True : cJSON_False;
    return 0;
}

int JsonObject::removeValue (const char *pszName)
{
    if (pszName == NULL) {
        return -1;
    }
    cJSON_DeleteItemFromObject (_pRoot, pszName);
    return 0;
}

int JsonObject::getBoolean (const char *pszName, bool &bVal) const
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (JSON::valueSanityCheck (pBody, cJSON_True, cJSON_False) < 0) {
        return -2;
    }
    bVal = (pBody->type == cJSON_True);
    return 0;
}

int JsonObject::getNumber (const char *pszName, int &iVal) const
{
    return JSON::getIntegralNumber<int> (_pRoot, pszName, iVal, true);
}

int JsonObject::getNumber (const char *pszName, uint16 &ui16Val) const
{
    return JSON::getIntegralNumber<uint16> (_pRoot, pszName, ui16Val, false);
}

int JsonObject::getNumber (const char *pszName, uint32 &ui32Val) const
{
    return JSON::getIntegralNumber<uint32> (_pRoot, pszName, ui32Val, false);
}

int JsonObject::getNumber (const char *pszName, uint64 &ui64Val) const
{
    return JSON::getIntegralNumber<uint64> (_pRoot, pszName, ui64Val, false);
}

int JsonObject::getNumber (const char *pszName, int64 &i64Val) const
{
    return JSON::getIntegralNumber<int64> (_pRoot, pszName, i64Val, true);
}

int JsonObject::getNumber (const char *pszName, double &dVal) const
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (JSON::valueSanityCheck (pBody, cJSON_Number) < 0) {
        return -2;
    }
    dVal = pBody->valuedouble;
    return 0;
}

int JsonObject::getString (const char *pszName, NOMADSUtil::String &sVal) const
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (JSON::valueSanityCheck (pBody, cJSON_String) < 0) {
        return -2;
    }
    if (pBody->valuestring == NULL) {
        return -3;
    }
    sVal = pBody->valuestring;
    return 0;
}

int JsonObject::getAsString (const char *pszName, NOMADSUtil::String &sVal) const
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return -1;
    }
    cJSON *pBody = cJSON_GetObjectItem (_pRoot, pszName);
    if (pBody == NULL) {
        return -2;
    }
    switch (pBody->type) {
        case cJSON_False: sVal = "false"; break;
        case cJSON_True: sVal = "true"; break;
        case cJSON_NULL: break;
        case cJSON_Number: {
            char buf[128];
            snprintf (buf, 128, "%g", pBody->valuedouble);
            sVal = buf;
            break;
        }
        case cJSON_String: sVal = pBody->valuestring; return 0;
        case cJSON_Array:
        case cJSON_Object: {
            char *pszJson = cJSON_Print (_pRoot);
            if (pszJson != NULL) {
                sVal = pszJson;
                free (pszJson);
            }
            break;
        }
        default:
            return -1;
    }
    return 0;
}

bool JsonObject::hasObject (const char *pszName) const
{
    if (JSON::inputSanityCheck (_pRoot, pszName) < 0) {
        return false;
    }
    return cJSON_GetObjectItem (_pRoot, pszName) != NULL;
}

String JsonObject::toString (bool bMinimize) const
{
    String sJson;
    if (_pRoot == NULL) {
        return sJson;
    }
    char *pszJson = cJSON_Print (_pRoot);
    if (pszJson == NULL) {
        return sJson;
    }
    if (bMinimize) {
        cJSON_Minify (pszJson);
    }
    sJson = pszJson;
    free (pszJson);
    return sJson;
}

int JsonObject::read (Reader *pReader, bool bCompressed)
{
    if (pReader == NULL) {
        return -1;
    }
    String json;
    if (bCompressed) {
        if (JSON::readCompressedString (json, pReader) < 0) {
            return -2;
        }
    }
    else {
        char *pszJson = NULL;
        if (pReader->readString (&pszJson) < 0) {
            return -3;
        }
        json = pszJson;
        free (pszJson);
    }
    if (init (json) < 0) {
        return -4;
    }
    return 0;
}

int JsonObject::write (Writer *pWriter, bool bCompressed)
{
    if (pWriter == NULL) {
        return -1;
    }
    const String s (toString (true));
    if (bCompressed) {
        if (JSON::writeCompressedString (s, pWriter) < 0) {
            return -2;
        }
    }
    else if (pWriter->writeString (s) < 0) {
        return -3;
    }
    return 0;
}

