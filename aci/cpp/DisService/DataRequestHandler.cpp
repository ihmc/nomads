/*
 * DataRequestHandler.cpp
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

#include "DataRequestHandler.h"

#include "DisServiceMsg.h"
#include "DisServiceDefs.h"
#include "DisServiceMsgHelper.h"
#include "DisseminationService.h"
#include "TransmissionService.h"

#include "Logger.h"
#include "NLFLib.h"

#ifdef WIN32
    #include <stdint.h>
#endif

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const int64 DataRequestHandler::DEFAULT_SLEEP_TIME = 200;
const int64 DataRequestHandler::DEFAULT_BASE_TIME = 500;
const uint16 DataRequestHandler::DEFAULT_OFFSET_RANGE = 50;
const float DataRequestHandler::DEFAULT_RECEIVE_RATE_THRESHOLD = 0.80f;

//------------------------------------------------------------------------------
// DataRequestHandler
//------------------------------------------------------------------------------
DataRequestHandler::DataRequestHandler (Type type, DisseminationService *pDisService)
    : _type (type), _mx (7)
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
                                                      int64 iMissingFragReqTimeout, uint16 u16OffsetRange,
                                                      float fReceiveRateThreshold, bool bHandleRequests)
{
    const int type = 4;
    AsynchronousDataRequestHandler *pDRH;
    if (bHandleRequests) {
        switch (type) {
            case 0:
                pDRH = new AsynchronousDataRequestHandler (pDisService, pTrSvc,
                                                           i64SleepTime, i64BaseTimeMs,
                                                           u16OffsetRange, fReceiveRateThreshold);
                break;
            case 2:
                pDRH = new AsynchronousDataRequestHandlerV2 (pDisService, pTrSvc,
                                                             i64SleepTime, i64BaseTimeMs,
                                                             u16OffsetRange, fReceiveRateThreshold);
                break;
            case 4:
            default:
                pDRH = new AsynchronousDataRequestHandlerV4 (pDisService, pTrSvc,
                                                             i64SleepTime, iMissingFragReqTimeout,
                                                             u16OffsetRange, fReceiveRateThreshold);
        }

        if (pDRH == NULL) {
            checkAndLogMsg ("DataRequestHandler::getInstance", Logger::L_SevereError,
                            "AsynchronousDataRequestHandler could not be instantiated.\n");
            return NULL;
        }
        pDRH->start();
        return pDRH;
    }
    return new SynchronousDataRequestHandler (pDisService);
}

int64 DataRequestHandler::getRandomSleepTime (int64 i64SleepTime)
{
    const int64 i64SleepRandomRange = i64SleepTime / 5;
    // Calculate a random amount of msecs to use as sleep time
    int64 i64TimeOffset = (rand() % i64SleepRandomRange);
    if (((rand() % 100) + 1.0f) < 50.0f) {
        i64TimeOffset = (0 - i64TimeOffset);
    }
    return (i64SleepTime + i64TimeOffset);
}

void DataRequestHandler::halt (DataRequestHandler *pDRH)
{
    if ((pDRH != NULL) && (pDRH->_type == ASYNC)) {
        AsynchronousDataRequestHandler *pAsynch = static_cast<AsynchronousDataRequestHandler *>(pDRH);
        pAsynch->requestTerminationAndWait();
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
    if (_pDisService == NULL) {
        return;
    }
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
    : DataRequestHandler (ASYNC, pDisService)
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
        sleepForMilliseconds (i64SleepTime);
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "queued data requests: %u\n", _llreceivedDataReqMessages.getCount());
        if ((_pTrSvc != NULL) && (_pTrSvc->isInitialized())) {
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
                checkAndLogMsg (pszMethodName, Logger::L_Info, "no available interfaces\n");
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
    if ((pDSDRMsg == NULL) || (pszIncomingInterface == NULL)) {
        return;
    }

    PtrLList<DisServiceDataReqMsg::FragmentRequest> *pRequests = pDSDRMsg->getRequests();
    if (pRequests == NULL) {
        return;
    }

    RequestByInterface *pReqByIface = new RequestByInterface (pszIncomingInterface);
    if (pReqByIface == NULL) {
        return;
    }
    RequestByInterface *pOldReqByIface = _requestsByIncomingInterface.insertUnique (pReqByIface);
    if (pOldReqByIface != NULL) {
        // The new element was not added because already present. It can be deleted
        delete pReqByIface;
        pReqByIface = pOldReqByIface;
    }

    for (DisServiceDataReqMsg::FragmentRequest *pReq = pRequests->getFirst(); pReq != NULL; pReq = pRequests->getNext()) {
        if (pReq->pRequestedRanges != NULL) {
            const bool bIsChunk = (pReq->pMsgHeader->getChunkId() != MessageHeader::UNDEFINED_CHUNK_ID) || pReq->pMsgHeader->isChunk();
            // Add the first requested range
            DisServiceMsg::Range *pRange = pReq->pRequestedRanges->getFirst();
            uint32 ui32RequestedBytes = 0;
            if (pRange != NULL) {
                int64 i64QueuedReqCreationTime = getTimeInMilliseconds();
                QueuedRequest *pQueuedReq =  new QueuedRequest (pReq->pMsgHeader->getMsgId(), *pRange,
                                                                bIsChunk, pDSDRMsg->getNumberOfActiveNeighbors(),
                                                                pDSDRMsg->getSenderNodeId());
                ui32RequestedBytes += (pRange->getTo() - pRange->getFrom());
                // Add the rest of the requested ranges (if any)
                while ((pRange = pReq->pRequestedRanges->getNext()) != NULL) {
                    if (pQueuedReq->_requestedFragments.addTSN (pRange->getFrom(), pRange->getTo()) == 1) {
                        ui32RequestedBytes += (pRange->getTo() - pRange->getFrom());
                    }
                }
                int64 i64Now = getTimeInMilliseconds();
                i64QueuedReqCreationTime = i64Now - i64QueuedReqCreationTime;
                bool bMerged = pReqByIface->_reqByMsgId.insertMerge (pQueuedReq);
                if (bMerged) {
                    // The values in the new QueuedRequest were merged, it can
                    // therefore be deallocated
                    delete pQueuedReq;
                }

                checkAndLogMsg (bMerged ? "MERGED" : "ADDED", Logger::L_Info, "it took %lld millisecs to add/merge, %lld millisecs to create the queued req for %u bytes\n", (getTimeInMilliseconds() - i64Now), i64QueuedReqCreationTime, ui32RequestedBytes);
            }
        }
    }
}

void AsynchronousDataRequestHandlerV2::display (FILE *pOutputFile)
{
    if (pOutputFile == NULL) {
        return;
    }

    RequestByInterface *pReqByIface = _requestsByIncomingInterface.getFirst();
    for (; pReqByIface != NULL; pReqByIface = _requestsByIncomingInterface.getNext()) {
        const String iface (pReqByIface->_incomingInterface);
        PtrLList<QueuedRequest> *pReqByMsgId = pReqByIface->_reqByMsgId.getRequests();
        for (QueuedRequest *pReq = pReqByMsgId->getFirst(); pReq != NULL; pReq = pReqByMsgId->getNext()) {
            uint32 ui32BeginEl, ui32EndEl;
            for (int rc = pReq->_requestedFragments.getFirst (ui32BeginEl, ui32EndEl, true);
                    rc == 0; rc = pReq->_requestedFragments.getNext (ui32BeginEl, ui32EndEl)) {

                String requestors (DisServiceMsgHelper::getMultiNodeTarget (pReq->_requestingPeers));
                fprintf (pOutputFile, "%s, %u %u (%s, %s) %lld %lld\n", pReq->_messageId.getId(),
                 ui32BeginEl, ui32EndEl, requestors.c_str(), iface.c_str(),
                 pReq->_i64ArrivalOfFirstRequestTimestamp, pReq->_i64ArrivalOfLatestRequestTimestamp);
            }
        }
    }
}

void AsynchronousDataRequestHandlerV2::run (void)
{
    const char *pszMethodName = "AsynchronousDataRequestHandlerV2::run";
    setName (pszMethodName);

    started();

    while (!terminationRequested()) {

        int64 i64SleepTime = DataRequestHandler::getRandomSleepTime (_i64BaseTimeMs);
        sleepForMilliseconds (i64SleepTime);

        serveRequest();

    }

    terminating();
}

bool AsynchronousDataRequestHandlerV2::serveRequest (void)
{
    const char *pszMethodName = "AsynchronousDataRequestHandlerV2::serveRequest";

    // Retrieve the available interfaces
    char **ppszInterfaces = NULL;
    if (!_pTrSvc->getAsyncTransmission()) {
        ppszInterfaces = _pTrSvc->getInterfacesByReceiveRate (_fReceiveRateThreshold);
    }
    else {
        ppszInterfaces = _pTrSvc->getInterfacesByOutgoingQueueLength (_fReceiveRateThreshold);
    }
    if (ppszInterfaces == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "no available interface for "
                        "receive rate threshold set to %f.\n", _fReceiveRateThreshold);
        return false;
    }
    // No need to clone keys! ppszInterfaces contains elements that can and must
    // be deallocated, therefore StringHashset is set to deallocate its elements
    StringHashset availableIfaces (true, false, true);
    for (unsigned int i = 0; ppszInterfaces[i] != NULL; i++) {
        availableIfaces.put (ppszInterfaces[i]);
    }
    free (ppszInterfaces);

    // Serve requests
    for (RequestByInterface *pByIface = _requestsByIncomingInterface.getFirst(); pByIface != NULL; pByIface = _requestsByIncomingInterface.getNext()) {
        if (availableIfaces.containsKey (pByIface->_incomingInterface)) {
            const uint32 ui32AgeThreshold = (_i64BaseTimeMs / 3);
            for (QueuedRequest *pReq; (pReq = pByIface->_reqByMsgId.pop (ui32AgeThreshold)) != NULL;) {

                // Discard it if it's too "old"
                const int64 i64Diff = getTimeInMilliseconds() - pReq->_i64ArrivalOfLatestRequestTimestamp;
                if (i64Diff > _i64DiscardTime) {
                    delete pReq;
                    continue;
                }

                char szOutgoingInterfaces[40];  // IPv6 ready
                strcpy (szOutgoingInterfaces, pByIface->_incomingInterface);
                char *pszOutgoingInterfaces[2];
                pszOutgoingInterfaces[0] = szOutgoingInterfaces;
                pszOutgoingInterfaces[1] = NULL;

                const bool RESET_GET = true;
                uint32 ui32BeginEl, ui32EndEl;
                uint64 ui64ServedBytes = 0;
                for (int rc = pReq->_requestedFragments.getFirst (ui32BeginEl, ui32EndEl, RESET_GET);
                        rc == 0; rc = pReq->_requestedFragments.getNext (ui32BeginEl, ui32EndEl)) {
                    DisServiceMsg::Range range (ui32BeginEl, ui32EndEl);
                    String target (DisServiceMsgHelper::getMultiNodeTarget (pReq->_requestingPeers));
                    checkAndLogMsg ("SERVING", Logger::L_Info, "%lld %u %u\n", getTimeInMilliseconds(), range.getFrom(), range.getTo());
                    handleDataRequestMessage (pReq->_messageId.getId(), &range, pReq->_bIsChunk, target,
                                              pReq->_avgMinimumNumberOfActiveNeighbors.getMin(),
                                              pReq->_i64ArrivalOfLatestRequestTimestamp,
                                             (const char **)&pszOutgoingInterfaces, (int64) 0);
                    ui64ServedBytes += (ui32EndEl - ui32BeginEl);
                }
                checkAndLogMsg ("SERVED", Logger::L_Info, "%llu bytes\n", ui64ServedBytes);

                delete pReq;
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------------
// AsynchronousDataRequestHandlerV2::EnqueuedRequests
//------------------------------------------------------------------------------

AsynchronousDataRequestHandlerV2::RequestByInterfaceList::RequestByInterfaceList (void)
{
}

AsynchronousDataRequestHandlerV2::RequestByInterfaceList::~RequestByInterfaceList (void)
{
}

AsynchronousDataRequestHandlerV2::RequestByInterface * AsynchronousDataRequestHandlerV2::RequestByInterfaceList::getFirst (void)
{
    _m.lock();
    RequestByInterface *pRet = _requestsByIncomingInterface.getFirst();
    _m.unlock();
    return pRet;
}
/*
AsynchronousDataRequestHandlerV2::RequestByInterfaceList::~RequestByInterfaceList (void)
{
}

AsynchronousDataRequestHandlerV2::RequestByInterface * AsynchronousDataRequestHandlerV2::RequestByInterfaceList::getFirst (void)
{
    _m.lock();
    RequestByInterface *pRet = _requestsByIncomingInterface.getFirst();
    _m.unlock();
    return pRet;
}*/

AsynchronousDataRequestHandlerV2::RequestByInterface * AsynchronousDataRequestHandlerV2::RequestByInterfaceList::getNext (void)
{
    _m.lock();
    RequestByInterface *pRet = _requestsByIncomingInterface.getNext();
    _m.unlock();
    return pRet;
}

AsynchronousDataRequestHandlerV2::RequestByInterface * AsynchronousDataRequestHandlerV2::RequestByInterfaceList::insertUnique (RequestByInterface *pByIface)
{
    if (pByIface == NULL) {
        return NULL;
    }
    _m.lock();
    RequestByInterface *pOld = _requestsByIncomingInterface.search (pByIface);
    if (pOld == NULL) {
        _requestsByIncomingInterface.prepend (pByIface);
    }
    _m.unlock();
    return pOld;
}

//------------------------------------------------------------------------------
// AsynchronousDataRequestHandlerV2::QueuedRequest
//------------------------------------------------------------------------------

AsynchronousDataRequestHandlerV2::QueuedRequest::QueuedRequest (const char *pszMsgId, DisServiceMsg::Range &range, bool bIsChunk,
                                                                uint16 ui16MinimumNumberOfActiveNeighbors, const char *pszRequestingNodeId)
    : _bIsChunk (bIsChunk),
      _i64ArrivalOfFirstRequestTimestamp (getTimeInMilliseconds()),
      _i64ArrivalOfLatestRequestTimestamp (_i64ArrivalOfFirstRequestTimestamp),
      _messageId (pszMsgId),
      _requestedFragments (false),
      _avgMinimumNumberOfActiveNeighbors (10000)
{
    _requestedFragments.addTSN (range.getFrom(), range.getTo());
    _requestingPeers.put (pszRequestingNodeId);
    _avgMinimumNumberOfActiveNeighbors.add (_i64ArrivalOfFirstRequestTimestamp, ui16MinimumNumberOfActiveNeighbors);
}

AsynchronousDataRequestHandlerV2::QueuedRequest::~QueuedRequest (void)
{
}

bool AsynchronousDataRequestHandlerV2::QueuedRequest::operator > (const AsynchronousDataRequestHandlerV2::QueuedRequest &rhsQueuedReq) const
{
    if (_i64ArrivalOfFirstRequestTimestamp > rhsQueuedReq._i64ArrivalOfFirstRequestTimestamp) {
        return true;
    }
    if (_requestingPeers.getCount() > rhsQueuedReq._requestingPeers.getCount()) {
        return true;
    }
    return false;
}

bool AsynchronousDataRequestHandlerV2::QueuedRequest::operator < (const AsynchronousDataRequestHandlerV2::QueuedRequest &rhsQueuedReq) const
{
    if (_i64ArrivalOfFirstRequestTimestamp < rhsQueuedReq._i64ArrivalOfFirstRequestTimestamp) {
        return true;
    }
    if (_requestingPeers.getCount() < rhsQueuedReq._requestingPeers.getCount()) {
        return true;
    }
    return false;
}

int AsynchronousDataRequestHandlerV2::QueuedRequest::operator == (const AsynchronousDataRequestHandlerV2::QueuedRequest &rhsQueuedReq) const
{
    return (_messageId == rhsQueuedReq._messageId) ? 1 : 0;
}

AsynchronousDataRequestHandlerV2::QueuedRequest &  AsynchronousDataRequestHandlerV2::QueuedRequest::operator += (QueuedRequest &rhsQueuedReq)
{
    _avgMinimumNumberOfActiveNeighbors.add (rhsQueuedReq._i64ArrivalOfLatestRequestTimestamp, (uint16) rhsQueuedReq._avgMinimumNumberOfActiveNeighbors.getAverage());
    _i64ArrivalOfLatestRequestTimestamp = rhsQueuedReq._i64ArrivalOfLatestRequestTimestamp;
    const bool RESET_GET = true;
    uint32 ui32BeginEl, ui32EndEl;
    for (int rc = rhsQueuedReq._requestedFragments.getFirst (ui32BeginEl, ui32EndEl, RESET_GET);
            rc == 0; rc = rhsQueuedReq._requestedFragments.getNext (ui32BeginEl, ui32EndEl)) {
        _requestedFragments.addTSN (ui32BeginEl, ui32EndEl);
    }

    StringHashset::Iterator iter = rhsQueuedReq._requestingPeers.getAllElements();
    for ( ; !iter.end(); iter.nextElement()) {
        if (!_requestingPeers.containsKey (iter.getKey())) {
            _requestingPeers.put (iter.getKey());
        }
    }

    return *this;
}

//------------------------------------------------------------------------------

AsynchronousDataRequestHandlerV4::AsynchronousDataRequestHandlerV4 (DisseminationService *pDisService, TransmissionService *pTrSvc,
                                                                    int64 i64SleepTime, int64 i64BaseTimeMs, uint16 u16OffsetRange,
                                                                    float fReceiveRateThreshold)
    : AsynchronousDataRequestHandler (pDisService, pTrSvc, i64SleepTime, i64BaseTimeMs, u16OffsetRange, fReceiveRateThreshold),
      _nodeId (pDisService->getNodeId()),
      _pIncompleteMsgsByIface (new StringHashtable<RangesByMsgId> (true, true, true, true))
{
}

AsynchronousDataRequestHandlerV4::~AsynchronousDataRequestHandlerV4 (void)
{
}

void AsynchronousDataRequestHandlerV4::dataRequestMessageArrived (DisServiceDataReqMsg *pDSDRMsg, const char *pszIncomingInterface)
{
    if ((pDSDRMsg == NULL) || (pszIncomingInterface == NULL)) {
        return;
    }
    PtrLList<DisServiceDataReqMsg::FragmentRequest> *pRequests = pDSDRMsg->getRequests();
    if (pRequests == NULL) {
        return;
    }

    _m.lock();
    if (_pIncompleteMsgsByIface == NULL) {
        _m.unlock();
        return;
    }
    const int64 i64Now = getTimeInMilliseconds();
    RangesByMsgId *pRangeByMsgId = _pIncompleteMsgsByIface->get (pszIncomingInterface);
    if (pRangeByMsgId == NULL) {
        pRangeByMsgId = new RangesByMsgId();
        if (pRangeByMsgId == NULL) {
            _m.unlock();
            return;
        }
        _pIncompleteMsgsByIface->put (pszIncomingInterface, pRangeByMsgId);
    }
    for (DisServiceDataReqMsg::FragmentRequest *pReq = pRequests->getFirst(); pReq != NULL; pReq = pRequests->getNext()) {
        MessageHeader *pMH = pReq->pMsgHeader;
        const String msgId (pMH->getMsgId());
        Ranges *pRanges = pRangeByMsgId->get (msgId);
        if (pRanges == NULL) {
            const bool bOriginatedLocally = ((_nodeId == pMH->getPublisherNodeId()) == 1);
            pRanges = new Ranges (bOriginatedLocally);
            if (pRanges == NULL) {
                continue;
            }
            pRangeByMsgId->put (msgId, pRanges);
        }
        else if ((pRanges->_requestingPeers.containsKey (pDSDRMsg->getSenderNodeId())) &&
                 ((i64Now - pRanges->_i64LatestRequest) > _pDisService->getMissingFragmentTimeout())) {
            pRanges->reset();
        }
        pRanges->_i64LatestRequest = i64Now;
        if (!pRanges->_requestingPeers.containsKey (pDSDRMsg->getSenderNodeId())) {
            pRanges->_requestingPeers.put (pDSDRMsg->getSenderNodeId());
        }
        const uint16 ui16NumberOfActiveNeighbors = maximum (pDSDRMsg->getNumberOfActiveNeighbors(), static_cast<uint16>(1));
        pRanges->_avgMinimumNumberOfActiveNeighbors.add (getTimeInMilliseconds(), ui16NumberOfActiveNeighbors);
        for (DisServiceMsg::Range *pRange = pReq->pRequestedRanges->getFirst(); pRange != NULL; pRange = pReq->pRequestedRanges->getNext()) {
            pRanges->_ranges.addTSN (pRange->getFrom(), pRange->getTo());
        }
    }
    _m.unlock();
    DisServiceMsgHelper::deallocatedDisServiceDataReqMsg (pDSDRMsg);
}

void AsynchronousDataRequestHandlerV4::display (FILE *pOutputFile)
{
}

namespace IHMC_ACI
{
    struct RequestServingStat
    {
        RequestServingStat (void) : uiNMsgs (0U), uiNRanges (0U), ui64TotalBytes (0U), i64ReplyingTime (0) {};
        ~RequestServingStat (void) {};
        void log (const char *pszMethodName)
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "served %u message requests for %u fragments "
                            "(total %u bytes) in %lld milliseconds.", uiNMsgs, uiNRanges, ui64TotalBytes,
                            i64ReplyingTime);
        }

        unsigned int uiNMsgs;
        unsigned int uiNRanges;
        uint64 ui64TotalBytes;
        int64 i64ReplyingTime;
    };
}

void AsynchronousDataRequestHandlerV4::run (void)
{
    const char *pszMethodName = "AsynchronousDataRequestHandlerV4::run";
    setName (pszMethodName);
    started();

    StringHashtable<RangesByMsgId> *pIncompleteMsgsByIface = NULL;
    for (int64 i64SleepTime = 0; !terminationRequested();) {
        //calculate a random amount of msecs to use as sleep time
        const int64 i64FragReqTimeout = _pDisService->getMissingFragmentTimeout();
        const int64 i64SleepRandomRange = i64FragReqTimeout / 10;

        int64 i64DeletionStart = getTimeInMilliseconds();
        if (pIncompleteMsgsByIface != NULL) {
            delete pIncompleteMsgsByIface;
        }
        int64 i64DeletionTime = getTimeInMilliseconds() - i64DeletionStart;
        i64SleepTime = (i64DeletionTime > i64SleepTime ? 0 : i64SleepTime - i64DeletionTime);
        sleepForMilliseconds (i64SleepTime);

        _m.lock();
        const int64 i64ArrivalTime = getTimeInMilliseconds();
        pIncompleteMsgsByIface = _pIncompleteMsgsByIface;
        _pIncompleteMsgsByIface = new StringHashtable<RangesByMsgId> (true, true, true, true);
        _m.unlock();

        if ((_pTrSvc == NULL) || (!_pTrSvc->isInitialized())) {
            i64SleepTime = 200;
            continue;
        }
        if ((pIncompleteMsgsByIface == NULL) || (pIncompleteMsgsByIface->getCount() == 0)) {
            i64SleepTime = 200;
            continue;
        }

        static const unsigned int IN_QUEUE_THRESHOLD = 100;
        // If the incoming queue is big, wait a little bit...
        for (const int64 iStart = getTimeInMilliseconds(); (_pTrSvc->getIncomingQueueSize() >= IN_QUEUE_THRESHOLD)
            && ((getTimeInMilliseconds() - iStart) < 2000);) {
            sleepForMilliseconds (100);
        }

        // If the traffic is too high, do not serve the requests at this time
        char **ppszInterfaces = _pTrSvc->getAsyncTransmission() ?
            _pTrSvc->getInterfacesByOutgoingQueueLength (_fReceiveRateThreshold) :
            _pTrSvc->getInterfacesByReceiveRate (_fReceiveRateThreshold);
        if ((ppszInterfaces == NULL) || (_pTrSvc->getIncomingQueueSize() >= IN_QUEUE_THRESHOLD)) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "no available interfaces\n");
            // if no interface is available, wait some time before checking again
            // HACK. the value should be dependant on the interface's receive
            // rate estimation window
            i64SleepTime = i64SleepRandomRange;
            continue;
        }

        RequestServingStat stats;
        stats.i64ReplyingTime = getTimeInMilliseconds();
        //const int64 i64ReplyTimeoutWindow = ((i64SleepTime - i64SleepRandomRange) * 0.9);
        for (unsigned int i = 0; ppszInterfaces[i] != NULL; i++) {
            RangesByMsgId *pRangeByMsgId = pIncompleteMsgsByIface->get (ppszInterfaces[i]);
            if (pRangeByMsgId == NULL) {
                free (ppszInterfaces[i]);
                ppszInterfaces[i] = NULL;
                continue;
            }
            const char *pszOutgoingInterfaces[2] = { ppszInterfaces[i], NULL };
            RangesByMsgId::Iterator iter = pRangeByMsgId->getAllElements();
            for (; !iter.end(); iter.nextElement()) {
                stats.uiNMsgs++;
                Ranges *pRanges = iter.getValue();
                uint16 ui16MinActiveNeighbor = pRanges->_avgMinimumNumberOfActiveNeighbors.getMin();
                uint16 ui16AvgActiveNeighbor = pRanges->_avgMinimumNumberOfActiveNeighbors.getAverage();
                checkAndLogMsg (pszMethodName, Logger::L_SevereError, "active neighbors: avg: %u min: %u\n", ui16AvgActiveNeighbor, ui16MinActiveNeighbor);
                uint16 ui16ActiveNeighbor = 18;
                //const float fReplyTimeoutFactor = (pRanges->_bOriginatedLocally && (ui16ActiveNeighbor > 2) ? 0.618f : 3.0f); // allows the peers reply more
                //int64 i64ReplyTimeout = i64ArrivalTime + (i64ReplyTimeoutWindow * fReplyTimeoutFactor);
                uint32 ui32Begin, ui32End;
                String target (DisServiceMsgHelper::getMultiNodeTarget (pRanges->_requestingPeers));
                for (int rc = pRanges->_ranges.getFirst (ui32Begin, ui32End, true); rc == 0; rc = pRanges->_ranges.getNext (ui32Begin, ui32End)) {
                    static const int64 MAXIMUM_INT64 =
                    #ifdef WIN32
                        INT64_MAX;
                    #else
                        9223372036854775807LL;
                    #endif
                    stats.uiNRanges++;
                    stats.ui64TotalBytes += (ui32End - ui32Begin);
                    DisServiceMsg::Range range (ui32Begin, ui32End);
                    handleDataRequestMessage (iter.getKey(), &range, false, target, ui16ActiveNeighbor, i64ArrivalTime,
                                              (const char **)&pszOutgoingInterfaces, MAXIMUM_INT64);
                }
            }
            free (ppszInterfaces[i]);
            ppszInterfaces[i] = NULL;
        }
        stats.i64ReplyingTime = getTimeInMilliseconds() - stats.i64ReplyingTime;
        stats.log (pszMethodName);
        free (ppszInterfaces);
        const int64 i64TimeOffset = (rand() % i64SleepRandomRange);
        i64SleepTime = i64FragReqTimeout + ((((rand() % 100) + 1.0f) < 50.0f) ? -i64TimeOffset : i64TimeOffset);
    }

    terminating();
}

AsynchronousDataRequestHandlerV4::Ranges::Ranges (bool bOriginatedLocally)
    : _bOriginatedLocally (bOriginatedLocally),
      _i64FirstRequest (getTimeInMilliseconds()),
      _i64LatestRequest (_i64FirstRequest),
      _ranges (false),
      _avgMinimumNumberOfActiveNeighbors (7000)
{
}

AsynchronousDataRequestHandlerV4::Ranges::~Ranges (void)
{
}

void AsynchronousDataRequestHandlerV4::Ranges::reset (void)
{
    _ranges.reset();
    _requestingPeers.removeAll();
}

//------------------------------------------------------------------------------
// AsynchronousDataRequestHandler::RequestByInterface
//------------------------------------------------------------------------------

AsynchronousDataRequestHandlerV2::RequestByInterface::RequestByInterface (const char *pszIncomingInterface)
    : _incomingInterface (pszIncomingInterface)
{
}

AsynchronousDataRequestHandlerV2::RequestByInterface::~RequestByInterface (void)
{
}

//------------------------------------------------------------------------------
// AsynchronousDataRequestHandler::RequestByMessageId
//------------------------------------------------------------------------------

AsynchronousDataRequestHandlerV2::RequestsByMessageIdList::RequestsByMessageIdList (void)
    : _queuedRequests (false)
{
}

AsynchronousDataRequestHandlerV2::RequestsByMessageIdList::~RequestsByMessageIdList (void)
{
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
    : DataRequestHandler (SYNC, pDisService)
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

AsynchronousDataRequestHandlerV2::QueuedRequest * AsynchronousDataRequestHandlerV2::RequestsByMessageIdList::pop (uint32 ui32AgeThreshold)
{
    _m.lock();
    QueuedRequest *pRet = _queuedRequests.getFirst();
    if (pRet != NULL) {
        const int64 i64Diff = getTimeInMilliseconds() - pRet->_i64ArrivalOfFirstRequestTimestamp;
        // Serve it later if it's too "young"
        if (i64Diff < ui32AgeThreshold) {
            // Because the request are ordered by _i64ArrivalOfFirstRequestTimestamp,
            // if the current is "too young", the next requests are as well.
            // Therefore I can go to the next interface
            _m.unlock();
            return NULL;
        }
        _queuedRequests.remove (pRet);
    }
    _m.unlock();
    return pRet;
}

