/* 
 * StringStringWildMultimap.cpp
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us) 
 * Created on October 8, 2014, 11:34 PM
 */

#include "StringStringWildMultimap.h"

using namespace NOMADSUtil;

StringStringWildMultimap::StringStringWildMultimap()
{
}

StringStringWildMultimap::~StringStringWildMultimap()
{
}

void StringStringWildMultimap::put (const char *pszKeyTemplate, const char *pszValue)
{
    for (KeyValues *pKVs = _map.getFirst(); pKVs != NULL; pKVs = _map.getNext()) {
        if (pKVs->key == pszKeyTemplate) {
            pKVs->values.put (pszValue);
            return;
        }
    }
    KeyValues *pKVs = new KeyValues (pszKeyTemplate, pszValue);
    if (pKVs != NULL) {
        _map.prepend (pKVs);
    }
}

bool StringStringWildMultimap::hasKeyValue (const char *pszKey, const char *pszValue)
{
    for (KeyValues *pKVs = _map.getFirst(); pKVs != NULL; pKVs = _map.getNext()) {
        if (wildcardStringCompare (pszKey, pKVs->key)) {
            return (pKVs->values.containsKey (pszValue));
        }
    }
    return false;
}

StringStringWildMultimap::KeyValues::KeyValues (const char *pszKey, const char *pszValue)
    : key (pszKey)
{
    values.put (pszValue);
}

StringStringWildMultimap::KeyValues::~KeyValues (void)
{
}

