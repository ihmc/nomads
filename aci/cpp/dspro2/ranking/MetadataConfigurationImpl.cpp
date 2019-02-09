/*
 * MetaDataConfiguration.cpp
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

#include "MetadataConfigurationImpl.h"

#include "Defs.h"
#include "MetaData.h"

#include "C45AVList.h"

#include "AVList.h"
#include "BufferReader.h"
#include "ConfigManager.h"

#include "Logger.h"
#include "StringHashset.h"

#include "tinyxml.h"
#include "MetadataInterface.h"
#include "Json.h"
#include "cJSON.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace IHMC_C45;
using namespace NOMADSUtil;

const String MetadataConfigurationImpl::XML_HEADER = "<?xml version=\"1.0\"?>";
const String MetadataConfigurationImpl::XML_METADATA_ELEMENT = "Metadata";
const String MetadataConfigurationImpl::XML_FIELD_ELEMENT = "Field";
const String MetadataConfigurationImpl::XML_FIELD_NAME_ELEMENT = "FieldName";
const String MetadataConfigurationImpl::XML_FIELD_TYPE_ELEMENT = "FieldType";
const String MetadataConfigurationImpl::XML_FIELD_VALUE_ELEMENT = "FieldValue";

MetadataConfigurationImpl * MetadataConfigurationImpl::_pINSTANCE = nullptr;

MetadataFieldInfo::MetadataFieldInfo (const char *pszFieldName,
                                      const char *pszFieldType)
    : _bVolatile (false),
      _bNotNull (false),
      _bUsedInPolicies (false),
      _bUsedInLearning (false),
      _sFieldName (pszFieldName),
      _sFieldType (pszFieldType)
{
}

MetadataFieldInfo::~MetadataFieldInfo (void)
{
}

MetadataConfigurationImpl::MetadataConfigurationImpl (void)
    : //----------------------------------------------
      // NOT WRITTEN
      //----------------------------------------------
      _usMessageIDIndex (0),
      _usUsageIndex (_usMessageIDIndex+1),
      _usReceiverTimeStampIndex (_usUsageIndex+1),

      //----------------------------------------------
      // WRITTEN
      //----------------------------------------------
      _usReferredDataObjectIdIndex (_usReceiverTimeStampIndex+1),
      _usReferredDataInstanceIdIndex (_usReferredDataObjectIdIndex+1),
      _usPedigreeIndex (_usReferredDataInstanceIdIndex+1),
      _usRefersToIndex (_usPedigreeIndex+1),
      _usAnnotationTargetMsgIdIndex (_usRefersToIndex + 1),
      _usPrevMsgIdIndex (_usAnnotationTargetMsgIdIndex + 1),
      _usSourceIndex (_usPrevMsgIdIndex + 1),
      _usSourceTimeStampIndex (_usSourceIndex + 1),
      _usExpirationTimeIndex (_usSourceTimeStampIndex + 1),
      _usRelevantMissionIndex (_usExpirationTimeIndex + 1),
      _usLeftUpperLatitudeIndex (_usRelevantMissionIndex + 1),
      _usLeftUpperLongitudeIndex (_usLeftUpperLatitudeIndex + 1),
      _usRightLowerLatitudeIndex (_usLeftUpperLongitudeIndex + 1),
      _usRightLoweLongitudeIndex (_usRightLowerLatitudeIndex + 1),
      _usImportanceIndex (_usRightLoweLongitudeIndex + 1),
      _usSourceReliabilityIndex (_usImportanceIndex + 1),
      _usInformationContentIndex (_usSourceReliabilityIndex + 1),
      _usDataContentIndex (_usInformationContentIndex + 1),
      _usDataFomatIndex (_usDataContentIndex+1),
      _usTargetIdIndex (_usDataFomatIndex+1),
      _usTargetRoleIndex (_usTargetIdIndex+1),
      _usTargetTeamIndex (_usTargetRoleIndex+1),

      //----------------------------------------------
      // COUNTERS
      //----------------------------------------------
      _metadataNotWritten (_usReceiverTimeStampIndex+1),
      _usMetadataFixedFieldsNumber (_usTargetTeamIndex+1)
{
    _ui16MetadataFieldsNumber = _ui16VolatileMetadataNumber = _ui16MetadataLearningNumber = 0;
    _pAttributesValues = nullptr;
    _pMetadataFieldInfos = nullptr;
}

MetadataConfigurationImpl::~MetadataConfigurationImpl (void)
{
    if (_pMetadataFieldInfos != nullptr && _ui16MetadataFieldsNumber > 0) {
        for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
            delete _pMetadataFieldInfos[i];
        }
        free (_pMetadataFieldInfos);
    }
}

MetadataConfigurationImpl * MetadataConfigurationImpl::getConfiguration (void)
{
    if (_pINSTANCE == nullptr) {
        _pINSTANCE = new MetadataConfigurationImpl();
        _pINSTANCE->setFixedFields();
    }
    return _pINSTANCE;
}

MetadataConfigurationImpl * MetadataConfigurationImpl::getConfiguration (const char *pszXMLMetadataFields)
{
    if (pszXMLMetadataFields == nullptr) {
        return nullptr;
    }

    getConfiguration();

    // Parse XML document
    TiXmlDocument doc;
    doc.Parse (pszXMLMetadataFields);
    TiXmlElement *pRoot = doc.FirstChildElement (XML_METADATA_ELEMENT);
    if (pRoot == nullptr) {
        return nullptr;
    }
    int rc = _pINSTANCE->addCustomMetadata (pRoot);
    if (rc != 0) {
        checkAndLogMsg ("MetadataConfiguration::getConfiguration", Logger::L_SevereError,
                        "MetadataConfiguration could not be initialized correctly. returned %d\n", rc);
        delete _pINSTANCE;
        _pINSTANCE = nullptr;
    }

    return _pINSTANCE;
}

MetadataConfigurationImpl * MetadataConfigurationImpl::getExistingConfiguration (void)
{
    return _pINSTANCE;
}

unsigned int MetadataConfigurationImpl::getNumberOfLearningFields (void) const
{
    return _ui16MetadataLearningNumber;
}

unsigned int MetadataConfigurationImpl::getNumberOfFields (void) const
{
    return _ui16MetadataFieldsNumber;
}

String MetadataConfigurationImpl::getFieldName (unsigned int uiIndex) const
{
    String fieldName;
    if (uiIndex >= _ui16MetadataFieldsNumber) {
        return fieldName;
    }
    fieldName = _pMetadataFieldInfos[uiIndex]->_sFieldName;
    return fieldName;
}

bool MetadataConfigurationImpl::isLearningField (unsigned int uiIndex) const
{
    if (uiIndex >= _ui16MetadataFieldsNumber) {
        return false;
    }
    return _pMetadataFieldInfos[uiIndex]->_bUsedInLearning;
}

const char * MetadataConfigurationImpl::getFieldType (const char *pszFieldName) const
{
    if (pszFieldName == nullptr || _pMetadataFieldInfos == nullptr) {
        return nullptr;
    }

    for (unsigned int i = 0; i < _ui16MetadataFieldsNumber && _pMetadataFieldInfos[i] != nullptr; i++) {
        if (1 == (_pMetadataFieldInfos[i]->_sFieldName == pszFieldName)) {
            return _pMetadataFieldInfos[i]->_sFieldType.c_str();
        }
    }

    return nullptr;
}

bool MetadataConfigurationImpl::isStringFieldType (const char *pszFieldName) const
{
    const char *pszFieldType = getFieldType (pszFieldName);
    if (pszFieldType == nullptr) {
        return false;
    }
    return ((MetadataType::TEXT == pszFieldType) == 1);
}

void MetadataConfigurationImpl::getMetadataNotWritten (DArray2<String> &nonTransientAttributes) const
{
    for (uint16 i = 0; i < _metadataNotWritten; i++) {
        nonTransientAttributes[i] = _pMetadataFieldInfos[i]->_sFieldName;
    }
}

bool MetadataConfigurationImpl::hasField (const char *pszFieldName)
{
    if (pszFieldName == nullptr || _pMetadataFieldInfos == nullptr) {
        return false;
    }
    for (unsigned int i = 0; i < _ui16MetadataFieldsNumber && _pMetadataFieldInfos[i] != nullptr; i++) {
        if ((_pMetadataFieldInfos[i]->_sFieldName == pszFieldName) == 1) {
            return true;
        }
    }
    return false;
}

int MetadataConfigurationImpl::setMetadataFields (const char *pszXMLMetaDataValues)
{
    const char *pszMethodName = "MetadataConfiguration::setMetadataFields";
    if (pszXMLMetaDataValues == nullptr) {
        return -1;
    }
    TiXmlElement *pTxtField;
    TiXmlDocument xmlDoc;
    xmlDoc.Parse (pszXMLMetaDataValues);
    TiXmlElement *pRoot = xmlDoc.FirstChildElement (XML_METADATA_ELEMENT);
    if ((pRoot == nullptr) || ((pTxtField = pRoot->FirstChildElement (XML_FIELD_ELEMENT)) == nullptr)) {
        return -2;
    }
    AVList avlist;
    do {
        TiXmlElement *pTxtElement = pTxtField->FirstChildElement (XML_FIELD_NAME_ELEMENT);
        TiXmlElement *pTxtValue = pTxtField->FirstChildElement (XML_FIELD_VALUE_ELEMENT);
        if ((pTxtElement != nullptr) && (pTxtValue != nullptr)) {
            const String nameText (pTxtElement->GetText());
            if (nameText.length() <= 0) {
                pTxtField = pTxtField->NextSiblingElement();
                continue;
            }
            do {
                const String valueText (pTxtValue->GetText());
                if (valueText.length() <= 0) {
                    pTxtValue = pTxtValue->NextSiblingElement();
                    continue;
                }
                avlist.addPair (nameText, valueText);
                pTxtValue = pTxtValue->NextSiblingElement();
            } while (pTxtValue != nullptr);
            pTxtField = pTxtField->NextSiblingElement();
        }
    } while (pTxtField != nullptr);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "AVList at the end is:\n");
    for (unsigned int i = 0; i < avlist.getLength(); i++) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Attribute[%d] = <%s>, Value[%d] = <%s>\n",
                        i, avlist.getAttribute (i), i, avlist.getValueByIndex (i));
    }
    return setupClassifierConfiguration (&avlist);
}

int MetadataConfigurationImpl::addCustomMetadata (TiXmlElement *pRoot)
{
    const char *pszMethodName = "MetadataConfiguration::addCustomMetadata (1)";
    _ui16MetadataFieldsNumber = _usMetadataFixedFieldsNumber;
    TiXmlElement *pXmlField = pRoot->FirstChildElement (XML_FIELD_ELEMENT);
    if (pXmlField == nullptr) {
        return -1;
    }

    StringHashset currentFields;
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
        if (_pMetadataFieldInfos[i] != nullptr) {
            currentFields.put (_pMetadataFieldInfos[i]->_sFieldName);
        }
    }

    do {
        TiXmlElement *pXmlElName = pXmlField->FirstChildElement (XML_FIELD_NAME_ELEMENT);
        TiXmlElement *pXmlElType = pXmlField->FirstChildElement (XML_FIELD_TYPE_ELEMENT);
        if ((pXmlElName != nullptr) && (pXmlElType != nullptr)) {
            const String nameText (pXmlElName->GetText());
            const String typeText (pXmlElType->GetText());
            if ((nameText.length () <= 0) || (typeText.length() <= 0)) {
                pXmlField = pXmlField->NextSiblingElement();
                continue;
            }
            if (currentFields.containsKey (nameText)) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "trying to add duplicate field %s.\n",
                                nameText.c_str());
                pXmlField = pXmlField->NextSiblingElement ();
                continue;
            }

            String type (typeText);
            type.convertToUpperCase ();
            _ui16MetadataFieldsNumber++;
            _pMetadataFieldInfos = static_cast<MetadataFieldInfo **>(realloc (_pMetadataFieldInfos,
                                   _ui16MetadataFieldsNumber * sizeof (MetadataFieldInfo *)));
            if (_pMetadataFieldInfos == nullptr) {
                checkAndLogMsg (pszMethodName, memoryExhausted);
                return -2;
            }
            if ((type != MetadataType::INTEGER8) && (type != MetadataType::INTEGER16) &&
                (type != MetadataType::INTEGER32) && (type != MetadataType::INTEGER64) &&
                (type != MetadataType::FLOAT) && (type != MetadataType::DOUBLE) &&
                (type != MetadataType::TEXT)) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "unsupported data type for %s attribute: "
                    "%s. Setting it to %s.\n", nameText.c_str (), type.c_str (),
                    MetadataType::TEXT.c_str ());
                // Set "TEXT" as default type in case the specified
                // type doesn't match any type in MetadataType.
                type = MetadataType::TEXT;
            }
            _pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1] = new MetadataFieldInfo (nameText, type);
            if ((_pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1] == nullptr) ||
                (_pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1]->_sFieldName.length () <= 0)) {
                checkAndLogMsg ("MetadataConfiguration::addCustomMetadata (1)", memoryExhausted);
                return -3;
            }
            _pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1]->_bVolatile = ((_ui16MetadataFieldsNumber - 1U) == _usPedigreeIndex);

            pXmlField = pXmlField->NextSiblingElement();
        }
    } while (pXmlField != nullptr);

    return 0;
}

int MetadataConfigurationImpl::addCustomMetadata (AVList *pMetadataFields)
{
    if (pMetadataFields == nullptr) {
        return -1;
    }

    _ui16MetadataFieldsNumber = pMetadataFields->getLength() + _usMetadataFixedFieldsNumber;
    _pMetadataFieldInfos = static_cast<MetadataFieldInfo **>(realloc (_pMetadataFieldInfos, _ui16MetadataFieldsNumber * sizeof(MetadataFieldInfo *)));
    if (_pMetadataFieldInfos == nullptr) {
        checkAndLogMsg ("MetadataConfiguration::addCustomMetadata (2)", memoryExhausted);
        return -2;
    }

    for (uint16 i = _usMetadataFixedFieldsNumber; i < _ui16MetadataFieldsNumber; i++) {
        // Set Attribute Name
        _pMetadataFieldInfos[i] = new MetadataFieldInfo (pMetadataFields->getAttribute (i - _usMetadataFixedFieldsNumber),
                                                         pMetadataFields->getValueByIndex (i - _usMetadataFixedFieldsNumber));
        if ((_pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1] == nullptr) ||
            (_pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1]->_sFieldName.length() <= 0)) {
            checkAndLogMsg ("MetadataConfiguration::addCustomMetadata (2)", memoryExhausted);
            return -3;
        }
        _pMetadataFieldInfos[i]->_bVolatile = (i == _usPedigreeIndex);
    }

    return 0;
}

int MetadataConfigurationImpl::setFixedFields()
{
    _ui16MetadataFieldsNumber = _usMetadataFixedFieldsNumber;
    _pMetadataFieldInfos = static_cast<MetadataFieldInfo **>(calloc (_usMetadataFixedFieldsNumber, sizeof (MetadataFieldInfo *)));
    if (_pMetadataFieldInfos == nullptr) {
        checkAndLogMsg ("MetadataConfiguration::setFixedFields", memoryExhausted);
        return -1;
    }

    _pMetadataFieldInfos[_usMessageIDIndex] = new MetadataFieldInfo (MetadataInterface::MESSAGE_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usUsageIndex] = new MetadataFieldInfo (MetadataInterface::USAGE, MetadataType::INTEGER8);
    _pMetadataFieldInfos[_usReceiverTimeStampIndex] = new MetadataFieldInfo (MetadataInterface::RECEIVER_TIME_STAMP, MetadataType::INTEGER64);
    _pMetadataFieldInfos[_usReferredDataObjectIdIndex] = new MetadataFieldInfo (MetadataInterface::REFERRED_DATA_OBJECT_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usReferredDataInstanceIdIndex] = new MetadataFieldInfo (MetadataInterface::REFERRED_DATA_INSTANCE_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usPedigreeIndex] = new MetadataFieldInfo (MetadataInterface::PEDIGREE, MetadataType::TEXT);
    _pMetadataFieldInfos[_usRefersToIndex] = new MetadataFieldInfo (MetadataInterface::REFERS_TO, MetadataType::TEXT);
    _pMetadataFieldInfos[_usAnnotationTargetMsgIdIndex] = new MetadataFieldInfo (MetadataInterface::ANNOTATION_TARGET_OBJ_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usPrevMsgIdIndex] = new MetadataFieldInfo (MetadataInterface::PREV_MSG_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usSourceIndex] = new MetadataFieldInfo (MetadataInterface::SOURCE, MetadataType::TEXT);
    _pMetadataFieldInfos[_usSourceTimeStampIndex] = new MetadataFieldInfo (MetadataInterface::SOURCE_TIME_STAMP, MetadataType::INTEGER64);
    _pMetadataFieldInfos[_usExpirationTimeIndex] = new MetadataFieldInfo (MetadataInterface::EXPIRATION_TIME, MetadataType::INTEGER64);
    _pMetadataFieldInfos[_usRelevantMissionIndex] = new MetadataFieldInfo (MetadataInterface::RELEVANT_MISSION, MetadataType::TEXT);
    _pMetadataFieldInfos[_usLeftUpperLatitudeIndex] = new MetadataFieldInfo (MetadataInterface::LEFT_UPPER_LATITUDE, MetadataType::FLOAT);
    _pMetadataFieldInfos[_usLeftUpperLongitudeIndex] = new MetadataFieldInfo (MetadataInterface::LEFT_UPPER_LONGITUDE, MetadataType::FLOAT);
    _pMetadataFieldInfos[_usRightLowerLatitudeIndex] = new MetadataFieldInfo (MetadataInterface::RIGHT_LOWER_LATITUDE, MetadataType::FLOAT);
    _pMetadataFieldInfos[_usRightLoweLongitudeIndex] = new MetadataFieldInfo (MetadataInterface::RIGHT_LOWER_LONGITUDE, MetadataType::FLOAT);
    _pMetadataFieldInfos[_usImportanceIndex] = new MetadataFieldInfo (MetadataInterface::IMPORTANCE, MetadataType::DOUBLE);
    _pMetadataFieldInfos[_usSourceReliabilityIndex] = new MetadataFieldInfo (MetadataInterface::SOURCE_RELIABILITY, MetadataType::DOUBLE);
    _pMetadataFieldInfos[_usInformationContentIndex] = new MetadataFieldInfo (MetadataInterface::INFORMATION_CONTENT, MetadataType::DOUBLE);
    _pMetadataFieldInfos[_usDataContentIndex] = new MetadataFieldInfo (MetadataInterface::DATA_CONTENT, MetadataType::TEXT);
    _pMetadataFieldInfos[_usDataFomatIndex] = new MetadataFieldInfo (MetadataInterface::DATA_FORMAT, MetadataType::TEXT);
    _pMetadataFieldInfos[_usTargetIdIndex] = new MetadataFieldInfo (MetadataInterface::TARGET_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usTargetRoleIndex] = new MetadataFieldInfo (MetadataInterface::TARGET_ROLE, MetadataType::TEXT);
    _pMetadataFieldInfos[_usTargetTeamIndex] = new MetadataFieldInfo (MetadataInterface::TARGET_TEAM, MetadataType::TEXT);

    for (unsigned int i = 0; i < _usMetadataFixedFieldsNumber; i++) {
        if ((_pMetadataFieldInfos[i] == nullptr) || (_pMetadataFieldInfos[i]->_sFieldName.length() <= 0)) {
            checkAndLogMsg ("MetadataConfiguration::setFixedFields", memoryExhausted);
            //_usMetadataFixedFieldsNumber = i;
            return -2;
        }
    }

    _pMetadataFieldInfos[_usUsageIndex]->_bUsedInLearning = true;
    _ui16MetadataLearningNumber = 1;

    _pMetadataFieldInfos[_usMessageIDIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usReceiverTimeStampIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usPedigreeIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usSourceIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usSourceTimeStampIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usExpirationTimeIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usLeftUpperLatitudeIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usLeftUpperLongitudeIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usRightLowerLatitudeIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usRightLoweLongitudeIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usImportanceIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usSourceReliabilityIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usInformationContentIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usDataContentIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usDataFomatIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usTargetIdIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usTargetRoleIndex]->_bUsedInPolicies = true;
    _pMetadataFieldInfos[_usTargetTeamIndex]->_bUsedInPolicies = true;

    _pMetadataFieldInfos[_usPedigreeIndex]->_bVolatile = true;
    _pMetadataFieldInfos[_usPrevMsgIdIndex]->_bVolatile = true;

    _pMetadataFieldInfos[_usRefersToIndex]->_bNotNull = true;

    return 0;
}

int MetadataConfigurationImpl::setupClassifierConfiguration (AVList *pValueList)
{
    const char *pszMethodName = "MetadataConfigurationImpl::setupClassifierConfiguration";
    if (_ui16MetadataLearningNumber < 1) {
        return -1;
    }
    if (_pAttributesValues != nullptr) {
        delete _pAttributesValues;
    }
    _pAttributesValues = new C45AVList (_ui16MetadataLearningNumber + 1);
    String classes = PredictionClass::USEFUL;
    classes += ", ";
    classes += PredictionClass::NOT_USEFUL;
    String values = "\0";
    _pAttributesValues->addPair (C45AVList::_CLASS, classes.c_str());
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
        if (i == _usUsageIndex) {
            continue;
        }

        int count = 0;  // counts the values for the XML attribute
        int index = 0;  // index to the first value for the XML element
        for (unsigned int j = 0; j < pValueList->getLength(); j++) {
            if (1 == (_pMetadataFieldInfos[i]->_sFieldName == pValueList->getAttribute (j))) {
                count ++;
                if (count == 1) {
                    index = j;
                }
            }
        }
        if (count >= 1) {
            if (1 == (_pMetadataFieldInfos[i]->_sFieldType == MetadataType::TEXT)) {
                if(count == 1) {
                    String discrete = C45AVList::_DISCRETE;
                    discrete += pValueList->getValueByIndex (index);
                    _pAttributesValues->addPair (_pMetadataFieldInfos[i]->_sFieldName, discrete);
                }
                if (count > 1) {
                    int count2 = 0;
                    for (unsigned int j = 0; j < pValueList->getLength(); j++) {
                        if (1 == (_pMetadataFieldInfos[i]->_sFieldName == pValueList->getAttribute (j))) {
                            count2 ++;
                            values += pValueList->getValueByIndex (j);
                            if (count2 < count) {
                                values += ", ";
                            }
                        }
                    }
                    _pAttributesValues->addPair (_pMetadataFieldInfos[i]->_sFieldName, values);
                    values = "\0";
                }
            }
            else {
                _pAttributesValues->addPair (_pMetadataFieldInfos[i]->_sFieldName, C45AVList::_CONTINUOUS);
            }
            _pMetadataFieldInfos[i]->_bUsedInLearning = true;
        }
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "C45AVList at the end is:\n");
    for (unsigned int i = 0; i < pValueList->getLength(); i++) {
        checkAndLogMsg (pszMethodName, Logger::L_Info,  "attribute[%d] = <%s>, value[%d] = <%s>\n",
                        i, _pAttributesValues->getAttribute (i), i, _pAttributesValues->getValueByIndex (i));
    }

    return 0;
}

