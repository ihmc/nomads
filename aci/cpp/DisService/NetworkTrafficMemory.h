/*
 * NetworkTrafficMemory.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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
 * Author: Giacomo Benincasa (gbenincasa@ihmc.us)
 * Created on January 17, 2009, 4:07 PM
 */

#ifndef INCL_NETWORK_TRAFFIC_MEMORY_H
#define	INCL_NETWORK_TRAFFIC_MEMORY_H

#include "LoggingMutex.h"
#include "PtrLList.h"
#include "StringHashtable.h"
#include "UInt32Hashtable.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class Message;
    class MessageHeader;

    /**
     * NetworkTrafficMemory mantains the IDs of all the data messages recently
     * sent by the node itself an its neighbors.
     *
     * NOTE: the size of this cache may rise quickly.
     * TODO: decide a policy to eliminate unseful references.
     * (for instance, when the non replication time expires, delete the entry)
     */
    class NetworkTrafficMemory
    {
        public:
            /**
             * i64IgnoreRequestTime = 0 means that the it must not be done any
             * filtering.
             */
            NetworkTrafficMemory (uint16 ui16IgnoreRequestTime=DEFAULT_IGNORE_REQUEST_TIME);
            NetworkTrafficMemory (NOMADSUtil::ConfigManager *pConfigManager);

            virtual ~NetworkTrafficMemory (void);

            /**
             * Add a message to NetworkTrafficMemory
             * NOTE: the content of pMSg is copied.
             */
            int add (MessageHeader * pMH, int64 ui64ReceivingTime);

            /**
             * Accept a message (fragment) containing the requested range.
             * Returns only the part of the message which has not been sent in a
             * time in the last IGNORE_REQUEST_TIME.
             *
             * NOTE: while pMsg->pMH is cloned (because its elements need to be
             *       modified), pMsg->pData, or its parts, are not (for efficiency
             *       reasons). 
             */
            NOMADSUtil::PtrLList<Message> * filterRecentlySent (Message *pMsg, int64 i64RequestArrivalTime);

            static const uint16 DEFAULT_IGNORE_REQUEST_TIME;

        private:
            struct FragmentWrapper
            {
                FragmentWrapper (uint32 ui32FragOffset, uint32 ui32FragEnd, int64 i64ReceivingTime);
                ~FragmentWrapper (void);

                bool operator > (const FragmentWrapper &rhsFragment);
                bool operator == (const FragmentWrapper &rhsFragment);
                bool operator < (const FragmentWrapper &rhsFragment);

                bool overlaps (uint32 ui32FragOffset, uint32 ui32FragEnd);

                uint32 ui32Offset;
                uint32 ui32End;
                int64 i64LastServingTime;
            };

            struct FragmentedMessageHeader
            {
                FragmentedMessageHeader (void);
                virtual ~FragmentedMessageHeader (void);

                NOMADSUtil::PtrLList<FragmentWrapper> fragments;  // In ascending order
            };

            struct FragmentedChunk : public FragmentedMessageHeader
            {
                FragmentedChunk (void);
                ~FragmentedChunk (void);
            };

            struct FragmentedMessage : public FragmentedMessageHeader
            {
                FragmentedMessage (void);
                ~FragmentedMessage (void);

                NOMADSUtil::UInt32Hashtable<FragmentedChunk> chunksByChunkId;
            };

            struct MessagesBySender
            {
                MessagesBySender (void);
                ~MessagesBySender (void);

                NOMADSUtil::UInt32Hashtable<FragmentedMessage> messageBySeqId;
            };

            struct MessagesByGroup
            {
                MessagesByGroup (void);
                ~MessagesByGroup (void);

                NOMADSUtil::StringHashtable<MessagesBySender> messageBySender;
            };

            int add (const char *pszGroupName, const char *pszPublisherNodeId, uint32 ui32MsgSeqId,
                     uint32 ui32FragOffset, uint32 ui32FragLength, int64 ui64ReceivingTime);
            int add (const char *pszGroupName, const char *pszPublisherNodeId, uint32 ui32MsgSeqId,
                     uint8 ui8ChunkId, uint32 ui32FragOffset, uint32 ui32FragLength, int64 ui64ReceivingTime);
            int add (FragmentedMessageHeader *pFM, uint32 ui32FragOffset, uint32 ui32FragLength, int64 ui64ReceivingTime);

            void cleanUp (void);
            void cleanEmptyMessageBySender (MessagesByGroup * pMG);
            void cleanEmptyFragmentedMessage (MessagesBySender * pMS);
            void cleanFragmentedMessageHeader (FragmentedMessageHeader * pFMH);

            FragmentedMessage * getOrAddFragmentedMessage (const char *pszGroupName, const char *pszSenderNodeId,
                                                           uint32 ui32MsgSeqId);

        private:
            uint16 _ui16IgnoreRequestTime;

            NOMADSUtil::StringHashtable<MessagesByGroup> _messageByGroup;
            NOMADSUtil::LoggingMutex _m;;
    };

    inline NetworkTrafficMemory::FragmentWrapper::FragmentWrapper(uint32 ui32FragOffset, uint32 ui32FragEnd, int64 i64ReceivingTime)
    {
        ui32Offset = ui32FragOffset;
        ui32End = ui32FragEnd;
        i64LastServingTime = i64ReceivingTime;
    }

    inline NetworkTrafficMemory::FragmentWrapper::~FragmentWrapper ()
    {
    }

    inline bool NetworkTrafficMemory::FragmentWrapper::operator > (const FragmentWrapper &rhsFragment)
    {
        return (ui32Offset > rhsFragment.ui32Offset);
    }

    inline bool NetworkTrafficMemory::FragmentWrapper::operator == (const FragmentWrapper &rhsFragment)
    {
        return (ui32Offset == rhsFragment.ui32Offset);
    }

    inline bool NetworkTrafficMemory::FragmentWrapper::operator < (const FragmentWrapper &rhsFragment)
    {
        return (ui32Offset < rhsFragment.ui32Offset);
    }

    inline bool NetworkTrafficMemory::FragmentWrapper::overlaps (uint32 ui32FragOffset, uint32 ui32FragEnd)
    {
        return (!((ui32FragEnd <= ui32Offset) || (ui32FragOffset >= ui32End)));
    }

    inline NetworkTrafficMemory::FragmentedMessageHeader::FragmentedMessageHeader()
        : fragments (false)
    {
    }

    inline NetworkTrafficMemory::FragmentedMessageHeader::~FragmentedMessageHeader()
    {
        FragmentWrapper *pFW;
        FragmentWrapper *pFWTmp = fragments.getFirst();
        while ((pFW = pFWTmp)!= NULL) {
            pFWTmp = fragments.getNext();
            delete fragments.remove (pFW);
        }
    }

    inline NetworkTrafficMemory::FragmentedChunk::FragmentedChunk()
    {
    }

    inline NetworkTrafficMemory::FragmentedChunk::~FragmentedChunk()
    {
    }

    inline NetworkTrafficMemory::FragmentedMessage::FragmentedMessage()
        : chunksByChunkId (US_INITSIZE, true)
    {
    }

    inline NetworkTrafficMemory::FragmentedMessage::~FragmentedMessage()
    {
        chunksByChunkId.removeAll();
    }

    inline NetworkTrafficMemory::MessagesBySender::MessagesBySender()
        : messageBySeqId (US_INITSIZE, true)
    {
    }

    inline NetworkTrafficMemory::MessagesBySender::~MessagesBySender()
    {
        messageBySeqId.removeAll();
    }

    inline NetworkTrafficMemory::MessagesByGroup::MessagesByGroup()
        : messageBySender (true, true, true, true)
    {
    }

    inline NetworkTrafficMemory::MessagesByGroup::~MessagesByGroup()
    {
        messageBySender.removeAll();
    }
}

#endif  // INCL_NETWORK_TRAFFIC_MEMORY_H
