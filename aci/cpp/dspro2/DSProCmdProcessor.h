/* 
 * DSProCmdProcessor.h
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
 * Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on April 17, 2014, 1:21 AM
 */

#ifndef INCL_DSPRO_CMD_PROCESSOR_H
#define	INCL_DSPRO_CMD_PROCESSOR_H

#include "CommandProcessor.h"

#include "DSProListener.h"
#include "Listener.h"

#include "AtomicVar.h"
#include "Logger.h"

namespace IHMC_ACI
{
    class DSPro;
    class NodePath;

    class DSProCmdProcessor : public NOMADSUtil::CommandProcessor,
                              public DSProListener,
                              public SearchListener
    {
        public:
            explicit DSProCmdProcessor (DSPro *pDSPro);
            ~DSProCmdProcessor (void);

            bool pathRegistered (NodePath *pNodePath, const char *pszNodeId, const char *pszTeam,
                                 const char *pszMission);
            bool positionUpdated (float fLatitude, float fLongitude, float fAltitude,
                                  const char *pszNodeId);

            void newPeer (const char *pszPeerNodeId);
            void deadPeer (const char *pszDeadPeer);

            int dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                             const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                             const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks,
                             const char *pszQueryId);
            int metadataArrived (const char *pszId, const char *pszGroupName, const char *pszReferredDataObjectId,
                                const char *pszReferredDataInstanceId, const char *pszXMLMetadada, const char *pszReferredDataId,
                                const char *pszQueryId);

            void searchArrived (const char* pszQueryId, const char* pszGroupName, const char* pszQuerier,
                                const char* pszQueryType, const char* pszQueryQualifiers, const void* pszQuery,
                                unsigned uiQueryLen);
            void searchReplyArrived (const char* pszQueryId, const char** ppszMatchingMessageIds,
                                     const char* pszMatchingNodeId);
            void volatileSearchReplyArrived (const char *pszQueryId, const void *pReply,
                                             uint16 ui162ReplyLen, const char *pszMatchingNodeId);

            int processCmd (const void *pToken, char *pszCmdLine);

        protected:
            void displayGeneralHelpMsg (const void *pToken);
            void displayHelpMsgForCmd (const void *pToken, const char *pszCmdLine);
            void handleGetDataCmd (const void *pToken, const char *pszCmdLine);
            void handleGetSProIdsCmd (const void *pToken, const char *pszCmdLine);
            void handlePeersCmd (const void *pToken, const char *pszCmdLine);
            void handleAddUserIdCmd (const void *pToken, const char *pszCmdLine);
            void handleRequestMoreChunks (const void *pToken, const char *pszCmdLine);
            void handleGenDataCmd (const void *pToken, const char *pszCmdLine);
            void handleGetDisServiceCmd (const void *pToken, const char *pszCmdLine);
            void handlePropCmd (const void *pToken, const char *pszCmdLine);
            void handleScreenOutputCmd (const void *pToken, const char *pszCmdLine);
            void handleSetLogLevel (const void *pToken, const char *pszCmdLine);
            void handleSearch (const void *pToken, const char *pszCmdLine);
            void handleSearchReply (const void *pToken, const char *pszCmdLine);
            void handleVolatileSearchReply (const void *pToken, const char *pszCmdLine);
            void handleStoreIncomingData (const void *pToken, const char *pszCmdLine);

        protected:
            DSPro *_pDSPro;
            NOMADSUtil::AtomicVar<bool> _bStoreIncomingData;
    };

    extern NOMADSUtil::Logger *pCmdProcLog;
}

#endif   // #ifndef INCL_DSPRO_CMD_PROCESSOR_H

