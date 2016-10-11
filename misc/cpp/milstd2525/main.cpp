/*
 * main.cpp
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

#include "SymbolCodeTemplateTable.h"

#include "FileReader.h"
#include "FileUtils.h"
#include "LineOrientedReader.h"
#include "LList.h"
#include "StrClass.h"
#include "StringTokenizer.h"

#include <stdio.h>

using namespace IHMC_MISC_MIL_STD_2525;
using namespace NOMADSUtil;

bool validate (const char *pszSymbol, SymbolCodeTemplateTable &templates)
{
    String sSymbol (pszSymbol);
    SymbolCode symbol (sSymbol);
    
    bool bOutput = false;
    if (symbol.isValid()) {
        SymbolCodeTemplate symbolTemplate (templates.getBestMatchingTemplate (symbol));
        if (symbolTemplate.isValid()) {
            printf ("Symbol %s %s template %s.\n", sSymbol.c_str(), symbolTemplate.matches (symbol) ?
                    "matches" : "does not match",  symbolTemplate.toString().c_str());
            bOutput = true;
        }
    }

    if (!bOutput) {
        printf ("Symbol %s %s valid.\tBattle dimension: %s.\tOrder of Battle %s\n",
                sSymbol.c_str(), symbol.isValid() ? "is" : "is not",
                getAsString (symbol.getBattleDimension()),
                getAsString (symbol.getOrderOfBattle()));
    }

    return symbol.isValid();
}

int main (int argc, char **ppszArgv)
{
    if (argc < 2) {
        printf ("Usage: %s <MIL_STD_2525_SYBOL>\n", ppszArgv[0]);
        return 1;
    }

    SymbolCodeTemplateTable templates;
    for (unsigned int i = 2U; i < (unsigned int) argc; i++) {
        if (ppszArgv[i] != NULL) {
            SymbolCodeTemplate symbTempl (ppszArgv[i]);
            if (symbTempl.isValid()) {
                templates.addTemplate (symbTempl);
            }
            else {
                printf ("Symbol Template %s is not valid.\n", symbTempl.toString().c_str());
            }
        }
    }

    int rc = 0;
    if (FileUtils::fileExists (ppszArgv[1])) {
        int i = 1;
        FileReader fr (ppszArgv[1], "r");
        LineOrientedReader lr (&fr, false);
        for (char buf[1024]; lr.readLine (buf, 1024) >= 0; i++) {
            if (!validate (buf, templates)) {
                rc = i;
                break;
            }
        }
    }
    else if (!validate(ppszArgv[1], templates)) {
        rc = 1;
    }

    const SymbolCode symb ("SFPG-----------");
    const SymbolCodeTemplate templ1 ("**P*-----------");
    assert (templ1.matches (symb));

    const SymbolCodeTemplate templ2 ("**A*-----------");
    assert (!templ2.matches (symb));

    SymbolCodeTemplateTable table;
    table.addTemplate (templ1);
    table.addTemplate (templ2);

    SymbolCodeTemplate ret1 = table.getBestMatchingTemplate (symb);
    assert (ret1.isValid());
    assert (ret1 == templ1);

    // SymbolCodeTemplate ret2 = table.getBestMatchingTemplate (symb);
    // assert (!ret2.isValid());

    return rc;
}


