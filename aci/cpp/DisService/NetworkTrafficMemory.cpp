/*
 * NetworkTrafficMemory.cpp
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

#include "NetworkTrafficMemory.h"

#include "DisServiceDefs.h"
#include "Message.h"
#include "MessageInfo.h"

#include "ConfigManager.h"
#include "DArray.h"
#include "DArray2.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const uint16 NetworkTrafficMemory::DEFAULT_IGNORE_REQUEST_TIME = 0;

NetworkTrafficMemory::NetworkTrafficMemory (uint16 ui16IgnoreRequestTime)
    : _messageByGroup (true, true, true, true),
      _m (23)
{
    _ui16IgnoreRequestTime = ui16IgnoreRequestTime;
}

NetworkTrafficMemory::NetworkTrafficMemory (ConfigManager *pConfigManager)
    : _m (23)
{
    _ui16IgnoreRequestTime = (uint16) pConfigManager->getValueAsInt ("aci.disService.transmission.ignoreReqTime",
                                                                     DEFAULT_IGNORE_REQUEST_TIME);
}

NetworkTrafficMemory::~NetworkTrafficMemory()
{
    _messageByGroup.removeAll();
}

int NetworkTrafficMemory::add (MessageHeader *pMI, int64 ui64ReceivingTime)
{
    _m.lock (185);
    int rc;
    if (_ui16IgnoreRequestTime > 0) {
        rc = add (pMI->getGroupName(), pMI->getPublisherNodeId(), pMI->getMsgSeqId(),
                  pMI->getFragmentOffset(), pMI->getFragmentLength(), ui64ReceivingTime);
    }
    else {
        // _ui16IgnoreRequestTime == 0 means the filtering is not enabled, thus
        // there is no need to store
        rc = 0;
    }
    _m.unlock (185);
    return rc;
}

int NetworkTrafficMemory::add (const char *pszGroupName, const char *pszPublisherNodeId,
                               uint32 ui32MsgSeqId, uint32 ui32FragOffset,
                               uint32 ui32FragLength, int64 ui64ReceivingTime)
{
    cleanUp();

    FragmentedMessage *pFM = getOrAddFragmentedMessage (pszGroupName, pszPublisherNodeId, ui32MsgSeqId);
    if (pFM == NULL) {
        checkAndLogMsg ("NetworkTrafficMemory::add (1)", memoryExhausted);
        return -1;
    }

    return add (pFM, ui32FragOffset, ui32FragLength, ui64ReceivingTime);
}

int NetworkTrafficMemory::add (const char *pszGroupName, const char *pszPublisherNodeId,
                               uint32 ui32MsgSeqId, uint8 ui8ChunkId,
                               uint32 ui32FragOffset, uint32 ui32FragLength,
                               int64 ui64ReceivingTime)
{
    cleanUp();

    FragmentedMessage *pFM = getOrAddFragmentedMessage (pszGroupName, pszPublisherNodeId, ui32MsgSeqId);
    if (pFM == NULL) {
        checkAndLogMsg ("NetworkTrafficMemory::add (2)", memoryExhausted);
        return -1;
    }

    FragmentedChunk *pFC = pFM->chunksByChunkId.get (ui8ChunkId);
    if (pFC == NULL) {
        pFC = new FragmentedChunk();
        if (pFC != NULL) {
            pFM->chunksByChunkId.put (ui32MsgSeqId, pFC);
        }
        else {
            checkAndLogMsg ("NetworkTrafficMemory::add (2)", memoryExhausted);
            return -2;
        }
    }

    return add (pFC, ui32FragOffset, ui32FragLength, ui64ReceivingTime);
}

int NetworkTrafficMemory::add (FragmentedMessageHeader *pFM, uint32 ui32FragOffset, uint32 ui32FragLength, int64 ui64ReceivingTime)
{
    uint32 ui32FragEnd = ui32FragOffset + ui32FragLength;
    FragmentWrapper *pTail, *pHead, *pTmpFW;

    for (FragmentWrapper *pFW = pFM->fragments.getFirst(); (pFW != NULL) && (ui32FragEnd > pFW->ui32Offset); pFW = pTmpFW) {
        pTmpFW = pFM->fragments.getNext();  // I need to get the next element
                                            // _before_ deleting the current
                                            // element to avoid skipping it
        if ((ui32FragOffset == pFW->ui32Offset) && (pFW->ui32End == ui32FragEnd)) {
            // It's exactly the same fragment, there is no need to create
            // and insert a new FragmentWrapper, just update the i64LastServingTime,
            // of the current element to make the code a bit more efficient
            pFW->i64LastServingTime = ui64ReceivingTime;
            return 0;
        }
        if (pFW->overlaps (ui32FragOffset, ui32FragEnd)) {
            if ((ui32FragOffset <= pFW->ui32Offset) && (pFW->ui32End <= ui32FragEnd)) {
                // The current range is included in the range to add:
                // just remove the current range and continue
                delete pFM->fragments.remove (pFW);
            }
            else {
                pTail = pHead = NULL;
                if (pFW->ui32Offset < ui32FragOffset) {
                    pHead = new FragmentWrapper (pFW->ui32Offset, ui32FragOffset, pFW->i64LastServingTime);
                    if (pHead == NULL) {
                        checkAndLogMsg ("NetworkTrafficMemory::add", memoryExhausted);
                        return -3;
                    }
                }
                if (pFW->ui32End > ui32FragEnd) {
                    pTail = new FragmentWrapper (ui32FragEnd, pFW->ui32End, pFW->i64LastServingTime);
                    if (pTail == NULL) {
                        if (pHead != NULL) {
                            delete pHead;
                            pHead = NULL;
                        }
                        checkAndLogMsg ("NetworkTrafficMemory::add", memoryExhausted);
                        return -4;
                    }
                }
                if (pHead != NULL || pTail != NULL) {
                    // remove the current range
                    delete pFM->fragments.remove (pFW);
                    if (pHead != NULL) {
                        pFM->fragments.insert (pHead);
                    }
                    if (pTail != NULL) {
                        pFM->fragments.insert (pTail);
                    }
                }
            }
        }
    }
    pTmpFW = new FragmentWrapper (ui32FragOffset, ui32FragEnd, ui64ReceivingTime);
    if (pTmpFW == NULL) {
        checkAndLogMsg ("NetworkTrafficMemory::add", memoryExhausted);
        return -5;
    }
    pFM->fragments.insert (pTmpFW);
    return 0;
}

PtrLList<Message> * NetworkTrafficMemory::filterRecentlySent (Message *pMsg, int64 i64RequestArrivalTime)
{
    _m.lock (186);
    MessageHeader *pMH = pMsg->getMessageHeader();
    if (pMH == NULL) {
        checkAndLogMsg ("NetworkTrafficMemory::filterRecentlySent", Logger::L_Warning,
                        "Message object does not contain message header.\n");
    }

    PtrLList<Message> *pRet = new PtrLList<Message>();
    if (pRet == NULL) {
        checkAndLogMsg ("NetworkTrafficMemory::filterRecentlySent", memoryExhausted);
        _m.unlock (186);
        return NULL;
    }

    if (_ui16IgnoreRequestTime == 0) {
        pRet->insert (new Message (pMH->clone(), pMsg->getData()));
         _m.unlock (186);
        return pRet;
    }

    MessagesByGroup *pBG = _messageByGroup.get (pMH->getGroupName());
    if (pBG == NULL) {
        pRet->insert (new Message (pMH->clone(), pMsg->getData()));
         _m.unlock (186);
        return pRet;
    }

    MessagesBySender *pMS = pBG->messageBySender.get (pMH->getPublisherNodeId());
    if (pMS == NULL) {
        pRet->insert (new Message (pMH->clone(), pMsg->getData()));
         _m.unlock (186);
        return pRet;
    }

    FragmentedMessageHeader *pFMH = pMS->messageBySeqId.get (pMH->getMsgSeqId());
    if (pFMH == NULL) {
        pRet->insert (new Message (pMH->clone(), pMsg->getData()));
         _m.unlock (186);
        return pRet;
    }

    if (pMH->isChunk()) {
        FragmentedChunk *pFC = ((FragmentedMessage *) pFMH)->chunksByChunkId.get (pMsg->getChunkMsgInfo()->getChunkId());
        if (pFC == NULL) {
            pRet->insert (new Message (pMH->clone(), pMsg->getData()));
            _m.unlock (186);
            return pRet;
        }
        else {
            pFMH = pFC;
        }
    }

    char *psFragToSendData;
    psFragToSendData = (char *) pMsg->getData();
    uint32 ui32FragToSendStart = pMH->getFragmentOffset();
    uint32 ui32FragToSendEnd = ui32FragToSendStart + pMH->getFragmentLength();

    MessageHeader *pTmpMI;

    for (FragmentWrapper *pFW = pFMH->fragments.getFirst(); (pFW != NULL) && (ui32FragToSendEnd > pFW->ui32Offset); pFW = pFMH->fragments.getNext()) {
        if (pFW->overlaps (ui32FragToSendStart, ui32FragToSendEnd)) {
            if ((i64RequestArrivalTime - pFW->i64LastServingTime) < _ui16IgnoreRequestTime) {
                // I need to split
                // write everything before pFW->ui32Offset
                uint32 ui32TmpFragLength = (ui32FragToSendStart < pFW->ui32Offset ? pFW->ui32Offset - ui32FragToSendStart : (uint32) 0);
                if (ui32TmpFragLength > 0) {
                    pTmpMI = pMH->clone();
                    pTmpMI->setFragmentOffset (ui32FragToSendStart);
                    pTmpMI->setFragmentLength (ui32TmpFragLength);
                    pRet->insert (new Message (pTmpMI, psFragToSendData));
                }
                // ui32FragToSendStart = pFW->ui32End;
                // psFragToSendData = psFragToSendData + ui32TmpFragLength;
                ui32TmpFragLength = minimum (pFW->ui32End, ui32FragToSendEnd) - ui32FragToSendStart;
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

void NetworkTrafficMemory::cleanUp()
{
    MessagesByGroup *pMG;

    DArray2<String> groupsToRemove;
    uint8 ui8GroupsToRemove = 0;

    for (StringHashtable<MessagesByGroup>::Iterator i = _messageByGroup.getAllElements(); !i.end(); i.nextElement()) {
        pMG = i.getValue();

        cleanEmptyMessageBySender (pMG);

        if (pMG->messageBySender.getCount() == 0) {
            groupsToRemove[ui8GroupsToRemove] = i.getKey(); // DArray2::operator[] instantiate
                                                            // a new string, and string makes a
                                                            // copy of the assigned const char *
            ui8GroupsToRemove++;
        }
    }

    for (uint8 i = 0; i < ui8GroupsToRemove; i++) {
        delete _messageByGroup.remove ((const char *)groupsToRemove[i]);
    }
}

void NetworkTrafficMemory::cleanEmptyMessageBySender (MessagesByGroup *pMG)
{
    MessagesBySender *pMS;
    DArray2<String> sendersToRemove;
    uint8 ui8SendersToRemove = 0;

    for (StringHashtable<MessagesBySender>::Iterator iGroup = pMG->messageBySender.getAllElements(); !iGroup.end(); iGroup.nextElement()) {
        pMS = iGroup.getValue();

        cleanEmptyFragmentedMessage (pMS);

        if (pMS->messageBySeqId.getCount() == 0) {
            sendersToRemove[ui8SendersToRemove] = iGroup.getKey(); // DArray2::operator[] instantiate
                                                                   // a new string, and string makes a
                                                                   // copy of the assigned const char *
            ui8SendersToRemove++;
        }
    }

    for (uint8 i = 0; i < ui8SendersToRemove; i++) {
        delete pMG->messageBySender.remove ((const char *)sendersToRemove[i]);
    }
}

void NetworkTrafficMemory::cleanEmptyFragmentedMessage (MessagesBySender *pMS)
{
    FragmentedMessage *pFM; FragmentedChunk *pCFM;
    DArray<int64> chunkToRemove;
    uint8 ui8ChunksToRemove;

    DArray<int64> messagesToRemove;
    uint8 ui8MessagesToRemove = 0;

    for (UInt32Hashtable<FragmentedMessage>::Iterator iSender = pMS->messageBySeqId.getAllElements(); !iSender.end(); iSender.nextElement()) {
        pFM = iSender.getValue();

        // Remove entries for message fragments which "ignore request
        // time" has expired
        cleanFragmentedMessageHeader (pFM);

        ui8ChunksToRemove = 0;
        for (UInt32Hashtable<FragmentedChunk>::Iterator iFragChunks = pFM->chunksByChunkId.getAllElements(); !iFragChunks.end(); iFragChunks.nextElement()) {
            pCFM = iFragChunks.getValue();
            // Remove entries for chunk fragments which "ignore request
            // time" has expired
            cleanFragmentedMessageHeader (pCFM);
            if (pCFM->fragments.getFirst() == NULL) {
                chunkToRemove[ui8ChunksToRemove] = iFragChunks.getKey();
                ui8ChunksToRemove++;
            }
        }
        for (uint8 i = 0; i < ui8ChunksToRemove && chunkToRemove[i] >= 0; i++) {
            delete pFM->chunksByChunkId.remove ((uint32)chunkToRemove[i]);
            chunkToRemove[i] = -1; // Reset value
        }

        if ((pFM->fragments.getFirst() == NULL) &&
            (pFM->chunksByChunkId.getCount() == 0)) {
            messagesToRemove[ui8MessagesToRemove] = iSender.getKey();
            ui8MessagesToRemove++;
        }
    }

    for (uint8 i = 0; i < ui8MessagesToRemove; i++) {
        delete pMS->messageBySeqId.remove ((uint32)messagesToRemove[i]);
    }
}

void NetworkTrafficMemory::cleanFragmentedMessageHeader (FragmentedMessageHeader *pFMH)
{
    FragmentWrapper *pFW;
    FragmentWrapper *pFWTmp = pFMH->fragments.getFirst();
    while ((pFW = pFWTmp) != NULL) {
        pFWTmp = pFMH->fragments.getNext();
        if ((getTimeInMilliseconds() - pFW->i64LastServingTime) > _ui16IgnoreRequestTime) {
            // delete the entry
            delete pFMH->fragments.remove (pFW);
        }
    }
}

NetworkTrafficMemory::FragmentedMessage * NetworkTrafficMemory::getOrAddFragmentedMessage (const char *pszGroupName,
                                                                                           const char *pszSenderNodeId,
                                                                                           uint32 ui32MsgSeqId)
{
    MessagesByGroup *pBG = _messageByGroup.get (pszGroupName);
    if (pBG == NULL) {
        pBG = new MessagesByGroup();
        if (pBG != NULL) {
            _messageByGroup.put (pszGroupName, pBG);
        }
        else {
            return NULL;
        }
    }

    MessagesBySender *pMS = pBG->messageBySender.get (pszSenderNodeId);
    if (pMS == NULL) {
        pMS = new MessagesBySender();
        if (pMS != NULL) {
            pBG->messageBySender.put (pszSenderNodeId, pMS);
        }
        else {
            return NULL;
        }
    }

    FragmentedMessage *pFM = pMS->messageBySeqId.get (ui32MsgSeqId);
    if (pFM == NULL) {
        pFM = new FragmentedMessage();
        if (pFM != NULL) {
            pMS->messageBySeqId.put (ui32MsgSeqId, pFM);
        }
        else {
            return NULL;
        }
    }

    return pFM;
}

