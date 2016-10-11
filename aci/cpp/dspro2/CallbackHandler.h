/*
 * CallbackHandler.h
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#ifndef INCL_CALLBACK_HANDLER_H
#define	INCL_CALLBACK_HANDLER_H

#include "DArray2.h"
#include "LoggingMutex.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class ControlMessageListener;
    class DSProImpl;
    class DSProListener;
    class MatchmakingLogListener;
    class NodePath;
    class SearchListener;
    struct SearchProperties;

    class CallbackHandler
    {
        public:
            CallbackHandler (void);
            ~CallbackHandler (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);

            int dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                             const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                             const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks,
                             const char *pszQueryId);
            int metadataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                                 const char *pszInstanceId, const char *pszXMLMetadata, const char *pszReferredDataId,
                                 const char *pszQueryId, bool bIsTarget);

            int newPeer (const char *pszNewPeerId);
            int deadPeer (const char *pszDeadPeerId);

            void pathRegistered (NodePath *pPath, const char *pszNodeId, const char *pszTeam, const char *pszMission);
            void positionUpdated (float latitude, float longitude, float altitude, const char *pszNodeId);

            void searchArrived (SearchProperties *pSearchProperties);
            void searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId);
            void volatileSearchReplyArrived (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen, const char *pszMatchingNodeId);

            int registerDSProListener (uint16 ui16ClientId, DSProListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterDSProListener (uint16 ui16ClientId, DSProListener *pListener);

            // Matchmaking Log Listener
            int registerMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener);

            // Control Message Listener
            int registerControlMessageListener (uint16 ui16ClientId, ControlMessageListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterControlMessageListener (uint16 ui16ClientId, ControlMessageListener *pListener);

            // Search Listener
            int registerSearchListener (uint16 ui16ClientId, SearchListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterSearchListener (uint16 ui16ClientId, SearchListener *pListener);

            static const char * LOOPBACK_NOTIFICATION;

        private:
            struct ClientInfoPro
            {
                ClientInfoPro (void);
                ~ClientInfoPro (void);

                DSProListener *pListener;
            };

            struct SearchInfo
            {
                SearchInfo (void);
                ~SearchInfo (void);

                SearchListener *pListener;
            };

            mutable NOMADSUtil::LoggingMutex _mCallback;
            NOMADSUtil::DArray2<ClientInfoPro> _clientsPro;
            NOMADSUtil::DArray2<SearchInfo> _searchListners;
    };
}

#endif    /* INCL_CALLBACK_HANDLER_H */

