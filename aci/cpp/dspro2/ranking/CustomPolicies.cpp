/*
 * CustumRanker.cpp
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
 * Created on February 12, 2015, 7:09 PM
 */

#include "CustomPolicies.h"
#include "Defs.h"
#include "InstrumentedWriter.h"
#include "MetadataInterface.h"

#include "ConfigManager.h"
#include "FileUtils.h"
#include "Logger.h"
#include "StringTokenizer.h"

#include "tinyxml.h"

#include <limits.h>

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

CustomPolicyImpl::CustomPolicyImpl (Type type)
    : _type (type),
      _fRankWeight (0.0f)
{
    assert (_type < 0xFF);  // because it is serialized ad uint8
}

CustomPolicyImpl::CustomPolicyImpl (Type type, float rankWeight, const String &attributeName)
    : _type (type),
      _fRankWeight (rankWeight),
      _attributeName (attributeName)
{
    assert (_type < 0xFF);  // because it is serialized ad uint8
}

CustomPolicyImpl::~CustomPolicyImpl ()
{
}

const char * CustomPolicyImpl::getAttribute (void) const
{
    return _attributeName.c_str();
}

float CustomPolicyImpl::getRankWeight (void) const
{
    return _fRankWeight;
}

CustomPolicyImpl::Type CustomPolicyImpl::getType (void) const
{
    return _type;
}

int CustomPolicyImpl::operator == (const CustomPolicyImpl &rhsPolicy) const
{
    return (rhsPolicy._attributeName == _attributeName);
}

int CustomPolicyImpl::read (Reader *pReader, bool bSkip)
{
    if (pReader == nullptr) {
        return -1;
    }

    float fRankWeight = 0;
    if (pReader->readFloat (&fRankWeight) < 0) {
        return -2;
    }
    if (!bSkip) {
        _fRankWeight = fRankWeight;
    }

    uint16 ui16Len = 0;
    if (pReader->read16 (&ui16Len) < 0) {
        return -3;
    }
    char buf[1024];
    if (ui16Len > 0 && pReader->readBytes (buf, ui16Len) < 0) {
        return -4;
    }
    buf[ui16Len] = '\0';
    if (!bSkip) {
        _attributeName = buf;
    }

    return 0;
}

int CustomPolicyImpl::write (Writer *pWriter)
{
    if (pWriter == nullptr) {
        return -1;
    }
    if (pWriter->writeFloat (&_fRankWeight) < 0) {
        return -2;
    }
    uint16 ui16Len = _attributeName.length();
    if (pWriter->write16 (&ui16Len) < 0) {
        return -3;
    }
    if ((ui16Len > 0) && (pWriter->writeBytes (_attributeName, ui16Len) < 0)) {
        return -4;
    }
    return 0;
}

CustomPolicies::CustomPolicies (void)
    : _ui8Count (0)
{
}

CustomPolicies::~CustomPolicies (void)
{
}

int CustomPolicies::add (CustomPolicyImpl *pPolicy)
{
    prepend (pPolicy);
    return 0;
}

int CustomPolicies::add (const char *pszCustomPolicyXML)
{
    const char *pszMethodName = "CustomPolicies::add";
    if (pszCustomPolicyXML == nullptr) {
        return -1;
    }

    TiXmlDocument doc;
    doc.Parse (pszCustomPolicyXML);
    TiXmlElement *pRoot = doc.FirstChildElement ("RankerPolicy");
    if (pRoot == nullptr) {
        return -2;
    }

    TiXmlElement *pXml = pRoot->FirstChildElement ("Type");
    if ((pXml == nullptr) || (pXml->GetText() == nullptr)) {
        return -3;
    }
    String type (pXml->GetText());

    pXml = pRoot->FirstChildElement ("Attribute");
    if ((pXml == nullptr) || (pXml->GetText() == nullptr)) {
        return -4;
    }
    const String attr (pXml->GetText());

    pXml = pRoot->FirstChildElement ("Weight");
    if ((pXml == nullptr) || (pXml->GetText() == nullptr)) {
        return -5;
    }
    const float fWeight = (float) atof (pXml->GetText());

    CustomPolicyImpl *pPolicy = nullptr;
    type.convertToUpperCase();
    if ((type == "STATIC") || (type == "1")) {
        pPolicy = new StaticPolicy (fWeight, attr);
    }
    else if ((type == "DISTANCE") || (type == "2")) {
        // not yet supported!
        return -6;
    }
    else if ((type == "ONTOLOGY") || (type == "3")) {
        // not yet supported!
        return -6;
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "cannot parse "
                        "custom policy of type %s.\n", type.c_str());
        return -11;
    }
    if (pPolicy != nullptr) {
        int rc = pPolicy->init (pRoot);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "cannot parse "
                        "custom policy of type %s. Returned code: %d\n",
                        type.c_str(), rc);
            return -6;
        }
        add (pPolicy);
        checkAndLogMsg (pszMethodName, Logger::L_Info, "custom policy of type "
                        "%s on attribute %s with weight %f added to the custom policies list. "
                        "There now are %d policies set.\n", type.c_str(), pPolicy->getAttribute(),
                        pPolicy->getRankWeight(), getCount());
    }

    return 0;
}

int CustomPolicies::init (ConfigManager *pCfgMgr)
{
    const char *pszMethodName = "CustomPolicies::init";
    if (pCfgMgr == nullptr) {
        return -1;
    }

    const char *pszCustomPoliciesFiles = pCfgMgr->getValue ("aci.dspro.localNodeContext.policies");
    if (pszCustomPoliciesFiles == nullptr) {
        return 0;
    }

    StringTokenizer tokenizer (pszCustomPoliciesFiles, ',', ',');
    for (String token; (token = tokenizer.getNextToken()) != nullptr;) {
        token.trim();
        if (!FileUtils::fileExists (token)) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "cannot find "
                            "file %s.\n", token.c_str());
            return -2;
        }
        int64 i64FileLen = 0;
        void *pBuf = FileUtils::readFile (token, &i64FileLen);
        if (pBuf == nullptr) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "cannot load file %s.\n",
                            token.c_str());
            return -3;
        }
        String customPolicyXML ((const char *) pBuf, i64FileLen);
        free (pBuf);
        if (add (customPolicyXML) < 0) {
            return -4;
        }
    }

    return 0;
}

void CustomPolicies::prepend (CustomPolicyImpl *pPolicy)
{
    PtrLList<CustomPolicyImpl>::prepend (pPolicy);
    _ui8Count++;
}

void CustomPolicies::removeAll (void)
{
    CustomPolicyImpl *pNext = getFirst();
    for (CustomPolicyImpl *pCurr; (pCurr = pNext) != nullptr;) {
        pNext = getNext();
        delete remove (pCurr);
    }
}

int CustomPolicies::skip (Reader *pReader)
{
    return readInternal (pReader, true);
}

int CustomPolicies::read (Reader *pReader)
{
    return readInternal (pReader, false);
}

int CustomPolicies::readInternal (Reader *pReader, bool bSkip)
{
    if (pReader == nullptr) {
        return -1;
    }
    uint8 ui8Count = 0;
    if (pReader->read8 (&ui8Count) < 0) {
        return -2;
    }
    if (!bSkip) {
        _ui8Count = ui8Count;
    }
    if (ui8Count > 0) {
        uint8 i = 0;
        for (; i < ui8Count; i++) {
            uint8 ui8Type = 0;
            if (pReader->read8 (&ui8Type) < 0) {
                return -3;
            }
            CustomPolicyImpl *pPolicy = nullptr;
            switch (ui8Type) {
                case CustomPolicyImpl::STATIC:
                    pPolicy = new StaticPolicy();
                    break;

                default:
                    return -4;
            }
            if (pPolicy == nullptr) {
                return -5;
            }
            const int rc = pPolicy->read (pReader, bSkip);
            if (rc < 0) {
                return -6;
            }
            if (!bSkip) {
                prepend (pPolicy);
            }
        }
        assert (i == ui8Count);
        if (i != ui8Count) {
            return -7;
        }
    }
    return 0;
}

int CustomPolicies::write (Writer *pWriter)
{
    if (pWriter == nullptr) {
        return -1;
    }
    InstrumentedWriter iw (pWriter, false);
    if (iw.write8 (&_ui8Count) < 0) {
        return -2;
    }
    if (_ui8Count > 0) {
        for (CustomPolicyImpl *pPolicy = getFirst(); pPolicy != nullptr; pPolicy = getNext()) {
            uint8 ui8 = pPolicy->getType();
            if (iw.write8 (&ui8) < 0) {
                return -3;
            }
            int rc = pPolicy->write (&iw);
            if (rc < 0) {
                return -4;
            }
        }
    }
    if (iw.getBytesWritten() > INT_MAX) {
        return -5;
    }
    return 0;
}

namespace CUSTUM_POLICIES
{
    static const char * POLICIES = "policies";
    static const char * TYPE = "type";
    static const char * ATTRIBUTE = "attribute";
    static const char * WEIGHT = "weight";
    static const char * MATCHES = "matches";
    static const char * RANK = "rank";
    static const char * VALUE = "value";
}

int CustomPolicies::fromJson (const JsonObject *pJson)
{
    JsonArray *pArray = pJson->getArrayReference (CUSTUM_POLICIES::POLICIES);
    if (pArray == nullptr) {
        return 0;
    }
    int rcAll = 0;
    for (int i = 0; i < pArray->getSize(); i++) {
        JsonObject *pJsonPolicy = pArray->getObject (i);
        if (pJsonPolicy == nullptr) {
            continue;
        }
        uint16 ui16Type = CustomPolicyImpl::STATIC;
        if (pJsonPolicy->getNumber (CUSTUM_POLICIES::TYPE, ui16Type) < 0) {
            return -1;
        }
        String attribute;
        if (pJsonPolicy->getString (CUSTUM_POLICIES::ATTRIBUTE, attribute) < 0) {
            return -2;
        }
        double dWeight = 0.0f;
        if (pJsonPolicy->getNumber (CUSTUM_POLICIES::WEIGHT, dWeight) < 0) {
            return -3;
        }
        CustomPolicyImpl *pPolicy = nullptr;
        switch (ui16Type)
        {
            case CustomPolicyImpl::STATIC:
                pPolicy = new StaticPolicy (dWeight, attribute);
                break;
            default:
                delete pArray;
                return -4;
        }
        if (pPolicy == nullptr) {
            return -5;
        }
        JsonArray *pMatches = pJsonPolicy->getArrayReference (CUSTUM_POLICIES::MATCHES);
        if (pMatches != nullptr) {
            int rc = pPolicy->fromJson (pMatches);
            delete pMatches;
            if (rc == 0) {
                add (pPolicy);
            }
            else {
                rcAll += rc;
            }
        }
    }
    return rcAll;
}

JsonObject * CustomPolicies::toJson (void) const
{
    JsonObject *pJson = new JsonObject();
    if (pJson == nullptr) {
        return nullptr;
    }
    JsonArray policies;
    for (PtrLList<CustomPolicyImpl>::Node *pnext = pRoot; pnext != nullptr; pnext = pnext->pNextNode) {
        CustomPolicyImpl *pPolicy = pnext->pel;
        JsonObject *pPolJson = pPolicy->toJson();
        if (pPolJson != nullptr) {
            policies.addObject (pPolJson);
        }
    }
    pJson->setObject (CUSTUM_POLICIES::POLICIES, &policies);
    return pJson;
}

CustomPolicyImpl * CustomPolicies::remove (CustomPolicyImpl *pel)
{
    return PtrLList<CustomPolicyImpl>::remove (pel);
}

StaticPolicy::StaticPolicy (void)
    : CustomPolicyImpl (STATIC)
{
}

StaticPolicy::StaticPolicy (float rankWeight, const NOMADSUtil::String &attributeName)
    : CustomPolicyImpl (STATIC, rankWeight, attributeName)
{
}

StaticPolicy::~StaticPolicy (void)
{
}

/*
 *
 */
int StaticPolicy::init (TiXmlElement *pXmlField)
{
    const char *pszMethodName = "StaticPolicy::init";

    if (pXmlField == nullptr) {
        return -1;
    }
    TiXmlElement *pXmlMatch = pXmlField->FirstChildElement ("Match");
    if (pXmlMatch == nullptr) {
        return 0;
    }
    do {
        TiXmlElement *pXml = pXmlMatch->FirstChildElement ("Value");
        if ((pXml == nullptr) || (pXml->GetText() == nullptr)) {
            return -2;
        }
        String value (pXml->GetText());

        pXml = pXmlMatch->FirstChildElement ("Rank");
        if ((pXml == nullptr) || (pXml->GetText() == nullptr)) {
            return -3;
        }
        const float fRank = (float)atof(pXml->GetText());
        if ((fRank < 0.0f) || (fRank > 10.0f)) {
            return -1;
        }
        Rank *pRank = new Rank (fRank);

        if (pRank != nullptr) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "storing match for "
                            "attribute %s: %f.\n", value.c_str(), fRank);
            value.trim();
            // The matching between value types is case-insensitive!
            value.convertToLowerCase();
            _valueToRank.put (value, pRank);
        }

        pXmlMatch = pXmlMatch->NextSiblingElement();
    }
    while (pXmlMatch != nullptr);

    return 0;
}

Match StaticPolicy::rank (MetadataInterface *pMetadata)
{
    if (pMetadata == nullptr) {
        Match match (Match::NOT_SURE);
        return match;
    }
    String value;
    if (pMetadata->getFieldValue (_attributeName, value) < 0) {
        Match match (Match::NOT_SURE);
        return match;
    }

    value.trim();
    // The matching between value types is case-insensitive!
    value.convertToLowerCase();
    Rank *pRank = _valueToRank.get (value);
    if (pRank == nullptr) {
        // Check whether "value" is actually a comma-separated list of values
        StringTokenizer tokenizer (value, ',', ',');
        for (const char *pszToken; (pszToken = tokenizer.getNextToken()) != nullptr;) {
            Rank *pRank = _valueToRank.get (pszToken);
            if (pRank != nullptr) {
                Match match (pRank->_rank);
                return match;
            }
        }
        Match match (Match::NO);
        return match;
    }


    Match match (pRank->_rank);
    return match;
}

int StaticPolicy::read (Reader *pReader, bool bSkip)
{
    if (CustomPolicyImpl::read (pReader, bSkip) < 0) {
        return -1;
    }
    uint16 ui16 = 0;
    if (pReader->read16 (&ui16) < 0) {
        return -2;
    }
    const uint16 ui16NElements = ui16;
    for (uint16 i = 0; i < ui16NElements; i++) {
        if (pReader->read16 (&ui16) < 0) {
            return -3;
        }
        char buf[1024];
        if (ui16 > 0 && pReader->readBytes (buf, ui16) < 0) {
            return -4;
        }
        buf[ui16] = '\0';
        float fRank = 0.0f;
        if (pReader->readFloat (&fRank) < 0) {
            return -5;
        }
        if (!bSkip) {
            Rank *pRank = new Rank (fRank);
            if (pRank != nullptr) {
                delete _valueToRank.put (buf, pRank);
            }
        }
    }
    return 0;
}

int StaticPolicy::write (Writer *pWriter)
{
    if (CustomPolicyImpl::write (pWriter) < 0) {
        return -1;
    }
    unsigned int uiCount = _valueToRank.getCount();
    if (uiCount > 0xFFFF) {
        return -2;
    }
    if (pWriter->write16 (&uiCount) < 0) {
        return -3;
    }
    StringHashtable<Rank>::Iterator iter = _valueToRank.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        String attr (iter.getKey());
        uint16 ui16Len = attr.length();
        if (pWriter->write16 (&ui16Len) < 0) {
            return -4;
        }
        if (ui16Len > 0 && pWriter->writeBytes (attr.c_str(), ui16Len) < 0) {
            return -5;
        }
        Rank *pRank = iter.getValue();
        float rank = (pRank->_rank);
        if (pWriter->writeFloat (&rank) < 0) {
            return -2;
        }
    }
    return 0;
}

int StaticPolicy::fromJson (JsonArray *pMatches)
{
    if (pMatches == nullptr) {
        return -1;
    }
    for (int i = 0; i < pMatches->getSize(); i++) {
        JsonObject *pMatch = pMatches->getObject (i);
        if (pMatch != nullptr) {
            String value;
            if (pMatch->getString (CUSTUM_POLICIES::VALUE, value) < 0) {
                return -2;
            }
            double dRank = 0.0f;
            if (pMatch->getNumber (CUSTUM_POLICIES::RANK, dRank) < 0) {
                return -2;
            }
            Rank *pRank = new Rank ((float) dRank);
            if (pRank != nullptr) {
                _valueToRank.put (value, pRank);
            }
        }
    }
    return 0;
}

JsonObject * StaticPolicy::toJson (void) const
{
    JsonObject *pJson = new JsonObject();
    if (pJson == nullptr) {
        return nullptr;
    }
    pJson->setString (CUSTUM_POLICIES::ATTRIBUTE, getAttribute());
    pJson->setNumber (CUSTUM_POLICIES::TYPE, getType());
    pJson->setNumber (CUSTUM_POLICIES::WEIGHT, getRankWeight());
    JsonArray array;
    StringHashtable<Rank>::Iterator iter = _valueToRank.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        String val (iter.getKey());
        float rank = (iter.getValue()->_rank);
        JsonObject match;
        match.setString (CUSTUM_POLICIES::VALUE, val);
        match.setNumber (CUSTUM_POLICIES::RANK, rank);
        array.setObject (&match);
    }
    pJson->setObject (CUSTUM_POLICIES::MATCHES, &array);
    return pJson;
}

StaticPolicy::Rank::Rank (float rank)
    : _rank (rank)
{
}

StaticPolicy::Rank::~Rank (void)
{
}

