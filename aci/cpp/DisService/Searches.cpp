/*
 * Searches.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#include "Searches.h"

#include "DisServiceDefs.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const uint16 Searches::FORWARDED_SEARCH_CLIENT_ID = 0xFFFF;
Searches * Searches::_pInstance = NULL;

Searches::Searches()
    : _queryIdToSearchInfo (true,  // bCaseSensitiveKeys
                            true,  // bCloneKeys
                            true,  // bDeleteKeys
                            true)  // bDeleteValues
{
}

Searches::~Searches()
{
}

Searches * Searches::getSearches (void)
{
    if (_pInstance == NULL) {
        _pInstance = new Searches();
    }
    return _pInstance;
}

int Searches::addSearchInfo (const char *pszQueryId, const char *pszQueryType, const char *pszQuerier, uint16 ui16ClientId)
{
    if (pszQueryId == NULL || pszQueryType == NULL || pszQuerier == NULL) {
        return -1;
    }
    _m.lock();
    if (_queryIdToSearchInfo.containsKey (pszQueryId)) {
        _m.unlock();
        return 0;
    }

    SearchInfo *pSI = new SearchInfo (pszQuerier, pszQueryType, ui16ClientId);
    if (pSI == NULL) {
        _m.unlock();
        return -2;
    }

    delete _queryIdToSearchInfo.put (pszQueryId, pSI);
    checkAndLogMsg ("Searches::addSearchInfo", Logger::L_Info, "added search info for "
                    "query id %s, for client %d\n", pszQueryId, (int) ui16ClientId);
    _m.unlock();
    return 0;
}

int Searches::receivedSearchInfo (const char *pszQueryId, const char *pszQueryType, const char *pszQuerier)
{
    return addSearchInfo (pszQueryId, pszQueryType, pszQuerier, FORWARDED_SEARCH_CLIENT_ID);
}

int Searches::getSearchInfo (const char *pszQueryId, String &queryType, String &querier,uint16 &ui16ClientId)
{
    if (pszQueryId == NULL) {
        return -1;
    }
    _m.lock();

    SearchInfo *pSI = _queryIdToSearchInfo.get (pszQueryId);
    if (pSI == NULL) {
        _m.unlock();
        return -2;
    }

    ui16ClientId = pSI->_ui16ClientId;
    queryType = pSI->_queryType;
    querier = pSI->_querier;

    _m.unlock();
    return 0;
}

int Searches::getSearchQueryId (const char *pszQueryId, uint16 &ui16ClientId)
{
    String queryType;
    String querier;
    return Searches::getSearchInfo (pszQueryId, queryType, querier, ui16ClientId);
}

bool Searches::hasSearchInfo (const char *pszQueryId)
{
    if (pszQueryId == NULL) {
        return false;
    }

    _m.lock();
    bool bHasSearchInfo = (_queryIdToSearchInfo.get (pszQueryId) != NULL);
    _m.unlock();

    return bHasSearchInfo;
}

bool Searches::isSearchFromPeer (const char *pszQueryId, const char *pszQuerierNodeId)
{
    String queryType;
    String querier;
    uint16 ui16QueryId;
    int rc = Searches::getSearchInfo (pszQueryId, queryType, querier, ui16QueryId);
    return ((rc == 0) && ((querier == pszQuerierNodeId) == 1));
}

int Searches::addQueryReply (const char *pszQueryId, const char *pszMatchingNodeId)
{
    if (pszQueryId == NULL || pszMatchingNodeId == NULL) {
        return -1;
    }

    StringHashset *pMatchingIds = _queryReplies.get (pszQueryId);
    if (pMatchingIds == NULL) {
        pMatchingIds = new StringHashset (
            true,  // bCaseSensitiveKeys = true,
            true,  // bCloneKeys
            true); // bDeleteKeys
        _queryReplies.put (pszQueryId, pMatchingIds);
    }
    pMatchingIds->put (pszMatchingNodeId);
    return 0;
}

bool Searches::hasQueryReply (const char *pszQueryId, const char *pszMatchingNodeId)
{
    if (pszQueryId == NULL || pszMatchingNodeId == NULL) {
        return false;
    }
    StringHashset *pMatchingIds = _queryReplies.get (pszQueryId);
    if (pMatchingIds == NULL) {
        return false;
    }
    return pMatchingIds->containsKey (pszMatchingNodeId);
}

Searches::SearchInfo::SearchInfo (const char *pszQuerier, const char *pszQueryType, uint16 ui16ClientId)
    : _ui16ClientId (ui16ClientId),
      _querier (pszQuerier),
      _queryType (pszQueryType)
{
}

Searches::SearchInfo::~SearchInfo (void)
{
}

