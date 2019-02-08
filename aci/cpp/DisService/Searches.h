/*
 * ResourceLocker.h
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
 *
 * Author: Giacomo Benincasa	(gbenincasa@ihmc.us)
 * Created on March 3, 2010, 5:02 PM
 */

#ifndef INCL_INCOMING_SEARCHES_H
#define	INCL_INCOMING_SEARCHES_H

#include "Mutex.h"
#include "StrClass.h"
#include "StringHashset.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{
    class Searches
    {
        public:
            static const uint16 FORWARDED_SEARCH_CLIENT_ID;

            Searches (void);
            virtual ~Searches (void);

            static Searches * getSearches (void);

            int addSearchInfo (const char *pszQueryId, const char *pszQueryType, const char *pszQuerier, uint16 ui16ClientId);
            int receivedSearchInfo (const char *pszQueryId, const char *pszQueryType, const char *pszQuerier);
            int getSearchInfo (const char *pszQueryId, NOMADSUtil::String &queryType, NOMADSUtil::String &querier, uint16 &ui16ClientId);
            int getSearchQueryId (const char *pszQueryId, uint16 &ui16ClientId);
            bool hasSearchInfo (const char *pszQueryId);
            bool isSearchFromPeer (const char *pszQueryId, const char *pszQuerierNodeId);

            int addQueryReply (const char *pszQueryId, const char *pszMatchingNodeId);
            bool hasQueryReply (const char *pszQueryId, const char *pszMatchingNodeId);

        private:
            struct SearchInfo
            {
                SearchInfo (const char *pszQuerier, const char *pszQueryType, uint16 ui16QueryId);
                ~SearchInfo (void);

                const uint16 _ui16ClientId;
                const NOMADSUtil::String _querier;
                const NOMADSUtil::String _queryType;
            };

            static Searches *_pInstance;
            NOMADSUtil::Mutex _m;
            NOMADSUtil::StringHashtable<SearchInfo> _queryIdToSearchInfo;
            NOMADSUtil::StringHashtable<NOMADSUtil::StringHashset> _queryReplies;
    };
}

#endif	/* INCL_INCOMING_SEARCHES_H */

