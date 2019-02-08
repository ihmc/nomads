/*
 * Subscription.cpp
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

#include "Subscription.h"

#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "History.h"
#include "Message.h"
#include "MessageInfo.h"

#include "BufferWriter.h"
#include "NLFLib.h"
#include "Writer.h"

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4267)
#endif

#include "xpath_static.h"
#include "tinyxml.h"

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

using namespace IHMC_ACI;
using namespace NOMADSUtil;
using namespace TinyXPath;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

//------------------------------------------------------------------------------
//  Subscription
//------------------------------------------------------------------------------

Subscription::Subscription (void)
{
}

Subscription::~Subscription (void)
{
}

uint8 Subscription::getSubscriptionType (void) const
{
    return _ui8SubscriptionType;
}

bool Subscription::requireFullMessage (void)
{
    return _bRequireFullMessage;
}

//------------------------------------------------------------------------------
//  Subscription::Parameters
//------------------------------------------------------------------------------

Subscription::Parameters::Parameters (uint8 ui8Priority, bool bReliable, bool bMsgReliable, bool bSequenced)
    : _ui8Priority (ui8Priority), _bGrpReliable (bReliable),
      _bMsgReliable (bMsgReliable), _bSequenced (bSequenced)
{
}

Subscription::Parameters::~Parameters (void)
{
}

int Subscription::Parameters::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }

    uint8 ui8Priority;
    bool bGrpReliable;
    bool bMsgReliable;
    bool bSequenced;

    if (pReader->read8 (&ui8Priority) < 0) {
        return -2;
    }
    if (pReader->readBool (&bGrpReliable) < 0) {
        return -3;
    }
    if (pReader->readBool (&bMsgReliable) < 0) {
        return -4;
    }
    if (pReader->readBool (&bSequenced) < 0) {
        return -5;
    }

    // Update the status only if all the values were read correctly
    _ui8Priority = ui8Priority;
    _bGrpReliable = bGrpReliable;
    _bMsgReliable = bMsgReliable;
    _bSequenced = bSequenced;

    return 0;
}

int Subscription::Parameters::write (Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (pWriter->write8 (&_ui8Priority) < 0) {
        return -2;
    }
    if (pWriter->writeBool (&_bGrpReliable) < 0) {
        return -3;
    }
    if (pWriter->writeBool (&_bMsgReliable) < 0) {
        return -4;
    }
    if (pWriter->writeBool (&_bSequenced) < 0) {
        return -5;
    }
    return 0;
}

//------------------------------------------------------------------------------
// GroupSubscription
//------------------------------------------------------------------------------

GroupSubscription::GroupSubscription (uint8 ui8Priority, bool bGrpReliable, bool bMsgReliable, bool bSequenced)
    : _parameters (ui8Priority, bGrpReliable, (bGrpReliable ? true : bMsgReliable), bSequenced)
{
    _ui8SubscriptionType = Subscription::GROUP_SUBSCRIPTION;
    _bRequireFullMessage = false;
    _pHistory = NULL;
}

GroupSubscription::~GroupSubscription (void)
{
}

uint8 GroupSubscription::addFilter (uint16 ui16Tag)
{
    _ui32HashFilteredTags.put (ui16Tag);
    return 0;
}

int GroupSubscription::addHistory (History *pHistory, uint16 ui16Tag)
{
    /* Do not do the following test because it prevents a history request with a specific tag number
       even though the group subscription is for all tags
    if (ui16Tag != Subscription::DUMMY_TAG) {
        return -1;
    } */
    if (pHistory == NULL) {
        return -2;
    }
    if (hasHistory()) {
        return -3;
    }
    _pHistory = pHistory;
    return 0;
}

Subscription * GroupSubscription::clone (void)
{
    return new GroupSubscription (_parameters._ui8Priority, _parameters._bGrpReliable,
                                  _parameters._bMsgReliable, _parameters._bSequenced);
}

Subscription * GroupSubscription::getOnDemandSubscription (void)
{
    return new GroupSubscription (_parameters._ui8Priority, false, true, false);
}

DArray<uint16> * GroupSubscription::getAllFilters (void)
{
    DArray<uint16> *pRet = NULL;
    if (_ui32HashFilteredTags.getCount() > 0) {
        uint32 ui32NumOfEl = (_ui32HashFilteredTags.getCount() > 4294967295U ? 4294967295U : _ui32HashFilteredTags.getCount());
        pRet = new DArray<uint16>(ui32NumOfEl);
        uint32 j = 0;
        for (UInt32Hashset::Iterator i = _ui32HashFilteredTags.getAllElements(); !i.end(); i.nextElement()) {
            (*pRet)[j] = i.getKey();
            j++;
        }
    }
    return pRet;
}

int GroupSubscription::getHistoryRequest (const char * pszGroupName, PtrLList<HistoryRequest> &historyRequest)
{
    HistoryRequestGroupTag * pReq = new HistoryRequestGroupTag;
    pReq->_pHistory = _pHistory;
    pReq->_pszGroupName = pszGroupName;
    pReq->_ui16Tag = 0;
    historyRequest.prepend(pReq);
    return 0;
}

uint8 GroupSubscription::getPriority (void)
{
    return _parameters._ui8Priority;
}

bool GroupSubscription::hasFilter (uint16 ui16Tag)
{
    return _ui32HashFilteredTags.contains (ui16Tag);
}

bool GroupSubscription::includes (Subscription *pSubscription)
{
    switch (pSubscription->getSubscriptionType()) {
        case GROUP_SUBSCRIPTION:
            return false;
        case GROUP_TAG_SUBSCRIPTION:
        {
            if (pSubscription->getPriority() > _parameters._ui8Priority) {
                return false;
            }
            return true;
        }
        case GROUP_PREDICATE_SUBSCRIPTION:
        {
            if (pSubscription->getPriority() > _parameters._ui8Priority) {
                return false;
            }
            return true;
        }
    }
    return false;
}

bool GroupSubscription::hasHistory (void)
{
    if (_pHistory) {
        if (_pHistory->isExpired()) {
            delete _pHistory;
            _pHistory = NULL;
        }
    }
    return (_pHistory != NULL);
}

bool GroupSubscription::isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender)
{
    if (_pHistory) {
        if (matches (pMsg)) {
            return _pHistory->isInHistory (pMsg, ui32LatestMsgRcvdPerSender);
        }
    }
    return false;
}

bool GroupSubscription::isGroupReliable (void)
{
    return _parameters._bGrpReliable;
}

bool GroupSubscription::isMsgReliable (void)
{
    return _parameters._bMsgReliable;
}

bool GroupSubscription::isSequenced (void)
{
    return _parameters._bSequenced;
}

bool GroupSubscription::matches (uint16 ui16Tag)
{
    return (!_ui32HashFilteredTags.contains (0));
}

bool GroupSubscription::matches (const Message *pMessage)
{
    uint16 ui16MessageTag = pMessage->getMessageInfo()->getTag();
    return (!_ui32HashFilteredTags.contains (ui16MessageTag));
}

/*
bool GroupSubscription::merge (Subscription *pSubscription)
{
    bool bRet = false;
    if (pSubscription->getSubscriptionType() == GROUP_SUBSCRIPTION) {
        GroupSubscription *pGS = (GroupSubscription *) pSubscription;
        DArray<uint16> *pFiltersInSubscription = pGS->getAllFilters();
        if (pFiltersInSubscription != NULL) {
            for (unsigned int i = 0; i < pFiltersInSubscription->size(); i++) {
                uint16 ui16 = (*pFiltersInSubscription)[i];
                if (_ui32HashFilteredTags.contains (ui16)) {
                    pGS->removeFilter (ui16);
                    bRet = true;
                }
            }
        }
    }
    return bRet;
}
*/

bool GroupSubscription::merge (Subscription *pSubscription)
{
    bool bRet = false;
    switch (pSubscription->getSubscriptionType()) {
        case GROUP_SUBSCRIPTION:
        {
            GroupSubscription *pGS = (GroupSubscription *) pSubscription;
            DArray<uint16> *pFiltersInSubscription = pGS->getAllFilters();
            if (pFiltersInSubscription != NULL) {
                for (unsigned int i = 0; i < pFiltersInSubscription->size(); i++) {
                    uint16 ui16 = (*pFiltersInSubscription)[i];
                    if (_ui32HashFilteredTags.contains (ui16)) {
                        pGS->removeFilter (ui16);
                        bRet = true;
                    }
                }
            }
            if (getPriority() > pGS->getPriority()) {
                pGS->setPriority (getPriority());
                bRet = true;
            }
            if (isGroupReliable() > pGS->isGroupReliable()) {
                pGS->setGroupReliable (isGroupReliable());
                bRet = true;
            }
            if (isMsgReliable() > pGS->isMsgReliable()) {
                pGS->setMsgReliable (isMsgReliable());
                bRet = true;
            }
            if (isSequenced() > pGS->isSequenced()) {
                pGS->setSequenced (isSequenced());
                bRet = true;
            }
            break;
        }
        case GROUP_PREDICATE_SUBSCRIPTION:
        {
            //TODO
            break;
        }
        case GROUP_TAG_SUBSCRIPTION:
        {
            //TODO
            break;
        }
    }
    return bRet;
}

int GroupSubscription::removeAllFilters (void)
{
    _ui32HashFilteredTags.removeAll();
    return 0;
}

int GroupSubscription::removeFilter (uint16 ui16Tag)
{
    if (_ui32HashFilteredTags.remove (ui16Tag)) {
        return 0;
    }
    return -1;
}

int GroupSubscription::setPriority (uint8 ui8Priority)
{
    _parameters._ui8Priority = ui8Priority;
    return 0;
}

int GroupSubscription::setGroupReliable (bool bGrpReliable)
{
    _parameters._bGrpReliable = bGrpReliable;
    return 0;
}

int GroupSubscription::setMsgReliable (bool bMsgReliable)
{
    _parameters._bMsgReliable = bMsgReliable;
    return 0;
}

int GroupSubscription::setSequenced (bool bSequenced)
{
    _parameters._bSequenced = bSequenced;
    return 0;
}

int GroupSubscription::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }

    return _parameters.read (pReader, ui32MaxSize);
}

int GroupSubscription::write (Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return -1;
    }
    return _parameters.write (pWriter, ui32MaxSize);;
}

int GroupSubscription::printInfo (void)
{
    checkAndLogMsg ("GroupSubscription::printInfo", Logger::L_Info, "                   Group subscription\n");
    checkAndLogMsg ("GroupSubscription::printInfo", Logger::L_Info, "                   prio %d, grel %d, mrel %d, seq %d\n", getPriority(), isGroupReliable(), isMsgReliable(), isSequenced());
    return 0;
}

//------------------------------------------------------------------------------
// GroupPredicateSubscription
//------------------------------------------------------------------------------

GroupPredicateSubscription::GroupPredicateSubscription (const char *pszPredicate, uint8 ui8PredicateType, uint8 ui8Priority,
                                                        bool bGrpReliable, bool bMsgReliable, bool bSequenced)
    : _ui8PredicateType (ui8PredicateType),
      _predicate (pszPredicate),
      _parameters (ui8Priority, bGrpReliable, (bGrpReliable ? true : bMsgReliable), bSequenced)
{
    _ui8SubscriptionType = Subscription::GROUP_PREDICATE_SUBSCRIPTION;
    _bRequireFullMessage = true;
    _pHistory = NULL;
}

GroupPredicateSubscription::~GroupPredicateSubscription (void)
{
}

int GroupPredicateSubscription::addHistory (History *pHistory, uint16 ui16Tag)
{
    if (ui16Tag != 0) {
        return -1;
    }
    if (pHistory == NULL) {
        return -2;
    }
    if (_pHistory != NULL) {
        return -3;
    }
    _pHistory = pHistory;
    return 0;
}

int GroupPredicateSubscription::addPredicate (const char *pszPredicate)
{
    // TODO: implement this
    String mergedPred = (String) _predicate + " | " + pszPredicate;
    return 0;
}

Subscription * GroupPredicateSubscription::clone (void)
{
    return new GroupPredicateSubscription (_predicate, _ui8PredicateType, _parameters._ui8Priority,
                                           _parameters._bGrpReliable, _parameters._bMsgReliable,
                                           _parameters._bSequenced);
}

Subscription * GroupPredicateSubscription::getOnDemandSubscription (void)
{
    return new GroupPredicateSubscription (_predicate, _ui8PredicateType, _parameters._ui8Priority,
                                           false, true, false);
}

int GroupPredicateSubscription::getHistoryRequest (const char * pszGroupName, PtrLList<HistoryRequest> &historyRequest)
{
    HistoryRequestPredicate * pReq = new HistoryRequestPredicate;
    pReq->_pHistory = _pHistory;
    pReq->_pszGroupName = pszGroupName;
    pReq->ui8PredicateType = _ui8PredicateType;
    pReq->_pszPredicate =  _predicate;
    historyRequest.prepend (pReq);
    return 0;
}

const char * GroupPredicateSubscription::getPredicate (void)
{
    return (const char *) _predicate;
}

uint8 GroupPredicateSubscription::getPredicateType (void)
{
    return _ui8PredicateType;
}

uint8 GroupPredicateSubscription::getPriority (void)
{
    return _parameters._ui8Priority;
}

bool GroupPredicateSubscription::includes (Subscription *pSubscription)
{
    switch (pSubscription->getSubscriptionType()) {
        case GROUP_SUBSCRIPTION:
        {
            return false;
            break;
        }
        case GROUP_TAG_SUBSCRIPTION:
        {
            if (pSubscription->getPriority() > _parameters._ui8Priority) {
                return false;
            }
            return true;
            break;
        }
        case GROUP_PREDICATE_SUBSCRIPTION:
        {
            return false;
            break;
        }
    }
    return true;
}

bool GroupPredicateSubscription::hasHistory (void)
{
    if (_pHistory) {
        if (_pHistory->isExpired()) {
            delete _pHistory;
            _pHistory = NULL;
        }
    }
    return (_pHistory != NULL);
}

bool GroupPredicateSubscription::isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender)
{
    if (_pHistory) {
        if (matches (pMsg)) {
            return _pHistory->isInHistory (pMsg, ui32LatestMsgRcvdPerSender);
        }
    }
    return false;
}

bool GroupPredicateSubscription::isGroupReliable (void)
{
    return _parameters._bGrpReliable;
}

bool GroupPredicateSubscription::isMsgReliable (void)
{
    return _parameters._bMsgReliable;
}

bool GroupPredicateSubscription::isSequenced (void)
{
    return _parameters._bSequenced;
}

bool GroupPredicateSubscription::matches (uint16 ui16Tag)
{
    return true;
}

bool GroupPredicateSubscription::matches (const Message *pMessage)
{
    if (_bRequireFullMessage && pMessage->getMessageInfo()->getTotalMessageLength() != pMessage->getMessageInfo()->getFragmentLength()) {
        printf ("Received a fragment for a full message subscription (keeping the fragment)\n");
        return true;
    }

    if (_ui8PredicateType == 0) {    //to change with the constant

        //simple subscriptions (group or group/tag) not found

        TiXmlElement * XEp_main;
        TiXmlDocument * XDp_doc;
        XDp_doc = new TiXmlDocument ("Doc title");
        const uint32 ui32Length = pMessage->getMessageInfo()->getTotalMessageLength() + 1U;

        char* pXmlDoc = (char*)malloc(ui32Length*sizeof(char));

        memcpy(pXmlDoc, pMessage->getData(), pMessage->getMessageInfo()->getTotalMessageLength());
        pXmlDoc [pMessage->getMessageInfo()->getTotalMessageLength()] = '\0';

        printf("XDp_doc: %s\n", XDp_doc -> Parse (pXmlDoc));

        printf("RECEIVED DATA: (%d/%d bytes)***********************************\n",pMessage->getMessageInfo()->getFragmentLength(), pMessage->getMessageInfo()->getTotalMessageLength());
        printf("%s\n",pXmlDoc);
        printf("END RECEIVED DATA: ***********************************\n");

        //printf("Data: %s\n",(char*)pMessage->getData());
        if( (XDp_doc -> Parse (pXmlDoc)) == NULL) { //data parsed
            XEp_main = XDp_doc -> RootElement ();
            if (XEp_main != NULL) {
                TIXML_STRING out = TinyXPath::S_xpath_string (XEp_main, _predicate);
                bool result = TinyXPath::o_xpath_bool(XEp_main, _predicate);
                printf("xpath result: %s %d\n", out.c_str(), result);

                free(XEp_main);

                if (result) {
                    free(XDp_doc);
                    return true;
                }
            }
            else {
                printf("Received non-XML data\n");
            }
        }
        else {
            printf("Received non-XML data\n");
        }
        free(XDp_doc);
    }

    return false;
}

/*
bool GroupPredicateSubscription::merge (Subscription *pSubscription)
{
    bool bRet = false;
    if (pSubscription->getSubscriptionType() == GROUP_PREDICATE_SUBSCRIPTION) {
        GroupPredicateSubscription *pGPS = (GroupPredicateSubscription*) pSubscription;
        String predicateToBeMerged = pGPS->getPredicate();
        if (predicateToBeMerged != _predicate) {
            pGPS->addPredicate (_predicate);
            bRet = true;
        }
    }
    return bRet;
}
*/

bool GroupPredicateSubscription::merge (Subscription *pSubscription)
{
    bool bRet = false;
    switch (pSubscription->getSubscriptionType()) {
        case GROUP_SUBSCRIPTION:
        {
            //TODO
            break;
        }
        case GROUP_PREDICATE_SUBSCRIPTION:
        {
            GroupPredicateSubscription *pGPS = (GroupPredicateSubscription*) pSubscription;
            String predicateToBeMerged = pGPS->getPredicate();
            if (predicateToBeMerged != _predicate) { //TODO if multiple predicates, I need to search among them
                pGPS->addPredicate (_predicate);
                bRet = true;
            }
            break;
        }
        case GROUP_TAG_SUBSCRIPTION:
        {
            //TODO
            break;
        }
    }
    return bRet;
}

int GroupPredicateSubscription::setPriority (uint8 ui8Priority)
{
    _parameters._ui8Priority = ui8Priority;
    return 0;
}

int GroupPredicateSubscription::setGroupReliable (bool bGrpReliable)
{
    _parameters._bGrpReliable = bGrpReliable;
    return 0;
}

int GroupPredicateSubscription::setMsgReliable (bool bMsgReliable)
{
    _parameters._bMsgReliable = bMsgReliable;
    return 0;
}

int GroupPredicateSubscription::setSequenced (bool bSequenced)
{
    _parameters._bSequenced = bSequenced;
    return 0;
}

/*
int GroupPredicateSubscription::read (Reader *pReader, uint32 ui32MaxSize)
{
    uint16 ui16lenght;
    pReader->read8 (&_ui8PredicateType);
    pReader->read16 (&ui16lenght);
    pReader->readBytes (&_predicate, ui16lenght);

    return 0;
}
*/

int GroupPredicateSubscription::read (Reader *pReader, uint32 ui32MaxSize)
{
    //TODO TEST
    if (pReader == NULL) {
        return -1;
    }

    // Read predicate
    uint16 ui16lenght;
    if (pReader->read16 (&ui16lenght) < 0) {
        return -2;
    }
    if (ui16lenght <= 0) {
        return -3;
    }
    char *pszBuf = (char *) calloc (ui16lenght +1, sizeof (char));
    if (pszBuf == NULL) {
        checkAndLogMsg ("GroupPredicateSubscription::read", memoryExhausted);
        return -4;
    }
    if (pReader->readBytes (&pszBuf, ui16lenght) < 0) {
        return -5;
    }
    pszBuf[ui16lenght] = '\0';

    // Read predicate type
    uint8 ui8PredicateType;
    if (pReader->read8 (&ui8PredicateType) < 0) {
        free (pszBuf);
        return -6;
    }

    if (_parameters.read (pReader, ui32MaxSize) < 0) {
        free (pszBuf);
        return -7;
    }

    _ui8PredicateType = ui8PredicateType;
    _predicate = pszBuf;
    free (pszBuf);

    return 0;
}

/*
int GroupPredicateSubscription::write (Writer *pWriter, uint32 ui32MaxSize)
{
    uint16 ui16lenght = _predicate.length();
    pWriter->write8 (&_ui8PredicateType);
    pWriter->write16 (&ui16lenght);
    pWriter->writeBytes (&_predicate, ui16lenght);

    return 0;
}
*/

int GroupPredicateSubscription::write (Writer *pWriter, uint32 ui32MaxSize)
{
    //TODO TEST
    if (pWriter == NULL) {
        return -1;
    }

    uint16 ui16Length = _predicate.length();
    if (ui16Length == 0) {
        return -2;
    }
    if (pWriter->write16 (&ui16Length) < 0) {
        return -3;
    }
    if (pWriter->writeBytes (_predicate.c_str(), ui16Length) < 0) {
        return -4;
    }
    if (pWriter->write8 (&_ui8PredicateType) < 0) {
        return -5;
    }
    if (_parameters.write (pWriter, ui32MaxSize) < 0) {
        return -6;
    }

    return 0;
}

int GroupPredicateSubscription::printInfo (void)
{
    checkAndLogMsg ("GroupPredicateSubscription::printInfo", Logger::L_Info, "          Group predicate subscription\n");
    checkAndLogMsg ("GroupPredicateSubscription::printInfo", Logger::L_Info, "          prio %d, grel %d, mrel %d, seq %d\n", getPriority(), isGroupReliable(), isMsgReliable(), isSequenced());
    return 0;
}

//------------------------------------------------------------------------------
// GroupTagSubscription
//------------------------------------------------------------------------------

GroupTagSubscription::GroupTagSubscription (uint16 ui16Tag, uint8 ui8Priority,
                                            bool bGrpReliable, bool bMsgReliable, bool bSequenced)
        : _ui16Tags()
{
    _ui8SubscriptionType = Subscription::GROUP_TAG_SUBSCRIPTION;
    _bRequireFullMessage = false;
    _ui8HighestPriority = ui8Priority;

    TagInfo *pNewTag = new TagInfo;
    pNewTag->pHistory = NULL;
    pNewTag->_parameters._ui8Priority = ui8Priority;
    pNewTag->_parameters._bGrpReliable = bGrpReliable;
    pNewTag->_parameters._bMsgReliable = (bGrpReliable ? true : bMsgReliable);
    pNewTag->_parameters._bSequenced = bSequenced;

    _ui16Tags.put (ui16Tag, pNewTag);
}

GroupTagSubscription::~GroupTagSubscription (void)
{
    _ui16Tags.removeAll();
}

int GroupTagSubscription::addHistory (History *pHistory, uint16 ui16Tag)
{
    if (ui16Tag == DUMMY_TAG) {
        return -1;
    }

    TagInfo *pTI = _ui16Tags.get (ui16Tag);
    if ((pTI != NULL) && (pTI->pHistory == NULL)) {
        pTI->pHistory = pHistory;
        return 0;
    }
    return -2;
}

/*
int GroupTagSubscription::addTag (uint16 ui16Tag, uint8 ui8Priority, bool bReliable, bool bSequenced)
{
    if (_ui16Tags.contains(ui16Tag)) {
        return -1;
    }

    TagInfo *pNewTag = new TagInfo;

    pNewTag->pHistory = NULL;

    pNewTag->ui8Priority = ui8Priority;
    pNewTag->bGrpReliable = bReliable;
    pNewTag->bSequenced = bSequenced;

    _ui16Tags.put (ui16Tag, pNewTag);

    if (_ui8HighestPriority < ui8Priority) {
        _ui8HighestPriority = ui8Priority;
    }

    return 0;
}
*/

int GroupTagSubscription::addTag (uint16 ui16Tag, uint8 ui8Priority, bool bGrpReliable, bool bMsgReliable, bool bSequenced)
{
    if (_ui16Tags.contains (ui16Tag)) {
        return -1;
    }
    TagInfo *pNewTag = new TagInfo;
    pNewTag->pHistory = NULL;
    pNewTag->_parameters._ui8Priority = ui8Priority;
    pNewTag->_parameters._bGrpReliable = bGrpReliable;
    pNewTag->_parameters._bMsgReliable = (bGrpReliable ? true : bMsgReliable);
    pNewTag->_parameters._bSequenced = bSequenced;
    _ui16Tags.put (ui16Tag, pNewTag);
    if (_ui8HighestPriority < ui8Priority) {
        _ui8HighestPriority = ui8Priority;
    }
    return 0;
}

Subscription * GroupTagSubscription::clone (void)
{
    UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements();
    GroupTagSubscription *pGTS = new GroupTagSubscription (i.getKey(), i.getValue()->_parameters._ui8Priority,
                                                           i.getValue()->_parameters._bGrpReliable, i.getValue()->_parameters._bMsgReliable,
                                                           i.getValue()->_parameters._bSequenced);
    for (; !i.end(); i.nextElement()) {
        pGTS->addTag (i.getKey(), i.getValue()->_parameters._ui8Priority, i.getValue()->_parameters._bGrpReliable,
                      i.getValue()->_parameters._bMsgReliable, i.getValue()->_parameters._bSequenced);
    }
    return pGTS;
}

Subscription * GroupTagSubscription::getOnDemandSubscription (void)
{
    UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements();
    GroupTagSubscription *pGTS = new GroupTagSubscription (i.getKey(), i.getValue()->_parameters._ui8Priority,
                                                           false, true, false);
    for (; !i.end(); i.nextElement()) {
        pGTS->addTag (i.getKey(), i.getValue()->_parameters._ui8Priority, false, true, false);
    }
    return pGTS;
}

int GroupTagSubscription::getHistoryRequest (const char * pszGroupName, PtrLList<HistoryRequest> &historyRequest)
{
    UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements();
    TagInfo *pTI;
    History *pHistory;
    while (!i.end()) {
        pTI = i.getValue();
        pHistory = pTI->pHistory;
        if (pHistory) {
            if (pHistory->isExpired()) {
                delete pHistory;
                pHistory = NULL;
            }
            else {
                HistoryRequestGroupTag * pReq = new HistoryRequestGroupTag;
                pReq->_pHistory = pTI->pHistory;
                pReq->_pszGroupName = pszGroupName;
                pReq->_ui16Tag = (uint16) i.getKey();
                historyRequest.prepend(pReq);
            }
        }
        i.nextElement();
    }
    return 0;
}

uint8 GroupTagSubscription::getPriority (void)
{
    return _ui8HighestPriority;
}

uint8 GroupTagSubscription::getPriority (uint16 ui16Tag)
{
    TagInfo *pTagInfo = _ui16Tags.get (ui16Tag);
    if (pTagInfo) {
        return pTagInfo->_parameters._ui8Priority;
    }
    return 0;
}

LList<uint16> *  GroupTagSubscription::getTags (void)
{
    if (_ui16Tags.getCount() <= 0) {
        return NULL;
    }
    LList<uint16> *pRet = new LList<uint16>();
    for (UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements(); !i.end(); i.nextElement()) {
        pRet->add (i.getKey());
    }
    return pRet;
}

bool GroupTagSubscription::hasTag (uint16 ui16Tag)
{
    return _ui16Tags.contains(ui16Tag);
}

bool GroupTagSubscription::includes (Subscription *pSubscription)
{
    if (pSubscription->getSubscriptionType() == GROUP_TAG_SUBSCRIPTION) {
        GroupTagSubscription *pGTS = (GroupTagSubscription*) pSubscription;
        LList<uint16> *pTags = pGTS->getTags();
        uint16 ui16Tag;
        pTags->getFirst (ui16Tag);
        for (int i = 0; i < pTags->length; i++) {
            if (!_ui16Tags.contains (ui16Tag)) {
                return false;
            }
            pTags->getNext (ui16Tag);
        }
        return true;
    }
    return false;
}

bool GroupTagSubscription::hasHistory()
{
    UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements();
    TagInfo *pTI;
    History *pHistory;
    while (!i.end()) {
        pTI = i.getValue();
        pHistory = pTI->pHistory;
        if (pHistory) {
            if (pHistory->isExpired()) {
                delete pHistory;
                pHistory = NULL;
            }
            else {
                return true;
            }
        }
        i.nextElement();
    }
    return false;
}

bool GroupTagSubscription::isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender)
{
    if (matches (pMsg)) {
        TagInfo *pTI = _ui16Tags.get (pMsg->getMessageInfo()->getTag());
        if (pTI) {
            History *pHistory = pTI->pHistory;
            if (pHistory) {
                return pHistory->isInHistory (pMsg, ui32LatestMsgRcvdPerSender);
            }
        }
    }
    return false;
}

bool GroupTagSubscription::isGroupReliable (uint16 ui16Tag)
{
    TagInfo *tInfo = _ui16Tags.get (ui16Tag);

    if (tInfo == NULL) {
        return false;
    }
    else {
        return tInfo->_parameters._bGrpReliable;
    }
}

bool GroupTagSubscription::isMsgReliable (uint16 ui16Tag)
{
    TagInfo *tInfo =_ui16Tags.get (ui16Tag);

    if (tInfo == NULL) {
        return false;
    }
    else {
        return tInfo->_parameters._bMsgReliable;
    }
}

bool GroupTagSubscription::isSequenced (uint16 ui16Tag)
{
    TagInfo *tInfo =_ui16Tags.get (ui16Tag);

    if (tInfo == NULL) {
        return false;
    }
    else {
        return tInfo->_parameters._bSequenced;
    }
}

bool GroupTagSubscription::matches (uint16 ui16Tag)
{
    return (_ui16Tags.contains (ui16Tag));
}

bool GroupTagSubscription::matches (const Message *pMessage)
{
    MessageInfo *pMI = pMessage->getMessageInfo();
    if (pMI == NULL) {
        return false;
    }
    uint16 ui16Tag = pMI->getTag();
    return (_ui16Tags.contains (ui16Tag));
}

/*
bool GroupTagSubscription::merge (Subscription *pSubscription)
{
    bool bRet = false;
    switch (pSubscription->getSubscriptionType()) {
        case GROUP_SUBSCRIPTION:
        {
            GroupSubscription *pGS = (GroupSubscription *) pSubscription;
            for (UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements(); !i.end(); i.nextElement()) {
                if ((0 == pGS->removeFilter (i.getKey())) && !bRet) {
                    bRet = true;
                }
            }
            break;
        }
        case GROUP_TAG_SUBSCRIPTION:
        {
            GroupTagSubscription *pGTS = (GroupTagSubscription *) pSubscription;
            for (UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements(); !i.end(); i.nextElement()) {
                if ((0 == pGTS->addTag (i.getKey(), 0, false, false)) && !bRet) {
                    bRet = true;
                }
            }
            break;
        }
    }
    return bRet;
}
*/

bool GroupTagSubscription::merge (Subscription *pSubscription)
{
    bool bRet = false;
    switch (pSubscription->getSubscriptionType()) {
        case GROUP_SUBSCRIPTION:
        {
            GroupSubscription *pGS = (GroupSubscription *) pSubscription;
            for (UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements(); !i.end(); i.nextElement()) {
                if ((0 == pGS->removeFilter (i.getKey())) && !bRet) {
                    bRet = true;
                }
            }
            TagInfo *pTagInfo;
            for (UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements(); !i.end(); i.nextElement()) {
                pTagInfo = i.getValue();
                if (pTagInfo->_parameters._ui8Priority  > pGS->getPriority()) {
                    pGS->setPriority (pTagInfo->_parameters._ui8Priority);
                    bRet = true;
                }
                if (pTagInfo->_parameters._bGrpReliable > pGS->isGroupReliable()) {
                    pGS->setGroupReliable (pTagInfo->_parameters._bGrpReliable);
                    bRet = true;
                }
                if (pTagInfo->_parameters._bMsgReliable > pGS->isMsgReliable()) {
                    pGS->setMsgReliable (pTagInfo->_parameters._bMsgReliable);
                    bRet = true;
                }
                if (pTagInfo->_parameters._bSequenced   > pGS->isSequenced()) {
                    pGS->setSequenced (pTagInfo->_parameters._bSequenced); bRet = true;
                }
            }
            break;
        }
        case GROUP_PREDICATE_SUBSCRIPTION:
        {
            //TODO
            break;
        }
        case GROUP_TAG_SUBSCRIPTION:
        {
            GroupTagSubscription *pGTS = (GroupTagSubscription *) pSubscription;
            TagInfo *pTagInfo;
            uint16 ui16Tag;
            for (UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements(); !i.end(); i.nextElement()) {
                ui16Tag = i.getKey();
                if (pGTS->hasTag (ui16Tag)) {
                    pTagInfo = i.getValue();
                    if (pTagInfo->_parameters._ui8Priority  > pGTS->getPriority (ui16Tag)) {
                        pGTS->setPriority (pTagInfo->_parameters._ui8Priority, ui16Tag);
                        bRet = true;
                    }
                    if (pTagInfo->_parameters._bGrpReliable > pGTS->isGroupReliable (ui16Tag)) {
                        pGTS->setGroupReliable (pTagInfo->_parameters._bGrpReliable, ui16Tag);
                        bRet = true;
                    }
                    if (pTagInfo->_parameters._bMsgReliable > pGTS->isMsgReliable (ui16Tag)) {
                        pGTS->setMsgReliable (pTagInfo->_parameters._bMsgReliable, ui16Tag);
                        bRet = true;
                    }
                    if (pTagInfo->_parameters._bSequenced   > pGTS->isSequenced (ui16Tag)) {
                        pGTS->setSequenced (pTagInfo->_parameters._bSequenced, ui16Tag);
                        bRet = true;
                    }
                }
                else if ((0 == pGTS->addTag (i.getKey(), pTagInfo->_parameters._ui8Priority,
                          pTagInfo->_parameters._bGrpReliable, pTagInfo->_parameters._bMsgReliable,
                          pTagInfo->_parameters._bSequenced)) && !bRet) {
                    bRet = true;
                }
            }
            break;
        }
    }
    return bRet;
}

int GroupTagSubscription::setPriority (uint8 ui8Priority, uint16 ui16Tag)
{
     TagInfo *pTagInfo = _ui16Tags.get(ui16Tag);
     if (pTagInfo) {
        pTagInfo->_parameters._ui8Priority = ui8Priority;
     }
     else {
        return -1;
     }
     return 0;
}

int GroupTagSubscription::setGroupReliable (bool bGrpReliable, uint16 ui16Tag)
{
     TagInfo *pTagInfo = _ui16Tags.get (ui16Tag);
     if (pTagInfo) {
        pTagInfo->_parameters._bGrpReliable = bGrpReliable;
     }
     else {
        return -1;
     }
     return 0;
}

int GroupTagSubscription::setMsgReliable (bool bMsgReliable, uint16 ui16Tag)
{
     TagInfo *pTagInfo = _ui16Tags.get(ui16Tag);
     if (pTagInfo) {
        pTagInfo->_parameters._bMsgReliable = bMsgReliable;
     }
     else {
        return -1;
     }
     return 0;
}

int GroupTagSubscription::setSequenced (bool bSequenced, uint16 ui16Tag)
{
     TagInfo *pTagInfo = _ui16Tags.get (ui16Tag);
     if (pTagInfo) {
        pTagInfo->_parameters._bSequenced = bSequenced;
     }
     else {
        return -1;
     }
     return 0;
}

int GroupTagSubscription::removeTag (uint16 ui16Tag)
{
    if (_ui16Tags.remove (ui16Tag)) {
        return 0;
    }
    return -1;
}

/*
int GroupTagSubscription::read (Reader *pReader, uint32 ui32MaxSize)
{
    uint16 ui16Count;
    pReader->read16 (&ui16Count);
    uint16 ui16Tag;
    for ( ; ui16Count > 0 ; ui16Count--) {
        pReader->read16 (&ui16Tag);
        addTag (ui16Tag, 0, 0, 0);
    }
    return 0;
}
*/

int GroupTagSubscription::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (pReader == NULL) {
        return -1;
    }
    if (pReader->read8 (&_ui8HighestPriority) < 0) {
        return -2;
    }
    uint16 ui16Count;
    if (pReader->read16 (&ui16Count) < 0) {
        return -3;
    }

    for ( ; ui16Count > 0 ; ui16Count--) {
        uint16 ui16Tag;
        pReader->read16 (&ui16Tag);

        Parameters parameters;
        if (parameters.read (pReader, ui32MaxSize) < 0) {
            return -2;
        }

        addTag (ui16Tag, parameters._ui8Priority, parameters._bGrpReliable,
                parameters._bMsgReliable, parameters._bSequenced);
    }
    return 0;
}

/*
int GroupTagSubscription::write (Writer *pWriter, uint32 ui32MaxSize)
{
    BufferWriter bw (ui32MaxSize, ui32MaxSize);
    uint16 ui16Count = 0;
    for (UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements() ; !i.end() ; i.nextElement()) {
        uint16 ui16Tag = i.getKey();
        if ((bw.getBufferLength() + sizeof (uint16)) <= ui32MaxSize) {
            bw.write16 (&ui16Tag);
            ui16Count++;
        }
    }

    pWriter->write16 (&ui16Count);
    pWriter->writeBytes (bw.getBuffer(), bw.getBufferLength());
    return 0;
}
*/

int GroupTagSubscription::write (Writer *pWriter, uint32 ui32MaxSize)
{
    if (pWriter == NULL) {
        return -1;
    }
    BufferWriter bw (ui32MaxSize, ui32MaxSize);
    bw.write8 (&_ui8HighestPriority);
    uint16 ui16Count = 0;
    for (UInt32Hashtable<TagInfo>::Iterator i = _ui16Tags.getAllElements() ; !i.end() ; i.nextElement()) {
        uint16 ui16Tag = i.getKey();
        if ((bw.getBufferLength() + sizeof (uint16) + sizeof (uint8) + sizeof (uint8) + sizeof (uint8) + sizeof (uint8)) <= ui32MaxSize) {
            if (bw.write16 (&ui16Tag) < 0) {
                return -2;
            }
            TagInfo *pTagInfo = i.getValue();
            if (pTagInfo->_parameters.write (pWriter, ui32MaxSize) < 0) {
                return -3;
            }
            ui16Count++;
        }
    }
    pWriter->write16 (&ui16Count);
    pWriter->writeBytes (bw.getBuffer(), bw.getBufferLength());
    return 0;
}

int GroupTagSubscription::printInfo (void)
{
    checkAndLogMsg ("GroupTagSubscription::printInfo", Logger::L_Info, "                Group tag subscription\n");
    LList<uint16> * pTags = getTags();
    uint16 ui16Tag;
    pTags->getFirst (ui16Tag);
    for (int i = 0; i < pTags->length; i++) {
        checkAndLogMsg ("GroupTagSubscription::printInfo", Logger::L_Info, "                ui16Tag %d, prio %d, grel %d, mrel %d, seq %d\n", ui16Tag, getPriority (ui16Tag), isGroupReliable (ui16Tag), isMsgReliable (ui16Tag), isSequenced (ui16Tag));
        pTags->getNext (ui16Tag);
    }
    return 0;
}
