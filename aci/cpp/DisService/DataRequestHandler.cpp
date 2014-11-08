/*
 * DataRequestHandler.cpp
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

#include "DataRequestHandler.h"

#include "DisServiceMsg.h"
#include "DisServiceDefs.h"
#include "DisServiceMsgHelper.h"
#include "DisseminationService.h"
#include "TransmissionService.h"

#include "Logger.h"
#include "NLFLib.h"
#include "NetUtils.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const int64 DataRequestHandler::DEFAULT_SLEEP_TIME = 200;
const int64 DataRequestHandler::DEFAULT_BASE_TIME = 500;
const uint16 DataRequestHandler::DEFAULT_OFFSET_RANGE = 50;
const float DataRequestHandler::DEFAULT_RECEIVE_RATE_THRESHOLD = 0.80f;

//------------------------------------------------------------------------------
// DataRequestHandler
//------------------------------------------------------------------------------
DataRequestHandler::DataRequestHandler (DisseminationService *pDisService)
    : _mx (7)
{
    if (pDisService == NULL) {
        checkAndLogMsg ("DataRequestHandler::DataRequestHandler", Logger::L_SevereError,
                        "DataRequestHandler could not be initialized because pDisService is NULL.\n");
        _pDisService = NULL;
    }
    else {
        _pDisService = pDisService;
        _i64DiscardTime = _pDisService->getMissingFragmentTimeout() * 3;
    }

    // initialize random seed
    srand((unsigned int) getTimeInMilliseconds());
}

DataRequestHandler::~DataRequestHandler()
{
    _pDisService = NULL;
}

DataRequestHandler * DataRequestHandler::getInstance (DisseminationService *pDisService,
                                                      TransmissionService *pTrSvc,
                                                      int64 i64SleepTime, int64 i64BaseTimeMs,
                                                      uint16 u16OffsetRange, float fReceiveRateThreshold,
                                                      bool bHandleRequests)
{
    if (bHandleRequests) {
        AsynchronousDataRequestHandler *pDRH = new AsynchronousDataRequestHandler (pDisService, pTrSvc,
                                                                                   i64SleepTime, i64BaseTimeMs,
                                                                                   u16OffsetRange, fReceiveRateThreshold);
        if (pDRH == NULL) {
            checkAndLogMsg ("DataRequestHandler::getInstance", Logger::L_SevereError,
                            "AsynchronousDataRequestHandler could not be instantiated. Exiting.\n");
            exit(1);
        }
        pDRH->start();
        return pDRH;
    }
    else {
        return new SynchronousDataRequestHandler (pDisService);
    }
}

void DataRequestHandler::handleDataRequestMessage (DisServiceDataReqMsg *pDSDRMsg, const char *pszIncomingInterface, int64 i64Timeout)
{
    _pDisService->handleDataRequestMessage (pDSDRMsg, pszIncomingInterface, i64Timeout);
}

void DataRequestHandler::handleDataRequestMessage (const char *pszMsgId, DisServiceMsg::Range *pRange,
                                                   bool bIsChunk, const char *pszTarget,
                                                   unsigned int uiNumberOfActiveNeighbors,
                                                   int64 i64RequestArrivalTime,
                                                   const char **ppszOutgoingInterfaces, int64 i64Timeout)
{
    _pDisService->handleDataRequestMessage (pszMsgId, pRange, bIsChunk, pszTarget,
                                            uiNumberOfActiveNeighbors, i64RequestArrivalTime,
                                            ppszOutgoingInterfaces, i64Timeout);
}

//------------------------------------------------------------------------------
// AsynchronousDataRequestHandler
//------------------------------------------------------------------------------
AsynchronousDataRequestHandler::AsynchronousDataRequestHandler (DisseminationService *pDisService, TransmissionService *pTrSvc,
                                                                int64 i64SleepTime, int64 i64BaseTimeMs, uint16 u16OffsetRange,
                                                                float fReceiveRateThreshold)
    : DataRequestHandler (pDisService)
{
    if (pTrSvc == NULL) {
        checkAndLogMsg ("AsynchronousDataRequestHandler::AsynchronousDataRequestHandler", Logger::L_SevereError,
                        "DataRequestHandler could not be initialized because pTrSvc is NULL.\n");
        _pTrSvc = NULL;
    }
    else {
        _pTrSvc = pTrSvc;
    }

    _i64SleepTime = i64SleepTime;
    _i64BaseTimeMs = i64BaseTimeMs;
    _u16OffsetRange = u16OffsetRange;

    _fReceiveRateThreshold = fReceiveRateThreshold;

    // initialize random seed
    srand((unsigned int) getTimeInMilliseconds());

    checkAndLogMsg ("AsynchronousDataRequestHandler::AsynchronousDataRequestHandler",
                    Logger::L_Info, "AsynchronousDataRequestHandler started\n");
}

AsynchronousDataRequestHandler::~AsynchronousDataRequestHandler()
{
    if (isRunning()) {
        requestTerminationAndWait();
    }

    _pTrSvc = NULL;
}

void AsynchronousDataRequestHandler::dataRequestMessageArrived (DisServiceDataReqMsg *pDSDRMsg, const char *pszIncomingInterface)
{
    /**
     * When a data request message arrives, we can use this method to
     * store the request in the _receivedDataReqMessages hashtable until
     * we're ready to serve it
     */

    _mx.lock (42);

    int64 i64TimeToServe = calculateServingTime(); // the current request will not be
                                                   // served before this instant. It's
                                                   // an absolute value, not relative.

    // store the request
    DataReqMsgWrapper *pNewMsg = new DataReqMsgWrapper (pDSDRMsg, pszIncomingInterface, i64TimeToServe);
    _llreceivedDataReqMessages.add (pNewMsg);
    checkAndLogMsg ("AsynchronousDataRequestHandler::newIncomingMessage", Logger::L_LowDetailDebug,
                    "Data Request Msg from ip %s stored by DataRequestHandler with time to serve as %I64d\n",
                    pszIncomingInterface, i64TimeToServe);

    _mx.unlock (42);
}

int64 AsynchronousDataRequestHandler::calculateServingTime()
{
    int64 i64TimeOffset;
    int64 i64ServingTime;

    i64TimeOffset = _i64BaseTimeMs * _u16OffsetRange / 100;
    i64TimeOffset = (rand() % i64TimeOffset);

    if (((rand() % 100) + 1.0f) < 50.0f) {
        i64ServingTime = _i64BaseTimeMs - i64TimeOffset;
    }
    else {
        i64ServingTime = _i64BaseTimeMs + i64TimeOffset;
    }
    checkAndLogMsg ("AsynchronousDataRequestHandler::calculateServingTime", Logger::L_LowDetailDebug,
                    "calculated serving time to be %I64d ms from now\n", i64ServingTime);
    i64ServingTime += getTimeInMilliseconds();
    return i64ServingTime;
}

void AsynchronousDataRequestHandler::run()
{
    const char *pszMethodName = "AsynchronousDataRequestHandler::run";
    setName (pszMethodName);

    started();
    DataReqMsgWrapper *pMsgToBeServed;
    char **ppszInterfaces;

    int64 i64SleepRandomRange = _i64BaseTimeMs / 5;
    int64 i64SleepTime, i64TimeOffset;

    //calculate a random amount of msecs to use as sleep time
    i64TimeOffset = (rand() % i64SleepRandomRange);
    if (((rand() % 100) + 1.0f) < 50.0f) {
        i64SleepTime = _i64BaseTimeMs - i64TimeOffset;
    }
    else {
        i64SleepTime = _i64BaseTimeMs + i64TimeOffset;
    }

    while (!terminationRequested()) {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "sleeping for %I64d ms\n", i64SleepTime);
        sleepForMilliseconds (i64SleepTime);
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "queued data requests: %u\n", _llreceivedDataReqMessages.getCount());
        if (_pTrSvc != NULL) {
            if (!_pTrSvc->getAsyncTransmission()) {
                ppszInterfaces = _pTrSvc->getInterfacesByReceiveRate (_fReceiveRateThreshold);
            }
            else {
                ppszInterfaces = _pTrSvc->getInterfacesByOutgoingQueueLength (_fReceiveRateThreshold);
            }
            if (ppszInterfaces != NULL) {
                pMsgToBeServed = getRequestToServe (ppszInterfaces);
                //serve all the messages whose waiting time is over
                while (pMsgToBeServed != NULL) {
                    checkAndLogMsg (pszMethodName, Logger::L_Info, "Serving a request.\n");
                    handleDataRequestMessage (pMsgToBeServed->_pDataReqMsg,
                                              pMsgToBeServed->_pszIncomingInterface,
                                              pMsgToBeServed->_i64ServingTime + _i64DiscardTime);
                    DisServiceMsgHelper::deallocatedDisServiceDataReqMsg (pMsgToBeServed->_pDataReqMsg);
                    delete pMsgToBeServed;
                    pMsgToBeServed = getRequestToServe (ppszInterfaces);
                    //sleepForMilliseconds (i64SleepTime/10);
                }

                if (ppszInterfaces) {
                    for (int i = 0; ppszInterfaces[i] != NULL; i++) {
                        free (ppszInterfaces[i]);
                    }
                    free (ppszInterfaces);
                }
            }
            else {
                // if no interface is available, wait some time before checking again
                // HACK. the value should be dependant on the interface's receive
                // rate estimation window
                sleepForMilliseconds (500 + (rand() % (_pDisService->getMissingFragmentTimeout() / 2)));
            }
        }

        //calculate a random amount of msecs to use as sleep time
        i64TimeOffset = (rand() % i64SleepRandomRange);
        if (((rand() % 100) + 1.0f) < 50.0f) {
            i64SleepTime = _i64BaseTimeMs - i64TimeOffset;
        }
        else {
            i64SleepTime = _i64BaseTimeMs + i64TimeOffset;
        }
    }

    terminating();
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Terminated\n");
}

AsynchronousDataRequestHandler::DataReqMsgWrapper * AsynchronousDataRequestHandler::getRequestToServe (char **ppszInterfaces)
{
    _mx.lock (43);
    DataReqMsgWrapper *pMg, *pMgNext;
    int64 i64CurrTime;

    // just gets the first element it can find that has waited enough and can
    // now be processed. could be randomized or prioritized better.
    i64CurrTime = getTimeInMilliseconds();

    int status = _llreceivedDataReqMessages.getFirst (pMg);
    while (status != 0 && pMg != NULL) {
        status = _llreceivedDataReqMessages.getNext (pMgNext);
        //if the message is too old (the discard time has passed), delete it
        if (pMg->_i64ServingTime + _i64DiscardTime < i64CurrTime) {
            checkAndLogMsg ("AsynchronousDataRequestHandler::getRequestToServe", Logger::L_Info,
                            "dropping DataRequestMsg received from interface with address %s "
                            "because it is too old\n", pMg->_pszIncomingInterface);
            _llreceivedDataReqMessages.remove (pMg);
            DisServiceMsgHelper::deallocatedDisServiceDataReqMsg (pMg->_pDataReqMsg);
            delete pMg;
            pMg = NULL;
        }
        // if the message is ready to be served and not too old, you might
        // serve it
        // TODO: this code does not work when two nodes are not in the same
        //       subnetwork (it's possible  that there are routers in between...)
        //       FIX THIS!
        else if (pMg->_i64ServingTime < i64CurrTime) {
            // check if it originate don one of the specified interfaces
            for (int i = 0; ppszInterfaces[i] != NULL && pMg->_pszIncomingInterface != NULL; i++) {
                if (strcmp (ppszInterfaces[i], pMg->_pszIncomingInterface) == 0) {
                    checkAndLogMsg ("AsynchronousDataRequestHandler::getRequestToServe",
                                    Logger::L_LowDetailDebug, "Fetched request from %s\n",
                                    pMg->_pszIncomingInterface);
                   _llreceivedDataReqMessages.remove (pMg);
                   _mx.unlock (43);
                   return pMg;
                }
            }
       }
       pMg = pMgNext;
    }
    _mx.unlock (43);
    return NULL;
}

//------------------------------------------------------------------------------
// AsynchronousDataRequestHandlerV2
//------------------------------------------------------------------------------

AsynchronousDataRequestHandlerV2::AsynchronousDataRequestHandlerV2 (DisseminationService *pDisService, TransmissionService *pTrSvc,
                                                                    int64 i64SleepTime, int64 i64BaseTimeMs, uint16 u16OffsetRange,
                                                                    float fReceiveRateThreshold)
    : AsynchronousDataRequestHandler (pDisService, pTrSvc, i64SleepTime, i64BaseTimeMs,
                                      u16OffsetRange, fReceiveRateThreshold)
{
    
}

AsynchronousDataRequestHandlerV2::~AsynchronousDataRequestHandlerV2 (void)
{
}

void AsynchronousDataRequestHandlerV2::dataRequestMessageArrived (DisServiceDataReqMsg *pDSDRMsg,
                                                                  const char *pszIncomingInterface)
{
    if (pDSDRMsg == NULL) {
        return;
    }
    PtrLList<DisServiceDataReqMsg::FragmentRequest> *pRequests = pDSDRMsg->getRequests();
    DisServiceDataReqMsg::FragmentRequest *pReq = pRequests->getFirst();
    for (; pReq != NULL; pReq = pRequests->getNext()) {
        DisServiceMsg::Range *pRange = pReq->pRequestedRanges->getFirst();
        for (; pRange != NULL; pRange = pReq->pRequestedRanges->getNext()) {
            bool bIsChunk = (pReq->pMsgHeader->getChunkId() != MessageHeader::UNDEFINED_CHUNK_ID) || pReq->pMsgHeader->isChunk();
            QueuedRequest *pNewQueuedReq = new QueuedRequest (pReq->pMsgHeader->getMsgId(), *pRange, bIsChunk);
            if (pNewQueuedReq != NULL) {
                pNewQueuedReq->_ui16NumberOfActiveNeighbors = pDSDRMsg->getNumberOfActiveNeighbors();
                pNewQueuedReq->_incomingInterfaces.put (pszIncomingInterface);
                pNewQueuedReq->_requestingPeers.put (pDSDRMsg->getSenderNodeId());
                requestArrived (pNewQueuedReq, pDSDRMsg->getSenderNodeId(), pszIncomingInterface);
            }
        }
    }
}

void AsynchronousDataRequestHandlerV2::display (FILE *pOutputFile)
{
    if (pOutputFile == NULL) {
        return;
    }

    QueuedRequest *pQueuedReq = _queuedRequests.getFirst();
    for (; pQueuedReq != NULL; pQueuedReq = _queuedRequests.getNext()) {
        char *pszRequestors = DisServiceMsgHelper::getMultiNodeTarget (pQueuedReq->_requestingPeers);
        char *pszInterfaces = DisServiceMsgHelper::getMultiNodeTarget (pQueuedReq->_incomingInterfaces);
        fprintf (pOutputFile, "%s, %u %u (%s, %s) %u\n", pQueuedReq->_messageId.getId(),
                 pQueuedReq->_range.getFrom(), pQueuedReq->_range.getTo(), pszRequestors,
                 pszInterfaces, pQueuedReq->_ui16RequestCount);
    }
}

void AsynchronousDataRequestHandlerV2::run (void)
{
    const char *pszMethodName = "AsynchronousDataRequestHandler::run";
    setName (pszMethodName);

    started();

    int64 i64SleepRandomRange = _i64BaseTimeMs / 5;
    int64 i64SleepTime, i64TimeOffset;

    //calculate a random amount of msecs to use as sleep time
    i64TimeOffset = (rand() % i64SleepRandomRange);
    if (((rand() % 100) + 1.0f) < 50.0f) {
        i64SleepTime = _i64BaseTimeMs - i64TimeOffset;
    }
    else {
        i64SleepTime = _i64BaseTimeMs + i64TimeOffset;
    }

    while (!terminationRequested()) {

        sleepForMilliseconds (i64SleepTime);
        serveRequest();

        i64TimeOffset = (rand() % i64SleepRandomRange);
        if (((rand() % 100) + 1.0f) < 50.0f) {
            i64SleepTime = _i64BaseTimeMs - i64TimeOffset;
        }
        else {
            i64SleepTime = _i64BaseTimeMs + i64TimeOffset;
        }
    }

    terminating();
}

void AsynchronousDataRequestHandlerV2::serveRequest (void)
{
    QueuedRequest *pQueuedReq;
    QueuedRequest *pNextQueuedReq = _queuedRequests.getFirst();
    while ((pQueuedReq = pNextQueuedReq) != NULL) {
        pNextQueuedReq = _queuedRequests.getNext();
        char **ppszOutgoingInterfaces = NULL;
        unsigned short usIfaces = pQueuedReq->_incomingInterfaces.getCount();
        if (usIfaces > 0) {
            ppszOutgoingInterfaces = (char **) calloc (usIfaces + 1, sizeof (char*));
            if (ppszOutgoingInterfaces != NULL) {
                StringHashset::Iterator iter = pQueuedReq->_incomingInterfaces.getAllElements();
                for (unsigned short i = 0; !iter.end() && i < usIfaces; iter.nextElement()) {
                    ppszOutgoingInterfaces[i] = strDup (iter.getKey());
                    if (ppszOutgoingInterfaces[i] != NULL) {
                        i++;
                    }
                }
            }
        }
        handleDataRequestMessage (pQueuedReq->_messageId.getId(), &(pQueuedReq->_range),
                                  pQueuedReq->_bIsChunk,
                                  DisServiceMsgHelper::getMultiNodeTarget (pQueuedReq->_requestingPeers),
                                  pQueuedReq->_ui16NumberOfActiveNeighbors, pQueuedReq->_i64ArrivalTimestamp,
                                  (const char **)ppszOutgoingInterfaces, 0);
        if (ppszOutgoingInterfaces != NULL) {
            deallocateNullTerminatedPtrArray (ppszOutgoingInterfaces);
        }
    }
}

void AsynchronousDataRequestHandlerV2::requestArrived (AsynchronousDataRequestHandlerV2::QueuedRequest *pNewQueuedReq,
                                                       const char *pszSender, const char *pszIncomingInterface)
{
    QueuedRequest *pQueuedReq = _queuedRequests.getFirst();
    for (; pQueuedReq != NULL; pQueuedReq = _queuedRequests.getNext()) {
        if (pQueuedReq->_range.getFrom() == 0 && pQueuedReq->_range.getTo() == 0) {
            // It was a request for the whole message, replace it
            delete _queuedRequests.remove (pQueuedReq);
            _queuedRequests.insert (pNewQueuedReq);
            return;
        }
        else if (!(pNewQueuedReq->_range.getFrom() >= pQueuedReq->_range.getTo() ||
                   pNewQueuedReq->_range.getTo() <= pQueuedReq->_range.getFrom())) {
            // There's overlap, remove and re-insert the 
            _queuedRequests.remove (pQueuedReq);
            pQueuedReq->_ui16RequestCount++;
            pQueuedReq->_incomingInterfaces.put (pszIncomingInterface);
            pQueuedReq->_ui16NumberOfActiveNeighbors = minimum (pQueuedReq->_ui16NumberOfActiveNeighbors, pNewQueuedReq->_ui16NumberOfActiveNeighbors);
            pQueuedReq->_requestingPeers.put (pszSender);
            _queuedRequests.insert (pQueuedReq);
            if (pNewQueuedReq->_range.getFrom() < pQueuedReq->_range.getFrom()) {
                DisServiceMsg::Range range (pNewQueuedReq->_range.getFrom(), pQueuedReq->_range.getFrom());
                QueuedRequest *pLeftNewQueuedReq = new QueuedRequest (pNewQueuedReq->_messageId.getId(), range, pNewQueuedReq->_bIsChunk);
                pLeftNewQueuedReq->_incomingInterfaces.put (pszIncomingInterface);
                pLeftNewQueuedReq->_incomingInterfaces.put (pszIncomingInterface);
                pLeftNewQueuedReq->_requestingPeers.put (pszSender);
                pLeftNewQueuedReq->_ui16NumberOfActiveNeighbors = pNewQueuedReq->_ui16NumberOfActiveNeighbors;
                requestArrived (pLeftNewQueuedReq, pszSender, pszIncomingInterface);
            }
            if (pNewQueuedReq->_range.getTo() > pQueuedReq->_range.getTo()) {
                DisServiceMsg::Range range (pQueuedReq->_range.getTo(), pNewQueuedReq->_range.getTo());
                QueuedRequest *pRightNewQueuedReq = new QueuedRequest (pNewQueuedReq->_messageId.getId(), range, pNewQueuedReq->_bIsChunk);
                pRightNewQueuedReq->_incomingInterfaces.put (pszIncomingInterface);
                pRightNewQueuedReq->_incomingInterfaces.put (pszIncomingInterface);
                pRightNewQueuedReq->_requestingPeers.put (pszSender);
                pRightNewQueuedReq->_ui16NumberOfActiveNeighbors = pNewQueuedReq->_ui16NumberOfActiveNeighbors;
                delete pNewQueuedReq;
                requestArrived (pRightNewQueuedReq, pszSender, pszIncomingInterface);
            }
            else {
                delete pNewQueuedReq;
            }
            return;
        }
    }

    _queuedRequests.insert (pNewQueuedReq);
    return;
}

AsynchronousDataRequestHandlerV2::QueuedRequest::QueuedRequest (const char *pszMsgId, DisServiceMsg::Range &range, bool bIsChunk)
    : _bIsChunk (bIsChunk), _messageId (pszMsgId), _range (range.getFrom(), range.getTo())
{
    _ui16RequestCount = 0;
    _ui16NumberOfActiveNeighbors = 0;
    _i64ArrivalTimestamp = getTimeInMilliseconds();
}

AsynchronousDataRequestHandlerV2::QueuedRequest::~QueuedRequest (void)
{
}

bool AsynchronousDataRequestHandlerV2::QueuedRequest::operator > (AsynchronousDataRequestHandlerV2::QueuedRequest &rhsQueuedReq)
{
    if (_i64ArrivalTimestamp > rhsQueuedReq._i64ArrivalTimestamp) {
        return true;
    }
    if (_ui16RequestCount > rhsQueuedReq._ui16RequestCount) {
        return true;
    }
    return false;
}

bool AsynchronousDataRequestHandlerV2::QueuedRequest::operator < (AsynchronousDataRequestHandlerV2::QueuedRequest &rhsQueuedReq)
{
    if (_i64ArrivalTimestamp < rhsQueuedReq._i64ArrivalTimestamp) {
        return true;
    }
    if (_ui16RequestCount < rhsQueuedReq._ui16RequestCount) {
        return true;
    }
    return false;
}

bool AsynchronousDataRequestHandlerV2::QueuedRequest::operator == (AsynchronousDataRequestHandlerV2::QueuedRequest &rhsQueuedReq)
{
    return (_messageId == rhsQueuedReq._messageId &&
            _range.getFrom() == rhsQueuedReq._range.getFrom() &&
            _range.getTo() == rhsQueuedReq._range.getTo());
}

//------------------------------------------------------------------------------
// AsynchronousDataRequestHandler::DataReqMsgWrapper
//------------------------------------------------------------------------------

AsynchronousDataRequestHandler::DataReqMsgWrapper::DataReqMsgWrapper (DisServiceDataReqMsg *pMessage,
                                                                      const char *pszIncomingInterface,
                                                                      int64 i64MessageServingTime)
{
    _pDataReqMsg = pMessage;
    _i64ServingTime = i64MessageServingTime;
    _pszIncomingInterface = strDup (pszIncomingInterface);
}

//------------------------------------------------------------------------------
// SynchronousDataRequestHandler
//------------------------------------------------------------------------------

SynchronousDataRequestHandler::SynchronousDataRequestHandler (DisseminationService *pDisService)
    : DataRequestHandler (pDisService)
{
    checkAndLogMsg ("SynchronousDataRequestHandler::SynchronousDataRequestHandler",
                    Logger::L_Info, "SynchronousDataRequestHandler started\n");
}

SynchronousDataRequestHandler::~SynchronousDataRequestHandler()
{
}

void SynchronousDataRequestHandler::dataRequestMessageArrived (DisServiceDataReqMsg *pDSDRMsg,
                                                               const char *pszIncomingInterface)
{
    _mx.lock (44);
    int64 i64DiscardTime = _i64DiscardTime + getTimeInMilliseconds();
    handleDataRequestMessage (pDSDRMsg, pszIncomingInterface, i64DiscardTime);
    _mx.unlock (44);
}

