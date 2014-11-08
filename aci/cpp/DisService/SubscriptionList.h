/**
 * SubscriptionList.h
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
 * Keeps the list of subscriptions subscribed by a client (application).
 * There are currently 3 types of subscription supported:
 * - group subscription
 * - group and tag
 * - group and XPath predicate subscription
 *
 * Attempts to add subscriptions to a group already subscribed (regardless the type
 * of the subscription) will fail.
 *
 * Attempts to add subscription whose group's template matches with an already
 * subscribed group's template will fail.
 *
 * NOTE: value 0 is reserved and CAN NOT be used.
 * NOTE: a subscription meant to accept any tag for the group must be created using 
 *       addGroup (pszGroupName, ui8Priority, bReliable, bSequenced),
 *       calling
 *       addGroup (pszGroupName, WildCard::ANY_TAG, ui8Priority, bReliable, bSequenced)
 *       will return an error.
 * NOTE: the wildcard WildCard::ANY_TAG can not be used neither to remove tags, nor
 *       to add tags.
 */

#ifndef INCL_SUBSCRIPTION_LIST_H
#define INCL_SUBSCRIPTION_LIST_H

#include "DisServiceMsg.h"
#include "History.h"
#include "Subscription.h"

#include "LList.h"
#include "PtrLList.h"
#include "StringHashtable.h"
#include "UInt32Hashtable.h"

namespace NOMADSUtil
{
    class String;
    class Reader;
    class Writer;
    template <class T> class LList;
    template <class T> class PtrLList;
    template <class T> class StringHashtable;
}

namespace IHMC_ACI
{
    class Message;

    class SubscriptionList
    {
        public:
            SubscriptionList (void);
            virtual ~SubscriptionList (void);

            void clear (void);

            // NOTE: should this method used also to update the priority of a group?
            // In the current implementation it does not do it. If an entry for the
            // group belongs the _groupSubscriptions addGroup does not add the new
            // entry and returns 1
            int addGroup (const char *pszGroupName, Subscription *pSubscription);

            int addFilterToGroup (const char *pszGroupName, uint16 ui16Tag);

            int modifyPriority (const char *pszGroupName, uint8 ui8NewPriority);
            int modifyPriority (const char *pszGroupName, uint16 ui16Tag, uint8 ui8NewPriority);

            int removeFilterFromGroup (const char *pszGroupName, uint16 ui16Tag);
            int removeAllFiltersFromGroup (const char *pszGroupName);

            int removeGroup (const char *pszGroupName);

            // Remove the subscription specified to the group name
            // If the Subscription is of type Group-Tag, then the specified tags are removed using removeGroupTag()
            int removeGroup (const char *pszGroupName, Subscription *pSubscription);

            // NOTE: if no more tags are associated to the group, the group itself is removed.
            int removeGroupTag (const char *pszGroupName, uint16 ui16Tag);

            int addSubscription (const char *pzsGroupName, Subscription *pSubscription);

            void getHistoryRequests (NOMADSUtil::PtrLList<HistoryRequest> &historyRequest);
            Subscription * getSubscription (const char *pszGroupName);
            NOMADSUtil::PtrLList<Subscription> * getSubscriptionWild (const char *pszTemplate);

            /**
             * Checks if the client has subscribed to the specified group (and tag)
             * Calling hasGenericSubscription (pszGroupName) means that there's 
             * no interest in the kind of the subscription (that is
             * GroupSubscription rather than GroupTagSubscription)
             *
             * The two methods hasSubscription look for exact matches in the
             * subscription list.
             */
            bool hasGenericSubscription (const char *pszGroupName);
            bool hasGenericSubscriptionWild (const char *pszTemplate);

            bool hasSubscription (Message *pMessage);
            bool hasSubscriptionWild (Message *pMessage);

            bool isGroupSubscription (const char *pszGroupName);
            bool isGroupTagSubscription (const char *pszGroupName);
            bool isGroupPredicateSubscription (const char *pszGroupName);

            /**
             * Checks whether there is any client with a subscription matching 
             * pszGroupName (and ui16Tag) the client has subscribed to the
             * specified group (and tag) and requested reliable delivery.
             */
            bool requireReliability (const char *pszGroupName);
            bool requireSequentiality (const char *pszGroupName);
            bool requireReliability (const char *pszGroupName, uint16 ui16Tag);
            bool requireSequentiality (const char *pszGroupName, uint16 ui16Tag);

            bool isEmpty (void);

            /**
             * It returns the priority of a certain subscription.
             * If no tag is specified and the subscription is of type GROUP_TAG
             * the highest value of priority is returned.
             * It returns 0 if no subscription matching the group name, or the
             * group name and tag is specified, are found.
             *
             * The getPriorityWild methods are analogous to the getPriority ones.
             * However, whereas getPriority looks for exact match between
             * pszGroupName and the name of the group, getPriorityWild methods
             * allows the use of wildcards and use pszGroupName as a template.
             * If more than one subscription is found the hightest value of
             * priority is returned.
             */
            uint8 getPriority (const char *pszGroupName);
            uint8 getPriority (const char *pszGroupName, uint16 ui16Tag);
            uint8 getPriorityWild (const char *pszGroupName);
            uint8 getPriorityWild (const char *pszGroupName, uint16 ui16Tag);

            NOMADSUtil::PtrLList<NOMADSUtil::String> * getAllSubscribedGroups (void);

            bool isWildGroup (const char *pszString);

            void display (void);
            
            uint8 getCount (void);
            NOMADSUtil::StringHashtable<Subscription>::Iterator getIterator (void);

        private:
            NOMADSUtil::StringHashtable<Subscription> _subscriptions;            // Key is the group name
    };
}

#endif   // #ifndef INCL_SUBSCRIPTION_LIST_H
