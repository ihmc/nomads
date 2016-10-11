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

#include "SQLAVList.h"

#include "DSSFLib.h"

#include "InstrumentedReader.h"
#include "Logger.h"
#include "NLFLib.h"
#include "Writer.h"

#include "StrClass.h"
#include "MetadataConfiguration.h"

#include <stdlib.h>
#include <string.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

MetaData::MetaData (const MetadataFieldInfo ** const pMetadataFieldInfos,
                    uint16 ui16MetadataFieldsNumber,
                    uint16 ui16MetadataNotWritten)
    : _pMetadataFieldInfos (pMetadataFieldInfos),
      _ui16MetadataFieldsNumber (ui16MetadataFieldsNumber),
      _ui16MetadataNotWritten (ui16MetadataNotWritten)
{
    _ppszMetaDataFieldsValues = (char **) calloc (_ui16MetadataFieldsNumber, sizeof (char *));
}

MetaData::~MetaData()
{
    if (_ppszMetaDataFieldsValues != NULL) {
        for (int i = 0; i < _ui16MetadataFieldsNumber; i ++) {
            if (_ppszMetaDataFieldsValues[i] != NULL) {
                free (_ppszMetaDataFieldsValues[i]);
                _ppszMetaDataFieldsValues[i] = NULL;
            }
        }
        free (_ppszMetaDataFieldsValues);
        _ppszMetaDataFieldsValues = NULL;
    }
}

MetaData * MetaData::clone()
{
    MetaData *pCopy = new MetaData (_pMetadataFieldInfos, _ui16MetadataFieldsNumber,
                                    _ui16MetadataNotWritten);
    if (pCopy != NULL) {
        for (int i = 0; i < _ui16MetadataFieldsNumber; i++) {
            pCopy->setFieldValue (_pMetadataFieldInfos[i]->_sFieldName, _ppszMetaDataFieldsValues[i]);
        }
    }
    return pCopy;
}

const char * MetaData::toString()
{
    NOMADSUtil::String out;
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
        out += _pMetadataFieldInfos[i]->_sFieldName;
        out += "\t";
        out += (_ppszMetaDataFieldsValues[i] != NULL ? _ppszMetaDataFieldsValues[i] : "NULL");
        out += "\t\t\t";
        out += (_pMetadataFieldInfos[i]->_bVolatile ? "volatile" : "");
        out += "\t\t\t";
        out += (i < _ui16MetadataNotWritten ? "transient" : "");
        out += "\n";
    }
    return out.r_str();
}

int MetaData::setFieldsValues (SQLAVList *pFieldsValues)
{
    for (unsigned int i = 0; i < pFieldsValues->getLength(); i++) {
        int rc = setFieldValue (pFieldsValues->getAttribute (i),
                                pFieldsValues->getValueByIndex (i));
        if (rc < 0) {
            return rc;
        }
    }
    return 0;
}

int MetaData::setFieldValue (const char *pszAttribute, const char *pszValue)
{
    int rc;
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
        if (1 == (_pMetadataFieldInfos[i]->_sFieldName == pszAttribute)) {
            if ((rc = setFieldValueInternal (_ppszMetaDataFieldsValues[i], pszValue)) < 0) {
                return rc;
            }
            break;
        }
    }
    return 0;
}

int MetaData::findFieldValue (const char *pszFieldName, const char **ppszValue) const
{
    if (pszFieldName == NULL || ppszValue == NULL) {
        return -1;
    }
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
        if (1 == (_pMetadataFieldInfos[i]->_sFieldName == pszFieldName)) {
            if (_ppszMetaDataFieldsValues[i] == NULL) {
                *ppszValue = NULL;
                return 1;
            }
            (*ppszValue) = (const char *) _ppszMetaDataFieldsValues[i];
            return 0;
        }
    }
    return -2;
}

int MetaData::setFieldValueInternal (char *&pszMetaDataFieldOldValue, const char *pszMetaDataFieldNewValue)
{
    if (pszMetaDataFieldOldValue != NULL) {
        free (pszMetaDataFieldOldValue);
    }
    if ((pszMetaDataFieldNewValue == NULL) ||
        (0 == strcmp (pszMetaDataFieldNewValue, SQLAVList::UNKNOWN))) {
        pszMetaDataFieldOldValue = NULL;
    }
    else {
        pszMetaDataFieldOldValue = strDup (pszMetaDataFieldNewValue);
        if (pszMetaDataFieldOldValue == NULL) {
            return -1;
        }
    }
    return 0;
}

int MetaData::resetFieldValue (const char *pszFieldName)
{
    if (pszFieldName == NULL) {
        return -1;
    }
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
        if (1 == (_pMetadataFieldInfos[i]->_sFieldName == pszFieldName)) {
            if (_ppszMetaDataFieldsValues[i] != NULL) {
                free (_ppszMetaDataFieldsValues[i]);
                _ppszMetaDataFieldsValues[i] = NULL;
            }
        }
    }
    return 0;
}

int64 MetaData::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }

    InstrumentedReader ir (pReader, false);
    for (uint16 i = _ui16MetadataNotWritten; i < _ui16MetadataFieldsNumber; i++) {
        if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::TEXT)) {
            if (ui32MaxSize < (ir.getBytesRead() + 2)) {
                return -2;
            }
            uint16 ui16TxtLength = 0;
            int rc = ir.read16 (&ui16TxtLength);
            if (rc < 0) {
                return rc;
            }
            if (_ppszMetaDataFieldsValues[i] != NULL) {
                free (_ppszMetaDataFieldsValues[i]);
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            if (ui16TxtLength == 0) {
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            else {
                if (ui32MaxSize < (ir.getBytesRead() + ui16TxtLength)) {
                    return -3;
                }
                _ppszMetaDataFieldsValues[i] = static_cast<char *>(calloc (ui16TxtLength + 1, sizeof(char)));
                rc = ir.readBytes (_ppszMetaDataFieldsValues[i], ui16TxtLength);
                if (rc < 0) {
                    free (_ppszMetaDataFieldsValues[i]);
                    _ppszMetaDataFieldsValues[i] = NULL;
                    return rc;
                }
                _ppszMetaDataFieldsValues[i][ui16TxtLength] = '\0';
            }
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER8)) {
            int8 value;
            if (ui32MaxSize < (ir.getBytesRead() + 1)) {
                return -4;
            }
            int rc = ir.read8 (&value);
            if (_ppszMetaDataFieldsValues[i] != NULL) {
                free (_ppszMetaDataFieldsValues[i]);
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            if (rc < 0) {
                _ppszMetaDataFieldsValues[i] = NULL;
                return rc;
            }
            if (value == -1) {
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            else {
                char buffer[60];
                sprintf (buffer, "%d", value);
                _ppszMetaDataFieldsValues[i] = strDup (buffer);
            }
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER16)) {
            int16 value;
            if (ui32MaxSize < (ir.getBytesRead() + 2)) {
                return -5;
            }
            int rc = ir.read16 (&value);
            if (_ppszMetaDataFieldsValues[i] != NULL) {
                free (_ppszMetaDataFieldsValues[i]);
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            if (rc < 0) {
                _ppszMetaDataFieldsValues[i] = NULL;
                return rc;
            }
            if (value == -1) {
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            else {
               char buffer[60];
               sprintf (buffer, "%hd", value);
               _ppszMetaDataFieldsValues[i] = strDup (buffer);
            }
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER32)) {
            int32 value;
            if (ui32MaxSize < (ir.getBytesRead() + 4)) {
                return -6;
            }
            int rc = ir.read32 (&value);
            if (_ppszMetaDataFieldsValues[i] != NULL) {
                free(_ppszMetaDataFieldsValues[i]);
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            if (rc < 0) {
                _ppszMetaDataFieldsValues[i] = NULL;
                return rc;
            }
            if (value == -1) {
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            else {
               char buffer[60];
               sprintf (buffer, "%d", value);
               _ppszMetaDataFieldsValues[i] = strDup (buffer);
            }
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER64)) {
            int64 value;
            if (ui32MaxSize < (ir.getBytesRead() + 8)) {
                return -7;
            }
            int rc = ir.read64 (&value);
            if (_ppszMetaDataFieldsValues[i] != NULL) {
                free (_ppszMetaDataFieldsValues[i]);
            }
            if (rc < 0) {
                _ppszMetaDataFieldsValues[i] = NULL;
                return rc;
            }
            if (value == -1) {
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            else {
                char buffer[60];
                sprintf(buffer, "%lld", value);
               _ppszMetaDataFieldsValues[i] = strDup (buffer);
            }

        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::FLOAT)) {
            float fValue;
            if (ui32MaxSize < (ir.getBytesRead() + 4)) {
                return -8;
            }
            int rc = ir.read32 (&fValue);
            if (_ppszMetaDataFieldsValues[i] != NULL) {
                free (_ppszMetaDataFieldsValues[i]);
            }
            if (rc < 0) {
                _ppszMetaDataFieldsValues[i] = NULL;
                return rc;
            }
            if (fValue == -1) {
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            else {
               char buffer[60];
               sprintf (buffer, "%f", fValue);
               _ppszMetaDataFieldsValues[i] = strDup (buffer);
            }
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::DOUBLE)) {
            double dValue;
            if (ui32MaxSize < (ir.getBytesRead() + 8)) {
                return -9;
            }
            int rc = ir.read64 (&dValue);
            if (_ppszMetaDataFieldsValues[i] != NULL) {
                free(_ppszMetaDataFieldsValues[i]);
            }
            if (rc < 0) {
                _ppszMetaDataFieldsValues[i] = NULL;
                return rc;
            }
            if (dValue == -1) {
                _ppszMetaDataFieldsValues[i] = NULL;
            }
            else {
               char buffer[60];
               sprintf (buffer, "%f", dValue);
               _ppszMetaDataFieldsValues[i] = strDup (buffer);
            }
        }
    }
    return ir.getBytesRead();
}

int64 MetaData::write (Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return -1;
    }
                            // for row each field has a 2 bytes long length
    uint32 totLength = 0;   // count the amount of bytes written
    int rc;
    for (int16 i = MetadataConfiguration::getConfiguration()->getMetadataNotWritten(); getFieldType (i) != NULL; i++) {
        rc = MetadataInterface::write (i, pWriter, ui32MaxSize, totLength);
        if (rc < 0) {
            return rc;
        }
    }
    return totLength;
}

const char * MetaData::getFieldName (unsigned int uiIndex) const
{
    if ((_pMetadataFieldInfos == NULL) || (uiIndex >= _ui16MetadataFieldsNumber)) {
        return NULL;
    }
    return  _pMetadataFieldInfos[uiIndex]->_sFieldName.c_str();
}

const char * MetaData::getFieldType (unsigned int uiIndex) const
{
    if ((_pMetadataFieldInfos == NULL) || (uiIndex >= _ui16MetadataFieldsNumber)) {
        return NULL;
    }
    return  _pMetadataFieldInfos[uiIndex]->_sFieldType.c_str();
}

const char * MetaData::getFieldValueByIndex (unsigned int uiIndex) const
{
    if ((_ppszMetaDataFieldsValues == NULL) || (uiIndex >= _ui16MetadataFieldsNumber)) {
        return NULL;
    }
    return  _ppszMetaDataFieldsValues[uiIndex];
}

int64 MetaData::getWriteLength() const
{
    int64 length = 0;
    for (uint16 i = _ui16MetadataNotWritten; i < _ui16MetadataFieldsNumber; i ++) {
        if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::TEXT)) {
            length += 2;        // strings are prefixed by their length
            if (_ppszMetaDataFieldsValues[i] != NULL) {
                length += strlen (_ppszMetaDataFieldsValues[i]);
            }
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER8)) {
            length += 1;
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER16)) {
            length += 2;
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER32)) {
            length += 4;
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER64)) {
            length += 8;
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::FLOAT)) {
            length += 4;
        }
        else if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::DOUBLE)) {
            length += sizeof (double);
        }
    }
    return length;
}

bool MetaData::isFieldAtIndexUnknown (unsigned int uiIndex) const
{
    if (uiIndex >= _ui16MetadataFieldsNumber) {
        return false;
    }
    return  MetadataInterface::isFieldValueUnknown (_ppszMetaDataFieldsValues[uiIndex]);
}

bool MetadataUtils::refersToDataFromSource (MetaData *pMetadata, const char *pszSource)
{
    char *pszReferredDataId = NULL;
    if (0 != pMetadata->getFieldValue (MetaData::REFERS_TO, &pszReferredDataId) ||
        pszReferredDataId == NULL) {
        return false;
    }
    const String dataPublisherId (extractSenderNodeIdFromKey (pszReferredDataId));
    const bool bRet = (dataPublisherId.length () > 0 ? (dataPublisherId == pszSource) : false);
    free (pszReferredDataId);
    return bRet;
}

