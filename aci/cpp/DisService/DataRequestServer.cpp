/*
 * DataRequestServer.cpp
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 5, 2015, 2:38 PM
 */

#include "DataRequestServer.h"

#include "DataCacheInterface.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DisServiceStats.h"
#include "Message.h"
#include "MessageId.h"
#include "NetworkTrafficMemory.h"
#include "TransmissionService.h"

#include "Logger.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    PtrLList<Message> * getMatchingFragments (const MessageId &msgId, uint32 ui32RequestedFragmentStart,
                                              uint32 ui32RequestedFragmentEnd, DataCacheInterface *pDataCacheInterface)
    {
        if (ui32RequestedFragmentStart < ui32RequestedFragmentEnd) {
            // Get all the fragments that match with a specific fragment
            return pDataCacheInterface->getMatchingFragments (msgId.getGroupName(), msgId.getOriginatorNodeId(),
                                                              msgId.getSeqId(), msgId.getChunkId(),
                                                              ui32RequestedFragmentStart, ui32RequestedFragmentEnd);
        }
        if (msgId.getChunkId() != MessageInfo::UNDEFINED_CHUNK_ID) {
            // Get all the fragments that match are part of the message/chunk
            return pDataCacheInterface->getMatchingFragments (msgId.getGroupName(), msgId.getOriginatorNodeId(),
                                                              msgId.getSeqId(), msgId.getChunkId());
        }

        // Get all the fragments that match are part of the message/large object
        return pDataCacheInterface->getMatchingFragments (msgId.getGroupName(), msgId.getOriginatorNodeId(),
                                                          msgId.getSeqId());
    }

    int shrinkToRequestedFragment (Message *pMsg, DisServiceMsg::Range *pRange, Message &fragmentToSend)
    {
        //const char *pszMethodName = "DataRequestServer::shrinkToRequestedFragment";
        if (pMsg == NULL || pRange == NULL) {
            return -1;
        }

        const uint32 ui32RequestedFragmentStart = pRange->getFrom();
        const uint32 ui32RequestedFragmentEnd = pRange->getTo();
        const bool bRequestTheWholeMessage = (ui32RequestedFragmentStart == 0) && (ui32RequestedFragmentEnd == 0);

        // Check whether the cached fragment that was retrieved matches the desired range
        MessageHeader *pCachedMsgHeader = pMsg->getMessageHeader();
        if (pCachedMsgHeader == NULL) {
            return -2;
        }
        const uint32 ui32CachedFragmentStart = pCachedMsgHeader->getFragmentOffset();
        const uint32 ui32CachedFragmentLength = pCachedMsgHeader->getFragmentLength();
        const uint32 ui32CachedFragmentEnd = ui32CachedFragmentStart + ui32CachedFragmentLength;
        if (!bRequestTheWholeMessage) {
            if (ui32CachedFragmentStart >= ui32RequestedFragmentEnd) {
                return -3;
            }
            if (ui32CachedFragmentEnd <= ui32RequestedFragmentStart) {
                return -4;
            }
        }
        const char *pDataToSend = static_cast<const char*>(pMsg->getData());
        // Shrink the cached frasgment from the left if necessary
        uint32 ui32Diff = 0U;
        uint32 ui32FragToSendLen = ui32CachedFragmentLength;
        if (!bRequestTheWholeMessage) {
            if (ui32CachedFragmentStart < ui32RequestedFragmentStart) {
                ui32Diff = ui32RequestedFragmentStart - ui32CachedFragmentStart;
                pDataToSend += ui32Diff;
                ui32FragToSendLen -= ui32Diff;
            }
            if ((ui32RequestedFragmentEnd > 0) &&  (ui32RequestedFragmentEnd < ui32CachedFragmentEnd)) {
                uint32 ui32 = ui32CachedFragmentEnd - ui32RequestedFragmentEnd;
                assert (ui32FragToSendLen > ui32);
                if (ui32 > ui32FragToSendLen) {
                    return -4;
                }
                ui32FragToSendLen -= ui32;
            }
        }

        MessageHeader *pMHToSend = pCachedMsgHeader->clone();
        pMHToSend->setFragmentOffset (ui32CachedFragmentStart + ui32Diff);
        pMHToSend->setFragmentLength (ui32FragToSendLen);
        fragmentToSend.setMessageHeader (pMHToSend);
        fragmentToSend.setData (pDataToSend);
        return 0;
    }
}

DataRequestServer::DataRequestServer (DisseminationService *pDisService, bool bTargetFilteringEnabled, bool bOppListeningEnabled)
    : _bOppListeningEnabled (bOppListeningEnabled),
      _bTargetFilteringEnabled (bTargetFilteringEnabled),
      _nodeId (pDisService->getNodeId()),
      _pDisService (pDisService),
      _randomlyIgnoredReqs (_bfParams)
{
}

DataRequestServer::~DataRequestServer (void)
{
}

int DataRequestServer::init (ConfigManager *pCfgMgr)
{
    if (_sevingReqProb.init (pCfgMgr) < 0) {
        return -1;
    }
    return 0;
}

void DataRequestServer::startedPublishingMessage (const char *pszMsgId)
{
    _m.lock ();
    _msgBeingSent.put (pszMsgId);
    _m.unlock();
}

void DataRequestServer::endedPublishingMessage (const char *pszMsgId)
{
    _m.lock();
    _msgBeingSent.remove (pszMsgId);
    _m.unlock();
}

int DataRequestServer::handleDataRequestMessage (DisServiceDataReqMsg *pDSDRMsg, const char *pszIncomingInterface, int64 i64Timeout)
{
    const int64 i64RequestArrivalTime = getTimeInMilliseconds();

    PtrLList<DisServiceDataReqMsg::FragmentRequest> *pRequests = pDSDRMsg->relinquishRequests();
    if (pRequests == NULL) {
        return 0;
    }

    char **ppszOutgoingInterfaces = NULL;
    bool bDeleteInterfaces = true;
    if (pszIncomingInterface != NULL) {
        ppszOutgoingInterfaces = (char **)calloc (2, sizeof (char *));
        if (ppszOutgoingInterfaces != NULL) {
            ppszOutgoingInterfaces[0] = strDup (pszIncomingInterface);
        }
    }
    else {
        // if pszIncomingInterface, use all the available interfaces
        ppszOutgoingInterfaces = _pDisService->_pTrSvc->getActiveInterfacesAddress();
        if (ppszOutgoingInterfaces == NULL) {
            bDeleteInterfaces = false;
        }
    }

    // NOTE: since DisServiceDataReqMsg does NOT deallocate pRequests, it is deleted
    // here (since I have to iterate throughout the whole list anyway).  Inner
    // objects, instances of DisServiceMsg::Range are deallocated too.
    DisServiceDataReqMsg::FragmentRequest *pRequest = pRequests->getFirst();
    DisServiceDataReqMsg::FragmentRequest *pRequestTmp;
    while (pRequest != NULL) {
        pRequestTmp = pRequests->getNext();

        handleDataRequest (pRequest->pMsgHeader->getMsgId(), pRequest->pRequestedRanges,
                           pDSDRMsg->getSenderNodeId(), pDSDRMsg->getNumberOfActiveNeighbors(),
                           i64RequestArrivalTime, (const char **) ppszOutgoingInterfaces, i64Timeout);

        // Delete current element's inner objects and the element itself.
        pRequests->remove (pRequest);

        delete pRequest->pRequestedRanges;
        pRequest->pRequestedRanges = NULL;
        delete pRequest->pMsgHeader;
        pRequest->pMsgHeader = NULL;
        delete pRequest;

        // Process the next element
        pRequest = pRequestTmp;
    }
    delete pRequests;
    pRequests = NULL;

    if (bDeleteInterfaces) {
        for (uint8 ui8 = 0; ppszOutgoingInterfaces[ui8] != NULL; ui8++) {
            free (ppszOutgoingInterfaces[ui8]);
            ppszOutgoingInterfaces[ui8] = NULL;
        }
        free (ppszOutgoingInterfaces);
        ppszOutgoingInterfaces = NULL;
    }

    return 0;
}

int DataRequestServer::handleDataRequest (const char *pszMsgId, PtrLList<DisServiceMsg::Range> *pRequestedRanges,
                                          const char *pszTarget, unsigned int uiNumberOfActiveNeighbors,
                                          int64 i64RequestArrivalTime, const char **ppszOutgoingInterfaces, int64 i64Timeout)
{
    const char *pszMethodName = " DataRequestServer::handleDataRequest";
    const MessageId msgId (pszMsgId);
    if (ignoreRequest (msgId, i64Timeout, uiNumberOfActiveNeighbors)) {
        return 0;
    }

    const int64 i64ServingUntil = i64RequestArrivalTime + _pDisService->getMissingFragmentTimeout();
    Message *pCompleteMessage = NULL;
    for (DisServiceMsg::Range *pRange = pRequestedRanges->getFirst(); pRange != NULL;) {
        if (getTimeInMilliseconds() >= i64ServingUntil) {
            break;
        }
        if (pCompleteMessage == NULL) {
            PtrLList<Message> *pPLL = getMatchingFragments (msgId, pRange->getFrom(), pRange->getTo(), _pDisService->getDataCacheInterface());
            if (pPLL == NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug, "There is no fragment %lu %lu for the message %s:%s:%lu\n",
                                pRange->getFrom(), pRange->getTo(), msgId.getGroupName(), msgId.getOriginatorNodeId(), msgId.getSeqId());
            }
            else {
                Message *pNext = pPLL->getFirst();
                for (Message *pCurr; (pCurr = pNext) != NULL;) {
                    pNext = pPLL->getNext();
                    filterAndSendMatchingFragment (msgId, pCurr, pRange, pszTarget, i64RequestArrivalTime, ppszOutgoingInterfaces);
                    pPLL->remove (pCurr);
                    if (pCurr->getMessageHeader()->isCompleteMessage()) {
                        pCompleteMessage = pCurr;  // the complete message matches all the next ranges, therefore
                                                   // it is not necessary to retrieve the matching fragments
                                                   // again, therefore making the code a bit more efficient (it
                                                   // when there are many requests!)
                        break;  // go to the next range
                    }
                    _pDisService->_pDataCacheInterface->release (pCurr);
                }
                delete pPLL;
            }
        }
        else {
            filterAndSendMatchingFragment (msgId, pCompleteMessage, pRange, pszTarget,
                                           i64RequestArrivalTime, ppszOutgoingInterfaces);
        }
        // Delete current element and process the next one
        DisServiceMsg::Range *pRangeTmp = pRequestedRanges->getNext();
        pRequestedRanges->remove (pRange);
        delete pRange;
        pRange = pRangeTmp;
    }

    // Clean the remaining items, if any
    for (DisServiceMsg::Range *pRange = pRequestedRanges->getFirst(); pRange != NULL;) {
        DisServiceMsg::Range *pRangeTmp = pRequestedRanges->getNext();
        pRequestedRanges->remove (pRange);
        delete pRange;
        pRange = pRangeTmp;
    }

    if (pCompleteMessage != NULL) {
        _pDisService->_pDataCacheInterface->release (pCompleteMessage);
    }
    return 0;
}

int DataRequestServer::handleDataRequestMessage (const char *pszMsgId, DisServiceMsg::Range *pRange,
                                                 bool bIsChunk, const char *pszTarget,
                                                 unsigned int uiNumberOfActiveNeighbors,
                                                 int64 i64RequestArrivalTime,
                                                 const char **ppszOutgoingInterfaces, int64 i64Timeout)
{
    const MessageId msgId (pszMsgId);
    if (ignoreRequest (msgId, i64Timeout, uiNumberOfActiveNeighbors)) {
        return 0;
    }

    // Get matching fragments
    PtrLList<Message> *pPLL = getMatchingFragments (msgId, pRange->getFrom(), pRange->getTo(), _pDisService->getDataCacheInterface());
    if (pPLL == NULL) {
        return 0;
    }

    // Send matching fragments
    Message *pNext = pPLL->getFirst();
    for (Message *pCurr; (pCurr = pNext) != NULL;) {
        pNext = pPLL->getNext();
        filterAndSendMatchingFragment (msgId, pCurr, pRange, pszTarget, i64RequestArrivalTime, ppszOutgoingInterfaces);
        pPLL->remove (pCurr);
        _pDisService->_pDataCacheInterface->release (pCurr);
    }
    delete pPLL;

    return 0;
}


int DataRequestServer::filterAndSendMatchingFragment (const MessageId &msgId, Message *pMsg, DisServiceMsg::Range *pRange, const char *pszTarget,
                                                      int64 i64RequestArrivalTime, const char **ppszOutgoingInterfaces)
{
    const char *pszMethodName = "DataRequestServer::handleDataRequestMessageInternal";

    const uint32 ui32RequestedFragmentStart = pRange->getFrom();
    const uint32 ui32RequestedFragmentEnd = pRange->getTo();
    const bool bRequestTheWholeMessage = (ui32RequestedFragmentStart == 0) && (ui32RequestedFragmentEnd == 0);

    Message fragmentToSend;
    if (shrinkToRequestedFragment (pMsg, pRange, fragmentToSend) < 0) {
        const uint32 ui32Off = pMsg->getMessageHeader()->getFragmentOffset();
        const uint32 ui32End = ui32Off + pMsg->getMessageHeader ()->getFragmentLength ();
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "No overlap between the wanted fragment (%u, %u) "
                        "and the cached fragment that was retrieved by the database query (%u, %u)\n",
                        ui32RequestedFragmentStart, ui32RequestedFragmentEnd, ui32Off, ui32End);
        return -1;
    }

    PtrLList<Message> *pFilteredMessages = _pDisService->_pNetTrafficMemory->filterRecentlySent (&fragmentToSend, i64RequestArrivalTime);
    if (pFilteredMessages == NULL) {
        return 0;
    }

    for (Message *pM = pFilteredMessages->getFirst(); pM != NULL;) {
        uint32 ui32CachedFragmentStart = pM->getMessageHeader()->getFragmentOffset();    // This is the starting index of the fragment that has been found in the cache
        uint32 ui32CachedFragmentLength = pM->getMessageHeader()->getFragmentLength();   // This is the length of the fragment in the cache
        uint32 ui32CachedFragmentEnd = ui32CachedFragmentStart + ui32CachedFragmentLength;

        uint32 ui32TmpRequestedFragmentEnd = bRequestTheWholeMessage ? pM->getMessageHeader()->getTotalMessageLength() :
                                                                       ui32RequestedFragmentEnd;

        if ((ui32CachedFragmentStart >= ui32TmpRequestedFragmentEnd) || (ui32CachedFragmentEnd <= ui32RequestedFragmentStart)) {
            // NO OVERLAP BETWEEN THE WANTED FRAGMENT AND THE CACHED FRAGMENT
            checkAndLogMsg (pszMethodName, Logger::L_Warning,
                            "No overlap between the wanted fragment (%u, %u) and the cached fragment that was retrieved by the database query (%u, %u)\n",
                            ui32RequestedFragmentStart, ui32TmpRequestedFragmentEnd,
                            ui32CachedFragmentStart, ui32CachedFragmentEnd);
        }
        else {
            uint32 ui32ToBeSentFragmentStart = maximum (ui32CachedFragmentStart, ui32RequestedFragmentStart);
            uint32 ui32ToBeSentFragmentEnd = minimum (ui32CachedFragmentEnd, ui32TmpRequestedFragmentEnd);
            uint32 ui32StartingOffsetInCachedFragment = ui32ToBeSentFragmentStart - ui32CachedFragmentStart;
            uint32 ui32ToBeSentFragmentLength = ui32ToBeSentFragmentEnd - ui32ToBeSentFragmentStart;
            //uint16 ui16MaxFragmentSize = getMaxFragmentSize();      // TODO: Check if this is needed
            MessageInfo *pNewMI = pM->getMessageInfo()->clone();
            uint32 ui32CurrentFragmentLength = ui32ToBeSentFragmentLength;
            pNewMI->setFragmentOffset (ui32ToBeSentFragmentStart);
            pNewMI->setFragmentLength (ui32CurrentFragmentLength);
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "Sending %lu-%lu starting at %lu in cached fragment\n",
                            ui32ToBeSentFragmentStart,
                            ui32CurrentFragmentLength,
                            ui32StartingOffsetInCachedFragment);
            Message newMsg (pNewMI, ((char*)pM->getData())+ui32StartingOffsetInCachedFragment);
            DisServiceDataMsg dsdm (_nodeId, &newMsg, _bTargetFilteringEnabled ? pszTarget : NULL);
            dsdm.setRepair (true);
            int rc;
            String logMsg = (String) "Handling Data Request";
            if (_pDisService->_pTrSvc->loggingEnabled()) {
                logMsg += " of ";
                logMsg += msgId.getGroupName();
                logMsg += ":";
                logMsg += msgId.getOriginatorNodeId();
                logMsg += ":";
                char *pszValue = (char *) calloc (13, sizeof(char));
                itoa (pszValue, msgId.getSeqId());
                logMsg += pszValue;
                free (pszValue);
                logMsg += " from peer ";
                logMsg += pszTarget;
                logMsg += ". Sending ";
                logMsg += dsdm.getMessageHeader()->getMsgId();
            }
            if (0 != (rc = _pDisService->broadcastDisServiceDataMsg (&dsdm, (const char *) logMsg, (const char **) ppszOutgoingInterfaces/*,
                                                       pRequest->pMsgHeader->getGroupName(),
                                                       pRequest->pMsgHeader->getSenderNodeId(),
                                                       pRequest->pMsgHeader->getTag(),
                                                       pRequest->pMsgHeader->getMsgSeqId(),
                                                       ui32RequestedFragmentStart,
                                                       ui32RequestedFragmentEnd*/
                                                       ))) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning,
                                "broadcastDisServiceMsg failed with rc = %d\n",
                                rc);
            }
            else if (_pDisService->_pNetTrafficMemory != NULL) {
                int rc2 = _pDisService->_pNetTrafficMemory->add (newMsg.getMessageHeader(), getTimeInMilliseconds());
                if (rc2 < 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_Warning,
                                    "message %s could not be added to network traffic memory. Error code: %d\n",
                                    dsdm.getMessageHeader()->getMsgId(), rc2);
                }
                else {
                    checkAndLogMsg (pszMethodName, Logger::L_Info,
                                    "message %s added to history\n",
                                    dsdm.getMessageHeader()->getMsgId());
                }
            }
            _pDisService->_pStats->onDemandFragmentPushed (pNewMI->getClientId(), pNewMI->getGroupName(),
                                                           pNewMI->getTag(), pNewMI->getFragmentLength());
            delete pNewMI;
        }
        Message *pMTmp = pFilteredMessages->getNext();
        pFilteredMessages->remove (pM);
        delete (pM->getMessageInfo());
        delete pM;
        pM = pMTmp;
    }
    delete pFilteredMessages;
    delete fragmentToSend.getMessageHeader();

    return 0;
}

bool DataRequestServer::ignoreRequest (const MessageId &msgId, int64 i64Timeout, unsigned int uiNumberOfActiveNeighbors)
{
    if ((i64Timeout > 0) && (getTimeInMilliseconds() > i64Timeout)) {
        // Request too old
        return true;
    }

    const bool bIsOriginator = (_nodeId == msgId.getOriginatorNodeId());
    if (bIsOriginator) {
        _m.lock();
        const bool bIsStillSendingIt = _msgBeingSent.containsKey (msgId.getId());
        _m.unlock();
        if (bIsStillSendingIt) {
            // if the node is still publishing
            return true;
        }
        if (!_bOppListeningEnabled) {
            return false;
        }
        if (_randomlyIgnoredReqs.contains (msgId.getId())) {
            return false;
        }
    }
    else if (!_bOppListeningEnabled) {
        return true;
    }

    // otherwise serve with certain probability
    bool bIgnoreReq = !_sevingReqProb.serveRequest (uiNumberOfActiveNeighbors);
    if (bIgnoreReq && bIsOriginator) {
        static unsigned int uiCounter = 0;
        if (uiCounter > 8000) {
            _randomlyIgnoredReqs.clear();
            uiCounter = 0;
        }
        _randomlyIgnoredReqs.insert (msgId.getId());
        uiCounter++;
    }

    return bIgnoreReq;
}

