/**
 * ReceivedMessagesInterface.h
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
 * Author: Marco Marchini    (mmarchini@ihmc.us)
 * Created on August 23, 2012
 */

#ifndef INCL_RECEIVED_MESSAGES_INTERFACE_H
#define INCL_RECEIVED_MESSAGES_INTERFACE_H

#include <stddef.h>
#include "MessageInfo.h"

#include "RangeDLList.h"
#include "StrClass.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{

    class ReceivedMessagesInterface
    {
        public:
            virtual ~ReceivedMessagesInterface (void);

            static ReceivedMessagesInterface * getReceivedMessagesInterface (const char *pszStorageFile=NULL);

            /**
             * Adds the triplet <grpName:senderId:msgSeqId> of a complete message to the received messages.
             * Returns 0 if the message was correctly stored, -1 if the message is already present,
             * a number <-1 if sql errors occurred.
             */
            virtual int addMessage (MessageHeader *pMH)=0;
            virtual int addMessage (const char *pszGroupName, const char *pszPublisherNodeId,
                                    uint32 ui32MsgSeqId)=0;

            /**
             * Checks if a triplet <grpName:senderId:msgSeqId> of a complete message is already present in ReceivedMessages.
             * The result of the check is in the parameter bContains.
             * Returns 0 if the operation was successful, a negative number otherwise.
             */
            virtual int contains (MessageHeader *pMH, bool &bContains)=0;
            virtual int contains (const char *pszGroupName, const char *pszPublisherNodeId,
                                  uint32 ui32MsgSeqId, bool &bContains)=0;

             /**
             * For a given couple <grpName:senderId>, retrieves the higher msgSeqId stored and puts it in the
             * parameter ui32MaxMsgSeqId.
             * Returns 0 if the operation was successful, a negative number otherwise.
             */
            virtual int getMaxSeqId (MessageHeader *pMH, uint32 &ui32MaxMsgSeqId)=0;
            virtual int getMaxSeqId (const char *pszGroupName, const char *pszPublisherNodeId,
                                     uint32 &ui32MaxMsgSeqId)=0;

            /**
             * Stores all the msgSeqIds (in ranges) associated to a publisherNodeId.
             */
            struct ReceivedMsgsByPub
            {
                ReceivedMsgsByPub (void);
                ~ReceivedMsgsByPub (void);

                NOMADSUtil::String publisherNodeId;
                NOMADSUtil::UInt32RangeDLList ranges;
            };

            /**
             * Stores all the publisherNodeIds associated to a groupName.
             */
            struct ReceivedMsgsByGrp
            {
                NOMADSUtil::String groupName;
                NOMADSUtil::StringHashtable<ReceivedMsgsByPub> msgsByPub;
            };

            /**
             * Returns all the message ids stored in a compact form (PtrLList<ReceivedMsgsByGrp>):
             * groupName --> publisherId --> ranges
                         --> publisherId --> ranges
                         ...
               groupName --> publisherId --> ranges
                         ...
               ...
             */
            virtual NOMADSUtil::StringHashtable<ReceivedMsgsByGrp> * getReceivedMsgsByGrpPub (void)=0;

        protected:
            ReceivedMessagesInterface (void);

        private:
            static ReceivedMessagesInterface * _pInstance;
    };

    inline ReceivedMessagesInterface::ReceivedMsgsByPub::ReceivedMsgsByPub (void)
        : ranges (true)
    {
    }

    inline ReceivedMessagesInterface::ReceivedMsgsByPub::~ReceivedMsgsByPub (void)
    {
    }
}

#endif // INCL_RECEIVED_MESSAGES_INTERFACE_H

