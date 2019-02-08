/*
 * SearchProperties.h
 *
 * This file is part of the IHMC DSPro Library/Component
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
 * Created on February 13, 2013, 6:48 PM
 */

#ifndef INCL_SEARCH_PROPERTIES_H
#define INCL_SEARCH_PROPERTIES_H

#include <stddef.h>
#include "FTypes.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    struct SearchProperties
    {
        SearchProperties (void);
        ~SearchProperties (void);

        const char *pszQueryId;
        const char *pszQuerier;
        const char *pszGroupName;
        const char *pszQueryType;
        const char *pszQueryQualifiers;
        const void *pQuery;
        unsigned int uiQueryLen;
        int64 i64TimeoutInMillis;

        static void deallocate (SearchProperties &searchProp);
        static int read (SearchProperties &searchProp, NOMADSUtil::Reader *pReader);
        static int write (SearchProperties &searchProp, NOMADSUtil::Writer *pWriter);

        static int read (char *&pszQueryId, char *&pszQuerier, char *&pszQueryType,
                         char **&ppszMatchingMsgIds, char *&pszMatchingNode,
                         NOMADSUtil::Reader *pReader);
        static int write (const char *pszQueryId, const char *pszQuerier, char *pszQueryType,
                          const char **ppszMatchingMsgIds,
                          const char *pszMatchingNode, NOMADSUtil::Writer *pWriter);

        static int read (char *&pszQueryId, char *&pszQuerier, char *&pszQueryType,
                         void *&pReply, uint16 &ui16ReplyLen, char *&pszMatchingNode,
                         NOMADSUtil::Reader *pReader);
        static int write (const char *pszQueryId, const char *pszQuerier, char *pszQueryType,
                          const void *pReply, uint16 ui16ReplyLen, const char *pszMatchingNode,
                          NOMADSUtil::Writer *pWriter);
    };

    inline SearchProperties::SearchProperties (void)
        : pszQueryId (nullptr), pszQuerier (nullptr), pszGroupName (nullptr),
          pszQueryType (nullptr), pszQueryQualifiers (nullptr), pQuery (nullptr),
          uiQueryLen (0U), i64TimeoutInMillis (0)
    {
    }

    inline SearchProperties::~SearchProperties (void)
    {
    }
}

#endif    /* INCL_SEARCH_PROPERTIES_H */
