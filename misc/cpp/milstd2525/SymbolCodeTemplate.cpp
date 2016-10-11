/*
 * SymbolCodeTemplate.cpp
 *
 * This file is part of the IHMC Database Connectivity Library.
 * Copyright (c) 2014-2016 IHMC.
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#include "SymbolCodeTemplate.h"

using namespace IHMC_MISC_MIL_STD_2525;

SymbolCodeTemplate::SymbolCodeTemplate (const char *pszSymbol)
    : SymbolCode (pszSymbol, true)
{
    _uiNWildcards = 0;
    for (unsigned char i = 0; i < 15; i++) {
        if (_codesBuf.buf[i] == WILDCARD) {
            _uiNWildcards++;
        }
    }
}

SymbolCodeTemplate::SymbolCodeTemplate (const SymbolCodeTemplate &SymbolCodeTemplate)
    : SymbolCode (SymbolCodeTemplate)
{
    _uiNWildcards = 0;
    for (unsigned char i = 0; i < 15; i++) {
        if (_codesBuf.buf[i] == WILDCARD) {
            _uiNWildcards++;
        }
    }
}

SymbolCodeTemplate::~SymbolCodeTemplate (void)
{
}

bool SymbolCodeTemplate::isValid (void) const
{
    if ((_codesBuf.codes.chCodingScheme != WILDCARD) && (getCodingScheme() == CS_Error)) {
        return false;
    }
    if ((_codesBuf.codes.chAffiliation != WILDCARD) && (getAffiliation() == A_Error)) {
        return false;
    }
    if ((_codesBuf.codes.chBattleDimension != WILDCARD) && (getBattleDimension() == BD_Error)) {
        return false;
    }
    if ((_codesBuf.codes.chStatus != WILDCARD) && (getStatus() == S_Error)) {
        return false;
    }
    if ((_codesBuf.codes.chOrderOfBattle != WILDCARD) && (getOrderOfBattle() == OB_Error)) {
        return false;
    }
    return true;
}

bool SymbolCodeTemplate::matches (const SymbolCode &rhsMilSTD2525Symbol) const
{
    for (unsigned char i = 0; i < 15; i++) {
        char chLeft = _codesBuf.buf[i];
        char chRight = rhsMilSTD2525Symbol.getValue (i);
        if ((chLeft != WILDCARD) && (chRight != WILDCARD) && (chLeft != chRight)) {
            return false;
        }
    }
    return true;
}

SymbolCodeTemplate & SymbolCodeTemplate::operator = (const SymbolCodeTemplate &rhsSymbolCodeTemplate)
{
    SymbolCode::operator = (rhsSymbolCodeTemplate);
    return *this;
}

int SymbolCodeTemplate::operator == (const SymbolCodeTemplate &rhsSymbolCodeTemplate) const
{
    for (unsigned char i = 0; i < 15; i++) {
        if (_codesBuf.buf[i] != rhsSymbolCodeTemplate._codesBuf.buf[i]) {
            return 0;
        }
    }
    return 1;
}

int SymbolCodeTemplate::operator < (const SymbolCodeTemplate &rhsSymbolCodeTemplate) const
{
    return (_uiNWildcards < rhsSymbolCodeTemplate._uiNWildcards ? 1 : 0);
}

int SymbolCodeTemplate::operator > (const SymbolCodeTemplate &rhsSymbolCodeTemplate) const
{
    return (_uiNWildcards > rhsSymbolCodeTemplate._uiNWildcards ? 1 : 0);
}


