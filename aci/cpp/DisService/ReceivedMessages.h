/*
 * ReceivedMessages.h
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

#ifndef INCL_RECEIVED_MESSAGES_H
#define INCL_RECEIVED_MESSAGES_H

#include "MessageInfo.h"
#include "ReceivedMessagesInterface.h"

#include "Mutex.h"
#include "StringHashtable.h"
#include "StrClass.h"

namespace IHMC_MISC
{
    class PreparedStatement;
}

namespace IHMC_ACI
{
    class ReceivedMessages : public ReceivedMessagesInterface
    {
        public:
            ReceivedMessages (const char *pszStorageFile=NULL);
            ~ReceivedMessages (void);

            int init (void);

            /**
             * Adds the triplet <grpName:senderId:msgSeqId> of a complete message
             * to the received messages.
             * Returns 0 if the message was correctly stored, -1 if the message
             * is already present, a number <-1 if sql errors occurred.
             */
            int addMessage (MessageHeader *pMH);
            int addMessage (const char *pszGroupName, const char *pszPublisherNodeId,
                            uint32 ui32MsgSeqId);

            /**
             * Checks if a triplet <grpName:senderId:msgSeqId> of a complete message
             * is already present in ReceivedMessages.
             * The result of the check is in the parameter bContains.
             * Returns 0 if the operation was successful, a negative number otherwise.
             */
            int contains (MessageHeader *pMH, bool &bContains);
            int contains (const char *pszGroupName, const char *pszPublisherNodeId,
                          uint32 ui32MsgSeqId, bool &bContains);

            /**
             * For a given couple <grpName:senderId>, retrieves the higher
             * msgSeqId stored and puts it in the parameter ui32MaxMsgSeqId.
             * Returns 0 if the operation was successful, a negative number otherwise.
             */
            int getMaxSeqId (MessageHeader *pMH, uint32 &ui32MaxMsgSeqId);
            int getMaxSeqId (const char *pszGroupName, const char *pszPublisherNodeId,
                             uint32 &ui32MaxMsgSeqId);

            NOMADSUtil::StringHashtable<ReceivedMsgsByGrp> * getReceivedMsgsByGrpPub (void);

        private:
            void construct (void);

            /**
             * Returns the identifier of the couple <pszGroupName:pszPublisherNodeId>.
             */
            int64 getGrpPubRowId (const char *pszGroupName, const char *pszPublisherNodeId);

        private:
            static const NOMADSUtil::String TABLE_GROUP_AND_PUBLISHER;
            static const NOMADSUtil::String GRP_NAME;
            static const NOMADSUtil::String PUBLISHER_NODE_ID;
            static const NOMADSUtil::String GROUP_AND_PUBLISHER_ROW_ID;
            static const NOMADSUtil::String MESSAGE_SEQUENCE_ID_TABLE;
            static const NOMADSUtil::String MESSAGE_SEQUENCE_ID;
            static const NOMADSUtil::String GROUP_AND_PUBLISHER_ID;

            IHMC_MISC::PreparedStatement *_psqlInsertGrpPub;
            IHMC_MISC::PreparedStatement *_psqlInsertMsgSeqId;
            IHMC_MISC::PreparedStatement *_psqlSelectGrpPubSeqCount;
            IHMC_MISC::PreparedStatement *_psqlSelectGrpPubSeq;
            IHMC_MISC::PreparedStatement *_psqlSelectGrpPubSeqMax;
            IHMC_MISC::PreparedStatement *_psqlSelectGrpPubSeqAll;
            IHMC_MISC::PreparedStatement *_psqlSelectGrpPubRowId;

            const char *_pszStorageFile;
            NOMADSUtil::Mutex _m;
    };

    inline int ReceivedMessages::addMessage (MessageHeader *pMH)
    {
        if (pMH == NULL) {
            return -1;
        }
        if (!pMH->isCompleteMessage()) {
            return -2;
        }
        return addMessage (pMH->getGroupName(), pMH->getPublisherNodeId(),
                           pMH->getMsgSeqId());
    }

    inline int ReceivedMessages::contains (MessageHeader *pMH, bool &bContains)
    {
        bContains = false;
        if (pMH == NULL) {
            return -1;
        }
        if (!pMH->isCompleteMessage()) {
            return -2;
        }
        return contains (pMH->getGroupName(), pMH->getPublisherNodeId(),
                         pMH->getMsgSeqId(), bContains);
    }

    inline int ReceivedMessages::getMaxSeqId (MessageHeader *pMH, uint32 &ui32MaxMsgSeqId)
    {
        ui32MaxMsgSeqId = 0;
        if (pMH == NULL) {
            return -1;
        }
        if (!pMH->isCompleteMessage()) {
            return -2;
        }
        return getMaxSeqId (pMH->getGroupName(), pMH->getPublisherNodeId(),
                            ui32MaxMsgSeqId);
    }
}

#endif
