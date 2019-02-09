/*
* RangeOfInfluence.cpp
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
*/

#include "RangeOfInfluence.h"

#include "Defs.h"
#include "Logger.h"
#include "Json.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;
using namespace IHMC_MISC_MIL_STD_2525;

const char * RangeOfInfluence::RANGE_OF_INFLUENCE_BY_MILSTD2525_SYMBOL_CODE = "aci.dspro.localNodeContext.rangeOfInfluenceByMilSTD2525SymbolCode";

namespace RANGE_OF_INFLUENCE
{
    static const char * RANGE_OF_INFLUENCE = "rangeOfInfluence";
    static const char * SYMBOL = "symbol";
    static const char * RANGE = "range";
}

RangeOfInfluence::RangeOfInfluence (void)
{
}

RangeOfInfluence::~RangeOfInfluence (void)
{
}

void RangeOfInfluence::clear (void)
{
    _symbols.clear();
    _symbolCodeToRangeOfInfluence.removeAll();
}

uint32 RangeOfInfluence::getRangeOfInfluence (const char *pszAttribute)
{
    if (pszAttribute == nullptr) {
        return 0U;
    }
    SymbolCode milSTD2525Symbol (pszAttribute);
    if (milSTD2525Symbol.isValid()) {
        return getRangeOfInfluence (milSTD2525Symbol);
    }
    uint32 *pUI32RangeOfInfluence = _symbolCodeToRangeOfInfluence.get (pszAttribute);
    if (pUI32RangeOfInfluence == nullptr) {
        return 0U;
    }
    return *pUI32RangeOfInfluence;
}

uint32 RangeOfInfluence::getMaximumRangeOfInfluence (void)
{
    StringHashtable<uint32>::Iterator iter = _symbolCodeToRangeOfInfluence.getAllElements();
    uint32 ui32Max = 0U;
    for (; !iter.end(); iter.nextElement()) {
        uint32 *pui32 = iter.getValue();
        if ((pui32 != nullptr) && (*pui32 > ui32Max)) {
            ui32Max = *pui32;
        }
    }
    return ui32Max;
}

bool RangeOfInfluence::setRangeOfInfluence (const char *pszAttribute, uint32 ui32RangeOfInfluenceInMeters)
{
    if (pszAttribute == nullptr) {
        return false;
    }
    SymbolCodeTemplate milSTD2525Symbol (pszAttribute);
    if (milSTD2525Symbol.isValid()) {
        return setRangeOfInfluence (milSTD2525Symbol, ui32RangeOfInfluenceInMeters);
    }
    uint32 *pui32 = static_cast<uint32*>(calloc (1, sizeof (uint32)));
    if (pui32 == nullptr) {
        return false;
    }
    *pui32 = ui32RangeOfInfluenceInMeters;
    uint32 *pui32Old = _symbolCodeToRangeOfInfluence.put (pszAttribute, pui32);
    const bool bUpdated = (pui32Old == nullptr) || (*pui32Old != ui32RangeOfInfluenceInMeters);
    if (pui32Old != nullptr) {
        free (pui32Old);
    }
    return bUpdated;
}

uint32 RangeOfInfluence::getRangeOfInfluence (SymbolCode &milSTD2525Symbol)
{
    SymbolCodeTemplate milSTD2525SymbolTemplate = _symbols.getBestMatchingTemplate (milSTD2525Symbol);
    NOMADSUtil::String sMilSTD2525SymbolTemplate = milSTD2525SymbolTemplate.toString();
    uint32 *pUI32RangeOfInfluence = _symbolCodeToRangeOfInfluence.get (sMilSTD2525SymbolTemplate);
    if (pUI32RangeOfInfluence == nullptr) {
        return 0U;
    }
    return *pUI32RangeOfInfluence;
}

bool RangeOfInfluence::setRangeOfInfluence (const SymbolCodeTemplate &milSTD2525SymbolTemplate, uint32 ui32RangeOfInfluenceInMeters)
{
    const char *pszMethodName = "RangeOfInfluence::setRangeOfInfluence";
    uint32 *pui32 = static_cast<uint32*>(calloc (1, sizeof (uint32)));
    if (pui32 == nullptr) {
        return false;
    }
    *pui32 = ui32RangeOfInfluenceInMeters;
    String sMilSTD2525SymbolTemplate = milSTD2525SymbolTemplate.toString ();
    checkAndLogMsg (pszMethodName, Logger::L_Info, "setting %s range of influence to %u\n",
                    sMilSTD2525SymbolTemplate.c_str(), ui32RangeOfInfluenceInMeters);
    uint32 *pui32Old = _symbolCodeToRangeOfInfluence.put (sMilSTD2525SymbolTemplate, pui32);
    _symbols.addTemplate (milSTD2525SymbolTemplate);
    const bool bUpdated = (pui32Old == nullptr) || (*pui32Old != ui32RangeOfInfluenceInMeters);
    if (pui32Old != nullptr) {
        free (pui32Old);
    }
    return bUpdated;
}

void RangeOfInfluence::reset (void)
{
    _symbols.clear();
    _symbolCodeToRangeOfInfluence.removeAll();
}

int RangeOfInfluence::fromJson (const JsonObject *pRoot)
{
    if (pRoot == nullptr) {
        return -1;
    }
    clear();
    const JsonArray *pRangesOfInfluence = pRoot->getArray (RANGE_OF_INFLUENCE::RANGE_OF_INFLUENCE);
    if (pRangesOfInfluence == nullptr) {
        return 0;
    }
    const int iLen = pRangesOfInfluence->getSize();
    for (int i = 0; i < iLen; i++) {
        JsonObject *pRangeOfInfluence = pRangesOfInfluence->getObject (i);
        if (pRangeOfInfluence != nullptr) {
            String symbol;
            uint32 ui32Range = 0U;
            if ((pRangeOfInfluence->getString (RANGE_OF_INFLUENCE::SYMBOL, symbol) == 0) &&
                (pRangeOfInfluence->getNumber (RANGE_OF_INFLUENCE::RANGE, ui32Range) == 0)) {
                setRangeOfInfluence (symbol, ui32Range);
            }
            delete pRangeOfInfluence;
        }
    }
    return 0;
}

JsonObject * RangeOfInfluence::toJson (void) const
{
    JsonObject *pJson = new JsonObject();
    if (pJson == nullptr) {
        return nullptr;
    }
    JsonArray *pArray = new JsonArray();
    if (pArray == nullptr) {
        delete pJson;
        return nullptr;
    }
    StringHashtable<uint32>::Iterator iter = _symbolCodeToRangeOfInfluence.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        JsonObject *pRangeOfInfl = new JsonObject();
        if (pRangeOfInfl != nullptr) {
            pRangeOfInfl->setString (RANGE_OF_INFLUENCE::SYMBOL, iter.getKey());
            pRangeOfInfl->setNumber (RANGE_OF_INFLUENCE::RANGE, *iter.getValue());
            pArray->addObject (pRangeOfInfl);
        }
    }
    if (pArray->getSize() > 0) {
        pJson->setObject (RANGE_OF_INFLUENCE::RANGE_OF_INFLUENCE, pArray);
        delete pArray;
        return pJson;
    }
    delete pArray;
    delete pJson;

    return nullptr;
}

