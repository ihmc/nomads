/*
 * SymbolCode.cpp
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

#include "SymbolCode.h"

#include <assert.h>
#include <string.h>

using namespace IHMC_MISC_MIL_STD_2525;
using namespace NOMADSUtil;

SymbolCode::SymbolCode (const char *pszSymbol, bool bAllowWildcards)
{
   const unsigned int uiSymbolLen = ((pszSymbol == NULL) ? 0U : strlen (pszSymbol));
   if (uiSymbolLen == CODE_LEN) {
       memcpy (_codesBuf.buf, pszSymbol, CODE_LEN);
   }
   else if (bAllowWildcards && (uiSymbolLen > 0) && (uiSymbolLen < CODE_LEN) && (pszSymbol[uiSymbolLen-1] == WILDCARD)) {
       memcpy (_codesBuf.buf, pszSymbol, CODE_LEN);
       memset (_codesBuf.buf+uiSymbolLen, WILDCARD, (CODE_LEN-uiSymbolLen));
   }
   else {
       memset (_codesBuf.buf, '\0', CODE_LEN);
   }
}

SymbolCode::SymbolCode (const SymbolCode &milStdSymbol)
{
    memcpy (_codesBuf.buf, milStdSymbol._codesBuf.buf, CODE_LEN);
}

SymbolCode::~SymbolCode (void)
{
}

Affiliation SymbolCode::getAffiliation (void) const
{
    switch (_codesBuf.codes.chAffiliation) {
        case 'P':
             return A_Pending;

        case 'U':
             return A_Unknown;

        case 'A':
            return A_Assumed_Friend;

        case 'F':
            return A_Friend;

        case 'N':
            return A_Neutral;

        case 'S':
            return A_Suspect;

        case 'H':
            return A_Hostile;

        case 'J':
            return A_Joker;

        case 'K':
            return A_Faker;

        case 'G':
            return A_Exercise_Pending;

        case 'W':
            return A_Exercise_Unknown;

        case 'M':
            return A_Exercise_Assumed_Friend;

        case 'D':
            return A_Exercise_Friend;

        case 'L':
            return A_Exercise_Neutral;

        case 'O':
            return A_None;

        case '-':
            return A_Unset;

        default:
            return A_Error;
    }
}

BattleDimension SymbolCode::getBattleDimension (void) const
{
    switch (_codesBuf.codes.chBattleDimension) {
        case 'P':
            return BD_Space;

        case 'A':
            return BD_Air;

        case 'G':
            return BD_Ground;

        case 'S':
            return BD_Sea_Surface;

        case 'U':
            return BD_Sea_Subsurface;

        case 'F':
            return BD_SOF;

        case 'X':
            return BD_Other;

        case '-':
            return BD_Unset;

        default:
            return BD_Error;
    }
}

CodingScheme SymbolCode::getCodingScheme (void) const
{
    switch (_codesBuf.codes.chCodingScheme) {
        case 'S':
            return CS_War_Fighting;

        case 'G':
            return CS_Tactical_Graphic;

        case 'W':
            return CS_METOC;

        case 'I':
            return CS_Intelligence;

        case 'M':
            return CS_Mapping;

        case 'O':
            return CS_Military_Operation_Other_Than_War;

        case 'E':
            return CS_Emergency_Management;

        case '-':
            return CS_Unset;

        default:
            return CS_Error;
    }
}

OrderOfBattle SymbolCode::getOrderOfBattle (void) const
{
    switch (_codesBuf.codes.chOrderOfBattle) {
        case 'A':
            return OB_Air;

        case 'E':
            return OB_Electronic;

        case 'C':
            return OB_Civilian;

        case 'G':
            return OB_Ground;

        case 'N':
            return OB_Maritime;

        case 'S':
            return OB_Strategic_Force_Related;

        case 'X':
            return OB_Control_Markings;

        case '-':
        case '*': // TODO: This is potentially a hack!
	          // Not sure * should be supported here...
            return OB_Unset;

        default:
            return OB_Error;
    }
}

Status SymbolCode::getStatus (void) const
{
    switch (_codesBuf.codes.chStatus) {
        case 'A':
            return S_Anticipated_Or_Planned;

        case 'P':
            return S_Present;

        case '-':
            return S_Unset;

        default:
            return S_Error;
    }
}

char SymbolCode::getValue (unsigned char uiIndex) const
{
    if (uiIndex >= 15) {
        return '\0';
    }
    return _codesBuf.buf[uiIndex];
}

bool SymbolCode::isValid (void) const
{
    if (getCodingScheme() == CS_Error) {
        return false;
    }
    if (getAffiliation() == A_Error) {
        return false;
    }
    if (getBattleDimension() == BD_Error) {
        return false;
    }
    if (getStatus() == S_Error) {
        return false;
    }
    if (getOrderOfBattle() == OB_Error) {
        return false;
    }
    return true;
}

SymbolCode & SymbolCode::operator = (const SymbolCode &rhsMilSTD2525Symbol)
{
    memcpy (_codesBuf.buf, rhsMilSTD2525Symbol._codesBuf.buf, CODE_LEN);
    return *this;
}

String SymbolCode::toString (void) const
{
    String sSymbol (_codesBuf.buf, CODE_LEN);
    return sSymbol;
}

