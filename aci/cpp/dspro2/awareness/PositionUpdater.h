/*
 * PositionUpdater.h
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
 * Created on June 27, 2012, 12:03 AM
 */

#ifndef INCL_POSITION_UPDATER_H
#define INCL_POSITION_UPDATER_H

#include "ManageableThread.h"

#include "MessageHeaders.h"

#include "ConditionVariable.h"
#include "DArray.h"
#include "Mutex.h"
#include "LList.h"
#include "PtrLList.h"
#include "StrClass.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{
    class DSProImpl;
    class MetaData;

    class NodeContextManager;

    class Message;

    class PositionUpdater : public NOMADSUtil::ManageableThread
    {
        public:
            PositionUpdater (NodeContextManager *pNodeContexMgr,
                             DSProImpl *pDSPro);
            virtual ~PositionUpdater (void);

            // Received metadata from a remote peer or a local client
            void addMetadataToNotify (const char *pszQueryId, const char **ppszMsgIds);
            // Received search from a remote peer or a local client
            void addSearchToNotify (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen);

            // Received updated position from a local client
            void positionUpdated (void);
            // Received updated topology
            void topologyHasChanged (void);

            // DSPro asynchronously requests a message
            void requestMessage (const char *pszMsgId);
            void requestMessage (const char *pszMsgId, const char *pszPublisherId, const char *pszSenderNodeId,
                                 NOMADSUtil::DArray<uint8> *pLocallyCachedChunkIds);

            // DSPro cancels an asynchronous message request
            void removeMessageRequest (const char *pszMsgId);

            void run (void);

        private:
            struct MsgIdWrapper
            {
                MsgIdWrapper (void);
                MsgIdWrapper (const char *pszMsgId, const char *pszPublisher, const char *pszSender,
                              NOMADSUtil::DArray<uint8> *pLocallyCachedChunkIds);
                MsgIdWrapper (MsgIdWrapper &msgWr);
                ~MsgIdWrapper (void);

                MsgIdWrapper & operator = (const MsgIdWrapper &newMsgIdWr);
                bool operator == (const MsgIdWrapper &rhsStr) const;

                NOMADSUtil::String msgId;
                NOMADSUtil::String publisherId;
                NOMADSUtil::String senderId;
                uint64 ui64LatestRequestTime;
                NOMADSUtil::DArray<uint8> locallyCachedChunkIds;
            };

            typedef NOMADSUtil::LList<NOMADSUtil::String> MsgIdList;

            void doMetadataArrived (NOMADSUtil::StringHashtable<MsgIdList > *pMsgToNotify);

        private:
            bool _bMessageRequested;
            bool _bTopologyHasChanged;
            int64 _i64TimeStamp;
            DSProImpl *_pDSPro;
            NodeContextManager *_pNodeContexMgr;
            NOMADSUtil::Mutex _m;
            NOMADSUtil::ConditionVariable _cv;
            NOMADSUtil::PtrLList<MsgIdWrapper> _msgToRequest;
            NOMADSUtil::StringHashtable<MsgIdList > *_pMsgToNotify; // Key is the pszQueryId
    };

    inline void PositionUpdater::topologyHasChanged (void)
    {
        _bTopologyHasChanged = true;
    }

    inline PositionUpdater::MsgIdWrapper::MsgIdWrapper (void)
    {
        ui64LatestRequestTime = 0;
    }

    inline PositionUpdater::MsgIdWrapper::MsgIdWrapper (const char *pszMsgId, const char *pszPublisher, const char *pszSender, NOMADSUtil::DArray<uint8> *pLocallyCachedChunkIds)
        : msgId (pszMsgId), publisherId (pszPublisher), senderId (pszSender)
    {
        assert (!msgId.contains (","));
        ui64LatestRequestTime = 0;
        if (pLocallyCachedChunkIds != nullptr) {
            for (unsigned int i = 0; i < pLocallyCachedChunkIds->size(); i++) {
                locallyCachedChunkIds[i] = (*pLocallyCachedChunkIds)[i];
            }
        }
    }

    inline PositionUpdater::MsgIdWrapper::MsgIdWrapper (MsgIdWrapper &msgIdWr)
        : msgId (msgIdWr.msgId)
    {
        ui64LatestRequestTime = msgIdWr.ui64LatestRequestTime;
    }

    inline PositionUpdater::MsgIdWrapper & PositionUpdater::MsgIdWrapper::operator = (const MsgIdWrapper &newMsgIdWr)
    {
        msgId = newMsgIdWr.msgId;
        publisherId = newMsgIdWr.publisherId;
        senderId = newMsgIdWr.senderId;
        for (unsigned int i = 0; i < newMsgIdWr.locallyCachedChunkIds.size(); i++) {
            locallyCachedChunkIds[i] = newMsgIdWr.locallyCachedChunkIds.get (i);
        }
        ui64LatestRequestTime = newMsgIdWr.ui64LatestRequestTime;

        return *this;
    }

    inline bool PositionUpdater::MsgIdWrapper::operator == (const MsgIdWrapper &rhsMsgIdWr) const
    {
        return (msgId == rhsMsgIdWr.msgId) == 1;
    }

    inline PositionUpdater::MsgIdWrapper::~MsgIdWrapper (void)
    {
    }
}

#endif    /* INCL_POSITION_UPDATER_H */
