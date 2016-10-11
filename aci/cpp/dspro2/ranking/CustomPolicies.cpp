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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 12, 2015, 7:09 PM
 */

#include "CustomPolicies.h"
#include "Defs.h"
#include "DSLib.h"
#include "InstrumentedReader.h"
#include "InstrumentedWriter.h"
#include "MetadataInterface.h"

#include "ConfigManager.h"
#include "FileUtils.h"
#include "Logger.h"
#include "StringTokenizer.h"

#include "tinyxml.h"

#include <limits.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

CustomPolicy::CustomPolicy (Type type)
    : _type (type),
      _fRankWeight (0.0f)
{
    assert (_type < 0xFF);  // because it is serialized ad uint8
}

CustomPolicy::CustomPolicy (Type type, float rankWeight, const String &attributeName)
    : _type (type),
      _fRankWeight (rankWeight),
      _attributeName (attributeName)
{
    assert (_type < 0xFF);  // because it is serialized ad uint8
}

CustomPolicy::~CustomPolicy()
{
}

const char * CustomPolicy::getAttribute (void) const
{
    return _attributeName.c_str();
}

float CustomPolicy::getRankWeight (void) const
{
    return _fRankWeight;
}

CustomPolicy::Type CustomPolicy::getType (void) const
{
    return _type;
}

int CustomPolicy::operator == (const CustomPolicy &rhsPolicy) const
{
    return (rhsPolicy._attributeName == _attributeName);
}

int CustomPolicy::read (Reader *pReader, uint32 i32MaxLen, bool bSkip)
{
    if (pReader == NULL) {
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

int CustomPolicy::write (Writer *pWriter, uint32 i32MaxLen)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (pWriter->writeFloat (&_fRankWeight) < 0) {
        return -2;
    }
    uint16 ui16Len = _attributeName.length();
    if (pWriter->write16 (&ui16Len) < 0) {
        return -3;
    }
    if (ui16Len > 0 && pWriter->writeBytes (_attributeName.c_str(), ui16Len) < 0) {
        return -4;
    }
    return 0;
}

int CustomPolicy::writeLength (void)
{
    int i = 4; // _rankWeight
    i += 2;    // _attributeName's length
    i += _attributeName.length();

    return i;
}

CustomPolicies::CustomPolicies (void)
    : _ui8Count (0)
{
}

CustomPolicies::~CustomPolicies (void)
{
}

int CustomPolicies::add (const char *pszCustomPolicyXML)
{
    const char *pszMethodName = "CustomPolicies::add";
    if (pszCustomPolicyXML == NULL) {
        return -1;
    }

    TiXmlDocument doc;
    doc.Parse (pszCustomPolicyXML);
    TiXmlElement *pRoot = doc.FirstChildElement ("RankerPolicy");
    if (pRoot == NULL) {
        return -2;
    }

    TiXmlElement *pXml = pRoot->FirstChildElement ("Type");
    if ((pXml == NULL) || (pXml->GetText() == NULL)) {
        return -3;
    }
    String type (pXml->GetText());

    pXml = pRoot->FirstChildElement ("Attribute");
    if ((pXml == NULL) || (pXml->GetText() == NULL)) {
        return -4;
    }
    const String attr (pXml->GetText());

    pXml = pRoot->FirstChildElement ("Weight");
    if ((pXml == NULL) || (pXml->GetText() == NULL)) {
        return -5;
    }
    const float fWeight = (float) atof (pXml->GetText());

    CustomPolicy *pPolicy = NULL;
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
                        "custom policy of type %s.\n", (const char *) type);
        return -11;
    }
    if (pPolicy != NULL) {
        int rc = pPolicy->init (pRoot);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "cannot parse "
                        "custom policy of type %s. Returned code: %d\n",
                        (const char *) type, rc);
            return -6;
        }
        prepend (pPolicy);
        checkAndLogMsg (pszMethodName, Logger::L_Info, "custom policy of type "
                        "%s on attribute %s with weight %f added to the custom policies list. "
                        "There now are %d policies set.\n", (const char *) type, pPolicy->getAttribute(),
                        pPolicy->getRankWeight(), getCount());
    }

    return 0;
}

int CustomPolicies::init (ConfigManager *pCfgMgr)
{
    const char *pszMethodName = "CustomPolicies::init";
    if (pCfgMgr == NULL) {
        return -1;
    }

    const char *pszCustomPoliciesFiles = pCfgMgr->getValue ("aci.dspro.localNodeContext.policies");
    if (pszCustomPoliciesFiles == NULL) {
        return 0;
    }

    StringTokenizer tokenizer (pszCustomPoliciesFiles, ',', ',');
    for (String token; (token = tokenizer.getNextToken()) != NULL;) {
        token.trim();

        if (!FileUtils::fileExists (token)) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "cannot find "
                            "file %s.\n", (const char *) token);
            return -2;
        }
        char *pszCustomPolicyXML = NULL;
        if ((DSLib::readFileIntoString (token, &pszCustomPolicyXML) < 0) || (pszCustomPolicyXML == NULL)) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "cannot load "
                            "file %s.\n", (const char *) token);
            return -3;
        }

        if (add (pszCustomPolicyXML) < 0) {
            return -4;
        }
    }

    return 0;
}

void CustomPolicies::prepend (CustomPolicy *pPolicy)
{
    PtrLList<CustomPolicy>::prepend (pPolicy);
    _ui8Count++;
}

void CustomPolicies::removeAll (void)
{
    CustomPolicy *pNext = getFirst();
    for (CustomPolicy *pCurr; (pCurr = pNext) != NULL;) {
        pNext = getNext();
        delete remove (pCurr);
    }
}

int CustomPolicies::skip (Reader *pReader, uint32 i32MaxLen)
{
    return readInternal (pReader, i32MaxLen, true);
}

int CustomPolicies::read (Reader *pReader, uint32 i32MaxLen)
{
    return readInternal (pReader, i32MaxLen, false);
}

int CustomPolicies::readInternal (NOMADSUtil::Reader *pReader, uint32 i32MaxLen, bool bSkip)
{
    if (pReader == NULL) {
        return -1;
    }
    InstrumentedReader ir (pReader, false);
    uint8 ui8Count = 0;
    if (ir.read8 (&ui8Count) < 0) {
        return -2;
    }
    if (!bSkip) {
        _ui8Count = ui8Count;
    }
    if (ui8Count > 0) {
        uint8 i = 0;
        for (; i < ui8Count; i++) {
            uint8 ui8Type = 0;
            if (ir.read8 (&ui8Type) < 0) {
                return -3;
            }
            CustomPolicy *pPolicy = NULL;
            switch (ui8Type) {
                case CustomPolicy::STATIC:
                    pPolicy = new StaticPolicy();
                    break;

                default:
                    return -4;
            }
            if (pPolicy == NULL) {
                return -5;
            }
            const int rc = pPolicy->read (&ir, i32MaxLen, bSkip);
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
    return ir.getBytesRead();
}

int CustomPolicies::write (Writer *pWriter, uint32 i32MaxLen)
{
    if (pWriter == NULL) {
        return -1;
    }
    InstrumentedWriter iw (pWriter, false);
    if (iw.write8 (&_ui8Count) < 0) {
        return -2;
    }
    if (_ui8Count > 0) {
        for (CustomPolicy *pPolicy = getFirst(); pPolicy != NULL; pPolicy = getNext()) {
            uint8 ui8 = pPolicy->getType();
            if (iw.write8 (&ui8) < 0) {
                return -3;
            }
            int rc = pPolicy->write (&iw, (i32MaxLen - iw.getBytesWritten()));
            if (rc < 0) {
                return -4;
            }
        }
    }
    if (iw.getBytesWritten() > INT_MAX) {
        return -5;
    }
    return iw.getBytesWritten();
}

int CustomPolicies::getWriteLength (void)
{
    int iBytes = 1;  // iCount
    if (_ui8Count > 0) {
        for (CustomPolicy *pPolicy = getFirst(); pPolicy != NULL; pPolicy = getNext()) {
            iBytes += 1;    // _type
            int rc = pPolicy->writeLength();
            if (rc < 0) {
                return -1;
            }
            else {
                iBytes += rc;
            }
        }
    }
    return iBytes;
}

CustomPolicy * CustomPolicies::remove (CustomPolicy *pel)
{
    return PtrLList<CustomPolicy>::remove (pel);
}

StaticPolicy::StaticPolicy (void)
    : CustomPolicy (STATIC)
{
}

StaticPolicy::StaticPolicy (float rankWeight, const NOMADSUtil::String &attributeName)
    : CustomPolicy (STATIC, rankWeight, attributeName)
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

    if (pXmlField == NULL) {
        return -1;
    }
    TiXmlElement *pXmlMatch = pXmlField->FirstChildElement ("Match");
    if (pXmlMatch == NULL) {
        return 0;
    }
    do {
        TiXmlElement *pXml = pXmlMatch->FirstChildElement ("Value");
        if ((pXml == NULL) || (pXml->GetText() == NULL)) {
            return -2;
        }
        String value (pXml->GetText());

        pXml = pXmlMatch->FirstChildElement ("Rank");
        if ((pXml == NULL) || (pXml->GetText() == NULL)) {
            return -3;
        }
        const float fRank = (float)atof(pXml->GetText());
        if ((fRank < 0.0f) || (fRank > 10.0f)) {
            return -1;
        }
        Rank *pRank = new Rank (fRank);

        if (pRank != NULL) {
            checkAndLogMsg(pszMethodName, Logger::L_Info, "storing match for "
                "attribute %s: %f.\n", (const char *) value, fRank);
            value.trim();
            // The matching between value types is case-insensitive!
            value.convertToLowerCase();
            _valueToRank.put (value, pRank);
        }

        pXmlMatch = pXmlMatch->NextSiblingElement();
    }
    while (pXmlMatch != 0);

    return 0;
}

Match StaticPolicy::rank (MetadataInterface *pMetadata)
{
    if (pMetadata == NULL) {
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
    if (pRank == NULL) {
        Match match (Match::NO);
        return match;
    }
    Match match (pRank->_rank);
    return match;
}

int StaticPolicy::read (Reader *pReader, uint32 i32MaxLen, bool bSkip)
{
    if (CustomPolicy::read (pReader, i32MaxLen, bSkip) < 0) {
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
            if (pRank != NULL) {
                delete _valueToRank.put (buf, pRank);
            }
        }
    }
    return 0;
}

int StaticPolicy::write (Writer *pWriter, uint32 i32MaxLen)
{
    if (CustomPolicy::write (pWriter, i32MaxLen) < 0) {
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

int StaticPolicy::writeLength (void)
{
    int i = CustomPolicy::writeLength();
    i += 2;    // uiCount
    StringHashtable<Rank>::Iterator iter = _valueToRank.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        String attr (iter.getKey());
        uint16 ui16Len = attr.length();
        i += 2;          // ui16Len
        i += ui16Len;    // attr
        i += 4;          // rank
    }

    return i;
}

StaticPolicy::Rank::Rank (float rank)
    : _rank (rank)
{
}

StaticPolicy::Rank::~Rank()
{
}

