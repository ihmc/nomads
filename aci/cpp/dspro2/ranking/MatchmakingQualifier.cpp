/*
 * MatchmakingQualifier.cpp
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
#include "MatchmakingQualifier.h"

#include "MetaData.h"

#include "StringTokenizer.h"
#include "MetadataConfigurationImpl.h"
#include "Writer.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const String MatchmakingQualifier::EQUALS = "=";
const String MatchmakingQualifier::GREATER = ">";
const String MatchmakingQualifier::LESS = "<";
const String MatchmakingQualifier::MATCH = "~";
const String MatchmakingQualifier::TOP = "]";

MatchmakingQualifier::MatchmakingQualifier (void)
{
}

MatchmakingQualifier::MatchmakingQualifier (const char *pszAttribute, const char *pszValue,
                                            const char *pszOperation)
    : _attribute (pszAttribute), _value (pszValue), _operation (pszOperation)
{
}

MatchmakingQualifier::~MatchmakingQualifier (void)
{
}

char * MatchmakingQualifier::getAsString (void)
{
    String qual (_attribute);
    qual += " ";
    qual += _operation;
    qual += " ";
    qual += _value;
    return qual.r_str();
}

int MatchmakingQualifier::parseQualifier (const char *pszQualifier, MatchmakingQualifier * &pMatchmakingQualifier)
{
    pMatchmakingQualifier = nullptr;
    if (pszQualifier == nullptr) {
        return -1;
    }
    String qualifier (pszQualifier);
    qualifier.trim();

    StringTokenizer st (qualifier.c_str(), ' ', ' ');

    String attribute = st.getNextToken();
    attribute.trim();
    if (attribute.length() <= 0) {
        return -2;
    }
    // attribute.convertToUpperCase();
    if (!MetadataConfigurationImpl::getConfiguration()->hasField (attribute.c_str())) {
        return -3;
    }

    String operation = st.getNextToken();
    operation.trim();
    if (operation.length() <= 0) {
        return -4;
    }
    String fieldType (MetadataConfigurationImpl::getConfiguration()->getFieldType (attribute.c_str()));
    if (fieldType.length() <= 0) {
        return -5;
    }
    if (fieldType == "INTEGER8") {

    }
    else if (fieldType == "INTEGER16") {

    }
    else if (fieldType == "INTEGER32") {

    }
    else if (fieldType == "INTEGER64") {

    }
    else if (fieldType == "FLOAT") {

    }
    else if (fieldType == "DOUBLE") {

    }

    if ((operation != "=") && (operation != ">") && (operation != "<") &&
        (operation != "~") && (operation != "]")) {
        return -5;
    }

    String value = st.getNextToken();
    value.trim();
    if (value.length() <= 0) {
        return -6;
    }

    pMatchmakingQualifier = new MatchmakingQualifier (attribute.c_str(), value.c_str(), operation.c_str());
    return 0;
}

int MatchmakingQualifier::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == nullptr) {
        return -1;
    }

    // Read _attribute
    uint16 ui16Len = 0;
    if (pReader->read16 (&ui16Len) < 0) {
        return -2;
    }
    char *pszTmp = (char *) calloc (ui16Len+1, sizeof (char));
    if (pszTmp == nullptr) {
        return -4;
    }
    if (pReader->readBytes (pszTmp, ui16Len) < 0) {
        return -5;
    }
    _attribute += pszTmp;
    free (pszTmp);

    // Read _value
    if (pReader->read16 (&ui16Len) < 0) {
        return -6;
    }
    pszTmp = (char *) calloc (ui16Len+1, sizeof (char));
    if (pszTmp == nullptr) {
        return -7;
    }
    if (pReader->readBytes (pszTmp, ui16Len) < 0) {
        return -8;
    }
    _value = pszTmp;
    free (pszTmp);

    // Read _operation
    if (pReader->read16 (&ui16Len) < 0) {
        return -9;
    }
    pszTmp = (char *) calloc (ui16Len+1, sizeof (char));
    if (pszTmp == nullptr) {
        return -10;
    }
    if (pReader->readBytes (pszTmp, ui16Len) < 0) {
        return -11;
    }
    _operation = pszTmp;

    return 0;
}

int MatchmakingQualifier::write (Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == nullptr) {
        return -1;
    }

    uint16 ui16Len = _attribute.length();
    pWriter->write16 (&ui16Len);
    if (ui16Len > 0) {
        pWriter->writeBytes (_attribute.c_str(), ui16Len);
    }

    ui16Len = _value.length();
    pWriter->write16 (&ui16Len);
    if (ui16Len > 0) {
        pWriter->writeBytes (_value.c_str(), ui16Len);
    }

    ui16Len = _operation.length();
    pWriter->write16 (&ui16Len);
    if (ui16Len > 0) {
        pWriter->writeBytes (_operation.c_str(), ui16Len);
    }

    return 0;
}

bool MatchmakingQualifier::operator == (const MatchmakingQualifier &rhMq)
{
    return _attribute == rhMq._attribute &&
           _value == rhMq._value &&
           _operation == rhMq._operation;
}

ComplexMatchmakingQualifier::ComplexMatchmakingQualifier (void)
{
}

ComplexMatchmakingQualifier::ComplexMatchmakingQualifier (NOMADSUtil::PtrLList<MatchmakingQualifier> &qualifiers)
    : _qualifiers (qualifiers)
{
}

ComplexMatchmakingQualifier::~ComplexMatchmakingQualifier (void)
{
}

const char * ComplexMatchmakingQualifier::getDataFormat (void)
{
    MatchmakingQualifier *pMatchmakingQualifier = _qualifiers.getFirst();
    while (pMatchmakingQualifier != nullptr) {
        if (strcmp (pMatchmakingQualifier->getAttribute(), MetaData::DATA_FORMAT) == 0) {
            return pMatchmakingQualifier->getValue();
        }
        pMatchmakingQualifier = _qualifiers.getNext();
    }
    return nullptr;
}

char * ComplexMatchmakingQualifier::getAsString (void)
{
    bool bIsFirst = true;
    String quals;
    for (MatchmakingQualifier *pQual = _qualifiers.getFirst();
         pQual != nullptr; pQual = _qualifiers.getNext()) {
        char *pszQual = pQual->getAsString();
        if (pszQual != nullptr) {
            if (!bIsFirst) {
                quals += " & ";
            }
            else {
                bIsFirst = false;
            }
            quals += pszQual;
            free (pszQual);
        }
    }

    return quals.r_str();
}

int ComplexMatchmakingQualifier::parseQualifier (const char *pszLine, ComplexMatchmakingQualifier * &pComplexMatchmakingQualifier)
{
    pComplexMatchmakingQualifier = nullptr;
    if (pszLine == nullptr) {
        return -1;
    }
    String line (pszLine);
    line.trim();

    StringTokenizer st (line.c_str(), '&', '&');
    PtrLList<MatchmakingQualifier> qualifiers;
    for (const char *pszToken; (pszToken = st.getNextToken()) != nullptr;) {
        MatchmakingQualifier *pMatchmakingQualifier = nullptr;
        if (MatchmakingQualifier::parseQualifier (pszToken, pMatchmakingQualifier) == 0 &&
            pMatchmakingQualifier != nullptr) {
            if (qualifiers.search (pMatchmakingQualifier) == nullptr) {
                qualifiers.prepend (pMatchmakingQualifier);
            }
        }
        else {
            return -2;
        }
    }

    pComplexMatchmakingQualifier = new ComplexMatchmakingQualifier (qualifiers);
    return 0;
}

int ComplexMatchmakingQualifier::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == nullptr) {
        return -1;
    }

    uint32 ui32Count = 0;
    pReader->read32 (&ui32Count);
    for (unsigned int i = 0; i < ui32Count; i++) {
        MatchmakingQualifier *pMatchmakingQualifier = new MatchmakingQualifier();
        pMatchmakingQualifier->read (pReader, ui32MaxSize);
        _qualifiers.prepend (pMatchmakingQualifier);
    }

    return 0;
}

int ComplexMatchmakingQualifier::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == nullptr) {
        return -1;
    }

    int iCount = _qualifiers.getCount();
    if (iCount <= 0) {
        iCount = 0;
    }
    pWriter->write32 (&iCount);
    for (MatchmakingQualifier *pMatchmakingQualifier = _qualifiers.getFirst();
         pMatchmakingQualifier != nullptr; pMatchmakingQualifier = _qualifiers.getNext()) {
        pMatchmakingQualifier->write (pWriter, ui32MaxSize);
    }

    return 0;
}

int ComplexMatchmakingQualifier::addQualifier (const char *pszAttribute, const char *pszValue,
                                                const char *pszOperation)
{
    MatchmakingQualifier *pMatchmakingQualifier = new MatchmakingQualifier (pszAttribute, pszValue, pszOperation);
    if (pMatchmakingQualifier != nullptr) {
        _qualifiers.prepend (pMatchmakingQualifier);
    }
    return 0;
}

MatchmakingQualifiers::MatchmakingQualifiers (void)
{
}

MatchmakingQualifiers::~MatchmakingQualifiers (void)
{
}

char * MatchmakingQualifiers::getAsString (void)
{
    bool bIsFirst = true;
    String quals;
    for (ComplexMatchmakingQualifier *pQual = _qualifiers.getFirst();
         pQual != nullptr; pQual = _qualifiers.getNext()) {
        char *pszCQual = pQual->getAsString();
        if (pszCQual != nullptr) {
            if (!bIsFirst) {
                quals += ";";
            }
            else {
                bIsFirst = false;
            }
            quals += pszCQual;
            free (pszCQual);
        }
    }

    return quals.r_str();
}

int MatchmakingQualifiers::parseAndAddQualifiers (const char *pszLine)
{
    if (pszLine == nullptr) {
        return -1;
    }
    String line (pszLine);
    line.trim();

    StringTokenizer st (line.c_str(), ';', ';');
    for (const char *pszToken; (pszToken = st.getNextToken()) != nullptr;) {
        ComplexMatchmakingQualifier *pCMatchmakingQualifier = nullptr;
        if (ComplexMatchmakingQualifier::parseQualifier (pszToken, pCMatchmakingQualifier) == 0 &&
            pCMatchmakingQualifier != nullptr) {
            _qualifiers.prepend (pCMatchmakingQualifier);
        }
        else {
            return -2;
        }
    }

    return 0;
}

int MatchmakingQualifiers::read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == nullptr) {
        return -1;
    }

    uint32 ui32Count = 0;
    pReader->read32 (&ui32Count);
    for (unsigned int i = 0; i < ui32Count; i++) {
        ComplexMatchmakingQualifier *pCMatchmakingQualifier = new ComplexMatchmakingQualifier();
        pCMatchmakingQualifier->read (pReader, ui32MaxSize);
        _qualifiers.prepend (pCMatchmakingQualifier);
    }

    return 0;
}

int MatchmakingQualifiers::write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == nullptr) {
        return -1;
    }

    int iCount = _qualifiers.getCount();
    if (iCount <= 0) {
        iCount = 0;
    }
    pWriter->write32 (&iCount);
    for (ComplexMatchmakingQualifier *pCMatchmakingQualifier = _qualifiers.getFirst();
         pCMatchmakingQualifier != nullptr; pCMatchmakingQualifier = _qualifiers.getNext()) {
        pCMatchmakingQualifier->write (pWriter, ui32MaxSize);
    }

    return 0;
}

