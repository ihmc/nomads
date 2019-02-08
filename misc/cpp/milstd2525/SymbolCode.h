/*
 * SymbolCode.h
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
 * SymbolCodes supports the following fields:
 * - Coding Scheme
 * - Standard Identity
 * - Battle Dimension
 * - Category
 * - Function ID
 * - Symbol Modifier
 * - Echelon
 * - Status
 * - Country Code
 * - Order of Battle
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#ifndef INCL_MIL_STANDARD_2525_SYMBOL_CODE_H
#define	INCL_MIL_STANDARD_2525_SYMBOL_CODE_H

#include "MilSTD2525.h"
#include "StrClass.h"

namespace IHMC_MISC_MIL_STD_2525
{
    class SymbolCode
    {
        public:
            SymbolCode (const char *pszSymbol, bool bAllowWildcards = false);
            SymbolCode (const SymbolCode &milStdSymbol);
            virtual ~SymbolCode (void);

            Affiliation getAffiliation (void) const;
            BattleDimension getBattleDimension (void) const;
            CodingScheme getCodingScheme (void) const;
            OrderOfBattle getOrderOfBattle (void) const;
            Status getStatus (void) const;
            char getValue (unsigned char uiIndex) const;

            virtual bool isValid (void) const;

            SymbolCode & operator = (const SymbolCode &rhsMilSTD2525Symbol);

            NOMADSUtil::String toString (void) const;

        protected:
            static const char WILDCARD = '*';
            static const unsigned int CODE_LEN = 15;
            static const char UNSET = '-';

            #pragma pack (1)
            union {
                struct {
                    char chCodingScheme;
                    char chAffiliation;
                    char chBattleDimension;
                    char chStatus;
                    char chFunctionId[6];
                    char chSymbolModifier[2];
                    char chCountryCode[2];
                    char chOrderOfBattle;
                } codes;
                char buf[15];
            } _codesBuf;
            #pragma pack()
    };
}

#endif	// INCL_MIL_STANDARD_2525_SYMBOL_CODE_H

