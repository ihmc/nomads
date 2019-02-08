/*
 * Subscription.h
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

#ifndef INCL_SUBSCRIPTION_H
#define INCL_SUBSCRIPTION_H

#include "History.h"

#include "DArray.h"
#include "LList.h"
#include "PtrLList.h"
#include "StrClass.h"
#include "UInt32Hashset.h"
#include "UInt32Hashtable.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class Message;

    class Subscription
    {
        public:
            Subscription (void);
            virtual ~Subscription (void);

            static const uint16 DUMMY_TAG = 0;  // If the dummy tag is used, it
                                                // means that the subscription
                                                // has no tag

            static const uint8 GROUP_SUBSCRIPTION = 0;
            static const uint8 GROUP_TAG_SUBSCRIPTION = 1;
            static const uint8 GROUP_PREDICATE_SUBSCRIPTION = 2;

            virtual int addHistory (History *pHistory, uint16 ui16Tag=0)=0;

            /**
             * Returns the reference to a copy of the cloned object.
             *
             * NOTE: this method does not copy the values of History !!!
             */
            virtual Subscription * clone (void)=0;
            virtual Subscription * getOnDemandSubscription (void) = 0;

            virtual int getHistoryRequest (const char *pszGroupName,
                                           NOMADSUtil::PtrLList<HistoryRequest> &historyRequest)=0;
            virtual uint8 getSubscriptionType (void) const;
            virtual uint8 getPriority (void)=0;

            virtual bool hasHistory (void)=0;
            virtual bool isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender)=0;

            /**
             * Returns true is the subscription includes pSubscription.
             *
             * In general:
             * GROUP_PREDICATE_SUBSCRIPTION includes
             *     GROUP_SUBSCRIPTION includes
             *         GROUP_TAG_SUBSCRIPTION
             *
             * NOTE: it should be called only for Subscriptions to the same group.
             */
            virtual bool includes (Subscription *pSubscription)=0;

            virtual bool matches (uint16 ui16Tag)=0;
            virtual bool matches (const Message *pMessage)=0;

            /**
             * The passed Subscription is modified so that it includes the
             * Subscription which calls the merge function.
             *
             * Returns true if pSubscription was modified, false otherwise
             */
            virtual bool merge (Subscription * pSubscription)=0;

            bool requireFullMessage (void);

            virtual int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)=0;
            virtual int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize)=0;

            virtual int printInfo (void) = 0;

        protected:
            struct Parameters
            {
                Parameters (uint8 ui8Priority=5, bool bReliable=false,
                            bool bMsgReliable=false, bool bSequenced=false);
                ~Parameters (void);

                int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
                int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

                uint8 _ui8Priority;
                bool _bGrpReliable;
                bool _bMsgReliable;
                bool _bSequenced;
            };

            uint8 _ui8SubscriptionType;
            bool _bRequireFullMessage;
    };

    //--------------------------------------------------------------------------
    // GroupSubscription
    //--------------------------------------------------------------------------

    class GroupSubscription : public Subscription
    {
        public:
            GroupSubscription (uint8 ui8Priority=5, bool bReliable=false,
                               bool bMsgReliable=false, bool bSequenced=false);
            ~GroupSubscription (void);

            uint8 addFilter (uint16 ui16Tag);
            int addHistory (History *pHistory, uint16 ui16Tag=0);

            /**
             * See the comment in the Subscription header
             */
            virtual Subscription * clone (void);
            Subscription * getOnDemandSubscription (void);

            NOMADSUtil::DArray<uint16> * getAllFilters();
            int getHistoryRequest (const char *pszGroupName,
                                   NOMADSUtil::PtrLList<HistoryRequest> &historyRequest);
            uint8 getPriority (void);

            bool hasFilter (uint16 ui16Tag);

            virtual bool includes (Subscription *pSubscription);

            bool hasHistory (void);
            bool isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender);
            bool isGroupReliable (void);
            bool isMsgReliable (void);
            bool isSequenced (void);

            bool matches (uint16 ui16Tag);
            bool matches (const Message *pMessage);
            bool merge (Subscription *pSubscription);

            int removeAllFilters (void);
            int removeFilter (uint16 ui16Tag);

            int setPriority (uint8 ui8Priority);

            int setGroupReliable (bool bGrpReliable);
            int setMsgReliable (bool bMsgReliable);
            int setSequenced (bool bSequenced);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            int printInfo (void);

        private:
            NOMADSUtil::UInt32Hashset _ui32HashFilteredTags;

            History *_pHistory;
            Parameters _parameters;
    };

    //--------------------------------------------------------------------------
    // GroupPredicateSubscription
    //--------------------------------------------------------------------------

    class GroupPredicateSubscription : public Subscription
    {
        public:
            GroupPredicateSubscription (const char *pszPredicate="", uint8 ui8PredicateType=0,
                                        uint8 ui8Priority=5, bool bReliable=false,
                                        bool bMsgReliable=false, bool bSequenced=false);
            ~GroupPredicateSubscription (void);

            int addHistory (History *pHistory, uint16 ui16Tag=0);

            /**
             * If the new predicate is not included in the current one, a new
             * predicate including the old one and the new one is generated.
             *
             * NOTE: for now, the merged predicate is generate just by ORing the
             * predicates to be merged.
             */
            virtual int addPredicate (const char *pszPredicate);

            /**
             * See the comment in the Subscription header
             */
            virtual Subscription * clone (void);
            Subscription * getOnDemandSubscription (void);

            int getHistoryRequest (const char *pszGroupName,
                                   NOMADSUtil::PtrLList<HistoryRequest> &historyRequest);
            const char * getPredicate (void);
            uint8 getPredicateType (void);
            uint8 getPriority (void);

            virtual bool includes (Subscription *pSubscription);

            bool hasHistory (void);
            bool isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender);
            bool isGroupReliable (void);
            bool isMsgReliable (void);
            bool isSequenced (void);

            bool matches (uint16 ui16Tag);
            bool matches (const Message *pMessage);
            bool merge (Subscription *pSubscription);

            int setPriority (uint8 ui8Priority);

            int setGroupReliable (bool bGrpReliable);
            int setMsgReliable (bool bMsgReliable);
            int setSequenced (bool bSequenced);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            int printInfo (void);

        private:
            uint8 _ui8PredicateType;
            NOMADSUtil::String _predicate;

            History *_pHistory;
            Parameters _parameters;
    };

    //--------------------------------------------------------------------------
    // GroupTagSubscription
    //--------------------------------------------------------------------------

    class GroupTagSubscription : public Subscription
    {
        public:
            GroupTagSubscription (uint16 ui16Tag = 0, uint8 ui8Priority=5, bool bReliable=false,
                                  bool bMsgReliable=false, bool bSequenced=false);
            ~GroupTagSubscription (void);

            int addHistory (History *pHistory, uint16 ui16Tag=0);
            int addTag (uint16 ui16Tag, uint8 ui8Priority, bool bGrpReliable, bool bMsgReliable, bool bSequenced);

            /**
             * See the comment in the Subscription header
             */
            virtual Subscription * clone (void);
            Subscription * getOnDemandSubscription (void);

            int getHistoryRequest (const char *pszGroupName,
                                   NOMADSUtil::PtrLList<HistoryRequest> &historyRequest);
            /**
             * Every tag has its own priority. Calling getPriority without
             * specifying the tag returns the highest value of priority in
             * the GroupTagSubscription.
             */
            uint8 getPriority (void);
            uint8 getPriority (uint16 ui16Tag);
            NOMADSUtil::LList<uint16> * getTags (void);

            bool hasTag (uint16 ui16Tag);

            virtual bool includes (Subscription *pSubscription);

            bool hasHistory (void);
            bool isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender);
            bool isGroupReliable (uint16 ui16Tag);
            bool isMsgReliable (uint16 ui16Tag);
            bool isSequenced (uint16 ui16Tag);

            bool matches (uint16 ui16Tag);
            bool matches (const Message *pMessage);
            bool merge (Subscription *pSubscription);

            int setPriority (uint8 ui8Priority, uint16 ui16Tag);

            int setGroupReliable (bool bGrpReliable, uint16 ui16Tag);
            int setMsgReliable (bool bMsgReliable, uint16 ui16Tag);
            int setSequenced (bool bSequenced, uint16 ui16Tag);

            int removeTag (uint16 ui16Tag);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            int printInfo (void);

        private:
            struct TagInfo
            {
                History *pHistory;

                Parameters _parameters;
            };
            NOMADSUtil::UInt32Hashtable<TagInfo> _ui16Tags;
            uint8 _ui8HighestPriority;
    };
}

#endif  // INCL_SUBSCRIPTION_H
