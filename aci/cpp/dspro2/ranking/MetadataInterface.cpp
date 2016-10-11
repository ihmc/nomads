/* 
 * MetadataInterface.cpp
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
 * Created on July 5, 2013, 11:52 AM
 */

#include "MetadataInterface.h"

#include "Pedigree.h"
#include "PreviousMessageIds.h"

#include "NLFLib.h"
#include "Writer.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

using namespace IHMC_ACI;
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

const char * const MetadataInterface::XML_METADATA_MIME_TYPE = "application/xml";

const char * const MetadataInterface::MESSAGE_ID = "Message_ID";
const char * const MetadataInterface::REFERS_TO = "Refers_To";
const char * const MetadataInterface::REFERRED_DATA_OBJECT_ID = "Referred_Data_Object_Id";
const char * const MetadataInterface::REFERRED_DATA_INSTANCE_ID = "Referred_Data_Instance_Id";
const char * const MetadataInterface::REFERRED_DATA_SIZE = "Referred_Data_Size";
const char * const MetadataInterface::PREV_MSG_ID = "Prev_Msg_ID";
const char * const MetadataInterface::ANNOTATION_TARGET_OBJ_ID = "Annotation_Target_Obj_Id";
const char * const MetadataInterface::RANKS_BY_TARGET_NODE_ID = "ComputedVOI";

const char * const MetadataInterface::CHECKSUM = "Checksum";
const char * const MetadataInterface::DATA_CONTENT = "Data_Content";
const char * const MetadataInterface::DATA_FORMAT = "Data_Format";
const char * const MetadataInterface::CLASSIFICATION = "Classification";
const char * const MetadataInterface::DESCRIPTION = "Description";

const char * const MetadataInterface::LATITUDE = "Latitude";
const char * const MetadataInterface::LONGITUDE = "Longitude";

const char * const MetadataInterface::LEFT_UPPER_LATITUDE = "Left_Upper_Latitude";
const char * const MetadataInterface::LEFT_UPPER_LONGITUDE = "Left_Upper_Longitude";
const char * const MetadataInterface::RIGHT_LOWER_LATITUDE = "Right_Lower_Latitude";
const char * const MetadataInterface::RIGHT_LOWER_LONGITUDE = "Right_Lower_Longitude";
const char * const MetadataInterface::SOURCE = "Source";
const char * const MetadataInterface::SOURCE_TIME_STAMP = "Source_Time_Stamp";
const char * const MetadataInterface::RECEIVER_TIME_STAMP = "Receiver_Time_Stamp";
const char * const MetadataInterface::EXPIRATION_TIME = "Expiration_Time";
const char * const MetadataInterface::RELEVANT_MISSION = "Relevant_Missions";
const char * const MetadataInterface::LOCATION = "Location";
const char * const MetadataInterface::PEDIGREE = "Pedigree";
const char * const MetadataInterface::IMPORTANCE = "Importance";
const char * const MetadataInterface::SOURCE_RELIABILITY = "Source_Reliability";
const char * const MetadataInterface::INFORMATION_CONTENT = "Information_Content";
const char * const MetadataInterface::VOI_LIST = "VOI_List";

const char * const MetadataInterface::TARGET_ID = "Target_ID";
const char * const MetadataInterface::TARGET_ROLE = "Target_Role";
const char * const MetadataInterface::TARGET_TEAM = "Target_Team";

const char * const MetadataInterface::USAGE = "Usage";
const char * const MetadataInterface::APPLICATION_METADATA = "Application_Metadata";

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
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || pszValue == NULL) {
        return -2;
    }
    (*ppszValue) = strDup (pszValue);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, NOMADSUtil::String &value) const
{
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || pszValue == NULL) {
        return -1;
    }
    value = pszValue;
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, int8 *pi8Value) const
{
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || isFieldValueUnknown (pszValue)) {
        return -1;
    }
    int value;
    int rc = sscanf (pszValue, "%d", &value);
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
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || isFieldValueUnknown (pszValue)) {
        return -1;
    }
    unsigned int value;
    int rc = sscanf (pszValue, "%u", &value);
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
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || isFieldValueUnknown (pszValue)) {
        return -1;
    }
    (*pi16Value) = atoi (pszValue);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, uint16 *pui16Value) const
{
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || isFieldValueUnknown (pszValue)) {
        return -1;
    }
    // TODO: replace with atoi16
    unsigned short int usiValue;
    int rc = sscanf (pszValue, "%hud", &usiValue);
    if (rc != 1) {
        return -2;
    }
    (*pui16Value) = (uint16) usiValue;
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, int32 *pi32Value) const
{
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || isFieldValueUnknown (pszValue)) {
        return -1;
    }
    (*pi32Value) = atol (pszValue);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, uint32 *pui32Value) const
{
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || isFieldValueUnknown (pszValue)) {
        return -1;
    }
    (*pui32Value) = atoui32 (pszValue);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, int64 *pi64value) const
{
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || isFieldValueUnknown (pszValue)) {
        return -1;
    }
    (*pi64value) = atoi64 (pszValue);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, uint64 *pui64Value) const
{
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || isFieldValueUnknown (pszValue)) {
        return -1;
    }
    (*pui64Value) = atoui64 (pszValue);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, float *pfValue) const
{
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || isFieldValueUnknown (pszValue)) {
        return -1;
    }    
    (*pfValue) = (float) atof (pszValue);
    return 0;
}

int MetadataInterface::getFieldValue (const char *pszFieldName, double *pdValue) const
{
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0 || isFieldValueUnknown (pszValue)) {
        return -1;
    }
    (*pdValue) = atof (pszValue);
    return 0;
}

int MetadataInterface::getFieldValue (Pedigree &pedigree) const
{
    const char *pszPedigree = NULL;
    if (findFieldValue (PEDIGREE, &pszPedigree) < 0 || pszPedigree == NULL) {
        return -1;
    }

    pedigree += (pszPedigree); // += operator makes a copy
    return 0;
}

int MetadataInterface::getFieldValue (PreviousMessageIds &previousMessageIds) const
{
    const char *pszPreviusMessages = NULL;
    if (findFieldValue (PREV_MSG_ID, &pszPreviusMessages) < 0 || pszPreviusMessages == NULL) {
        return -1;
    }

    previousMessageIds = pszPreviusMessages; // = operator makes a copy
    return 0;
}

bool MetadataInterface::isFieldUnknown (const char *pszFieldName) const
{
    if (pszFieldName == NULL) {
        return false;
    }
    const char *pszValue = NULL;
    if (findFieldValue (pszFieldName, &pszValue) < 0) {
        return false;
    }
    return  MetadataInterface::isFieldValueUnknown (pszValue);
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

int MetadataInterface::setFixedMetadataFields (int64 i64ExpirationTime, int64 i64ReceiverTimeStamp,
                                               const char *pszSource,  int64 i64SourceTimeStamp,
                                               const char *pszNodeID)
{
    int rc = 0;
    const char *pszFieldName;
    for (unsigned int i = 0; (pszFieldName = getFieldName (i)) != NULL; i++) {
        if (strcmp (pszFieldName, RECEIVER_TIME_STAMP) == 0 &&
            isFieldAtIndexUnknown (i)) {
            char buf[22];
            rc = setFieldValue (pszFieldName, i64toa (buf, i64ReceiverTimeStamp));
        }
        else if (pszSource != NULL && strcmp (pszFieldName, SOURCE) == 0 && isFieldAtIndexUnknown (i)) {
            rc = setFieldValue (pszFieldName, pszSource);
        }
        else if (strcmp (pszFieldName, SOURCE_TIME_STAMP) == 0 && isFieldAtIndexUnknown (i)) {
            char buf[22];
            rc = setFieldValue (pszFieldName, i64toa (buf, i64SourceTimeStamp));
        }
        else if (strcmp (pszFieldName, EXPIRATION_TIME) == 0) {
            if (isFieldAtIndexUnknown (i)) {
                char buf[22];
                rc = setFieldValue (pszFieldName, i64toa (buf, i64ExpirationTime));    
            }
            else {
                int64 i64Value;
                getFieldValue (pszFieldName, &i64Value);
                if ((i64Value == 0) && (i64ExpirationTime != 0)) {
                    char buf[22];
                    rc = setFieldValue (pszFieldName, i64toa (buf, i64ExpirationTime));
                }
            }
        }
        else if (strcmp (pszFieldName, PEDIGREE) == 0) {
            if (pszSource == NULL) {
                rc = setFieldValue (pszFieldName, pszNodeID);
            }
            else if (0 != strcmp (pszSource, pszNodeID)) {
                // The message obviously comes from its source, therefore it does not
                // need to be inserted in the pedigree, therefore saving some space.
                Pedigree pedigree (pszSource);
                if (getFieldValue (pedigree) == 0) {
                    pedigree += pszNodeID;
                    rc = setFieldValue (pszFieldName, pedigree.toString());
                }
                else {
                    rc = setFieldValue (pszFieldName, pszNodeID);
                }
            }
        }

        if (rc != 0) {
            break;
        }
    }

    return rc;
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

int MetadataInterface::write (uint16 i, Writer *pWriter, uint32 ui32MaxSize, uint32 &totLength)
{
    if (pWriter == NULL) {
        return -1;
    }

    const char *pszFieldType = getFieldType (i);
    if (pszFieldType == NULL) {
        return -2;
    }

    int rc = 0;
    if ((0 == rc) && (0 == strcmp (pszFieldType, MetadataType::TEXT))) {
        rc = writeText (i, pWriter, ui32MaxSize, totLength);
    }
    else if ((0 == rc) && (0 == strcmp (pszFieldType, MetadataType::INTEGER8))) {
        rc = writeInt8 (i, pWriter, ui32MaxSize, totLength);
    }
    else if ((0 == rc) && (0 == strcmp (pszFieldType, MetadataType::INTEGER16))) {
        rc = writeInt16 (i, pWriter, ui32MaxSize, totLength);
    }
    else if ((0 == rc) && (0 == strcmp (pszFieldType, MetadataType::INTEGER32))) {
        rc = writeInt32 (i, pWriter, ui32MaxSize, totLength);
    }
    else if ((0 == rc) && (0 == strcmp (pszFieldType, MetadataType::INTEGER64))) {
        rc = writeInt64 (i, pWriter, ui32MaxSize, totLength);
    }
    else if ((0 == rc) && (0 == strcmp (pszFieldType, MetadataType::FLOAT))) {
        rc = writeFloat (i, pWriter, ui32MaxSize, totLength);
    }
    else if ((0 == rc) && (0 == strcmp (pszFieldType, MetadataType::DOUBLE))) {
        rc = writeDouble (i, pWriter, ui32MaxSize, totLength);
    }
    else {
        rc = -3;
    }

    return rc;
}

int MetadataInterface::writeText (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength)
{
    const char *pszValue = getFieldValueByIndex (i);
    bool bUnilimitedSize = (ui32MaxSize == 0);
    if (pszValue == NULL || (strcmp (pszValue, UNKNOWN) == 0)) {
        assert (strcmp (getFieldName (i), REFERS_TO) == 0 ? false : true);
        uint16 length = 0;
        if (!bUnilimitedSize && (ui32MaxSize < ui32TotLength + 2)) {
            return -2;
        }
        int rc = pWriter->write16 (&length);
        if (rc < 0) {
            return rc;
        }
        ui32TotLength += 2;
    }
    else {
        uint16 length = strlen (pszValue);
        if (!bUnilimitedSize && (ui32MaxSize < (uint32) (length + 2))) {
            return -3;
        }
        int rc = pWriter->write16 (&length);
        if (rc < 0) {
            return rc;
        }
        ui32TotLength += 2;
        rc = pWriter->writeBytes ((const char *) pszValue, length);
        if (rc < 0) {
            return rc;
        }
        ui32TotLength += length;
    }

    return 0;
}

int MetadataInterface::writeInt8 (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength)
{
    const char *pszValue = getFieldValueByIndex (i);
    bool bUnilimitedSize = (ui32MaxSize == 0);
    if (!bUnilimitedSize && (ui32MaxSize < (ui32TotLength + 1))) {
        return -4;
    }
    int8 value;
    if (pszValue == NULL || (strcmp (pszValue, UNKNOWN) == 0)) {
        value = -1;
    }
    else {
        value = atoi (pszValue);
    }
    int rc = pWriter->write8 (&value);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += 1;

    return 0;
}

int MetadataInterface::writeInt16 (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength)
{
    const char *pszValue = getFieldValueByIndex (i);
    bool bUnilimitedSize = (ui32MaxSize == 0);
    if (!bUnilimitedSize && (ui32MaxSize < (ui32TotLength + 2))) {
        return -5;
    }
    int16 value;
    if (pszValue == NULL || (strcmp (pszValue, UNKNOWN) == 0)) {
        value = -1;
    }
    else {
        value = atoi (pszValue);
    }
    int rc = pWriter->write16 (&value);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += 2;

    return 0;
}

int MetadataInterface::writeInt32 (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength)
{
    const char *pszValue = getFieldValueByIndex (i);
    bool bUnilimitedSize = (ui32MaxSize == 0);
    if (!bUnilimitedSize && (ui32MaxSize < (ui32TotLength + 4))) {
        return -6;
    }
    int32 value;
    if (pszValue == NULL || (strcmp (pszValue, UNKNOWN) == 0)) {
        value = -1;
    }
    else {
        value = atoi (pszValue);
    }
    int rc = pWriter->write32 (&value);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += 4;
    return 0;
}

int MetadataInterface::writeInt64 (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength)
{
    const char *pszValue = getFieldValueByIndex (i);
    bool bUnilimitedSize = (ui32MaxSize == 0);
    if (!bUnilimitedSize && (ui32MaxSize < (ui32TotLength + 8))) {
        return -7;
    }
    int64 value;
    if (pszValue == NULL || (strcmp (pszValue, UNKNOWN) == 0)) {
        value = -1;
    }
    else {
        value = atoi64 (pszValue);
    }
    int rc = pWriter->write64 (&value);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += 8;
    return 0;
}

int MetadataInterface::writeFloat (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength)
{
    const char *pszValue = getFieldValueByIndex (i);
    bool bUnilimitedSize = (ui32MaxSize == 0);
    if (!bUnilimitedSize && (ui32MaxSize < (ui32TotLength + sizeof (float)))) {
        return -8;
    }
    float fValue;
    if (pszValue == NULL || (strcmp (pszValue, UNKNOWN) == 0)) {
        fValue = -1.0f;
    }
    else {
        fValue = (float) atof (pszValue);
    }
    int rc = pWriter->write32 (&fValue);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += sizeof (float);
    return 0;
}

int MetadataInterface::writeDouble (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength)
{
    const char *pszValue = getFieldValueByIndex (i);
    bool bUnilimitedSize = (ui32MaxSize == 0);
    if (!bUnilimitedSize && (ui32MaxSize < (ui32TotLength + sizeof (double)))) {
        return -9;
    }
    double dValue;
    if (pszValue == NULL || (strcmp (pszValue, UNKNOWN) == 0)) {
        dValue = -1;
    }
    else {
        dValue = atof (pszValue);
    }
    int rc = pWriter->write64 (&dValue);
    if (rc < 0) {
        return rc;
    }
    ui32TotLength += sizeof (double);
    return 0;
}

