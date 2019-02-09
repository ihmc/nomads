/*
 * Pedigree.cpp
 *
 * This file is part of the IHMC Voi Library/Component
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
 */

#include "Pedigree.h"

#include "VoiDefs.h"

#include "Logger.h"
#include "StringTokenizer.h"

#include <string.h>

using namespace IHMC_VOI;
using namespace NOMADSUtil;

const char Pedigree::SEPARATOR = ';';

Pedigree::Pedigree (const char *pszSource)
    : _source (pszSource)
{
}

Pedigree::Pedigree (const char *pszSource, const char *pszPedigree)
    : _source (pszSource)
{
    if (pszPedigree != NULL) {
        _pedigree = pszPedigree;
    }
}

Pedigree::~Pedigree()
{
}

const char * Pedigree::toString (void) const
{
    return _pedigree.c_str();
}

bool Pedigree::containsNodeID (const char *pszNodeID, bool bCheckSource) const
{
    if (pszNodeID == NULL) {
        return false;
    }
    if (bCheckSource && _source.length() > 0 &&
        0 == strcmp (_source.c_str(), pszNodeID)) {
        return true;
    }
    StringTokenizer tokenizer (_pedigree, SEPARATOR, SEPARATOR);
    for (const char *pszToken; (pszToken = tokenizer.getNextToken()) != NULL;) {
        if (0 == strcmp (pszToken, pszNodeID)) {
            return true;
        }
    }
    return false;
}

Pedigree & Pedigree::operator += (const Pedigree &ped)
{
    return operator += (ped._pedigree.c_str());
}

Pedigree & Pedigree::operator += (const String &str)
{
    return operator += (str.c_str());
}

Pedigree & Pedigree::operator += (const char *pszStr)
{
    if ((pszStr != NULL) && (strlen (pszStr) > 0)) {
        if (containsNodeID (pszStr, true)) {
            checkAndLogMsg ("Pedigree::operator +=", Logger::L_Warning, "trying "
                            "to add node %s to pedigree %s with source %s\n",
                            pszStr, _pedigree.c_str(), _source.c_str());
        }
        else {
            if (_pedigree.length() > 0) {
                _pedigree += SEPARATOR;
            }
            _pedigree += pszStr;
        }
    }
    return *this;
}

