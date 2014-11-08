/*
 * SubscriptionList.cpp
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
 */

#include "SubscriptionList.h"

#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "Message.h"
#include "MessageInfo.h"

#include "Subscription.h"

#include "LList.h"
#include "Logger.h"
#include "Reader.h"
#include "StrClass.h"
#include "Writer.h"
#include "History.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

SubscriptionList::SubscriptionList (void)
    : _subscriptions (true,
                      true, // bCloneKeys
                      true, // bDeleteKeys
                      true) // bDeleteValues)
{
}

SubscriptionList::~SubscriptionList(void)
{
}

void SubscriptionList::clear (void)
{
    _subscriptions.removeAll();
}

int SubscriptionList::addGroup (const char *pszGroupName, Subscription *pSubscription)
{
    if (_subscriptions.containsKey (pszGroupName)) {
        return -1;
    }
    long lCount = _subscriptions.getCount();
    _subscriptions.put (pszGroupName, pSubscription);
    return (_subscriptions.getCount() - lCount == 1) ? 0 : -2;   // Makes sure the
                                                                 // subscription was added
}

int SubscriptionList::addFilterToGroup (const char *pszGroupName, uint16 ui16Tag)
{
    Subscription *pSub = _subscriptions.get(pszGroupName);
    if ((pSub == NULL) || (pSub->getSubscriptionType() != Subscription::GROUP_SUBSCRIPTION)) {
        return -1;
    }
    ((GroupSubscription *) pSub)->addFilter (ui16Tag);
    return 0;
}

int SubscriptionList::modifyPriority (const char *pszGroupName, uint8 ui8NewPriority)
{
    Subscription *pSub = _subscriptions.get(pszGroupName);
    if ((pSub == NULL) || (pSub->getSubscriptionType() != Subscription::GROUP_SUBSCRIPTION)) {
        return -1;
    }
    ((GroupSubscription *) pSub)->setPriority (ui8NewPriority);
    return 0;
}

int SubscriptionList::removeFilterFromGroup (const char *pszGroupName, uint16 ui16Tag)
{
    Subscription *pSub = _subscriptions.get (pszGroupName);
    if ((pSub == NULL) || (pSub->getSubscriptionType() != Subscription::GROUP_SUBSCRIPTION)) {
        return -1;
    }
    ((GroupSubscription *) pSub)->removeFilter(ui16Tag);
    return 0;
}

int SubscriptionList::removeAllFiltersFromGroup (const char *pszGroupName)
{
    Subscription *pSub = _subscriptions.get(pszGroupName);
    if ((pSub == NULL) || (pSub->getSubscriptionType() != Subscription::GROUP_SUBSCRIPTION)) {
        return -1;
    }
    ((GroupSubscription *) pSub)->removeAllFilters();
    return 0;
}

int SubscriptionList::removeGroup (const char *pszGroupName)
{
    _subscriptions.remove (pszGroupName);
    return 0;
}

int SubscriptionList::removeGroup (const char *pszGroupName, Subscription *pSubscription)
{
    if (pSubscription->getSubscriptionType() == Subscription::GROUP_TAG_SUBSCRIPTION) {
        LList<uint16> *pTagsList = ((GroupTagSubscription*) pSubscription)->getTags();
        pTagsList->resetGet();
        uint16 ui16Tag;
        while (pTagsList->getNext (ui16Tag)) {
            if (0 != removeGroupTag (pszGroupName, ui16Tag)) {
                return -1;
            }
        }
    }
    else {
        return removeGroup (pszGroupName);
    }
    return 0;
}

int SubscriptionList::modifyPriority (const char *pszGroupName, uint16 ui16Tag, uint8 ui8NewPriority)
{
    Subscription *pSub = _subscriptions.get(pszGroupName);
    if ((pSub == NULL) || (pSub->getSubscriptionType() != Subscription::GROUP_TAG_SUBSCRIPTION)) {
        return -1;
    }

    return ((GroupTagSubscription *) pSub)->setPriority(ui8NewPriority, ui16Tag);
}

int SubscriptionList::removeGroupTag (const char *pszGroupName, uint16 ui16Tag)
{
    Subscription *pSub = _subscriptions.get(pszGroupName);
    if ((pSub == NULL) || (pSub->getSubscriptionType() != Subscription::GROUP_TAG_SUBSCRIPTION)) {
        return -1;
    }

    GroupTagSubscription * pGTS = (GroupTagSubscription *) pSub;
    if (pGTS->removeTag(ui16Tag) == 0) {
        if (pGTS->getTags()->length == 0) {
            _subscriptions.remove(pszGroupName);
        }
        return 0;
    }
    return -1;
}

int SubscriptionList::addSubscription (const char * pzsGroupName, Subscription *pSubscription)
{
    if (_subscriptions.containsKey(pzsGroupName)) {
        return -1;
    }
    _subscriptions.put (pzsGroupName, pSubscription);
    return 0;
}

void SubscriptionList::getHistoryRequests (PtrLList<HistoryRequest> &historyRequest)
{
    StringHashtable<Subscription>::Iterator i = _subscriptions.getAllElements();
    while (!i.end()) {
        const char * pszGroupName = i.getKey();
        Subscription *pS = i.getValue();
        if (pS) {
            if (pS->hasHistory()) {
                pS->getHistoryRequest(pszGroupName, historyRequest);
            }
        }
        i.nextElement();
    }
}

Subscription * SubscriptionList::getSubscription (const char *pszGroupName)
{
    return _subscriptions.get(pszGroupName);
}

PtrLList<Subscription> * SubscriptionList::getSubscriptionWild (const char *pszTemplate)
{
    PtrLList<Subscription> * pRet = NULL;
    for (StringHashtable<Subscription>::Iterator i = _subscriptions.getAllElements(); !i.end(); i.nextElement()) {
        if (wildcardStringCompare(pszTemplate, i.getKey()) || wildcardStringCompare(i.getKey(), pszTemplate)) {
            if (pRet == NULL) {
                pRet = new PtrLList<Subscription>();
                if (pRet == NULL) {
                    checkAndLogMsg ("SubscriptionList::getSubscriptionWild", memoryExhausted);
                    return NULL;
                }
            }
            pRet->append (i.getValue());
        }
    }
    return pRet;
}

bool SubscriptionList::hasGenericSubscription (const char *pszGroupName)
{
    return (_subscriptions.get(pszGroupName) != NULL);
}

bool SubscriptionList::hasGenericSubscriptionWild (const char *pszTemplate)
{
    for (StringHashtable<Subscription>::Iterator i = _subscriptions.getAllElements(); !i.end(); i.nextElement()) {
        if (wildcardStringCompare(pszTemplate, i.getKey()) || wildcardStringCompare(i.getKey(), pszTemplate)) {
            return true;
        }
    }
    return false;
}

bool SubscriptionList::hasSubscription (Message *pMessage)
{
    Subscription *pSub = _subscriptions.get(pMessage->getMessageInfo()->getGroupName());
    if (pSub != NULL){
        return pSub->matches(pMessage);
    }
    else {
        return false;
    }
}

bool SubscriptionList::hasSubscriptionWild (Message *pMessage)
{
    const char * pszGroupName = pMessage->getMessageInfo()->getGroupName();
    for (StringHashtable<Subscription>::Iterator i = _subscriptions.getAllElements(); !i.end(); i.nextElement()) {
        // for each subscription matching the group name
        if (wildcardStringCompare(pszGroupName, i.getKey()) || wildcardStringCompare(i.getKey(), pszGroupName)) {
            if ((i.getValue())->matches(pMessage)) {
                // if there's 1 or more matching the whole subscription return true
                return true;
            }
        }
    }
    // false otherwise
    return false;
}

bool SubscriptionList::isGroupSubscription (const char *pszGroupName)
{
    return (_subscriptions.get(pszGroupName)->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION);
}

bool SubscriptionList::isGroupTagSubscription (const char *pszGroupName)
{
    return (_subscriptions.get(pszGroupName)->getSubscriptionType() == Subscription::GROUP_TAG_SUBSCRIPTION);
}

bool SubscriptionList::isGroupPredicateSubscription (const char *pszGroupName)
{
    return (_subscriptions.get(pszGroupName)->getSubscriptionType() == Subscription::GROUP_PREDICATE_SUBSCRIPTION);
}

bool SubscriptionList::requireReliability (const char *pszGroupName)
{
    return requireReliability (pszGroupName, 0);
}

bool SubscriptionList::requireSequentiality (const char *pszGroupName)
{
    return requireSequentiality (pszGroupName, 0);
}

bool SubscriptionList::requireReliability (const char *pszGroupName, uint16 ui16Tag)
{
    PtrLList<Subscription> * pSubscriptions = getSubscriptionWild (pszGroupName);
    if (pSubscriptions == NULL) {
        // There is no subscription for pszGroupName
        return false;
    }

    bool bRet = false;
    for (Subscription * pS = pSubscriptions->getFirst(); pS != NULL; pS = pSubscriptions->getNext()) {
        if (ui16Tag == 0) {
            if (pS->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION) {
                if (((GroupSubscription*) pS)->isGroupReliable()) {
                    bRet = true;
                    break;
                }
            }
            else if (pS->getSubscriptionType() == Subscription::GROUP_PREDICATE_SUBSCRIPTION) {
                if (((GroupPredicateSubscription*) pS)->isGroupReliable()) {
                    bRet = true;
                    break;
                }
            }
            checkAndLogMsg ("SubscriptionList::requireReliability", Logger::L_Warning,
                            "Unknown type of Subscription or GroupTagSubscription with (ui16Tag = 0)\n");
        }
        else {
            if (pS->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION) {
                if (((GroupSubscription*) pS)->hasFilter(ui16Tag)) {
                    // This tag must be filtered!
                    continue;
                }
                else if (((GroupSubscription*) pS)->isGroupReliable()) {
                    bRet = true;
                    break;
                }
            }
            else {
                if (pS->getSubscriptionType() == Subscription::GROUP_TAG_SUBSCRIPTION) {
                    if (((GroupTagSubscription*) pS)->isGroupReliable(ui16Tag)) {
                        bRet = true;
                        break;
                    }
                }
            }
            checkAndLogMsg ("SubscriptionList::requireReliability", Logger::L_Warning,
                            "Unknown type of Subscription or GroupPredicateSucscription with (ui16Tag != 0)\n");
        }
    }

    delete pSubscriptions;
    pSubscriptions = NULL;
    return bRet;
 }

bool SubscriptionList::requireSequentiality (const char *pszGroupName, uint16 ui16Tag)
{
    PtrLList<Subscription> * pSubscriptions = getSubscriptionWild (pszGroupName);
    if (pSubscriptions == NULL) {
        // There is no subscription for pszGroupName
        return false;
    }

    bool bRet = false;
    for (Subscription * pS = pSubscriptions->getFirst(); pS != NULL; pS = pSubscriptions->getNext()) {
        if (ui16Tag == 0) {
            if (pS->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION) {
                if (((GroupSubscription*)pS)->isSequenced()) {
                    bRet = true;
                    break;
                }
            }
            else if (pS->getSubscriptionType() == Subscription::GROUP_PREDICATE_SUBSCRIPTION) {
                if (((GroupPredicateSubscription*) pS)->isSequenced()) {
                    bRet = true;
                    break;
                }
            }
            checkAndLogMsg ("SubscriptionList::requireSequentiality", Logger::L_Warning,
                            "Unknown type of Subscription or GroupTagSubscription with (ui16Tag = 0)\n");
        }
        else {
            if (pS->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION) {
                if (((GroupSubscription*) pS)->hasFilter(ui16Tag)) {
                    // This tag must be filtered!
                    continue;
                }
                else if (((GroupSubscription*) pS)->isSequenced()) {
                    bRet = true;
                    break;
                }
            }
            else {
                if (pS->getSubscriptionType() == Subscription::GROUP_TAG_SUBSCRIPTION) {
                    if (((GroupTagSubscription*) pS)->isSequenced(ui16Tag)) {
                        bRet = true;
                        break;
                    }
                }
            }
            checkAndLogMsg ("SubscriptionList::requireSequentiality", Logger::L_Warning,
                            "Unknown type of Subscription or GroupPredicateSucscription with (ui16Tag != 0)\n");
        }
    }

    delete pSubscriptions;
    pSubscriptions = NULL;
    return bRet;
}

bool SubscriptionList::isEmpty (void)
{
    checkAndLogMsg ("SubscriptionList::isEmpty", Logger::L_Info,
                    "Subscriptions: %d\n", _subscriptions.getCount());
    return (_subscriptions.getCount() == 0);
}

uint8 SubscriptionList::getPriority (const char * pszGroupName)
{
    Subscription * pSub = _subscriptions.get(pszGroupName);
    if (pSub) {
        return pSub->getPriority();
    }
    return 0;
}

uint8 SubscriptionList::getPriority (const char * pszGroupName, uint16 ui16Tag)
{
    Subscription * pSub = _subscriptions.get(pszGroupName);
    if (pSub != NULL) {
        if (pSub->getSubscriptionType() == Subscription::GROUP_TAG_SUBSCRIPTION) {
            if (ui16Tag == 0) {
                checkAndLogMsg ("SubscriptionList::requireSequentiality", Logger::L_Warning,
                                "Unknown type of Subscription or GroupTagSubscription with (ui16Tag = 0)\n");
            }
            else {
                return ((GroupTagSubscription *) pSub)->getPriority(ui16Tag);
            }
        }
        else if (pSub->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION && (((GroupSubscription *) pSub)->hasFilter(ui16Tag))) {
            return 0;
        }
        else {
            return pSub->getPriority();
        }
    }
    return 0;
}

uint8 SubscriptionList::getPriorityWild (const char * pszGroupName)
{
    uint8 ui8MaxPriority = 0;
    PtrLList<Subscription> * pSubscriptions = getSubscriptionWild (pszGroupName);
    if (pSubscriptions != NULL) {
        for (Subscription * pSub = pSubscriptions->getFirst(); pSub != NULL; pSub = pSubscriptions->getNext()) {
            if (pSub->getSubscriptionType() != Subscription::GROUP_TAG_SUBSCRIPTION) {
                if (pSub->getPriority() > ui8MaxPriority) {
                    ui8MaxPriority = pSub->getPriority();
                }
            }
        }

        delete pSubscriptions;
        pSubscriptions = NULL;
    }

    return ui8MaxPriority;
}

uint8 SubscriptionList::getPriorityWild (const char * pszGroupName, uint16 ui16Tag)
{
    uint8 ui8MaxPriority = 0;
    PtrLList<Subscription> * pSubscriptions = getSubscriptionWild (pszGroupName);
    if (pSubscriptions != NULL) {
        uint8 ui8TmpPriority;
        for (Subscription * pSub = pSubscriptions->getFirst(); pSub != NULL; pSub = pSubscriptions->getNext()) {
            if (pSub->getSubscriptionType() == Subscription::GROUP_TAG_SUBSCRIPTION) {
                if (ui16Tag == 0) {
                    checkAndLogMsg ("SubscriptionList::requireSequentiality", Logger::L_Warning,
                                    "Unknown type of Subscription or GroupTagSubscription with (ui16Tag = 0)\n");
                    ui8TmpPriority = 0;
                }
                else {
                    ui8TmpPriority = ((GroupTagSubscription *) pSub)->getPriority(ui16Tag);
                }
            }
            else if (pSub->getSubscriptionType() == Subscription::GROUP_SUBSCRIPTION && (((GroupSubscription *) pSub)->hasFilter(ui16Tag))) {
                ui8TmpPriority = 0;
            }
            else {
                ui8TmpPriority = pSub->getPriority();
            }

            if (ui8TmpPriority > ui8MaxPriority) {
                ui8MaxPriority = ui8TmpPriority;
            }
        }

        delete pSubscriptions;
        pSubscriptions = NULL;
    }
    return ui8MaxPriority;
}

PtrLList<String> * SubscriptionList::getAllSubscribedGroups (void)
{
    PtrLList<String> *pRet = new PtrLList<String>();
    if (pRet != NULL) {
        for (StringHashtable<Subscription>::Iterator i = _subscriptions.getAllElements(); !i.end(); i.nextElement()) {
            pRet->append(new String(i.getKey()));
        }
    }
    else {
        checkAndLogMsg ("SubscriptionList::getAllSubscribedGroups", memoryExhausted);
    }
    return pRet;
}

bool SubscriptionList::isWildGroup (const char * pszString)
{
    return ((pszString[0] == '*') || (pszString[strlen(pszString)-1] == '*'));
}

void SubscriptionList::display (void)
{
    for (StringHashtable<Subscription>::Iterator i = _subscriptions.getAllElements(); !i.end(); i.nextElement()) {
        const char * pszKey = i.getKey();
        Subscription * pSub = i.getValue();
        uint8 ui8Type = pSub->getSubscriptionType();
        printf ("Subscription to group <%s> of type <%d>", pszKey, pSub->getSubscriptionType());
        if (ui8Type == Subscription::GROUP_SUBSCRIPTION) {
            DArray<uint16> * pFilters = ((GroupSubscription*)pSub)->getAllFilters();
            if (pFilters) {
                long lHighestIndex = pFilters->getHighestIndex();
                if (lHighestIndex >= 0) {
                    printf (" with filters ");
                    for (long i = 0; i <= lHighestIndex; i++) {
                        printf ("%d ", (*pFilters)[i]);
                    }
                }
            }
            printf ("\n");
        }
        else if (ui8Type == Subscription::GROUP_TAG_SUBSCRIPTION) {
            printf (" with tags ");
            LList<uint16> * pTags = ((GroupTagSubscription*)pSub)->getTags();
            uint16 ui16Tag;
            for (int i = 0; i < pTags->length; i++) {
                pTags->getNext (ui16Tag);
                printf ("%d ", ui16Tag);
            }
            printf ("\n");
        }
        else if (ui8Type == Subscription::GROUP_PREDICATE_SUBSCRIPTION) {
            printf (" with predicate <%s>\n", ((GroupPredicateSubscription*)pSub)->getPredicate());
        }
    }
}

uint8 SubscriptionList::getCount (void)
{
    return _subscriptions.getCount();
}

StringHashtable<Subscription>::Iterator SubscriptionList::getIterator (void)
{
    return _subscriptions.getAllElements();
}
