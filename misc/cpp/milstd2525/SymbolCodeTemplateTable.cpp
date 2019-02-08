/*
 * SymbolCodeTemplateTable.cpp
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

using namespace IHMC_MISC_MIL_STD_2525;
using namespace NOMADSUtil;

SymbolCodeTemplateTable::SymbolCodeTemplateTable (void)
{
}

SymbolCodeTemplateTable::~SymbolCodeTemplateTable (void)
{
}

int SymbolCodeTemplateTable::addTemplate (const SymbolCodeTemplate &milSTD2525Template)
{
    bool bInserted = false;
    for (unsigned int i = 0; (!bInserted) && (i < _templates.size()); i++) {
        SymbolCodeTemplate *pRoot = _templates[i].getFirst();
        if (pRoot != NULL) {
            // Assume the first template is the most general (the fact that
            // MatchingTemplates is sorted in descending order ensures that)
            if (pRoot->matches (milSTD2525Template) || milSTD2525Template.matches (*pRoot)) {
                if (_templates[i].search (&milSTD2525Template) != NULL) {
                    // The same template was already added, so not add it again
                    return 1;
                }
                SymbolCodeTemplate *pTemplCpy = new SymbolCodeTemplate (milSTD2525Template);
                if (pTemplCpy != NULL) {
                    _templates[i].insert (pTemplCpy);
                }
                else {
                    return -1;
                }
                bInserted = true;
                break;
            }
        }
    }
    if (!bInserted) {
        SymbolCodeTemplate *pTemplCpy = new SymbolCodeTemplate (milSTD2525Template);
        if (pTemplCpy != NULL) {
            _templates[_templates.size()].insert (pTemplCpy);
        }
        else {
            return -2;
        }
    }
    return 0;
}

void SymbolCodeTemplateTable::clear (void)
{
    for (unsigned int i = 0; i < _templates.size(); i++) {
        if (_templates.used (i)) {
            _templates.clear (i);
        }
    }
}

SymbolCodeTemplate SymbolCodeTemplateTable::getBestMatchingTemplate (const SymbolCode &milSTD2525Symbol)
{
    for (unsigned int i = 0; (i < _templates.size()); i++) {
        SymbolCodeTemplate *pRoot = _templates[i].getFirst();
        if ((pRoot != NULL) && pRoot->matches (milSTD2525Symbol)) {
            SymbolCodeTemplate *pBestMatch = pRoot;
            for (SymbolCodeTemplate *pCurr; (pCurr = _templates[i].getNext()) != NULL; ) {
                if (pCurr->matches (milSTD2525Symbol)) {
                    pBestMatch = pCurr;
                }
                else {
                    break;
                }
            }
            SymbolCodeTemplate templ (*pBestMatch);
            return templ;
        }
    }
    SymbolCodeTemplate templ ((const char *) NULL);
    return templ;
}

unsigned int SymbolCodeTemplateTable::getCount (void)
{
    unsigned int uiCount = 0U;
    for (unsigned int i = 0; (i < _templates.size()); i++) {
        int iCount = _templates[i].getCount();
        if (iCount > 0) {
            uiCount += (unsigned int) iCount;
        }
    }
    return uiCount;
}

