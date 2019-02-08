/*
 * NetworkTrafficMemory.cpp
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

#include "NetworkTrafficMemory.h"

#include "DisServiceDefs.h"
#include "Message.h"
#include "MessageInfo.h"

#include "DArray.h"
#include "DArray2.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const uint16 NetworkTrafficMemory::DEFAULT_IGNORE_REQUEST_TIME = 0;

NetworkTrafficMemory::NetworkTrafficMemory (uint16 ui16IgnoreRequestTime)
    : _ui16IgnoreRequestTime (ui16IgnoreRequestTime),
      _messageByGroup (true, true, true, true),
      _m (23)
{
}

NetworkTrafficMemory::~NetworkTrafficMemory()
{
    _messageByGroup.removeAll();
}

int NetworkTrafficMemory::add (MessageHeader *pMI, int64 ui64ReceivingTime)
{
    if (_ui16IgnoreRequestTime == 0) {
        // _ui16IgnoreRequestTime == 0 means the filtering is not enabled, thus
        // there is no need to store
        return 0;
    }

    _m.lock (185);
    FragmentWrapperList *pFragList = getOrAddFragmentedMessage (pMI, true);
    if (pFragList == NULL) {
        checkAndLogMsg ("NetworkTrafficMemory::add", memoryExhausted);
        _m.unlock (185);
        return -1;
    }

    int rc = add (pFragList, pMI, ui64ReceivingTime);

    // pFragList may point to invalid memory after the cleanup!
    static int64 LAST_CLEAN_UP_TIME = 0U;
    if ((ui64ReceivingTime - LAST_CLEAN_UP_TIME) > _ui16IgnoreRequestTime) {
        cleanUp ();
        LAST_CLEAN_UP_TIME = ui64ReceivingTime;
    }
    _m.unlock (185);
    return rc;
}

int NetworkTrafficMemory::add (FragmentWrapperList *pFragList, MessageHeader *pMH, int64 ui64ReceivingTime)
{
    const char *pszMethodName = "NetworkTrafficMemory::add";
    if (pFragList == NULL) {
        return -1;
    }

    const uint32 ui32FragOffset = pMH->getFragmentOffset();
    const uint32 ui32FragEnd = ui32FragOffset + pMH->getFragmentLength();

    FragmentWrapper *pFW;
    for (FragmentWrapper *pTmpFW = pFragList->getFirst(); ((pFW = pTmpFW) != NULL) && (ui32FragEnd > pFW->_ui32Offset);) {
        pTmpFW = pFragList->getNext();
        if (expired (pFW, ui64ReceivingTime)) {
            delete pFragList->remove (pFW);
            continue;
        }
        if ((ui32FragOffset == pFW->_ui32Offset) && (pFW->_ui32End == ui32FragEnd)) {
            // It's exactly the same fragment, there is no need to create
            // and insert a new FragmentWrapper, just update the _i64LastServingTime,
            // of the current element to make the code a bit more efficient
            pFW->_i64LastServingTime = ui64ReceivingTime;
            return 0;
        }
        if (pFW->overlaps (ui32FragOffset, ui32FragEnd)) {
            if ((ui32FragOffset <= pFW->_ui32Offset) && (pFW->_ui32End <= ui32FragEnd)) {
                // The current range is included in the range to add:
                // just remove the current range and continue
                delete pFragList->remove (pFW);
            }
            else {
                if (pFW->_ui32Offset < ui32FragOffset) {
                    if (pFW->_ui32End > ui32FragEnd) {
                        // head and tail
                        FragmentWrapper *pTail = new FragmentWrapper (ui32FragEnd, pFW->_ui32End, pFW->_i64LastServingTime);
                        if (pTail == NULL) {
                            checkAndLogMsg (pszMethodName, memoryExhausted);
                            return -3;
                        }
                        pFragList->insert (pTail);
                        // also execute the "tail only" case
                    }
                    // head only
                    pFW->_ui32End = ui32FragOffset;
                }
                else if (pFW->_ui32End > ui32FragEnd) {
                    // tail only
                    pFW->_ui32Offset = ui32FragEnd;
                }
            }
        }
    }
    pFW = new FragmentWrapper (ui32FragOffset, ui32FragEnd, ui64ReceivingTime);
    if (pFW == NULL) {
        checkAndLogMsg ("NetworkTrafficMemory::add", memoryExhausted);
        return -5;
    }
    pFragList->insert (pFW);
    return 0;
}

PtrLList<Message> * NetworkTrafficMemory::filterRecentlySent (Message *pMsg, int64 i64RequestArrivalTime)
{
    const char *pszMethodName = "NetworkTrafficMemory::filterRecentlySent";

    MessageHeader *pMH = pMsg->getMessageHeader();
    if (pMH == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Message object does not contain message header.\n");
    }

    PtrLList<Message> *pRet = new PtrLList<Message>();
    if (pRet == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        return NULL;
    }

    if (_ui16IgnoreRequestTime == 0) {
        pRet->insert (new Message (pMH->clone(), pMsg->getData()));
        return pRet;
    }

    _m.lock (186);

    FragmentWrapperList *pFragList = getOrAddFragmentedMessage (pMH, false);
    if (pFragList == NULL) {
        pRet->insert (new Message (pMH->clone(), pMsg->getData()));
        _m.unlock (186);
        return pRet;
    }

    const char *psFragToSendData;
    psFragToSendData = static_cast<const char *>(pMsg->getData());
    uint32 ui32FragToSendStart = pMH->getFragmentOffset();
    uint32 ui32FragToSendEnd = ui32FragToSendStart + pMH->getFragmentLength();

    MessageHeader *pTmpMI;

    for (FragmentWrapper *pFW = pFragList->getFirst(); (pFW != NULL) && (ui32FragToSendEnd > pFW->_ui32Offset); pFW = pFragList->getNext()) {
        if (pFW->overlaps (ui32FragToSendStart, ui32FragToSendEnd)) {
            if (!expired (pFW, i64RequestArrivalTime)) {
                // I need to split
                // write everything before pFW->_ui32Offset
                uint32 ui32TmpFragLength = (ui32FragToSendStart < pFW->_ui32Offset ? pFW->_ui32Offset - ui32FragToSendStart : (uint32) 0);
                if (ui32TmpFragLength > 0) {
                    pTmpMI = pMH->clone();
                    pTmpMI->setFragmentOffset (ui32FragToSendStart);
                    pTmpMI->setFragmentLength (ui32TmpFragLength);
                    pRet->insert (new Message (pTmpMI, psFragToSendData));
                }
                // ui32FragToSendStart = pFW->_ui32End;
                // psFragToSendData = psFragToSendData + ui32TmpFragLength;
                ui32TmpFragLength = minimum (pFW->_ui32End, ui32FragToSendEnd) - ui32FragToSendStart;
                ui32FragToSendStart += ui32TmpFragLength;
                psFragToSendData += ui32TmpFragLength;
            }
        }
    }
    // Add the remaining part
    if (ui32FragToSendStart < ui32FragToSendEnd) {
        pTmpMI = pMH->clone();
        pTmpMI->setFragmentOffset (ui32FragToSendStart);
        pTmpMI->setFragmentLength (ui32FragToSendEnd - ui32FragToSendStart);
        pRet->insert (new Message (pTmpMI, psFragToSendData));
    }
    _m.unlock (186);
    return pRet;
}

void NetworkTrafficMemory::cleanUp (void)
{
    unsigned int uiGroupsToRemove = 0;
    DArray2<String> groupsToRemove;
    for (StringHashtable<MessagesByGroup>::Iterator i = _messageByGroup.getAllElements(); !i.end(); i.nextElement()) {
        MessagesByGroup *pMG = i.getValue();
        cleanEmptyMessageBySender (pMG);
        if (pMG->getCount() == 0) {
            // DArray2::operator[] instantiate a new string, and
            // string makes a copy of the assigned const char *
            groupsToRemove[uiGroupsToRemove] = i.getKey();
            uiGroupsToRemove++;
        }
    }
    for (unsigned int i = 0; i < uiGroupsToRemove; i++) {
        delete _messageByGroup.remove (groupsToRemove[i].c_str());
    }
}

void NetworkTrafficMemory::cleanEmptyMessageBySender (MessagesByGroup *pMG)
{
    unsigned int uiSendersToRemove = 0;
    DArray2<String> sendersToRemove;
    for (StringHashtable<MessagesBySender>::Iterator iGroup = pMG->getAllElements(); !iGroup.end(); iGroup.nextElement()) {
        MessagesBySender *pMS = iGroup.getValue();
        cleanEmptyFragmentedMessage (pMS);
        if (pMS->getCount() == 0) {
            // DArray2::operator[] instantiate a new string, and
            // string makes a copy of the assigned const char *
            sendersToRemove[uiSendersToRemove] = iGroup.getKey();
            uiSendersToRemove++;
        }
    }
    for (unsigned int i = 0; i < uiSendersToRemove; i++) {
        delete pMG->remove (sendersToRemove[i].c_str());
    }
}

void NetworkTrafficMemory::cleanEmptyFragmentedMessage (MessagesBySender *pMS)
{
    unsigned int uiFragMsgsToRemove = 0;
    DArray<uint64> fragMsgsToRemove;
    for (MessagesBySender::Iterator iFragMsg = pMS->getAllElements(); !iFragMsg.end(); iFragMsg.nextElement()) {
        FragmentedMessage *pFM = iFragMsg.getValue();
        cleanEmptyChunkList (pFM);
        if (pFM->getCount() == 0) {
            fragMsgsToRemove[uiFragMsgsToRemove] = iFragMsg.getKey();
            uiFragMsgsToRemove++;
        }
    }
    for (unsigned int i = 0; i < uiFragMsgsToRemove; i++) {
        delete pMS->remove (fragMsgsToRemove[i]);
    }
}

void NetworkTrafficMemory::cleanEmptyChunkList (FragmentedMessage *pFM)
{
    unsigned int uiChunksToRemove = 0;
    DArray<uint8> chunkToRemove;
    for (FragmentedMessage::Iterator iMsg = pFM->getAllElements(); !iMsg.end(); iMsg.nextElement()) {
        FragmentWrapperList *pFragList = iMsg.getValue();
        cleanExpiredFragments (pFragList);
        if (pFragList->getFirst() == NULL) {
            // the key it's a chunk id, casting to ui8 is safe
            chunkToRemove[uiChunksToRemove] = static_cast<uint8>(iMsg.getKey());
            uiChunksToRemove++;
        }
    }
    for (unsigned int i = 0; i < uiChunksToRemove; i++) {
        delete pFM->remove (chunkToRemove[i]);
    }
}

void NetworkTrafficMemory::cleanExpiredFragments (FragmentWrapperList *pFragList)
{
    FragmentWrapper *pFW;
    for (FragmentWrapper *pFWTmp = pFragList->getFirst(); (pFW = pFWTmp) != NULL;) {
        pFWTmp = pFragList->getNext();
        if (expired (pFW, getTimeInMilliseconds())) {
            // delete the entry
            delete pFragList->remove (pFW);
        }
    }
}

NetworkTrafficMemory::FragmentWrapperList * NetworkTrafficMemory::getOrAddFragmentedMessage (MessageHeader *pMH, bool bAdd)
{
    MessagesByGroup *pBG = _messageByGroup.get (pMH->getGroupName());
    if (pBG == NULL) {
        if (bAdd) {
            pBG = new MessagesByGroup (true, true, true, true);
        }
        if (pBG != NULL) {
            _messageByGroup.put (pMH->getGroupName(), pBG);
        }
        else {
            return NULL;
        }
    }

    MessagesBySender *pMS = pBG->get (pMH->getPublisherNodeId());
    if (pMS == NULL) {
        if (bAdd) {
            pMS = new MessagesBySender (US_INITSIZE, true);
        }
        if (pMS != NULL) {
            pBG->put (pMH->getPublisherNodeId(), pMS);
        }
        else {
            return NULL;
        }
    }

    FragmentedMessage *pFM = pMS->get (pMH->getMsgSeqId());
    if (pFM == NULL) {
        if (bAdd) {
            pFM = new FragmentedMessage (US_INITSIZE, true);
        }
        if (pFM != NULL) {
            pMS->put (pMH->getMsgSeqId(), pFM);
        }
        else {
            return NULL;
        }
    }

    FragmentWrapperList *pFC = pFM->get (pMH->getChunkId());
    if (pFC == NULL) {
        if (bAdd) {
            pFC = new FragmentWrapperList();
        }
        if (pFC != NULL) {
            pFM->put (pMH->getChunkId(), pFC);
        }
        else {
            return NULL;
        }
    }

    return pFC;
}

