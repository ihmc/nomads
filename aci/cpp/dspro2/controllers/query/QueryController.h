/*
 * QueryController.h
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
 * Created on February 21, 2013, 3:28 PM
 */

#ifndef INCL_QUERY_CONTROLLER_H
#define INCL_QUERY_CONTROLLER_H

#include "BaseController.h"
#include "DSProServices.h"
#include "Listener.h"
#include "MetadataInterface.h"
#include "StorageController.h"

#include "DArray2.h"
#include "StrClass.h"
#include "StringHashset.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{
    class CommAdaptorManager;
    class MetadataConfigurationImpl;

    class QueryController : public BaseController,
                            public SearchListener,
                            public StorageController,
                            public ApplicationNotificationSvc,
                            public MessagingSvc,
                            public TopologySvc
    {
        public:
            QueryController (const char *pszDescription, const char *pszSupportedQueryType, DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore);
            QueryController (const char *pszDescription, const char **ppszSupportedQueryTypes, DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore);
            virtual ~QueryController (void);

            int init (MetadataConfigurationImpl *pMetadataConf, CommAdaptorManager *pCommAdaptorMgr);

            // SearchListener
            virtual void searchArrived (const char *pszQueryId, const char *pszGroupName,
                                        const char *pszQuerier, const char *pszQueryType,
                                        const char *pszQueryQualifiers, const void *pszQuery,
                                        unsigned int uiQueryLen);
            virtual void searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId);
            virtual void volatileSearchReplyArrived (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen, const char *pszMatchingNodeId);

        protected:
            IHMC_VOI::MetadataList * getAllMetadata (const char *pszQueryQualifiers);
            int notifySearchReply (const char *pszQueryId, const char *pszQuerier,
                                   const char **ppszMatchingMsgIds, const char *pszMatchingNodeId);
            bool supportsQueryType (const char *pszQueryType);

        private:
            struct Reply
            {
                enum Type
                {
                    IDS,
                    RAW
                };

                virtual ~Reply (void);

                const Type _type;
                const char *_pszQueryId;
                const char *_pszQuerier;
                const char *_pszMatchingNodeId;

            protected:
                Reply (Type type, const char *pszQueryId, const char *pszQuerier, const char *pszMatchingNodeId);
            };

            bool isNewQueryReply (const char *pszQueryId, const char *pszQuerier);
            int sendSearchReply (const Reply &reply);
            int sendSearch (SearchProperties *pSearchProperties);
            int notifySearchReply (const Reply &reply);

        protected:
            uint16 _ui16CommAdaptorClientId;
            uint16 _ui16SearchClientId;
            MetadataConfigurationImpl *_pMetadataConf;
            CommAdaptorManager *_pCommAdaptMgr;

        private:


            struct MatchingIdReply : public Reply
            {
                MatchingIdReply (const char *pszQueryId, const char *pszQuerier, const char *pszMatchingNodeId, const char **ppszMatchingMsgIds);
                ~MatchingIdReply (void);

                const char **_ppszMatchingMsgIds;
            };

            struct RawDataReply : public Reply
            {
                RawDataReply (const char *pszQueryId, const char *pszQuerier, const char *pszMatchingNodeId, const void *pReply, uint16 ui162ReplyLen);
                ~RawDataReply (void);

                const uint16 _ui162ReplyLen;
                const void *_pReply;
            };

            const NOMADSUtil::String _nodeId;
            NOMADSUtil::StringHashtable<NOMADSUtil::StringHashset> _rcvdQueryReplies; // key is the queryId, value is a hashset containing
                                                                                      // the IDs of the peers from which the search was received
            NOMADSUtil::DArray2<NOMADSUtil::String> _supportedQueryTypes;
    };
}

#endif    /* INCL_QUERY_CONTROLLER_H */
