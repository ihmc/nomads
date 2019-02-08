/*
 * DefaultSearchController.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on March 13, 2014, 1:42 PM
 */

#ifndef INCL_DEFAULT_SEARCH_CONTROLLER_H
#define	INCL_DEFAULT_SEARCH_CONTROLLER_H

#include "SearchController.h"

namespace IHMC_ACI
{
    class DataCacheInterface;
    class DisseminationService;
    class MessageReassembler;
    class PropertyStoreInterface;

    class DefaultSearchController : public SearchController
    {
        public:
            DefaultSearchController (DisseminationService *pDisService,
                                     DataCacheInterface *pDataCache,
                                     MessageReassembler *pMessageReassembler);
            virtual ~DefaultSearchController (void);

            void searchArrived (const char *pszQueryId, const char *pszGroupName,
                                const char *pszQuerier, const char *pszQueryType,
                                const char *pszQueryQualifiers,
                                const void *pszQuery, unsigned int uiQueryLen);

            void searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds,
                                     const char *pszMatchingNodeId);

            void volatileSearchReplyArrived (const char *pszQueryId, const void *pReply,
                                             uint16 ui162ReplyLen, const char *pszMatchingNodeId);

        private:
            void sendMessageRequest (const char *pszMessageId, const char *pszQueryId, uint16 ui16ClientId);

        private:
            const NOMADSUtil::String _nodeId;
            DataCacheInterface *_pDataCache;
            MessageReassembler *_pMessageReassembler;
    };
}

#endif	/* INCL_DEFAULT_SEARCH_CONTROLLER_H */

