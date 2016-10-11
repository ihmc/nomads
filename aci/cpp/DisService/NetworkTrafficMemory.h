/*
 * NetworkTrafficMemory.h
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
 * Author: Giacomo Benincasa (gbenincasa@ihmc.us)
 * Created on January 17, 2009, 4:07 PM
 */

#ifndef INCL_NETWORK_TRAFFIC_MEMORY_H
#define	INCL_NETWORK_TRAFFIC_MEMORY_H

#include "DArray2.h"
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
            virtual ~NetworkTrafficMemory (void);

            /**
             * Add a message to NetworkTrafficMemory
             * NOTE: the content of pMSg is copied.
             */
            int add (MessageHeader *pMH, int64 ui64ReceivingTime);

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

            const uint16 _ui16IgnoreRequestTime;

        private:
            struct FragmentWrapper
            {
                FragmentWrapper (uint32 ui32FragOffset, uint32 ui32FragEnd, int64 i64ReceivingTime);
                ~FragmentWrapper (void);

                bool operator > (const FragmentWrapper &rhsFragment);
                bool operator == (const FragmentWrapper &rhsFragment);
                bool operator < (const FragmentWrapper &rhsFragment);

                bool overlaps (uint32 ui32FragOffset, uint32 ui32FragEnd);

                uint32 _ui32Offset;
                uint32 _ui32End;
                int64 _i64LastServingTime;
            };

            class FragmentWrapperList : public NOMADSUtil::PtrLList<FragmentWrapper>
            {
                public:
                    FragmentWrapperList (void);
                    ~FragmentWrapperList (void);
            };

            typedef NOMADSUtil::UInt32Hashtable<FragmentWrapperList> FragmentedMessage;
            typedef NOMADSUtil::UInt32Hashtable<FragmentedMessage> MessagesBySender;
            typedef NOMADSUtil::StringHashtable<MessagesBySender> MessagesByGroup;

            int add (FragmentWrapperList *pFM, MessageHeader *pMH, int64 ui64ReceivingTime);
            bool expired (FragmentWrapper *pFragWr, int64 i64Now);

            void cleanUp (void);
            void cleanEmptyMessageBySender (MessagesByGroup * pMG);
            void cleanEmptyFragmentedMessage (MessagesBySender * pMS);
            void cleanEmptyChunkList (FragmentedMessage *pFM);
            void cleanExpiredFragments (FragmentWrapperList *pFragList);

            FragmentWrapperList * getOrAddFragmentedMessage (MessageHeader *pMH, bool bAdd);

        private:
            NOMADSUtil::StringHashtable<MessagesByGroup> _messageByGroup;
            NOMADSUtil::LoggingMutex _m;
    };

    inline bool NetworkTrafficMemory::expired (NetworkTrafficMemory::FragmentWrapper *pFragWr, int64 i64Now)
    {
        return (_ui16IgnoreRequestTime > 0) && ((i64Now - pFragWr->_i64LastServingTime) > _ui16IgnoreRequestTime);
    }

    inline NetworkTrafficMemory::FragmentWrapper::FragmentWrapper (uint32 ui32FragOffset, uint32 ui32FragEnd, int64 i64ReceivingTime)
        : _ui32Offset (ui32FragOffset),
          _ui32End (ui32FragEnd),
          _i64LastServingTime (i64ReceivingTime)
    {
    }

    inline NetworkTrafficMemory::FragmentWrapper::~FragmentWrapper (void)
    {
    }

    inline bool NetworkTrafficMemory::FragmentWrapper::operator > (const FragmentWrapper &rhsFragment)
    {
        return (_ui32Offset > rhsFragment._ui32Offset);
    }

    inline bool NetworkTrafficMemory::FragmentWrapper::operator == (const FragmentWrapper &rhsFragment)
    {
        return (_ui32Offset == rhsFragment._ui32Offset);
    }

    inline bool NetworkTrafficMemory::FragmentWrapper::operator < (const FragmentWrapper &rhsFragment)
    {
        return (_ui32Offset < rhsFragment._ui32Offset);
    }

    inline bool NetworkTrafficMemory::FragmentWrapper::overlaps (uint32 ui32FragOffset, uint32 ui32FragEnd)
    {
        return (!((ui32FragEnd <= _ui32Offset) || (ui32FragOffset >= _ui32End)));
    }

    inline NetworkTrafficMemory::FragmentWrapperList::FragmentWrapperList (void)
        : NOMADSUtil::PtrLList<FragmentWrapper> (false)
    {
    }

    inline NetworkTrafficMemory::FragmentWrapperList::~FragmentWrapperList (void)
    {
        FragmentWrapper *pFW;
        FragmentWrapper *pFWTmp = getFirst();
        while ((pFW = pFWTmp) != NULL) {
            pFWTmp = getNext();
            delete remove (pFW);
            pFW = NULL;
        }
    }
}

#endif  // INCL_NETWORK_TRAFFIC_MEMORY_H
