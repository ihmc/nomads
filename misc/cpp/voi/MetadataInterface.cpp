/*
 * MetadataInterface.cpp
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

#include "MetadataInterface.h"

#include "Pedigree.h"

#include "NLFLib.h"

#include <stdlib.h>
#include <string.h>

using namespace IHMC_VOI;
using namespace NOMADSUtil;

const String PredictionClass::USEFUL = "Useful";
const String PredictionClass::NOT_USEFUL = "Not_Useful";

const String MetadataType::INTEGER8 = "INTEGER8";
const String MetadataType::INTEGER16 = "INTEGER16";
const String MetadataType::INTEGER32 = "INTEGER32";
const String MetadataType::INTEGER64 = "INTEGER64";
const String MetadataType::FLOAT = "FLOAT";
const String MetadataType::DOUBLE = "DOUBLE";
const String MetadataType::TEXT = "TEXT";

const String MetadataValue::UNKNOWN = "UNKNOWN";

const char * const MetadataInterface::NO_REFERRED_OBJECT = "NO_REF_OBJ";

const char * const MetadataInterface::JSON_METADATA_MIME_TYPE = "application/json";

const char * const MetadataInterface::MESSAGE_ID = "messageId";
const char * const MetadataInterface::REFERS_TO = "refersTo";
const char * const MetadataInterface::REFERRED_DATA_OBJECT_ID = "referredDataObjectId";
const char * const MetadataInterface::REFERRED_DATA_INSTANCE_ID = "referredDataInstanceId";
const char * const MetadataInterface::REFERRED_DATA_SIZE = "referredDataSize";
const char * const MetadataInterface::PREV_MSG_ID = "prevMsgId";
const char * const MetadataInterface::ANNOTATION_TARGET_OBJ_ID = "annotationTargetObjId";
const char * const MetadataInterface::VOI_LIST = "computedVoi";

const char * const MetadataInterface::CHECKSUM = "checksum";
const char * const MetadataInterface::DATA_CONTENT = "dataName";
const char * const MetadataInterface::DATA_FORMAT = "dataFormat";
const char * const MetadataInterface::CLASSIFICATION = "classification";
const char * const MetadataInterface::DESCRIPTION = "description";

const char * const MetadataInterface::LATITUDE = "latitude";
const char * const MetadataInterface::LONGITUDE = "longitude";

const char * const MetadataInterface::LEFT_UPPER_LATITUDE = "leftUpperLatitude";
const char * const MetadataInterface::LEFT_UPPER_LONGITUDE = "leftUpperLongitude";
const char * const MetadataInterface::RIGHT_LOWER_LATITUDE = "rightLowerLatitude";
const char * const MetadataInterface::RIGHT_LOWER_LONGITUDE = "rightLowerLongitude";
const char * const MetadataInterface::RADIUS = "radius";

const char * const MetadataInterface::SOURCE = "source";
const char * const MetadataInterface::SOURCE_TIME_STAMP = "sourceTimestamp";
const char * const MetadataInterface::RECEIVER_TIME_STAMP = "receiverTimestamp";
const char * const MetadataInterface::EXPIRATION_TIME = "expirationTime";
const char * const MetadataInterface::RELEVANT_MISSION = "relevantMissions";
const char * const MetadataInterface::LOCATION = "location";
const char * const MetadataInterface::PEDIGREE = "pedigree";
const char * const MetadataInterface::IMPORTANCE = "importance";
const char * const MetadataInterface::SOURCE_RELIABILITY = "sourceReliability";
const char * const MetadataInterface::INFORMATION_CONTENT = "informationContent";

const char * const MetadataInterface::TARGET_ID = "targetId";
const char * const MetadataInterface::TARGET_ROLE = "targetRole";
const char * const MetadataInterface::TARGET_TEAM = "targetTeam";

const char * const MetadataInterface::USAGE = "usage";
const char * const MetadataInterface::APPLICATION_METADATA = "applicationMetadata";
const char * const MetadataInterface::APPLICATION_METADATA_FORMAT = "applicationMetadataFormat";

const char * const MetadataInterface::RESOURCES = "resources";

const float MetadataInterface::LEFT_UPPER_LATITUDE_UNSET = -999999;
const float MetadataInterface::LEFT_UPPER_LONGITUDE_UNSET = -999999;
const float MetadataInterface::RIGHT_LOWER_LATITUDE_UNSET = -999999;
const float MetadataInterface::RIGHT_LOWER_LONGITUDE_UNSET = -999999;
const int64 MetadataInterface::SOURCE_TIME_STAMP_UNSET = 0;
const int64 MetadataInterface::EXPIRATION_TIME_UNSET = 1;
const double MetadataInterface::IMPORTANCE_UNSET = 99.0;

const NOMADSUtil::String MetadataInterface::UNKNOWN = "UNKNOWN";

MetadataInterface::MetadataInterface (void)
{
}

MetadataInterface::~MetadataInterface (void)
{
}

int MetadataInterface::getFieldValue (const char *pszFieldName, char **ppszValue) const
{
    String value;
    if (findFieldValue (pszFieldName, value) < 0 || (value.length() <= 0)) {
        return -2;
    }
    (*ppszValue) = value.r_str();
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, NOMADSUtil::String &value) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || (val.length() <= 0)) {
        return -1;
    }
    value = val;
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, int8 *pi8Value) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }

    int value;
    int rc = sscanf (val, "%d", &value);
    if (rc != 1) {
        return -2;
    }
    if (value > 0xFF) {
        return -3;
    }
    (*pi8Value) = (int8) value;
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, uint8 *pui8Value) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }
    unsigned int value;
    int rc = sscanf (val, "%u", &value);
    if (rc != 1) {
        return -2;
    }
    if (value > 0xFF) {
        return -3;
    }
    (*pui8Value) = (uint8) value;
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, int16 *pi16Value) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }
    (*pi16Value) = atoi (val);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, uint16 *pui16Value) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }
    // TODO: replace with atoi16
    unsigned short int usiValue;
    int rc = sscanf (val, "%hud", &usiValue);
    if (rc != 1) {
        return -2;
    }
    (*pui16Value) = (uint16) usiValue;
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, int32 *pi32Value) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }
    (*pi32Value) = atol (val);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, uint32 *pui32Value) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }
    (*pui32Value) = atoui32 (val);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, int64 *pi64value) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }
    (*pi64value) = atoi64 (val);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, uint64 *pui64Value) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }
    (*pui64Value) = atoui64 (val);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, float *pfValue) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }
    (*pfValue) = (float) atof (val);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, double *pdValue) const
{
    String val;
    if (findFieldValue (pszFieldName, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }
    (*pdValue) = atof (val);
    return 0;
}

int MetadataInterface::getFieldValue (Pedigree &pedigree) const
{
    String val;
    if (findFieldValue (PEDIGREE, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }

    pedigree += val; // += operator makes a copy
    return 0;
}

int MetadataInterface::getFieldValue (RankByTargetMap &rankByTargetMap) const
{
    String val;
    if (findFieldValue (VOI_LIST, val) < 0 || isFieldValueUnknown (val)) {
        return -1;
    }

    const String sRankByTarget (val);
    rankByTargetMap.fromString (sRankByTarget);
    return 0;
}

/*
int MetadataInterface::getFieldValue (PreviousMessageIds &previousMessageIds) const
{
    const char *pszPreviusMessages = NULL;
    if (findFieldValue (PREV_MSG_ID, &pszPreviusMessages) < 0 || pszPreviusMessages == NULL) {
        return -1;
    }

    previousMessageIds = pszPreviusMessages; // = operator makes a copy
    return 0;
}*/

bool MetadataInterface::isFieldUnknown (const char *pszFieldName) const
{
    if (pszFieldName == NULL) {
        return false;
    }
    String val;
    if (findFieldValue (pszFieldName, val) < 0) {
        return false;
    }
    return  MetadataInterface::isFieldValueUnknown (val);
}

bool MetadataInterface::isFieldValueUnknown (const char *pszValue)
{
    if (pszValue == NULL) {
        return true;
    }
    if (strlen (pszValue) == 0) {
        return true;
    }
    return (strcmp (pszValue, UNKNOWN) == 0);
}

int MetadataInterface::setMessageIdAndUsage (const char *pszMessageId)
{
    if (pszMessageId == NULL) {
        return -1;
    }
    setFieldValue (MESSAGE_ID, pszMessageId);
    setFieldValue (USAGE, UNKNOWN);
    return 0;
}

bool MetadataInterface::operator == (const MetadataInterface &rhsMetadataInterface)
{
    String lhsMetadataMsgId;
    String rhsMetadataMsgId;
    if ((getFieldValue (MESSAGE_ID, lhsMetadataMsgId) == 0) &&
        (rhsMetadataInterface.getFieldValue (MESSAGE_ID, rhsMetadataMsgId) == 0)) {
        return ((lhsMetadataMsgId == rhsMetadataMsgId) != 0);
    }
    return false;
}

