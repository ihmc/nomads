/*
 * RankByTargetMap.cpp
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on March 2, 2016, 2:26 PM
 */

#include "RankByTargetMap.h"

#ifdef UNIX
    #include <stdlib.h>
#endif

#include <stdio.h>

#ifdef WIN32
    #define snprintf _snprintf
#endif

using namespace IHMC_VOI;
using namespace NOMADSUtil;

RankByTargetMap::RankByTargetMap (void)
{
}

RankByTargetMap::RankByTargetMap (const char *pszNodeId, float fRank)
{
    char szRank[10];
    memset (szRank, 0, 10);
    snprintf (szRank, 10, "%.2f", fRank);
    _rankByTarget.put (pszNodeId, szRank);
}

RankByTargetMap::RankByTargetMap (RankByTargetMap &rhsRankByTargetMap)
{
    StringHashtable<char>::Iterator iter = rhsRankByTargetMap._rankByTarget.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        _rankByTarget.put (iter.getKey(), iter.getValue());
    }
}

RankByTargetMap::~RankByTargetMap (void)
{
}

void RankByTargetMap::add (const char *pszNodeId, float fRank)
{
    char szRank[10];
    memset (szRank, 0, 10);
    snprintf (szRank, 10, "%.2f", fRank);
    _rankByTarget.put (pszNodeId, szRank);
}

float RankByTargetMap::get (const char *pszNodeId)
{
    const char *pszRet = _rankByTarget.get (pszNodeId);
    return (pszRet == NULL ? -1.0f : (float) atof (pszRet));
}

bool RankByTargetMap::contains (const char *pszNodeId) const
{
    return _rankByTarget.containsKey (pszNodeId);
}

bool RankByTargetMap::isEmpty (void) const
{
    return _rankByTarget.getCount() == 0;
}

void RankByTargetMap::remove (const char *pszNodeId)
{
    delete _rankByTarget.remove (pszNodeId);
}

int RankByTargetMap::fromString (const String &sRankBtTargetMap)
{
    StringStringHashtable *pMap = StringStringHashtable::parseStringStringHashtable (sRankBtTargetMap);
    if (pMap == NULL) {
        return -1;
    }
    StringHashtable<char>::Iterator iter = pMap->getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        _rankByTarget.put (iter.getKey(), iter.getValue());
    }
    delete pMap;
    return 0;
}

String RankByTargetMap::toString (void)
{
    return _rankByTarget.toString();
}

RankByTargetMap & RankByTargetMap::operator += (RankByTargetMap &rhsRankByTargetMap)
{
    StringHashtable<char>::Iterator iter = rhsRankByTargetMap._rankByTarget.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        _rankByTarget.put (iter.getKey(), iter.getValue());
    }
    return *this;
}

RankByTargetMap & RankByTargetMap::operator = (RankByTargetMap &rhsRankByTargetMap)
{
    StringHashtable<char>::Iterator iter = rhsRankByTargetMap._rankByTarget.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        delete[] _rankByTarget.put (iter.getKey(), iter.getValue());
    }
    return *this;
}

