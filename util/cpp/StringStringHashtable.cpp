/*
 * StringStringHashtable.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#include "StringStringHashtable.h"

#include "StringTokenizer.h"

using namespace NOMADSUtil;

String StringStringHashtable::toString (void)
{
    String str;
    for (StringStringHashtable::Iterator iter = getAllElements(); !iter.end(); iter.nextElement()) {
        if (str.length() > 0) {
            str += OUTER_SEPARATOR;
        }
        str += iter.getKey();
        str += INNER_SEPARATOR;
        str += iter.getValue();
    }
    return str;
}

StringStringHashtable * StringStringHashtable::parseStringStringHashtable (const char *pszString)
{
    if (pszString == NULL) {
        return NULL;
    }
    const bool bCaseSensitiveKeys = true;
    const bool bCloneKeys = true;
    const bool bDeleteKeys = true;
    const bool bCloneValues = false; // because I copy the token before setting it
    const bool bDeleteValues = true;
    StringStringHashtable *pMap = new StringStringHashtable (bCaseSensitiveKeys ,
                                                             bCloneKeys, bDeleteKeys,
                                                             bCloneValues, bDeleteValues);
    if (pMap == NULL) {
        return NULL;
    }
    StringTokenizer outerTokenizer (pszString, OUTER_SEPARATOR, OUTER_SEPARATOR);
    StringTokenizer innerTokenizer;
    for (const char *pszOuterToken; (pszOuterToken = outerTokenizer.getNextToken()) != NULL;) {
        innerTokenizer.init (pszOuterToken, INNER_SEPARATOR, INNER_SEPARATOR);
        const char *pszKey = innerTokenizer.getNextToken();
        String value (innerTokenizer.getNextToken());
        if ((pszKey != NULL) && (value.length() > 0)) {
            // bCloneValues set on false (because String makes a copy of the token,
            // that then I relinquish with r_str()),and bDeleteValues set on true
            pMap->put (pszKey, value.r_str());
        }
    }
    return pMap;
}

