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

#include "MetadataConfiguration.h"

#include "Defs.h"
#include "MetaData.h"
#include "SQLAVList.h"

#include "C45AVList.h"

#include "BufferReader.h"
#include "ConfigManager.h"

#include "Logger.h"
#include "NLFLib.h"
#include "StringHashset.h"

#include "tinyxml.h"

using namespace IHMC_ACI;
using namespace IHMC_C45;
using namespace NOMADSUtil;

const String MetadataConfiguration::XML_HEADER = "<?xml version=\"1.0\"?>";
const String MetadataConfiguration::XML_METADATA_ELEMENT = "Metadata";
const String MetadataConfiguration::XML_FIELD_ELEMENT = "Field";
const String MetadataConfiguration::XML_FIELD_NAME_ELEMENT = "FieldName";
const String MetadataConfiguration::XML_FIELD_TYPE_ELEMENT = "FieldType";
const String MetadataConfiguration::XML_FIELD_VALUE_ELEMENT = "FieldValue";

MetadataConfiguration * MetadataConfiguration::_pINSTANCE = NULL;

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

MetadataConfiguration::MetadataConfiguration()
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
      _usRanksByTargetIndex (_usAnnotationTargetMsgIdIndex + 1),
      _usPrevMsgIdIndex (_usRanksByTargetIndex + 1),
      _usSourceIndex (_usPrevMsgIdIndex + 1),
      _usSourceTimeStampIndex (_usSourceIndex + 1),
      _usExpirationTimeIndex (_usSourceTimeStampIndex + 1),
      _usRelevantMissionIndex (_usExpirationTimeIndex + 1),
      _usLeftUpperLatitudeIndex (_usRelevantMissionIndex + 1),
      _usLeftUpperLongitudeIndex (_usLeftUpperLatitudeIndex + 1),
      _usRightLowerLatitudeIndex (_usLeftUpperLongitudeIndex + 1),
      _usRightLoweLongitudeIndex (_usRightLowerLatitudeIndex + 1),
      _usLocationIndex (_usRightLoweLongitudeIndex + 1),
      _usImportanceIndex (_usLocationIndex + 1),
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
    _pAttributesValues = NULL;
    _pMetadataFieldInfos = NULL;
}

MetadataConfiguration::~MetadataConfiguration()
{
    if (_pMetadataFieldInfos != NULL && _ui16MetadataFieldsNumber > 0) {
        for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
            delete _pMetadataFieldInfos[i];
        }
        free (_pMetadataFieldInfos);
    }
}

MetadataConfiguration * MetadataConfiguration::getConfiguration()
{
    if (_pINSTANCE == NULL) {
        _pINSTANCE = new MetadataConfiguration();
        _pINSTANCE->setFixedFields();
    }
    return _pINSTANCE;
}

MetadataConfiguration * MetadataConfiguration::getConfiguration (const char *pszXMLMetadataFields)
{
    if (pszXMLMetadataFields == NULL) {
        return NULL;
    }

    getConfiguration();

    // Parse XML document
    TiXmlDocument doc;
    doc.Parse (pszXMLMetadataFields);
    TiXmlElement *pRoot = doc.FirstChildElement (XML_METADATA_ELEMENT);
    if (pRoot == NULL) {
        return NULL;
    }
    int rc = _pINSTANCE->addCustomMetadata (pRoot);
    if (rc != 0) {
        checkAndLogMsg ("MetadataConfiguration::getConfiguration", Logger::L_SevereError,
                        "MetadataConfiguration could not be initialized correctly. returned %d\n", rc);
        delete _pINSTANCE;
        _pINSTANCE = NULL;
    }

    return _pINSTANCE;
}

MetadataConfiguration * MetadataConfiguration::getExistingConfiguration (void)
{
    return _pINSTANCE;
}

const char * MetadataConfiguration::getFieldAtIndex (unsigned int uiIndex) const
{
    if (uiIndex >= _ui16MetadataFieldsNumber) {
        return NULL;
    }
    return _pMetadataFieldInfos[uiIndex]->_sFieldName;
}

const char * MetadataConfiguration::getFieldType (const char *pszFieldName) const
{
    if (pszFieldName == NULL || _pMetadataFieldInfos == NULL) {
        return NULL;
    }

    for (unsigned int i = 0; i < _ui16MetadataFieldsNumber && _pMetadataFieldInfos[i] != NULL; i++) {
        if (1 == (_pMetadataFieldInfos[i]->_sFieldName == pszFieldName)) {
            return _pMetadataFieldInfos[i]->_sFieldType.c_str();
        }
    }

    return NULL;
}

bool MetadataConfiguration::isStringFieldType (const char *pszFieldName) const
{
    const char *pszFieldType = getFieldType (pszFieldName);
    if (pszFieldType == NULL) {
        return false;
    }
    return ((MetadataType::TEXT == pszFieldType) == 1);
}

MetaData * MetadataConfiguration::createNewMetadata (SQLAVList *pFieldsValues)
{
    // NOTE: this method uses shared objects, however these share objects are
    // only written at instantiation time, it is therefore not necessary to
    // ensure mutual exclusive access to this method.
    uint16 ui16MetadataFieldsNumber = 0;
    const MetadataFieldInfo ** const ppMetadataFieldInfos = getMetadataFieldInfos (ui16MetadataFieldsNumber);
    MetaData *pMetadata = new MetaData (ppMetadataFieldInfos, ui16MetadataFieldsNumber, _metadataNotWritten);
    if ((pMetadata != NULL) && (pFieldsValues != NULL)) {
        pMetadata->setFieldsValues (pFieldsValues);
    }
    return pMetadata;
}

MetaData * MetadataConfiguration::createNewMetadataFromBuffer (const void *pBuf, uint32 ui32Len)
{
    const char *pszMethodName = "MetadataConfiguration::createNewMetadataFromBuffer";
    uint16 ui16MetadataFieldsNumber = 0;
    const MetadataFieldInfo ** const ppMetadataFieldInfos = getMetadataFieldInfos (ui16MetadataFieldsNumber);
    MetaData *pMetadata = new MetaData (ppMetadataFieldInfos, ui16MetadataFieldsNumber, _metadataNotWritten);
    if (pMetadata == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
    }
    else {
        BufferReader br (pBuf, ui32Len);
        int64 rc = pMetadata->read (&br, ui32Len);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not read Metadata. Return code: %lld.\n", rc);
            delete pMetadata;
            pMetadata = NULL;
        }
        else if (rc < ui32Len) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "read %lld bytes of Metadata, but the buffer "
                            "was %u bytes. Some of the received fields may have not been read\n", rc, ui32Len);
        }
    }
    return pMetadata;
}

MetaData * MetadataConfiguration::createMetadataFromXML (const char *pszXmlDocument)
{
    // NOTE: this method uses shared objects, however these share objects are
    // only written at instantiation time, it is therefore not necessary to
    // ensure mutual exclusive access to this method.
    const char *pszMethodName = "MetadataConfiguration::createMetadataFromXML";
    TiXmlDocument xmlDoc;
    xmlDoc.Parse (pszXmlDocument);
    TiXmlElement *pField;
    TiXmlElement *pRoot = xmlDoc.FirstChildElement (XML_METADATA_ELEMENT);
    if ((pRoot == NULL) || ((pField = pRoot->FirstChildElement (XML_FIELD_ELEMENT)) == NULL)) {
        return NULL;
    }
    SQLAVList attributes;
    do {
        TiXmlElement *pszName = pField->FirstChildElement (XML_FIELD_NAME_ELEMENT);
        TiXmlElement *pszValue = pField->FirstChildElement (XML_FIELD_VALUE_ELEMENT);
        if ((pszName != NULL) && (pszValue != NULL)) {
            const String nameText (pszName->GetText());
            if (nameText.length() <= 0) {
                pField = pField->NextSiblingElement();
                continue;
            }
            const String valueText = (pszValue->GetText());
            if (1 == (_pMetadataFieldInfos[_usUsageIndex]->_sFieldName == nameText)) {
                checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                                "given usage = %s\n", valueText.c_str());
                pField = pField->NextSiblingElement();
                continue;
            }
            if (valueText.length() <= 0) {
                attributes.addPair (nameText, MetadataValue::UNKNOWN);
            }
            else {
                attributes.addPair (nameText, valueText);
            }
            pField = pField->NextSiblingElement();
        }
    } while (pField != NULL);
    uint16 ui16MetadataFieldsNumber = 0;
    const MetadataFieldInfo ** const ppMetadataFieldInfos = getMetadataFieldInfos (ui16MetadataFieldsNumber);
    MetaData *pMetadata = new MetaData (ppMetadataFieldInfos, ui16MetadataFieldsNumber, _metadataNotWritten);
    if (pMetadata != NULL) {
        pMetadata->setFieldsValues (&attributes);
    }
    return pMetadata;
}

char * MetadataConfiguration::convertMetadataToXML (MetadataInterface *pMetadata)
{
    // NOTE: this method uses shared objects, however these share objects are
    // only written at instantiation time, it is therefore not necessary to
    // ensure mutual exclusive access to this method.
    String xmlDoc = XML_HEADER;
    xmlDoc += "\n";
    xmlDoc += "<" + XML_METADATA_ELEMENT + ">";
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
        // don't convert "Usage"
        if (i != _usUsageIndex) {
            xmlDoc += "\n\t<Field>";
            xmlDoc += "\n\t\t<FieldName>";
            xmlDoc += _pMetadataFieldInfos[i]->_sFieldName;
            xmlDoc += "</FieldName>";
            xmlDoc += "\n\t\t<FieldValue>";
            String tmpvalue;
            if ((pMetadata->getFieldValue (_pMetadataFieldInfos[i]->_sFieldName, tmpvalue) == 0) && (tmpvalue.length() > 0)) {
                xmlDoc += tmpvalue;
            }
            xmlDoc += "</FieldValue>";
            xmlDoc += "\n\t</Field>";
        }
    }

    xmlDoc += "\n";
    xmlDoc += (String) "</" + XML_METADATA_ELEMENT + ">";

    return xmlDoc.r_str();
}

char * MetadataConfiguration::convertMetadataAndApplicationMetadataToXML (MetadataInterface *pMetadata)
{
    // NOTE: this method uses shared objects, however these share objects are
    // only written at instantiation time, is therefore not necessary to ensure
    // mutual exclusive access to this method.
    char *pszBuffer = NULL;
    String xmlDoc = XML_HEADER;
    xmlDoc += "\n";
    xmlDoc += (String) "<" + XML_METADATA_ELEMENT + ">";
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
        // don't convert "Usage"
        if (i != _usUsageIndex) {
            xmlDoc += "\n\t<Field>";
            xmlDoc += "\n\t\t<FieldName>";
            xmlDoc += _pMetadataFieldInfos[i]->_sFieldName;
            xmlDoc += "</FieldName>";
            xmlDoc += "\n\t\t<FieldValue>";
            if (pMetadata->getFieldValue (_pMetadataFieldInfos[i]->_sFieldName, (char **) &pszBuffer) == 0 &&
                pszBuffer != NULL) {
                if (strcmp (_pMetadataFieldInfos[i]->_sFieldName, MetaData::APPLICATION_METADATA) == 0) {
                    BufferReader br (pszBuffer, strlen (pszBuffer), false);
                    ConfigManager cfgMgr;
                    cfgMgr.read (&br, strlen (pszBuffer), true);
                    StringStringHashtable::Iterator iter = cfgMgr.getAllElements();
                     while (!iter.end()) {
                        const char *pszKey = iter.getKey();
                        if (pszKey == NULL) {
                            pszKey = "NULL";
                            continue;
                        }
                        const char *pszValue = iter.getValue();
                        if (pszValue == NULL) {
                            pszValue = "NULL";
                            continue;
                        }
                        printf ("%s=%s\n", pszKey, pszValue);
                        xmlDoc += "\n\t\t\t<";
                        xmlDoc += pszKey;
                        xmlDoc += ">";
                        xmlDoc += pszValue;
                        xmlDoc += "</";
                        xmlDoc += pszKey;
                        xmlDoc += ">";
                        iter.nextElement();
                    }
                }
                else {
                    xmlDoc += pszBuffer;
                }

                free (pszBuffer);
            }
            xmlDoc += "</FieldValue>";
            xmlDoc += "\n\t</Field>";
        }
    }

    xmlDoc += "\n";
    xmlDoc += (String) "</" + XML_METADATA_ELEMENT + ">";

    return xmlDoc.r_str();
}

C45AVList * MetadataConfiguration::getMetadataAsDataset (MetadataInterface *pMetadata)
{
    C45AVList *pValues = new C45AVList (_ui16MetadataLearningNumber + 1);
    char *pszValue = (char *) "\0";
    int rc;
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i ++) {
        if (_pMetadataFieldInfos[i]->_bUsedInLearning) {
            rc = pMetadata->getFieldValue (_pMetadataFieldInfos[i]->_sFieldName, &pszValue);
            if (rc < 0) {
                delete pValues;
                return NULL;
            }
            if (i == _usUsageIndex) {
                if (rc == 1) {
                    delete pValues;
                    return NULL;
                }
                if(rc == 0) {
                    if (0 == strcmp(pszValue, "0")) {
                        pValues->addPair (C45AVList::_CLASS, PredictionClass::NOT_USEFUL);
                    }
                    else if (0 == strcmp(pszValue, "1")) {
                        pValues->addPair (C45AVList::_CLASS, PredictionClass::USEFUL);
                    }
                    free(pszValue);
                }
            }
            else {
                if (rc == 0) {
                    pValues->addPair (_pMetadataFieldInfos[i]->_sFieldName, pszValue);
                    free(pszValue);
                }
                else if (rc == 1) {
                    pValues->addPair (_pMetadataFieldInfos[i]->_sFieldName,
                                      C45AVList::_UNKNOWN);
                }
            }
        }
    }

    if (pLogger != NULL && pLogger->getDebugLevel() >= Logger::L_HighDetailDebug) {
        pLogger->logMsg ("MetadataConfiguration::getMetaDataAsDataset", Logger::L_HighDetailDebug,
                         "C45AVList at the end is:\n");
        for (unsigned int i = 0; i < pValues->getLength(); i++) {
            pLogger->logMsg ("MetadataConfiguration::getMetaDataAsDataset", Logger::L_HighDetailDebug,
                             "attribute[%d] = <%s>, value[%d] = <%s>\n",
                             i, pValues->getAttribute (i), i, pValues->getValueByIndex (i));
        }
    }

    return pValues;
}

C45AVList * MetadataConfiguration::getMetadataAsC45List (MetadataInterface *pMetadata)
{
    C45AVList *pValues = new C45AVList (_ui16MetadataLearningNumber);
    char *pszValue = NULL;
    int rc;
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i ++) {
        if (i == _usUsageIndex) {
            continue;
        }
        if (_pMetadataFieldInfos[i]->_bUsedInLearning) {
            rc = pMetadata->getFieldValue (_pMetadataFieldInfos[i]->_sFieldName, &pszValue);
            if (rc < 0) {
                delete pValues;
                return NULL;
            }
            else if (rc == 0) {
                pValues->addPair (_pMetadataFieldInfos[i]->_sFieldName, pszValue);
            }
            else if (rc == 1) {
                pValues->addPair (_pMetadataFieldInfos[i]->_sFieldName, C45AVList::_UNKNOWN);
            }
            if (pszValue != NULL) {
                free (pszValue);
                pszValue = NULL;
            }
        }
    }

    return pValues;
}

char * MetadataConfiguration::getMetaDataStructureAsXML()
{
    String xmlDoc = "<?xml version=\"1.0\"?>";
    xmlDoc += "\n<Metadata>";
    for (int i = 0; i < _ui16MetadataFieldsNumber; i++) {
        xmlDoc += "\n\t<Field>";
        xmlDoc += "\n\t\t<FieldName>";
        xmlDoc += _pMetadataFieldInfos[i]->_sFieldName;
        xmlDoc += "</FieldName>";
        xmlDoc += "\n\t\t<FieldType>";
        xmlDoc + _pMetadataFieldInfos[i]->_sFieldType;
        xmlDoc += "</FieldType>";
        xmlDoc += "\n\t</Field>";
    }
    xmlDoc += "\n</Metadata>";

    char *pszRet = strDup ((const char *) xmlDoc);
    return (char *) pszRet;
}

SQLAVList * MetadataConfiguration::getMetaDataStructure()
{
    SQLAVList *pList = new SQLAVList (_ui16MetadataFieldsNumber);
    for (int i = 0; i < _ui16MetadataFieldsNumber; i++) {
        pList->addPair (_pMetadataFieldInfos[i]->_sFieldName,
                        _pMetadataFieldInfos[i]->_sFieldType);
    }
    return pList;
}

C45AVList * MetadataConfiguration::getFieldsAsRecord (AVList *pFields)
{
    if (pFields == NULL) {
        return NULL;
    }

    C45AVList *pValues = new C45AVList (_ui16MetadataLearningNumber);
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i ++) {
        if ((i != _usUsageIndex) && (_pMetadataFieldInfos[i]->_bUsedInLearning)) {
            for (unsigned int j = 0; j < pFields->getLength(); j ++) {
                if (0 == strcmp (_pMetadataFieldInfos[i]->_sFieldName, pFields->getAttribute (j))) {
                    pValues->addPair (_pMetadataFieldInfos[i]->_sFieldName, pFields->getValueByIndex (j));
                }
            }
        }
    }

    if (pLogger != NULL && pLogger->getDebugLevel() >= Logger::L_HighDetailDebug) {
        pLogger->logMsg ("MetadataConfiguration::getFieldsAsRecord", Logger::L_HighDetailDebug,
                          "C45AVList at the end is:\n");
        for (unsigned int i = 0; i < pValues->getLength(); i++) {
            pLogger->logMsg ("MetadataConfiguration::getFieldsAsRecord", Logger::L_HighDetailDebug,
                             "attribute[%d] = <%s>, value[%d] = <%s>\n",
                             i, pValues->getAttribute (i), i, pValues->getValueByIndex (i));
        }
    }

    return pValues;
}

bool MetadataConfiguration::hasField (const char *pszFieldName)
{
    if (pszFieldName == NULL || _pMetadataFieldInfos == NULL) {
        return false;
    }
    for (unsigned int i = 0; i < _ui16MetadataFieldsNumber && _pMetadataFieldInfos[i] != NULL; i++) {
        if ((_pMetadataFieldInfos[i]->_sFieldName == pszFieldName) == 1) {
            return true;
        }
    }
    return false;
}

int MetadataConfiguration::setMetadataFields (const char *pszXMLMetaDataValues)
{
    const char *pszMethodName = "MetadataConfiguration::setMetadataFields";
    if (pszXMLMetaDataValues == NULL) {
        return -1;
    }
    TiXmlElement *pTxtField;
    TiXmlDocument xmlDoc;
    xmlDoc.Parse (pszXMLMetaDataValues);
    TiXmlElement *pRoot = xmlDoc.FirstChildElement (XML_METADATA_ELEMENT);
    if ((pRoot == NULL) || ((pTxtField = pRoot->FirstChildElement (XML_FIELD_ELEMENT)) == NULL)) {
        return -2;
    }
    SQLAVList avlist;
    do {
        TiXmlElement *pTxtElement = pTxtField->FirstChildElement (XML_FIELD_NAME_ELEMENT);
        TiXmlElement *pTxtValue = pTxtField->FirstChildElement (XML_FIELD_VALUE_ELEMENT);
        if ((pTxtElement != NULL) && (pTxtValue != NULL)) {
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
            } while (pTxtValue != NULL);
            pTxtField = pTxtField->NextSiblingElement();
        }
    } while (pTxtField != NULL);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "SQLAVList at the end is:\n");
    for (unsigned int i = 0; i < avlist.getLength(); i++) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Attribute[%d] = <%s>, Value[%d] = <%s>\n",
                        i, avlist.getAttribute (i), i, avlist.getValueByIndex (i));
    }
    return setupClassifierConfiguration (&avlist);
}

int MetadataConfiguration::setMetadataFields (SQLAVList *pMetaDataValues)
{
    if (pMetaDataValues == NULL) {
        return -1;
    }
    return setupClassifierConfiguration (pMetaDataValues);
}

int MetadataConfiguration::addCustomMetadata (TiXmlElement *pRoot)
{
    const char *pszMethodName = "MetadataConfiguration::addCustomMetadata (1)";
    _ui16MetadataFieldsNumber = _usMetadataFixedFieldsNumber;
    TiXmlElement *pXmlField = pRoot->FirstChildElement (XML_FIELD_ELEMENT);
    if (pXmlField == NULL) {
        return -1;
    }

    StringHashset currentFields;
    for (uint16 i = 0; i < _ui16MetadataFieldsNumber; i++) {
        if (_pMetadataFieldInfos[i] != NULL) {
            currentFields.put (_pMetadataFieldInfos[i]->_sFieldName);
        }
    }

    do {
        TiXmlElement *pXmlElName = pXmlField->FirstChildElement (XML_FIELD_NAME_ELEMENT);
        TiXmlElement *pXmlElType = pXmlField->FirstChildElement (XML_FIELD_TYPE_ELEMENT);
        if ((pXmlElName != NULL) && (pXmlElType != NULL)) {
            const String nameText (pXmlElName->GetText());
            const String typeText (pXmlElType->GetText());
            if ((nameText.length () <= 0) || (typeText.length() <= 0)) {
                pXmlField = pXmlField->NextSiblingElement();
                continue;
            }
            else if (currentFields.containsKey (nameText)) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "trying to add duplicate field %s.\n",
                                nameText.c_str());
                pXmlField = pXmlField->NextSiblingElement ();
                continue;
            }
            else {
                String type (typeText);
                type.convertToUpperCase();
                _ui16MetadataFieldsNumber++;
                _pMetadataFieldInfos = static_cast<MetadataFieldInfo **>(realloc (_pMetadataFieldInfos,
                                       _ui16MetadataFieldsNumber * sizeof (MetadataFieldInfo *)));
                if (_pMetadataFieldInfos == NULL) {
                    checkAndLogMsg (pszMethodName, memoryExhausted);
                    return -2;
                }
                if ((type != MetadataType::INTEGER8) && (type != MetadataType::INTEGER16) &&
                    (type != MetadataType::INTEGER32) && (type != MetadataType::INTEGER64) &&
                    (type != MetadataType::FLOAT) && (type != MetadataType::DOUBLE) &&
                    (type != MetadataType::TEXT)) {
                    checkAndLogMsg (pszMethodName, Logger::L_Warning, "unsupported data type for %s attribute: "
                                    "%s. Setting it to %s.\n", nameText.c_str(), type.c_str(),
                                    MetadataType::TEXT.c_str ());
                    // Set "TEXT" as default type in case the specified
                    // type doesn't match any type in SQLAVList.
                    type = MetadataType::TEXT;
                }
                _pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1] = new MetadataFieldInfo (nameText, type);
                if ((_pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1] == NULL) ||
                    (_pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1]->_sFieldName.length() <= 0)) {
                    checkAndLogMsg ("MetadataConfiguration::addCustomMetadata (1)", memoryExhausted);
                    return -3;
                }
                _pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1]->_bVolatile = ((_ui16MetadataFieldsNumber - 1U) == _usPedigreeIndex);
            }
            pXmlField = pXmlField->NextSiblingElement();
        }
    } while (pXmlField != NULL);

    return 0;
}

int MetadataConfiguration::addCustomMetadata (SQLAVList *pMetadataFields)
{
    if (pMetadataFields == NULL) {
        return -1;
    }

    _ui16MetadataFieldsNumber = pMetadataFields->getLength() + _usMetadataFixedFieldsNumber;
    _pMetadataFieldInfos = static_cast<MetadataFieldInfo **>(realloc (_pMetadataFieldInfos, _ui16MetadataFieldsNumber * sizeof(MetadataFieldInfo *)));
    if (_pMetadataFieldInfos == NULL) {
        checkAndLogMsg ("MetadataConfiguration::addCustomMetadata (2)", memoryExhausted);
        return -2;
    }

    for (uint16 i = _usMetadataFixedFieldsNumber; i < _ui16MetadataFieldsNumber; i++) {
        // Set Attribute Name
        _pMetadataFieldInfos[i] = new MetadataFieldInfo (pMetadataFields->getAttribute (i - _usMetadataFixedFieldsNumber),
                                                         pMetadataFields->getValueByIndex (i - _usMetadataFixedFieldsNumber));
        if ((_pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1] == NULL) ||
            (_pMetadataFieldInfos[_ui16MetadataFieldsNumber - 1]->_sFieldName.length() <= 0)) {
            checkAndLogMsg ("MetadataConfiguration::addCustomMetadata (2)", memoryExhausted);
            return -3;
        }
        _pMetadataFieldInfos[i]->_bVolatile = (i == _usPedigreeIndex);
    }

    return 0;
}

int MetadataConfiguration::setFixedFields()
{
    _ui16MetadataFieldsNumber = _usMetadataFixedFieldsNumber;
    _pMetadataFieldInfos = static_cast<MetadataFieldInfo **>(calloc (_usMetadataFixedFieldsNumber, sizeof (MetadataFieldInfo *)));
    if (_pMetadataFieldInfos == NULL) {
        checkAndLogMsg ("MetadataConfiguration::setFixedFields", memoryExhausted);
        return -1;
    }

    _pMetadataFieldInfos[_usMessageIDIndex] = new MetadataFieldInfo (MetaData::MESSAGE_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usUsageIndex] = new MetadataFieldInfo (MetaData::USAGE, MetadataType::INTEGER8);
    _pMetadataFieldInfos[_usReceiverTimeStampIndex] = new MetadataFieldInfo (MetaData::RECEIVER_TIME_STAMP, MetadataType::INTEGER64);
    _pMetadataFieldInfos[_usReferredDataObjectIdIndex] = new MetadataFieldInfo (MetaData::REFERRED_DATA_OBJECT_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usReferredDataInstanceIdIndex] = new MetadataFieldInfo (MetaData::REFERRED_DATA_INSTANCE_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usPedigreeIndex] = new MetadataFieldInfo (MetaData::PEDIGREE, MetadataType::TEXT);
    _pMetadataFieldInfos[_usRefersToIndex] = new MetadataFieldInfo (MetaData::REFERS_TO, MetadataType::TEXT);
    _pMetadataFieldInfos[_usAnnotationTargetMsgIdIndex] = new MetadataFieldInfo (MetaData::ANNOTATION_TARGET_OBJ_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usRanksByTargetIndex] = new MetadataFieldInfo (MetaData::RANKS_BY_TARGET_NODE_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usPrevMsgIdIndex] = new MetadataFieldInfo (MetaData::PREV_MSG_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usSourceIndex] = new MetadataFieldInfo (MetaData::SOURCE, MetadataType::TEXT);
    _pMetadataFieldInfos[_usSourceTimeStampIndex] = new MetadataFieldInfo (MetaData::SOURCE_TIME_STAMP, MetadataType::INTEGER64);
    _pMetadataFieldInfos[_usExpirationTimeIndex] = new MetadataFieldInfo (MetaData::EXPIRATION_TIME, MetadataType::INTEGER64);
    _pMetadataFieldInfos[_usRelevantMissionIndex] = new MetadataFieldInfo (MetaData::RELEVANT_MISSION, MetadataType::TEXT);
    _pMetadataFieldInfos[_usLeftUpperLatitudeIndex] = new MetadataFieldInfo (MetaData::LEFT_UPPER_LATITUDE, MetadataType::FLOAT);
    _pMetadataFieldInfos[_usLeftUpperLongitudeIndex] = new MetadataFieldInfo (MetaData::LEFT_UPPER_LONGITUDE, MetadataType::FLOAT);
    _pMetadataFieldInfos[_usRightLowerLatitudeIndex] = new MetadataFieldInfo (MetaData::RIGHT_LOWER_LATITUDE, MetadataType::FLOAT);
    _pMetadataFieldInfos[_usRightLoweLongitudeIndex] = new MetadataFieldInfo (MetaData::RIGHT_LOWER_LONGITUDE, MetadataType::FLOAT);
    _pMetadataFieldInfos[_usLocationIndex] = new MetadataFieldInfo (MetaData::LOCATION, MetadataType::TEXT);
    _pMetadataFieldInfos[_usImportanceIndex] = new MetadataFieldInfo (MetaData::IMPORTANCE, MetadataType::DOUBLE);
    _pMetadataFieldInfos[_usSourceReliabilityIndex] = new MetadataFieldInfo (MetaData::SOURCE_RELIABILITY, MetadataType::DOUBLE);
    _pMetadataFieldInfos[_usInformationContentIndex] = new MetadataFieldInfo (MetaData::INFORMATION_CONTENT, MetadataType::DOUBLE);
    _pMetadataFieldInfos[_usDataContentIndex] = new MetadataFieldInfo (MetaData::DATA_CONTENT, MetadataType::TEXT);
    _pMetadataFieldInfos[_usDataFomatIndex] = new MetadataFieldInfo (MetaData::DATA_FORMAT, MetadataType::TEXT);
    _pMetadataFieldInfos[_usTargetIdIndex] = new MetadataFieldInfo (MetaData::TARGET_ID, MetadataType::TEXT);
    _pMetadataFieldInfos[_usTargetRoleIndex] = new MetadataFieldInfo (MetaData::TARGET_ROLE, MetadataType::TEXT);
    _pMetadataFieldInfos[_usTargetTeamIndex] = new MetadataFieldInfo (MetaData::TARGET_TEAM, MetadataType::TEXT);

    for (unsigned int i = 0; i < _usMetadataFixedFieldsNumber; i++) {
        if ((_pMetadataFieldInfos[i] == NULL) || (_pMetadataFieldInfos[i]->_sFieldName.length() <= 0)) {
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

int MetadataConfiguration::setupClassifierConfiguration (SQLAVList *pValueList)
{
    if (_ui16MetadataLearningNumber < 1) {
        return -1;
    }
    if (_pAttributesValues != NULL) {
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

    checkAndLogMsg ("MetadataConfiguration::setupClassifierConfiguration", Logger::L_Info,
                    "C45AVList at the end is:\n");
    for (unsigned int i = 0; i < pValueList->getLength(); i++) {
        checkAndLogMsg ("MetadataConfiguration::setupClassifierConfiguration", Logger::L_Info,
                        "attribute[%d] = <%s>, value[%d] = <%s>\n",
                        i, _pAttributesValues->getAttribute (i), i, _pAttributesValues->getValueByIndex (i));
    }

    return 0;
}

