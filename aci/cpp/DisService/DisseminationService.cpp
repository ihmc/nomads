/*
 * DisseminationService.cpp
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

#include "DisseminationService.h"

#include "ConfigFileReader.h"
#include "ControllerFactory.h"
#include "DataCache.h"
#include "DataRequestHandler.h"
#include "DataRequestServer.h"
#include "DefaultDataCacheExpirationController.h"
#include "DefaultDataCacheReplicationController.h"
#include "DefaultSearchController.h"
#include "ChunkRetrievalController.h"
#include "DisseminationServiceListener.h"
#include "DisServiceDefs.h"
#include "DisServiceMsg.h"
#include "DisServiceStats.h"
#include "DisServiceStatus.h"
#include "DisServiceStatusNotifier.h"
#include "DSSFLib.h"
#include "HistoryFactory.h"
#include "ListenerNotifier.h"
#include "Message.h"
#include "MessageId.h"
#include "MessageReassembler.h"
#include "NetworkTrafficMemory.h"
#include "NodeId.h"
#include "NodeInfo.h"
#include "PeerStatusListener.h"
#include "PersistentDataCache.h"
#include "PropertyStoreInterface.h"
#include "ReceivedMessagesInterface.h"
#include "RequestsState.h"
#include "SQLMessageHeaderStorage.h"
#include "Subscription.h"
#include "SubscriptionFactory.h"
#include "SubscriptionState.h"
#include "SQLTransmissionHistory.h"
#include "TransmissionService.h"
#include "TransmissionServiceListener.h"
#include "TransmissionHistoryInterface.h"
#include "Utils.h"
#include "RangeDLList.h"
#include "WorldState.h"
#include "TopologyWorldState.h"

#include "DataRequestHandler.h"
#include "RateEstimator.h"
#include "BandwidthSharing.h"

#include "ChunkingAdaptor.h"
#include "Chunker.h"
#include "MimeUtils.h"

#include "BufferReader.h"
#include "BufferWriter.h"
#include "ConfigManager.h"
#include "FileReader.h"
#include "FileWriter.h"
#include "Logger.h"
#include "MD5.h"
#include "NLFLib.h"
#include "PtrLList.h"
#include "StringTokenizer.h"
#include "SubscriptionForwardingController.h"
#include "Searches.h"
#include "DisServiceMsgHelper.h"
#include "DataRequestServer.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

#define bBandwidthSharingEnabled (_pBandwidthSharing != NULL)
#define bNetworkStateNotifierEnabled ((_pNetworkStateNotifier != NULL) && (_pNetworkStateNotifier->getListenerCount() > 0))

const float DisseminationService::DEFAULT_PROB_CONTACT = 1.0f;
const float DisseminationService::DEFAULT_PROB_THRESHOLD = 0.1f;
const float DisseminationService::DEFAULT_ADD_PARAM = 0.9f;
const float DisseminationService::DEFAULT_AGE_PARAM = 0.999f;

int strcpyAndFill (char *pszDst, uint32 ui32DestLen, const char *pszSrc, uint32 ui32SrcLen);

DisseminationService::DisseminationService (ConfigManager *pCfgMgr)
    : _m (8), _mBrcast (9), _mKeepAlive (10), _mGetData (11),
      _mControllers (12), _mToListeners (13), _mToPeerListeners (14),
      _mAsynchronousNotify (30)
{
    construct (pCfgMgr);
}

DisseminationService::DisseminationService (const char *pszNodeUID)
    : _m (8), _mBrcast (9), _mKeepAlive (10), _mGetData (11),
      _mControllers (12), _mToListeners (13), _mToPeerListeners (14),
      _mAsynchronousNotify (30)
{
    ConfigManager *pCfgMgr = new ConfigManager();
    if (pszNodeUID != NULL) {
        pCfgMgr->setValue ("aci.disService.nodeUUID", pszNodeUID);
    }
    construct (pCfgMgr);
}

DisseminationService::DisseminationService (uint16 ui16Port, const char *pszSenderId)
    : _m (8), _mBrcast (9), _mKeepAlive (10), _mGetData (11),
      _mControllers (12), _mToListeners (13), _mToPeerListeners (14),
      _mAsynchronousNotify (30)
{
    ConfigManager *pCfgMgr = new ConfigManager();
    if (pszSenderId != NULL) {
        pCfgMgr->setValue ("aci.disService.nodeUUID", pszSenderId);
    }
    if (ui16Port != 0) {
        pCfgMgr->setValue ("aci.disservice.networkMessageService.port", ui16Port);
    }
    construct (pCfgMgr);
}

DisseminationService::~DisseminationService (void)
{
    delete _pChunkingConf;              _pChunkingConf          = NULL;
    delete _pNetworkStateNotifier;      _pNetworkStateNotifier  = NULL;
    delete _pDCRepCtlr;                 _pDCRepCtlr             = NULL;
    delete _pDefaultDCRepCtlr;          _pDefaultDCRepCtlr      = NULL;
    delete _pDCExpCtlr;                 _pDCExpCtlr             = NULL;
    delete _pDefaultDCExpCtlr;          _pDefaultDCExpCtlr      = NULL;
    delete _pFwdCtlr;                   _pFwdCtlr               = NULL;
    delete _pDefaultFwdCtlr;            _pDefaultFwdCtlr        = NULL;
    delete _pSubFwdCtlr;                 _pSubFwdCtlr           = NULL;
    delete _pDataReqSvr;            _pDataReqSvr        = NULL;
    delete _pNetTrafficMemory;          _pNetTrafficMemory      = NULL;
    delete _pTrSvc;                     _pTrSvc                 = NULL;
    delete _pTrSvcListener;             _pTrSvcListener         = NULL;
    delete _pStats;                     _pStats                 = NULL;
    delete _pStatusNotifier;            _pStatusNotifier        = NULL;
    delete _pSubscriptionState;         _pSubscriptionState     = NULL;
    delete _pPeerState;                 _pPeerState             = NULL;
    delete _pDataCacheInterface;        _pDataCacheInterface    = NULL;
    delete _pDataRequestHandler;        _pDataRequestHandler    = NULL;
    delete _pBandwidthSharing;          _pBandwidthSharing      = NULL;
    delete _pChunkRetrCtrl;             _pChunkRetrCtrl         = NULL;
    delete _pDiscoveryCtrl;             _pDiscoveryCtrl         = NULL;
    delete _pSearchCtrl;                _pSearchCtrl            = NULL;
}

int DisseminationService::init (void)
{
    const char *pszMethodName = "DisseminationService::init";
    ConfigFileReader cfgReader (_pCfgMgr);

    _m.lock (32);

    // Instantiate data storage
    _pDataCacheInterface = DataCacheFactory::getDataCache (_pCfgMgr);

    NodeId nodeIdGen (_pDataCacheInterface->getStorageInterface()->getPropertyStore());
    _nodeId = nodeIdGen.generateNodeId (_pCfgMgr);
    _sessionId = _pCfgMgr->getValue ("aci.disService.sessionKey");
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Node ID set to: %s\n", getNodeId());

    int ret;
    // Get the Network Interfaces that should be used to send and/or receive
    char **ppszOutgoingInterfaces = cfgReader.getNetIFs();
    char **ppszIgnoredInterfaces = cfgReader.getIgnoredNetIFs();
    char **ppszAddedInterfaces = cfgReader.getAddedNetIFs();
    _ui8TransmissionMode = cfgReader.getTransmissionMode();
    _bKeepAliveMsgEnabled = cfgReader.getKeepAliveMsgEnabled();
    _bQueryDataCacheEnabled = cfgReader.getQueryDataCacheEnabled();
    _ui8QueryDataCacheReplyType = cfgReader.getQueryDataCacheReply();
    _bSubscriptionStateExchangeEnabled = cfgReader.getSubscriptionStateExchangeEnabled();
    _bTargetFilteringEnabled = cfgReader.isTargetIDFilteringEnabled();
    _bOppListeningEnabled = _pCfgMgr->getValueAsBool("aci.disService.propagation.oppListening", true);
    _ignoreMissingFragReqTime = cfgReader.getIgnoreRequestInterval();
    _ui16SubscriptionStatePeriod = cfgReader.getSubscriptionStatePeriod();
    _ui16KeepAliveInterval = (_bKeepAliveMsgEnabled ? cfgReader.getKeepAliveInterval() : DEFAULT_KEEP_ALIVE_INTERVAL);
    _ui32DeadPeerInterval = cfgReader.getDeadPeerInterval();
    int64 i64WindowSize = cfgReader.getConnectivityHistoryWindowSize();
    if (i64WindowSize < _ui32DeadPeerInterval) {
        i64WindowSize = (int64) _ui32DeadPeerInterval + i64WindowSize;
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "the size of the Connectivity History was lower than the dead peer interval - "
                        "adjusted the Connectivity History Window Size\n");
    }

    _pDataReqSvr = new DataRequestServer (this, _bTargetFilteringEnabled, _bOppListeningEnabled);
    _pDataReqSvr->init (_pCfgMgr);
    _ui16SubscriptionsExchangePeriod = cfgReader.getSubscriptionsExchangePeriod();
    _ui16TopologyExchangePeriod = cfgReader.getTopologyExchangePeriod();

    _pPeerState->setDeadPeerInterval(_ui32DeadPeerInterval);
    _pNetTrafficMemory = new NetworkTrafficMemory (_ignoreMissingFragReqTime);

    // Initialize the LocalNodeInfo
    uint32 ui32DefaultKbpsBandwidthLimit = cfgReader.getBandwidth();
    _pLocalNodeInfo = new LocalNodeInfo (getNodeId(), cfgReader.getMemorySpace(),
                                         cfgReader.getBandwidthIndex (ui32DefaultKbpsBandwidthLimit),
                                         i64WindowSize);

    // Instantiate the MessagePropagationService
    _pTrSvc = TransmissionService::getInstance (_pCfgMgr, getNodeId(), getSessionId());
    if (_pTrSvc == NULL) {
        return -1;
    }
    setMaxFragmentSize (DEFAULT_MAX_FRAGMENT_SIZE);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "MTU set to %u\n", getMaxFragmentSize());

    _i64TailTimeout = getMissingFragmentTimeout() * 3;
    checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "Setting tail timeout to %llu\n", _i64TailTimeout);

    // Instantiate SubscriptionState
    _pSubscriptionState = new SubscriptionState (this, _pLocalNodeInfo);

    // Instantiate and configure Message Reassembler
    float fDefMissingFragReqProb = MessageReassembler::DEFAULT_RANGE_REQUEST_PROB;
    if (_pCfgMgr->hasValue ("aci.disService.reassembler.request.probability.default")) {
        fDefMissingFragReqProb = (float) atof (_pCfgMgr->getValue ("aci.disService.reassembler.request.probability.default"));
    }

    // Read Message Reassembler Configuration Parameters
    float fReceiveRateThreshold = MessageReassembler::DEFAULT_RECEIVE_RATE_THRESHOLD;
    if (_pCfgMgr->hasValue ("aci.disService.reassembler.request.incomingRate.threshold")) {
        fReceiveRateThreshold = (float) atof (_pCfgMgr->getValue ("aci.disService.reassembler.request.incomingRate.threshold"));
    }

    bool bUseExpBackOff = _pCfgMgr->getValueAsBool ("aci.disService.reassembler.ExpBackoff.enabled",
                                                           MessageReassembler::DEFAULT_ENABLE_EXPONENTIAL_BACKOFF);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Exponential Backoff is %s\n",
                    (bUseExpBackOff ? "enabled" : "disabled"));

    bool bReqFragmentsForOpporAcquiredMsg = _pCfgMgr->getValueAsBool ("aci.disService.reassembler.opportRecvdFragments.request",
                                                                      MessageReassembler::DEFAULT_REQUEST_FRAGMENTS_FOR_OPPORTUNISTICALLY_ACQUIRED_MESSAGES);
    checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "Opportunistically received fragments will %s be reassembled\n",
                    (bReqFragmentsForOpporAcquiredMsg ? "" : "not"));

    // Instantiate Message Reassembler
    _pMessageReassembler = new MessageReassembler (this, _pPeerState, _pSubscriptionState, _pLocalNodeInfo,
                                                   fDefMissingFragReqProb, fReceiveRateThreshold,
                                                   bUseExpBackOff, bReqFragmentsForOpporAcquiredMsg);

    int iReqLimit = _pCfgMgr->getValueAsInt ("aci.disService.reassembler.missingFragReq.limit.discrete",
                                             MessageReassembler::UNLIMITED_MAX_NUMBER_OF_REQUESTS);
    checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "Missing Fragment Request Limit is set on %d\n", iReqLimit);
    _pMessageReassembler->setRequestLimit (iReqLimit);

    // and - if the data cache contains any message - load the most recent state
    PtrLList<StorageInterface::RetrievedSubscription> *pRetrievedSubs = _pDataCacheInterface->getSubscriptions (getNodeId());
    StorageInterface::RetrievedSubscription *pNextSubscription;
    if (pRetrievedSubs != NULL) {
        StorageInterface::RetrievedSubscription *pCurrSubscription = pRetrievedSubs->getFirst();
        uint32 ui32NextExpectedSeqId;
        while (pCurrSubscription != NULL) {
            // The sequence ID is incremented across the "tags", therefore the
            // tag does not have to be specified, otherwise the method will
            // return the last sequence ID for published with the specific tag,
            // not necessarily the sequence ID of the latest message published
            // within the group
            ui32NextExpectedSeqId = _pDataCacheInterface->getNextExpectedSeqId (pCurrSubscription->pszGroupName, getNodeId(), 0);
            _pLocalNodeInfo->setGroupPubState (pCurrSubscription->pszGroupName, ui32NextExpectedSeqId);

            pNextSubscription = pRetrievedSubs->getNext();
            pRetrievedSubs->remove (pCurrSubscription);
            free ((char*)pCurrSubscription->pszGroupName);
            pCurrSubscription->pszGroupName = NULL;
            delete pCurrSubscription;
            pCurrSubscription = NULL;
            pCurrSubscription = pNextSubscription;
        }
        delete pRetrievedSubs;
        pRetrievedSubs = NULL;
    }

    // Set Controllers
    ControllerFactory::init (this, _pCfgMgr);

    // configure WorldState
    _pPeerState->setLocalNodeInfo (_pLocalNodeInfo);
    _ui16HistoryReqCycle = 3;

    // configure TopologyWorldState if necessary
    if (_bSubscriptionsExchangeEnabled) { // if subscriptions exchange is enabled
       ((TopologyWorldState *) _pPeerState)->setSubscriptionsExchangeEnabled (_bSubscriptionsExchangeEnabled);
    }
    if (_bTopologyExchangeEnabled) { // if topology exchange is enabled
       ((TopologyWorldState *) _pPeerState)->setTopologyExchangeEnabled (_bTopologyExchangeEnabled);
    }
    if (_bTopologyExchangeEnabled) { // if topology exchange is enabled
        ((TopologyWorldState *) _pPeerState)->setParameters (cfgReader.getProbContact(),
                                                             cfgReader.getProbThreshold(),
                                                             cfgReader.getAddParam(),
                                                             cfgReader.getAgeParam()); 
    }

    // Transmission History
    _pTransmissionHistoryInterface = TransmissionHistoryInterface::getTransmissionHistory();
    if (_pTransmissionHistoryInterface == NULL) {
        _m.unlock (32);
        return -1;
    }

    // Received Messages
    _pReceivedMessagesInterface = ReceivedMessagesInterface::getReceivedMessagesInterface();
    if (_pReceivedMessagesInterface == NULL) {
        _m.unlock (32);
        return -2;
    }

    //Data Request Handler
    bool bUseDataRequestHandlerForRequests = _pCfgMgr->getValueAsBool ("aci.disService.dataRequestHandler.enable", true);
    int64 i64DRHSleepTime = _pCfgMgr->getValueAsInt64 ("aci.disService.dataRequestHandler.sleepTime",
                                                              DataRequestHandler::DEFAULT_SLEEP_TIME);
    int64 i64DRHBaseTime = _pCfgMgr->getValueAsInt64 ("aci.disService.dataRequestHandler.baseTime",
                                                             DataRequestHandler::DEFAULT_BASE_TIME);
    uint16 ui16DRHOffsetRange = _pCfgMgr->getValueAsInt ("aci.disService.dataRequestHandler.offsetRange",
                                                                DataRequestHandler::DEFAULT_OFFSET_RANGE);
    _pDataRequestHandler = DataRequestHandler::getInstance (this, _pTrSvc, i64DRHSleepTime, i64DRHBaseTime,
                                                            getMissingFragmentTimeout(), ui16DRHOffsetRange,
                                                            DataRequestHandler::DEFAULT_RECEIVE_RATE_THRESHOLD,
                                                            bUseDataRequestHandlerForRequests);

    if (_pTrSvc != NULL) {
        // Init Transmission Service
        ret = _pTrSvc->init (_pCfgMgr, (const char **) ppszOutgoingInterfaces,
                             (const char **) ppszIgnoredInterfaces, (const char **) ppszAddedInterfaces);

        // Instantiate the MessagePropagationServiceListener and register it
        const bool bUseTrafficMem = _pCfgMgr->getValueAsBool ("aci.disService.transmissionService.listener.trafficHistory.enable", false);
        _pTrSvcListener = new TransmissionServiceListener (this, _pDataRequestHandler, _pLocalNodeInfo, _pMessageReassembler,
                                                           _pSubscriptionState, (bUseTrafficMem ? _pNetTrafficMemory : NULL),
                                                           _bOppListeningEnabled, _bTargetFilteringEnabled);
        _pTrSvc->registerHandlerCallback (MPSMT_DisService, _pTrSvcListener);

        // Set Default Transmission Rate if any
        if (ui32DefaultKbpsBandwidthLimit > 0) {
            _pTrSvc->setTransmitRateLimit (ui32DefaultKbpsBandwidthLimit * (1024/8)); // Kbps -> Bytes per second
            _pTrSvc->setTransmitRateLimitCap (ui32DefaultKbpsBandwidthLimit * (1024/8)); // Kbps -> Bytes per second
        }
        // Set Default Transmission Rate for a specific interface
        char **ppszInterfaces = _pTrSvc->getActiveInterfacesAddress();
        if (ppszInterfaces != NULL) {
            String rateLimitByInterfaceBase = "aci.disService.nodeConfiguration.bandwidthByInterface.";
            String rateLimitByInterface;
            for (int i = 0; ppszInterfaces[i]; i++) {
                rateLimitByInterface = rateLimitByInterfaceBase + ppszInterfaces[i];
                if (_pCfgMgr->hasValue (rateLimitByInterface)) {
                    uint32 ui32RateLimit = _pCfgMgr->getValueAsUInt32 (rateLimitByInterface);
                    _pTrSvc->setTransmitRateLimit (ppszInterfaces[i], (uint32) 0, ui32RateLimit * (1024/8));
                }
                free (ppszInterfaces[i]);
                ppszInterfaces[i] = NULL;
            }
            free (ppszInterfaces);
            ppszInterfaces = NULL;
        }
        //Set Default Transmission Rate for a certain destination
        if (_pCfgMgr->hasValue ("aci.disService.nodeConfiguration.bandwidthByDestination")) {
            StringTokenizer tokenizer (_pCfgMgr->getValue ("aci.disService.nodeConfiguration.bandwidthByDestination"), ';', ';');
            const char *pszToken;
            const char *pszInnerToken;
            while ((pszToken = tokenizer.getNextToken()) != NULL) {
                StringTokenizer innerTokenizer (pszToken, ',', ',');
                DArray2<char*> tokens;
                for (int i = 0; (pszInnerToken = innerTokenizer.getNextToken()) != NULL; i++) {
                    tokens[i] = (char *) pszInnerToken;
                }
                switch (tokens.size()) {
                    case 2:
                        _pTrSvc->setTransmitRateLimit (tokens[0], atoui32 (tokens[1]) * (1024/8));    // Kbps -> Bytes per second
                        break;
                    case 3:
                         _pTrSvc->setTransmitRateLimit (tokens[0], tokens[1], atoui32 (tokens[2]) * (1024/8));    // Kbps -> Bytes per second
                         break;
                    default:
                        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                                        "Error when parsing aci.disService.nodeConfiguration.bandwidthByDestination property %s\n",
                                        pszToken);
                        exit (-1);
                }
                for (unsigned int i = 0; i < tokens.size(); i++) {
                    tokens.clear(i);
                }
            }
        }
    }
    if (ret != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "MessagePropagationService could not be initialized, "
                        "DisseminationService will not work correctly.\n");
        _m.unlock (32);
        return -3;
    }
    //BELOW HERE, WE CAN ASSUME THE TRANSMISSION SERVICE EXISTS

    //Enable rate estimation
    const bool bUseRateEstimator = _pCfgMgr->getValueAsBool ("aci.disService.nodeConfiguration.estimateCapacity", false);
    _pTrSvcHelper = new TransmissionServiceHelper (bUseRateEstimator);

    //Set up the node importance (for bandwidth sharing)
    _ui8NodeImportance = (uint8) _pCfgMgr->getValueAsInt ("aci.disService.nodeConfiguration.bandwidthSharing.nodeImportance", 0);
    checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "Node importance set to %u.\n", _ui8NodeImportance);
    //Enable BandwidthSharing
    if (_pCfgMgr->getValueAsBool("aci.disService.nodeConfiguration.bandwidthSharing.enabled", false)) {
        _pBandwidthSharing = new BandwidthSharing (_pPeerState, _pTrSvc);
        int iSharingRule = _pCfgMgr->getValueAsInt ("aci.disService.nodeConfiguration.bandwidthSharing.sharingRule", 0);
        _pBandwidthSharing->setSharingRule (iSharingRule);
    }
    else {
        _pBandwidthSharing = NULL;
    }

    // Instantiate Network State Notifier
    _pNetworkStateNotifier = new NetworkStateListenerNotifier();
    if (_pNetworkStateNotifier == NULL) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        _m.unlock (32);
        return -4;
    }

    // Instantiate the Chunking Configuration
    _pChunkingConf = new ChunkingConfiguration ();
    if (_pChunkingConf->init (_pCfgMgr) < 0) {
        _m.unlock (32);
        return -5;
    }
    // Run MessageReassembler
    _pMessageReassembler->registerTransmissionService (_pTrSvc);
    _pMessageReassembler->start();

    // Assuming all the DisService Components are now instantiated.
    // Register the listeners
    unsigned int uiIndex;
    PeerState::registerWithListeners (this, _pPeerState);
    _pTrSvc->registerWithListeners (this);
    _pDefaultDCRepCtlr = ControllerFactory::getRepControllerAndRegisterListeners();
    _pDefaultFwdCtlr = ControllerFactory::getForwControllerAndRegisterListeners();
    _pDefaultDCExpCtlr = ControllerFactory::getExpControllerAndRegisterListeners();
    _pSubFwdCtlr = ControllerFactory::getSubForwControllerAndRegisterListeners();    
    if (registerMessageListener (_pStats, uiIndex) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "_pStats", "MessageListener");
    }

    // In this case order matters!!! ChunkRetrievalController has to be registered
    // before the ChunkDiscoveryController, because it needs to be notified before.
    // TODO: relying on the registration order is dangerous though, consider refactoring!
    unsigned int uiListenerIndex;
    _pDiscoveryCtrl = new ChunkDiscoveryController (this, _pDataCacheInterface);
    _pChunkRetrCtrl = new ChunkRetrievalController (this, _pDiscoveryCtrl, _pDataCacheInterface);
    if (registerMessageListener (_pChunkRetrCtrl, uiListenerIndex) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "_pChunkretrCtrl", "MessageListener");
        _m.unlock (32);
        return -6;
    }
    if (registerMessageListener (_pDiscoveryCtrl, uiListenerIndex) < 0) {
        checkAndLogMsg (pszMethodName, listerRegistrationFailed, "_pDiscoveryCtrl", "MessageListener");
        _m.unlock (32);
        return -7;
    }

    String searchCtrlType;
    if ((searchCtrlType = _pCfgMgr->getValue ("aci.disService.searchController", "DEFAULT"))) {
        unsigned int uiSearchListener;
        if (searchCtrlType == "DEFAULT") {
            _pSearchCtrl = new DefaultSearchController (this, _pDataCacheInterface, _pMessageReassembler);
            if (registerSearchListener (_pSearchCtrl, uiSearchListener) < 0) {
                checkAndLogMsg (pszMethodName, listerRegistrationFailed, "_pSearchCtrl", "SearchListener");
            }
        }
        else {
            _pSearchCtrl = NULL; 
        }
    }

    const char *pszNotifyAddr = "127.0.0.1";
    if (_pCfgMgr->hasValue ("aci.disService.statusNotifyAddress")) {
        pszNotifyAddr = _pCfgMgr->getValue ("aci.disService.statusNotifyAddress");
    }

    _pStatusNotifier = new DisServiceStatusNotifier (getNodeId(), pszNotifyAddr);
    // Status Notifier
    if (_pStatusNotifier && (_pStatusNotifier->init (DEFAULT_DIS_SERVICE_STATUS_PORT) != 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Status Notifier could not be initialized.\n");
        _m.unlock (32);
        return -8;
    }

    _m.unlock (32);
    return 0;
}

int DisseminationService::start (void)
{
    _pTrSvc->start();
    return ManageableThread::start();
}

void DisseminationService::requestTermination (void)
{
    if (_pTrSvc != NULL) {
        _pTrSvc->requestTermination();
    }
    if (_pMessageReassembler != NULL) {
        _pMessageReassembler->requestTerminationAndWait();
    }
    DataRequestHandler::halt (_pDataRequestHandler);
    ManageableThread::requestTerminationAndWait();
}

void DisseminationService::requestTerminationAndWait (void)
{
    if (_pTrSvc != NULL) {
        _pTrSvc->stop();
    }
    if (_pMessageReassembler != NULL) {
        _pMessageReassembler->requestTerminationAndWait();
    }
    ManageableThread::requestTerminationAndWait();
}

const char * DisseminationService::getNodeId (void)
{
    return _nodeId.c_str();
}

const char * DisseminationService::getSessionId (void)
{
    return _sessionId.c_str();
}

int DisseminationService::setMaxFragmentSize (uint16 ui16MaxFragmentSize)
{
    int rc = -1;
    if (ui16MaxFragmentSize > 0) {
        _m.lock (45);
        if (_pTrSvc != NULL) {
            _pTrSvc->setMaxFragmentSize (ui16MaxFragmentSize);
            rc = 0;
        }
        _m.unlock (45);
    }
    return rc;
}

int DisseminationService::setMissingFragmentTimeout (uint32 ui32TimeoutInMS)
{
    if (ui32TimeoutInMS > 0) {
        _i64FragmentRequestTimeout = ui32TimeoutInMS;
        return 0;
    }
    return -1;
}

PeerState * DisseminationService::getPeerState (void)
{
    return _pPeerState;
}

int DisseminationService::store (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                 const char *pszMimeType, const void *pMetadata, uint32 ui32MetadataLength, const void *pData,
                                 uint32 ui32DataLength, int64 i64ExpirationTime, uint16 ui16HistoryWindow, uint16 ui16Tag,
                                 uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen)
{
    _m.lock (46);
    if (!checkGroupName (pszGroupName)) {
        _m.unlock (46);
        return -1;
    }
    int rc = storeInternal (ui16ClientId, pszGroupName, pszObjectId, pszInstanceId,
                            pMetadata, ui32MetadataLength, pData,
                            ui32DataLength, pszMimeType, false, i64ExpirationTime,
                            ui16HistoryWindow, ui16Tag, ui8Priority, pszIdBuf, ui32IdBufLen);
    _m.unlock (46);
    return rc;
}

int DisseminationService::storeInternal (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                         const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32DataLength,
                                         const char *pszDataMimeType, bool bChunkData, int64 i64ExpirationTime, uint16 ui16HistoryWindow,
                                         uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen, const char *pszRefObj)
{
    return storeInternalNoNotify (ui16ClientId, pszGroupName, pszObjectId, pszInstanceId, pMetadata, ui32MetadataLength,
                                  pData, ui32DataLength, pszDataMimeType, bChunkData, i64ExpirationTime, ui16HistoryWindow,
                                  ui16Tag, ui8Priority, pszIdBuf, ui32IdBufLen, pszRefObj, Notifier::ID_UNSET);
}

int DisseminationService::storeInternalNoNotify (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                                 const void *pMetadata, uint32 ui32MetadataLength, const void *pData,
                                                 uint32 ui32DataLength, const char *pszDataMimeType, bool bChunkData, int64 i64ExpirationTime,
                                                 uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen,
                                                 const char *pszRefObj, unsigned int uiListener)
{
    if (_pTrSvc == NULL) {
        return -1;
    }

    // Concatenate Data and MetaData
    uint32 ui32DataAndMetaDataSize = ui32MetadataLength + ui32DataLength;
    void *pDataAndMeta = NULL;
    bool bDeleteDataAndMeta = false;
    if (pMetadata != NULL) {
        pDataAndMeta = malloc (ui32DataAndMetaDataSize);
        memcpy (pDataAndMeta, pMetadata, ui32MetadataLength);
        memcpy (((char *)pDataAndMeta)+ui32MetadataLength, pData, ui32DataLength);
        bDeleteDataAndMeta = true;
    }
    else {
        pDataAndMeta = (void*)pData;
    }

    const char *pszMsgId = NULL;
    uint32 ui32MsgIdLen = 0;
    int rc = 0;
    if (bChunkData) {
        assert (pMetadata == NULL && ui32MetadataLength == 0);
        assert (pszRefObj == NULL);

        PtrLList<Chunker::Fragment> *pChunks = NULL;
        if (MimeUtils::mimeTypeToFragmentType (pszDataMimeType) == Chunker::UNSUPPORTED) {
            pChunks = new PtrLList<Chunker::Fragment> (ChunkingAdaptor::getChunkerFragment (pDataAndMeta, ui32DataLength));
        }
        else {
            const uint8 ui8NChunks = _pChunkingConf->getNumberofChunks (ui32DataAndMetaDataSize);
            pChunks = Chunker::fragmentBuffer (pDataAndMeta, ui32DataAndMetaDataSize,
                                               MimeUtils::mimeTypeToFragmentType (pszDataMimeType), ui8NChunks, Chunker::JPEG, 90);
        }
        if (pChunks == NULL) {
            return -2;
        }

        char *pszBaseGroupName = removeOnDemandSuffixFromGroupName (pszGroupName);
        if (pszBaseGroupName == NULL) {
            return -3;
        }
        uint32 ui32SeqId = _pLocalNodeInfo->getGroupPubState (pszBaseGroupName);
        free (pszBaseGroupName);
        Chunker::Fragment *pChunk = pChunks->getFirst();
        ui32MsgIdLen = 0;
        for (bool bFirst = true; pChunk != NULL;) {

            void *pBuf = malloc (pChunk->ui64FragLen);
            if (pBuf == NULL) {
                checkAndLogMsg ("DisseminationService::storeInternalNoNotify", memoryExhausted);
                return -4;
            }
            if (pChunk->pReader == NULL) {
                checkAndLogMsg ("DisseminationService::storeInternalNoNotify", Logger::L_SevereError,
                                "pChunk->pReader is NULL\n");
                return -5;
            }
            if (pChunk->pReader->readBytes (pBuf, (uint32) pChunk->ui64FragLen) < 0) {
                checkAndLogMsg ("DisseminationService::storeInternalNoNotify", Logger::L_SevereError,
                                "error reading from pChunk->pReader\n");
                return -6;
            }

            char *pszChecksum = MD5Utils::getMD5Checksum ((const void *)pBuf, (uint32) pChunk->ui64FragLen);
            ChunkMsgInfo *pCMI = ChunkingAdaptor::toChunkMsgInfo (pszGroupName, getNodeId(),
                                                                  ui32SeqId, pszObjectId, pszInstanceId,
                                                                  ui16Tag, ui16ClientId, 0, pszDataMimeType,
                                                                  pszChecksum, ui16HistoryWindow,
                                                                  ui8Priority, i64ExpirationTime,
                                                                  pChunk);
            if (pszChecksum != NULL) {
                free (pszChecksum);
                pszChecksum = NULL;
            }
            if (bFirst) {
                pszMsgId = pCMI->getLargeObjectId();
                ui32MsgIdLen = (uint32) strlen (pszMsgId);
                if ((pszIdBuf == NULL) || (ui32IdBufLen < ui32MsgIdLen)) {
                    // The ID of the message can not be returned to the application, and the
                    // message could not be retrieved by the application to push it.
                    // Therefore adding the message to the cache is likely to be useless,
                    // thus, fail
                    return -7;
                }
                bFirst = false;
            }

            rc = storeInternalNoNotify (pCMI, pBuf, uiListener);
            if (rc < 0) {
               break; 
            }

            delete pCMI;
            free (pBuf);

            Chunker::Fragment *pTmpChunk = pChunks->getNext();
            pChunks->remove (pChunk);
            free (pChunk->pReader);
            pChunk = pTmpChunk;
        }
    }
    else {
        char *pszChecksum = MD5Utils::getMD5Checksum (pDataAndMeta, ui32DataAndMetaDataSize);
        MessageInfo *pMI = new MessageInfo (pszGroupName, getNodeId(), _pLocalNodeInfo->getGroupPubState (pszGroupName),
                                            pszObjectId, pszInstanceId, ui16Tag, ui16ClientId, 0, pszDataMimeType,
                                            pszChecksum, ui32DataAndMetaDataSize, ui32DataAndMetaDataSize, 0,
                                            ui32MetadataLength, ui16HistoryWindow, ui8Priority, i64ExpirationTime);
        if (pszChecksum != NULL) {
            free (pszChecksum);
            pszChecksum = NULL;
        }
        if (pMI == NULL) {
            return -3;
        }
        if (pszRefObj != NULL) {
            pMI->setReferredObject (pszRefObj);
        }

        ui32MsgIdLen = (uint32) strlen (pMI->getMsgId());
        if ((pszIdBuf == NULL) || (ui32IdBufLen < ui32MsgIdLen)) {
            // The ID of the message can not be returned to the application, and the
            // message could not be retrieved by the application to push it.
            // Therefore adding the message to the cache is likely to be useless,
            // thus, fail
            return -2;
        }

        rc = storeInternalNoNotify (pMI, pDataAndMeta, uiListener);
        if (rc < 0) {
           
        }

        pszMsgId = strDup (pMI->getMsgId());
        delete pMI;
    }

    if (rc == 0) {
        // Increment the seq id for the group
        _pLocalNodeInfo->incrementGroupPubState (pszGroupName);
        // It was already verified that the message ID can fit in pszIdBuf and
        // that pszIdBuf is not NULL
        rc = strcpyAndFill (pszIdBuf, ui32IdBufLen, pszMsgId, ui32MsgIdLen);
    }

    if (bDeleteDataAndMeta) {
        free (pDataAndMeta);
    }
    free ((void*)pszMsgId);

    return rc;
}

int DisseminationService::storeInternalNoNotify (MessageHeader *pMH, const void *pData, unsigned int uiListener)
{
    return uiListener ==  Notifier::ID_UNSET ? _pDataCacheInterface->addData (pMH, pData) :
                                               _pDataCacheInterface->addDataNoNotify (pMH, pData, uiListener);
}

int DisseminationService::push (uint16 ui16ClientId, char *pszMsgId)
{
    _m.lock (47);
    int rc = pushInternal (ui16ClientId, pszMsgId);
    _m.unlock (47);
    return rc;
}

int DisseminationService::pushInternal (uint16 ui16ClientId, char *pszMsgId)
{
    if (_pTrSvc == NULL) {
        return -1;
    }
    if (_ui8TransmissionMode != PUSH_ENABLED) {
        return -2;
    }

    MessageHeader *pMH = _pDataCacheInterface->getMessageInfo (pszMsgId);
    assert (!pMH->isChunk());

    const void *pData = _pDataCacheInterface->getData (pszMsgId);
    if (pMH == NULL || pData == NULL) {
        return -3;
    }

    void *pDataCopy = malloc (pMH->getTotalMessageLength());
    memcpy (pDataCopy, pData, pMH->getTotalMessageLength());
    Message *pMsgCopy = new Message (pMH->clone(), pDataCopy);
    _pSubscriptionState->messageArrived (pMsgCopy, NULL); // send to other applications
                                                          // running on the same instance
                                                          // of DisService.
                                                          // SubscriptionState deallocates it!
    Message msg (pMH, pData);
    int rc = pushInternal (ui16ClientId, &msg);

    //first release the data, because when you release pMH, it gets deleted
    _pDataCacheInterface->release (pMH->getMsgId(), (void*) pData);
    _pDataCacheInterface->release (pMH->getMsgId(), pMH);
    return rc;
}

bool DisseminationService::doBroadcastDataMsg (DisServiceDataMsg *pDDMsg)
{
    //TODO CHECK AND FIX
    _pPeerState->lock();
    if (_pCfgMgr->getValueAsInt ("aci.disService.forwarding.mode") == 3 
        && _pPeerState->getType() == PeerState::IMPROVED_TOPOLOGY_STATE 
        && _bSubscriptionsExchangeEnabled && _bTopologyExchangeEnabled) {
        if (((TopologyWorldState *) _pPeerState)->getForwardingStrategy ((DisServiceMsg *) pDDMsg) == ForwardingStrategy::TOPOLOGY_FORWARDING) {
            PtrLList<String> *pTargetNodes = ((TopologyWorldState *) _pPeerState)->getTargetNodes (pDDMsg);
            if (pTargetNodes) {
                String pTarget;
                for (String *pNodeId = pTargetNodes->getFirst(); pNodeId; pNodeId = pTargetNodes->getNext()) {
                    if (pTarget) {
                        pTarget = pTarget + ":" + pNodeId->c_str();
                    } else {
                        pTarget = pNodeId->c_str();
                    }
                    delete pNodeId;
                }
                pDDMsg->setTargetNodeId (pTarget.c_str());
                delete pTargetNodes;
            } else {
                _pPeerState->unlock();
                return false;
            }
        }
    }
    _pPeerState->unlock();
    return true;
}

int DisseminationService::pushInternal (uint16 ui16ClientId, Message *pMsg)
{
    DisServiceDataMsg ddMsg (getNodeId(), pMsg);
    if (doBroadcastDataMsg (&ddMsg)) {
        int rc = broadcastDisServiceDataMsg (&ddMsg, "Push");    // send over the network
        if (rc == 0) {
            // Update statistics
            MessageHeader *pMH = pMsg->getMessageHeader();
            if (pMH != NULL) {
                _pStats->messagePushedByClient (ui16ClientId, pMH->getGroupName(),
                                                pMH->getTag(), pMH->getTotalMessageLength());
            }
        }
    }
    return 0;
}

int DisseminationService::push (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                const char *pszMimeType, const void *pMetadata, uint32 ui32MetadataLength, const void *pData,
                                uint32 ui32DataLength, int64 i64ExpirationTime, uint16 ui16HistoryWindow, uint16 ui16Tag,
                                uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen)
{
    _m.lock (48);
    int rc = storeInternal (ui16ClientId, pszGroupName, pszObjectId, pszInstanceId,
                            pMetadata, ui32MetadataLength, pData, ui32DataLength,
                            pszMimeType, false, i64ExpirationTime, ui16HistoryWindow, ui16Tag,
                            ui8Priority, pszIdBuf, ui32IdBufLen);
    if (rc != 0) {
        _m.unlock (48);
        return rc;
    }
    rc = pushInternal (ui16ClientId, pszIdBuf);
    _m.unlock (48);
    return rc;
}

int DisseminationService::modify (uint16 ui16ClientId, const char *pszId, int64 i64NewExpirationTime)
{
    // TODO: implement this
    return 0;
}

int DisseminationService::modify (uint16 ui16ClientId, const char *pszId, uint8 ui8NewPriority)
{
    return _pLocalNodeInfo->modifyGroupPriority (ui16ClientId, pszId, ui8NewPriority);
}

int DisseminationService::makeAvailable (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                         const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32Length,
                                         int64 i64Expiration, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf,
                                         uint32 ui32IdBufLen)
{
    return makeAvailable (ui16ClientId, pszGroupName, pszObjectId, pszInstanceId,
                          pMetadata, ui32MetadataLength, pData, ui32Length, NULL,
                          i64Expiration, ui16HistoryWindow, ui16Tag, ui8Priority,
                          pszIdBuf, ui32IdBufLen);
}

int DisseminationService::makeAvailable (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                         const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32Length,
                                         const char *pszDataMimeType, int64 i64Expiration, uint16 ui16HistoryWindow, uint16 ui16Tag,
                                         uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen)
{
    _m.lock (49);
    if (!checkGroupName (pszGroupName)) {
        _m.unlock (49);
        return -1;
    }

    // Store data
    char *pszOnDemandGroupName = getOnDemandDataGroupName (pszGroupName);
    int rc = storeInternal (ui16ClientId, pszOnDemandGroupName, pszObjectId, pszInstanceId,
                            NULL, (uint32) 0, pData, ui32Length, pszDataMimeType, true,
                            i64Expiration, ui16HistoryWindow, ui16Tag, ui8Priority,
                            pszIdBuf, ui32IdBufLen);
    if (rc < 0) {
        free (pszOnDemandGroupName);
        pszOnDemandGroupName = NULL;
        _m.unlock (49);
        return rc;
    }

    // Store metadata
    char *pszChecksum = MD5Utils::getMD5Checksum (pMetadata, ui32MetadataLength);
    MessageInfo mi (pszGroupName, getNodeId(), _pLocalNodeInfo->getGroupPubState (pszGroupName),
                    pszObjectId, pszInstanceId, ui16Tag, ui16ClientId, 0,
                    MessageHeader::DEFAULT_MIME_TYPE, pszChecksum, ui32MetadataLength,
                    ui32MetadataLength, 0, ui32MetadataLength, ui16HistoryWindow, ui8Priority,
                    i64Expiration, false, true);
    mi.setReferredObject (pszIdBuf);
    if (pszChecksum != NULL) {
        free (pszChecksum);
        pszChecksum = NULL;
    }
    rc = _pDataCacheInterface->addData (&mi, pMetadata);
    if (rc < 0) {
        free (pszOnDemandGroupName);
        pszOnDemandGroupName = NULL;
        _m.unlock (50);
        return rc;
    }

    // Increment pub-state
    _pLocalNodeInfo->incrementGroupPubState (pszGroupName);
    _pLocalNodeInfo->incrementGroupPubState (pszOnDemandGroupName);

    free (pszOnDemandGroupName);
    pszOnDemandGroupName = NULL;

    if (_ui8TransmissionMode == PUSH_ENABLED) {
        // Push only metadata
        Message msg (&mi, pMetadata);
        if ((rc = pushInternal (ui16ClientId, &msg)) < 0) {
            _m.unlock (50);
            return -3;
        }
    }

    _m.unlock (50);
    return 0;
}

int DisseminationService::makeAvailable (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId,
                                         const char *pszInstanceId, const void *pMetadata, uint32 ui32MetadataLength,
                                         const char *pszFilePath, int64 i64Expiration, uint16 ui16HistoryWindow, uint16 ui16Tag,
                                         uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen)
{
    return makeAvailable (ui16ClientId, pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                          ui32MetadataLength, pszFilePath, NULL, i64Expiration, ui16HistoryWindow,
                          ui16Tag, ui8Priority, pszIdBuf, ui32IdBufLen);
}

/*!!*/ // The following method has not been fully implemented
int DisseminationService::makeAvailable (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId,
                                         const char *pszInstanceId, const void *pMetadata, uint32 ui32MetadataLength,
                                         const char *pszFilePath, const char *pszDataMimeType, int64 i64Expiration,
                                         uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf,
                                         uint32 ui32IdBufLen)
{
    _m.lock (51);
    if (!checkGroupName (pszGroupName)) {
        _m.unlock (51);
        return -1;
    }

    struct stat results;
    if (stat (pszFilePath, &results) != 0) {
        return -1;
    }
    char *pszChecksum = MD5Utils::getMD5Checksum (pMetadata, ui32MetadataLength);
    MessageInfo miMetadata (pszGroupName, getNodeId(), _pLocalNodeInfo->getGroupPubState (pszGroupName),
                            pszObjectId, pszInstanceId, ui16Tag, ui16ClientId, 0, MessageHeader::DEFAULT_MIME_TYPE,
                            pszChecksum, ui32MetadataLength, ui32MetadataLength, 0, 0, ui16HistoryWindow, ui8Priority,
                            i64Expiration, false, true);
    if (pszChecksum != NULL) {
        free (pszChecksum);
        pszChecksum = NULL;
    }
    // Cache the message
    _pDataCacheInterface->addData (&miMetadata, pMetadata);

    // TODO: implement this
    _pDataCacheInterface->addData (&miMetadata, pszFilePath);

    // Create and send MetaData
    if (_ui8TransmissionMode == PUSH_ENABLED) {
        Message msg (&miMetadata, pMetadata);
        broadcastDisServiceDataMsg (new DisServiceDataMsg (getNodeId(), &msg), "Make available (file)");
    }

    _pLocalNodeInfo->incrementGroupPubState (pszGroupName);
    if ((pszIdBuf != NULL) && (ui32IdBufLen > strlen (miMetadata.getMsgId()))) {
        strcpy (pszIdBuf, miMetadata.getMsgId());
    }

    _pStats->messagePushedByClient (ui16ClientId, pszGroupName, ui16Tag, ui32MetadataLength); 

    _m.unlock (51);
    return 0;
}

int DisseminationService::cancel (uint16 ui16ClientId, const char *pszId)
{
    int rc;
    if (_pDCExpCtlr) {
        rc = _pDCExpCtlr->deleteMessage (pszId);
    }
    else {
        rc = _pDefaultDCExpCtlr->deleteMessage (pszId);
    }

    if (rc != 0) {
        checkAndLogMsg ("DisseminationService::cancel", Logger::L_MildError,
                        "Failed to delete message with id = <%s>\n", pszId);
    }
    else {
        checkAndLogMsg ("DisseminationService::cancel", Logger::L_Info,
                        "deleted message with id = <%s>\n", pszId);
    }

    return rc;
}

int DisseminationService::cancel (uint16 ui16ClientId, uint16 ui16Tag)
{
    // TODO: implement this

    return 0;
}

int DisseminationService::cancelAll (uint16 ui16ClientId)
{
    // TODO: implement this

    return 0;
}

int DisseminationService::subscribe (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8Priority,
                                     bool bGroupReliable, bool bMsgReliable, bool bSequenced)
{
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DisseminationService::subscribe (1)", Logger::L_Warning,
                        "pszGroupName is NULL\n");
        return -1;
    }

    Subscription *pSub = SubscriptionFactory::getSubsctiption (ui8Priority, bGroupReliable, bMsgReliable, bSequenced);
    if (pSub == NULL) {
        checkAndLogMsg ("DisseminationService::subscribe (1)", memoryExhausted);
    }
    int rc = _pLocalNodeInfo->subscribe (ui16ClientId, pszGroupName, pSub);
    if (rc < 0) {
        // Do not delete pSub - subscribe() will take care of it
        checkAndLogMsg ("DisseminationService::subscribe (1)", Logger::L_Warning,
                        "group %s was not subscribed; rc = %d\n", pszGroupName, rc);
    }
    else {
        setInitialSubscriptionStatus (pszGroupName, 0);
        checkAndLogMsg ("DisseminationService::subscribe (1)", Logger::L_Info,
                        "client %d subscribed to group %s with priority = %d, group reliability = %s, message reliability = %s, and sequencing = %s\n",
                        (int) ui16ClientId, pszGroupName, (int) ui8Priority,
                        bGroupReliable ? "true" : "false", bMsgReliable ? "true" : "false", bSequenced ? "true" : "false");
    }

    return rc;
}

int DisseminationService::addFilter (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag)
{
    return _pLocalNodeInfo->addFilter (ui16ClientId, pszGroupName, ui16Tag);
}

int DisseminationService::removeFilter (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag)
{
    return _pLocalNodeInfo->removeFilter (ui16ClientId, pszGroupName, ui16Tag);
}

int DisseminationService::removeAllFilters (uint16 ui16ClientId, const char *pszGroupName)
{
    return _pLocalNodeInfo->removeAllFilters (ui16ClientId, pszGroupName);
}

int DisseminationService::unsubscribe (uint16 ui16ClientId, const char *pszGroupName)
{
    return _pLocalNodeInfo->unsubscribe (ui16ClientId, pszGroupName);
}

int DisseminationService::subscribe (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint8 ui8Priority,
                                     bool bGroupReliable, bool bMsgReliable, bool bSequenced)
{
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DisseminationService::subscribe (2)", Logger::L_Warning,
                        "pszGroupName is NULL\n");
        return -1;
    }

    Subscription *pSub = SubscriptionFactory::getSubsctiption (ui16Tag, ui8Priority, bGroupReliable, bMsgReliable, bSequenced);
    if (pSub == NULL) {
        checkAndLogMsg ("DisseminationService::subscribe (2)", memoryExhausted);
    }

    int rc = _pLocalNodeInfo->subscribe (ui16ClientId, pszGroupName, pSub);
    if (rc < 0) {
        // Do not delete pSub - subscribe() will take care of it
        checkAndLogMsg ("DisseminationService::subscribe (2)", Logger::L_Warning,
                        "group %s was not subscribed; rc = %d\n", pszGroupName, rc);
        return -1;
    }

    setInitialSubscriptionStatus (pszGroupName, ui16Tag);
    checkAndLogMsg ("DisseminationService::subscribe (2)", Logger::L_Info,
                    "client %d subscribed to group %s with tag %d, priority = %d, group reliability = %s, message reliability = %s, and sequencing = %s\n",
                    (int) ui16ClientId, pszGroupName, (int) ui16Tag, (int) ui8Priority,
                    bGroupReliable ? "true":"false", bMsgReliable ? "true" : "false", bSequenced ? "true" : "false");

    return rc;
}

int DisseminationService::subscribe (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *pszPredicate,
                                     uint8 ui8Priority, bool bGroupReliable, bool bMsgReliable, bool bSequenced)
{
    if ((pszGroupName == NULL) || (pszPredicate == NULL)) {
        checkAndLogMsg ("DisseminationService::subscribe (3)", Logger::L_Warning,
                        "pszGroupName and/or pszPredicate is/are NULL\n");
        return -1;
    }
    if (bSequenced) {
        checkAndLogMsg ("DisseminationService::subscribe (3)", Logger::L_SevereError,
                        "sequenced XPath Predicate subscription not supported yet\n");
        // TODO: must modify the subscription state updating to increase expected
        // message id etc. even if getSubscribingClients returns no clients in
        // messageReassembled
        // not clients interested in that message, but interested in the group
        return -2;
    }

    Subscription *pSub = SubscriptionFactory::getSubsctiption (pszPredicate, ui8PredicateType, ui8Priority,
                                                               bGroupReliable, bMsgReliable, bSequenced);
    if (pSub == NULL) {
        checkAndLogMsg ("DisseminationService::subscribe (3)", memoryExhausted);
    }

    int rc = _pLocalNodeInfo->subscribe (ui16ClientId, pszGroupName, pSub);
    if (rc < 0) {
        // Do not delete pSub - subscribe() will take care of it
        checkAndLogMsg ("DisseminationService::subscribe (3)", Logger::L_Warning,
                        "group %s was not subscribed; rc = %d\n", pszGroupName, rc);
        return -1;
    }

    checkAndLogMsg ("DisseminationService::subscribe (3)", Logger::L_Info,
                    "client %d subscribed to group %s with priority = %d, group reliability = %s, message reliability = %s, and sequencing = %s\n",
                    (int) ui16ClientId, pszGroupName, (int) ui8Priority,
                    bGroupReliable ? "true":"false", bMsgReliable ? "true" : "false", bSequenced ? "true" : "false");
    return rc;
}

int DisseminationService::unsubscribe (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag)
{
    int iReturnValue = _pLocalNodeInfo->unsubscribe (ui16ClientId, pszGroupName, ui16Tag);
    checkAndLogMsg ("DisseminationService::unsubscribe", Logger::L_Info,
                    "client %d unsubscribed to group %s with tag = %d\n",
                    (int) ui16ClientId, pszGroupName, (int) ui16Tag);
    return iReturnValue;
}

void DisseminationService::setInitialSubscriptionStatus (const char *pszGroupName, uint16 ui16Tag)
{
    // Check in cache if I have already received some message with this subscription
    DArray2<String> *pQueryResult = _pDataCacheInterface->getSenderNodeIds (pszGroupName);
    if (pQueryResult != NULL) {
        const char *pszSenderNodeId;
        uint32 ui32NextExpectedSeqId;
        for (uint8 i = 0; i <= pQueryResult->getHighestIndex(); i++) {
            pszSenderNodeId = (*pQueryResult)[i].c_str();
            ui32NextExpectedSeqId = _pDataCacheInterface->getNextExpectedSeqId (pszGroupName, pszSenderNodeId, ui16Tag);
            if (ui32NextExpectedSeqId != 0) {
                _pSubscriptionState->setSubscriptionState (pszGroupName, pszSenderNodeId, ui32NextExpectedSeqId);
            }
        }
        delete pQueryResult;
        pQueryResult = NULL;
    }
}

int DisseminationService::registerDisseminationServiceListener (uint16 ui16ClientId, DisseminationServiceListener *pListener)
{
    _mToListeners.lock (52);
    int rc = registerDisseminationServiceListenerInternal (ui16ClientId, pListener, ClientInfo::DIRECTLY_CONNECTED);
    _mToListeners.unlock (52);
    return rc;
}

int DisseminationService::registerDisseminationServiceProxyAdaptor (uint16 ui16ClientId, DisseminationServiceListener *pListener)
{
    _mToListeners.lock (53);
    int rc = registerDisseminationServiceListenerInternal (ui16ClientId, pListener, ClientInfo::PROXY_CONNECTED);
    _mToListeners.unlock (53);
    return rc;
}

int DisseminationService::registerDisseminationServiceListenerInternal (uint16 ui16ClientId, DisseminationServiceListener *pListener, ClientInfo::Type type)
{
    if (_clients.used (ui16ClientId)) {
        checkAndLogMsg ("DisseminationService::subscribe", Logger::L_SevereError,
                        "Client ID %d in use.  Register client using a different"
                        " ui16ClientId.\n", ui16ClientId);
        return -1;
    }
    _clients[ui16ClientId].pListener = pListener;
    _clients[ui16ClientId].type = type;

    return 0;
}

int DisseminationService::deregisterDisseminationServiceListener (uint16 ui16ClientId, DisseminationServiceListener *pListener)
{
    _mToListeners.lock (54);
    _clients.clear (ui16ClientId);
    _pLocalNodeInfo->removeClient (ui16ClientId);
    _mToListeners.unlock (54);
    return 0;
}

int DisseminationService::registerPeerStatusListener (uint16 ui16ClientId, PeerStatusListener *pListener)
{
    _mToPeerListeners.lock (55);

    if (_peerStatusClients.used (ui16ClientId)) {
        checkAndLogMsg ("DisseminationService::registerPeerStatusListener", Logger::L_SevereError,
                        "Client ID %d in use.  Register client using a different"
                        " ui16ClientId.\n", ui16ClientId);
        _mToPeerListeners.unlock (55);
        return -1;
    }
    unsigned int uiIndex;
    int rc = registerPeerStateListener (pListener, uiIndex);
    if (rc != 0) {
        _mToPeerListeners.unlock (55);
        return rc;
    }
    _peerStatusClients[ui16ClientId].pListener = pListener;
    _peerStatusClients[ui16ClientId].uiIndex = uiIndex;
    _mToPeerListeners.unlock (55);
    return 0;
}

int DisseminationService::deregisterPeerStatusListener (uint16 ui16ClientId, PeerStatusListener *pListener)
{
    _mToPeerListeners.lock (56);
    if (!_peerStatusClients.used (ui16ClientId)) {
        _mToPeerListeners.unlock (56);
        return -1;
    }
    int rc = deregisterPeerStateListener (_peerStatusClients[ui16ClientId].uiIndex);
    if (rc != 0) {
        _mToPeerListeners.unlock (56);
        return rc;
    }
    _peerStatusClients.clear (ui16ClientId);
    _mToPeerListeners.unlock (56);
    return 0;
}

//------------------------------------------------------------------------------
// Registering/De-registering Data Cache Listeners
//------------------------------------------------------------------------------

int DisseminationService::deregisterAllDataCacheListeners()
{
    if (_pDataCacheInterface == NULL) {
        return -1;
    }
    return _pDataCacheInterface->deregisterAllDataCacheListeners();
}

int DisseminationService::deregisterDataCacheListener (unsigned int uiIndex)
{
    if (_pDataCacheInterface == NULL) {
        return -1;
    }
    return _pDataCacheInterface->deregisterDataCacheListener (uiIndex);
}

int DisseminationService::registerDataCacheListener (DataCacheListener *pListener, unsigned int &uiIndex)
{
    if (_pDataCacheInterface == NULL) {
        return -1;
    }
    int rc = _pDataCacheInterface->registerDataCacheListener (pListener);
    if (rc < 0) {
        return rc;
    }
    uiIndex = (unsigned int) rc;
    return 0;
}

//------------------------------------------------------------------------------
// Registering/De-registering Message Listeners
//------------------------------------------------------------------------------

int DisseminationService::deregisterAllMessageListeners()
{
    if (_pTrSvcListener == NULL) {
        return -1;
    }
    return _pTrSvcListener->deregisterAllMessageListeners();
}

int DisseminationService::deregisterMessageListener (unsigned int uiIndex)
{
    if (_pTrSvcListener == NULL) {
        return -1;
    }
    return _pTrSvcListener->deregisterMessageListener (uiIndex);
}

int DisseminationService::registerMessageListener (MessageListener *pListener, unsigned int &uiIndex)
{
    if (_pTrSvcListener == NULL) {
        return -1;
    }
    int rc = _pTrSvcListener->registerMessageListener (pListener);
    if (rc < 0) {
        return rc;
    }
    uiIndex = (unsigned int) rc;
    return 0;
}

//------------------------------------------------------------------------------
// Registering/De-registering Group Membership Listeners
//------------------------------------------------------------------------------

int DisseminationService::deregisterAllGroupMembershipListeners()
{
    if (_pLocalNodeInfo == NULL) {
        return -1;
    }
    return _pLocalNodeInfo->deregisterAllGroupMembershipListeners();
}

int DisseminationService::deregisterGroupMembershipListener (unsigned int uiIndex)
{
    if (_pLocalNodeInfo == NULL) {
        return -1;
    }
    return _pLocalNodeInfo->deregisterGroupMembershipListener (uiIndex);
}

int DisseminationService::registerGroupMembershiListener (GroupMembershipListener *pListener, unsigned int &uiIndex)
{
    if (_pLocalNodeInfo == NULL) {
        return -1;
    }
    int rc = _pLocalNodeInfo->registerGroupMembershipListener (pListener);
    if (rc < 0) {
        return rc;
    }
    uiIndex = rc;
    return 0;
}

//------------------------------------------------------------------------------
// Registering/De-registering Network State Listeners
//------------------------------------------------------------------------------

int DisseminationService::deregisterAllNetworkStateListeners()
{
    if (_pNetworkStateNotifier == NULL) {
        return -1;
    }
    return _pNetworkStateNotifier->deregisterAllListeners();
}

int DisseminationService::deregisterNetworkStateListener (unsigned int uiIndex)
{
    if (_pNetworkStateNotifier == NULL) {
        return -1;
    }
    return _pNetworkStateNotifier->deregisterListener (uiIndex);
}

int DisseminationService::registerNetworkStateListener (NetworkStateListener *pListener, unsigned int &uiIndex)
{
    if (_pNetworkStateNotifier == NULL) {
        return -1;
    }
    int rc = _pNetworkStateNotifier->registerListener (pListener);
    if (rc < 0) {
        return rc;
    }
    uiIndex = (unsigned int) rc;
    return 0;
}

//------------------------------------------------------------------------------
// Registering/De-registering Peer State Listeners
//------------------------------------------------------------------------------

int DisseminationService::deregisterAllPeerStateListeners()
{
    if (_pPeerState == NULL) {
        return -1;
    }
    return _pPeerState->deregisterAllPeerStateListeners();
}

int DisseminationService::deregisterPeerStateListener (unsigned int uiIndex)
{
    if (_pPeerState == NULL) {
        return -1;
    }
    return _pPeerState->deregisterPeerStateListener (uiIndex);
}

int DisseminationService::registerPeerStateListener (PeerStateListener *pListener, unsigned int &uiIndex)
{
    if (_pPeerState == NULL) {
        return -1;
    }
    int rc = _pPeerState->registerPeerStateListener (pListener);
    if (rc < 0) {
        return rc;
    }
    uiIndex = (unsigned int) rc;
    return 0;
}

//------------------------------------------------------------------------------
// Registering/De-registering Search Listeners
//------------------------------------------------------------------------------

int DisseminationService::deregisterSearchListeners (void)
{
    if (_pSearchNotifier == NULL) {
        return -1;
    }
    return _pSearchNotifier->deregisterAllListeners();
}

int DisseminationService::deregisterSearchListener (unsigned int uiIndex)
{
    if (_pSearchNotifier == NULL) {
        return -1;
    }
    return _pSearchNotifier->deregisterListener (uiIndex);
}

int DisseminationService::registerSearchListener (SearchListener *pListener, unsigned int &uiIndex)
{
    if (_pSearchNotifier == NULL) {
        return -1;
    }
    int rc = _pSearchNotifier->registerListener (pListener);
    if (rc < 0) {
        return rc;
    }
    uiIndex = (unsigned int) rc;
    return 0;
}

int DisseminationService::retrieve (const char *pszId, void **ppBuf, uint32 *pui32BufSize, int64 i64Timeout)
{
    if (pszId == NULL || ppBuf == NULL || pui32BufSize == NULL) {
        return -1;
    }
    *ppBuf = NULL;
    *pui32BufSize = 0;

    char *pszIdToRetrieve;    
    if (!isOnDemandDataID (pszId)) {
        pszIdToRetrieve = addOnDemandSuffixToId (pszId);
    }
    else {
        pszIdToRetrieve = strDup (pszId);
    }

    _m.lock (57);
    // Search locally first,        
    uint32 ui32MsgLength = 0;     
    Reader *pReader = getData (pszIdToRetrieve, ui32MsgLength);
    if (pReader != NULL) {
        (*ppBuf) = malloc (ui32MsgLength);
        if ((*ppBuf) != NULL) {
            pReader->readBytes (*ppBuf, ui32MsgLength);
            *pui32BufSize = ui32MsgLength;
            delete pReader;
            pReader = NULL;
            free (pszIdToRetrieve);
            _m.unlock (57);
            return ui32MsgLength;
        }
        else {
            checkAndLogMsg ("DisseminationService::retrieve", memoryExhausted);
            (*ppBuf) = NULL;
            *pui32BufSize = 0;
            free (pszIdToRetrieve);
             _m.unlock (57);
            return -2;
        }
    }
    else {
        // If not found, search on remote nodes
        _pDiscoveryCtrl->retrieve (pszIdToRetrieve, i64Timeout);
        checkAndLogMsg ("DisseminationService::retrieve", Logger::L_Info,
                        "object %s not found on local cache. Started discovery.\n", pszIdToRetrieve);
    }
    free (pszIdToRetrieve);
    _m.unlock (57);
    return 0;
}

int DisseminationService::retrieve (const char *pszId, const char *pszFilePath, int64 i64Timeout)
{
    _m.lock (58);
    FileWriter fw (pszFilePath, "a");
    int rc = 0;

    _m.unlock (58);
    return rc;
}

int DisseminationService::historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag,
                                          uint16 ui16HistoryLength, int64 i64RequestTimeout)
{
    // Retrieve the publishers of the last ui16HistoryLength messages belonging the group
    // For each of them request the last ui16HistoryLength / (#publishers) messages
    History *pHistory = HistoryFactory::getHistory (ui16HistoryLength, i64RequestTimeout);
    if (pHistory == NULL) {
        return -1;
    }
    int rc = _pLocalNodeInfo->addHistory (ui16ClientId, pszGroupName, ui16Tag, pHistory);
    if (rc < 0) {
        return -2;
    }

    // A history request on a group implicitly implies a history request on the on-demand group if it exists
    char *pszOnDemandGroupName = getOnDemandDataGroupName (pszGroupName);
    pHistory = HistoryFactory::getHistory (ui16HistoryLength, i64RequestTimeout);
    if (pHistory == NULL) {
        return -3;
    }
    rc = _pLocalNodeInfo->addHistory (ui16ClientId, pszOnDemandGroupName, ui16Tag, pHistory);
    if (rc < 0) {
        return -4;
    }
    const char * apszGroupNames[2];
    apszGroupNames[0] = pszGroupName;
    apszGroupNames[1] = pszOnDemandGroupName;

    const char *pszSenderNodeId = NULL;

    // Get all the senders that published message into group pszGroupName
    DArray2<String> *pDAGrpSenders = _pDataCacheInterface->getSenderNodeIds (pszGroupName);
    if (pDAGrpSenders == NULL) {
        // Nothing in the DB for the base group name - try the on-demand group name just in case
        // This is a fix to handle cases where data is made available with empty metadata, which
        // after replication, results in entries for the on-demand group but nothing for the
        // base group
        pDAGrpSenders = _pDataCacheInterface->getSenderNodeIds (pszOnDemandGroupName);
    }
    DisServiceHistoryRequest hmsg (getNodeId(), NULL,
                                   new DisServiceHistoryRequest::HWinReq (pszGroupName, ui16Tag, i64RequestTimeout,
                                                                          ui16HistoryLength));

    // For each sender, get the next expected sequence id (if any)
    DisServiceHistoryRequest::SenderState **ppSendersState = NULL;

    if (pDAGrpSenders != NULL) {
        ppSendersState = new DisServiceHistoryRequest::SenderState*[pDAGrpSenders->getSize()];
        for (uint8 i = 0; i < pDAGrpSenders->size(); i++) {
            uint32 ui32NextExpectedSeqNo;
            pszSenderNodeId = (*pDAGrpSenders)[i].c_str();
            if (0 == stricmp (getNodeId(), pszSenderNodeId)) {
                ui32NextExpectedSeqNo = _pLocalNodeInfo->getGroupPubState (pszGroupName);
            }
            else {
                ui32NextExpectedSeqNo = _pSubscriptionState->getSubscriptionState (pszGroupName, pszSenderNodeId);
                if (ui32NextExpectedSeqNo == 0) {
                    // Try the on-demand group name also
                    ui32NextExpectedSeqNo = _pSubscriptionState->getSubscriptionState (pszOnDemandGroupName, pszSenderNodeId);
                }
            }
            ppSendersState[i] = new DisServiceHistoryRequest::SenderState (pszSenderNodeId, ui32NextExpectedSeqNo);

            uint16 ui16From = 0;
            if (ui32NextExpectedSeqNo > ui16HistoryLength) {
                ui16From = ui32NextExpectedSeqNo - ui16HistoryLength;
            }
            for (uint32 ui32SeqNo = ui16From; ui32SeqNo < ui32NextExpectedSeqNo; ui32SeqNo++) {
                for (uint32 ui32GroupNameIdx = 0; ui32GroupNameIdx < 2; ui32GroupNameIdx++) {
                    PtrLList<Message> *pMessages = _pDataCacheInterface->getCompleteMessages (apszGroupNames[ui32GroupNameIdx], pszSenderNodeId, ui32SeqNo);
                    if ((pMessages == NULL) || (pMessages->getFirst() == NULL)) {
                        checkAndLogMsg ("DisseminationService::historyRequest", Logger::L_MildError,
                                        "getCompleteMessages() returned NULL for %s:%s:%lu\n",
                                        apszGroupNames[ui32GroupNameIdx], pszSenderNodeId, ui32SeqNo);
                    }
                    else if (pMessages->getCount() == 1) {
                        // We have a single message - so it is not a chunked message
                        Message *pMsg = pMessages->getFirst();
                        DArray<uint16> clients;
                        clients[0] = ui16ClientId;
                        notifyDisseminationServiceListeners (&clients, pMsg, pMsg->getMessageInfo()->isMetaData(),
                                                             pMsg->getMessageInfo()->isMetaDataWrappedInData(),
                                                             NULL);
                        _pDataCacheInterface->release (pMessages);
                    }
                    else {
                        // We have a chunked message with multiple chunks
                        // For now, call the listener for each chunk
                        Message *pMsg;
                        pMessages->resetGet();
                        while (NULL != (pMsg = pMessages->getNext())) {
                            DArray<uint16> clients;
                            clients[0] = ui16ClientId;
                            notifyDisseminationServiceListeners (&clients, pMsg, pMsg->getMessageInfo()->isMetaData(),
                                                                 pMsg->getMessageInfo()->isMetaDataWrappedInData(),
                                                                 NULL);
                        }
                        _pDataCacheInterface->release (pMessages);
                    }
                }
            }
        }

        long highestIndex = pDAGrpSenders->getHighestIndex()+1;
        if (highestIndex > 255) {
            checkAndLogMsg ("DisseminationService::historyRequest", Logger::L_SevereError, "sender state index too large");
        }
        else {
            hmsg.setSenderState (ppSendersState, (uint8) highestIndex);
        }
        delete pDAGrpSenders;
        pDAGrpSenders = NULL;
    }

    // Now send the request out on the network
    if (0 != (rc = broadcastDisServiceCntrlMsg (&hmsg, NULL, "Sending DisServiceHistoryRequest"))) {
        checkAndLogMsg ("DisseminationService::historyRequest", Logger::L_Warning,
                        "failed to send history request message over the network; rc = %d\n", rc);
    }

    free (pszOnDemandGroupName);
    pszOnDemandGroupName = NULL;

    return 0;        
}

int DisseminationService::historyRequest (uint16 ui16ClientId, HistoryRequest *pRequest)
{
    HistoryRequestGroupTag *pGTReq = (HistoryRequestGroupTag*) pRequest;   // For now only HistoryRequestGroupTag
    ShiftHistory *pShiftHistory = (ShiftHistory*)(pGTReq->_pHistory);      // For now only shift history

    const char *pszSenderNodeId = NULL;
    char *pszGroupNameCopy = strDup (pGTReq->_pszGroupName);
    uint16 ui16Tag = pGTReq->_ui16Tag;
    uint16 ui16HistoryLength = pShiftHistory->_ui16HistoryLength;

    // Get all the senders that published messages into pszGroupName group
    DArray2<String> *pDAGrpSenders = _pDataCacheInterface->getSenderNodeIds (pszGroupNameCopy);
    DisServiceHistoryRequest hmsg (getNodeId(), NULL,
                                   new DisServiceHistoryRequest::HWinReq (pszGroupNameCopy, ui16Tag, 0, ui16HistoryLength));

    // For eache sender, get the next expected sequence id (if any)
    DisServiceHistoryRequest::SenderState **ppSendersState = NULL;

    if (pDAGrpSenders != NULL) {
        ppSendersState = new DisServiceHistoryRequest::SenderState*[pDAGrpSenders->size()];
        for (uint8 i = 0; i < pDAGrpSenders->size(); i++) {
            uint32 ui32State;
            pszSenderNodeId = (*pDAGrpSenders)[i].c_str();
            ui32State = _pSubscriptionState->getSubscriptionState (pGTReq->_pszGroupName, pszSenderNodeId);
            ppSendersState[i] = new DisServiceHistoryRequest::SenderState (pszSenderNodeId, ui32State);
            uint16 ui16From = ((ui32State - ui16HistoryLength -1 > 0) ? (ui32State - ui16HistoryLength - 1) : 0);
            PtrLList<Message> *pMessages = _pDataCacheInterface->getMessages (pszGroupNameCopy, pszSenderNodeId, ui16Tag, ui16From, ui32State);
            Message *pMsg;
            if (pMessages != NULL && (pMsg = pMessages->getFirst()) != NULL) {
                // Only the client that requested the history request must receive
                // the history messages
                DArray<uint16> clients;
                clients[0] = ui16ClientId;
                do {
                    notifyDisseminationServiceListeners (&clients, pMsg, pMsg->getMessageInfo()->isMetaData(),
                                                         pMsg->getMessageInfo()->isMetaDataWrappedInData(),
                                                         NULL);
                } while ((pMsg = pMessages->getNext()) != NULL);
            }
            _pDataCacheInterface->release (pMessages);
        }
        hmsg.setSenderState (ppSendersState, pDAGrpSenders->size());
        delete pDAGrpSenders;
        pDAGrpSenders = NULL;
    }
    return broadcastDisServiceCntrlMsg (&hmsg, NULL, "Sending DisServiceHistoryRequest");
}

int DisseminationService::historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag,
                                          uint32 ui32StartSeqNo, uint32 ui32EndSeqNo, int64 i64RequestTimeout)
{
    History *pHistory = HistoryFactory::getHistory (ui32StartSeqNo, ui32EndSeqNo, i64RequestTimeout);
    if (pHistory == NULL) {
        return -1;
    }
    int rc;
    if (ui16Tag == 0) {
        rc = _pLocalNodeInfo->addHistory (ui16ClientId, pszGroupName, pHistory);
    }
    else {
        rc = _pLocalNodeInfo->addHistory (ui16ClientId, pszGroupName, ui16Tag, pHistory);
    }
    return rc;
}

int DisseminationService::historyRequest (uint16 ui16ClientId, const char *pszMsgId, int64 i64RequestTimeout)
{
    if (pszMsgId == NULL) {
        return -1;
    }

    int rc = 0;
    DArray2<String> tokens;
    uint8 ui8ChunkSeqId = MessageHeader::UNDEFINED_CHUNK_ID;
    if (isAllChunksMessageID (pszMsgId)) {
        rc = convertKeyToField (pszMsgId, tokens, 3,
                                MSG_ID_GROUP, MSG_ID_SENDER,
                                MSG_ID_SEQ_NUM);
    }
    else {
        rc = convertKeyToField (pszMsgId, tokens, 4,
                                MSG_ID_GROUP, MSG_ID_SENDER,
                                MSG_ID_SEQ_NUM, MSG_ID_CHUNK_ID);
        ui8ChunkSeqId = (uint8) atoui32 (tokens[MSG_ID_CHUNK_ID]);
    }

    if (rc < 0) {
        return -2;
    }

    uint32 ui32MsgSeqId = atoui32 (tokens[MSG_ID_SEQ_NUM]);

    if (ui8ChunkSeqId == MessageHeader::UNDEFINED_CHUNK_ID) {
        return historyRequest (ui16ClientId, tokens[MSG_ID_GROUP], tokens[MSG_ID_SENDER],
                               ui32MsgSeqId, ui32MsgSeqId, i64RequestTimeout);
    }
    else {
        return historyRequest (ui16ClientId, tokens[MSG_ID_GROUP], tokens[MSG_ID_SENDER],
                               ui32MsgSeqId, ui8ChunkSeqId, ui8ChunkSeqId, i64RequestTimeout);
    }
}

int DisseminationService::historyRequest (uint16 ui16ClientId, const char *pszGroupName, const char *pszSenderNodeId,
                                          uint32 ui32StartSeqNo, uint32 ui32EndSeqNo, int64 i64RequestTimeout)
{
    if (pszGroupName == NULL || pszSenderNodeId == NULL) {
        return -1;
    }
    if (i64RequestTimeout < 0) {
        i64RequestTimeout = 0;
    }

    UInt32RangeDLList ranges (true);
    ranges.addTSN (ui32StartSeqNo, ui32EndSeqNo);

    for (uint32 ui32CurrSeqId = ui32StartSeqNo; ui32CurrSeqId <= ui32EndSeqNo; ui32CurrSeqId++) {

        // Lock data cache
        _pDataCacheInterface->lock();
        _mAsynchronousNotify.lock (270);

        PtrLList<Message> *pMessages = _pDataCacheInterface->getCompleteMessages (pszGroupName, pszSenderNodeId, ui32CurrSeqId);
        if (pMessages != NULL) {
            uint32 ui32MsgSeqIdTmp;
            Message *pMsg;
            Message *pTmpMsg = pMessages->getFirst();
            while ((pMsg = pTmpMsg) != NULL) {
                pTmpMsg = pMessages->getNext();
                ui32MsgSeqIdTmp = pMsg->getMessageHeader()->getMsgSeqId();
                MessageToNotifyToClient *pMsgToNotif = new MessageToNotifyToClient;
                if (pMsgToNotif != NULL) {
                    pMsgToNotif->msgId = pMsg->getMessageHeader()->getMsgId();
                    pMsgToNotif->ui16ClientId = ui16ClientId;
                    _pMessagesToNotify->prepend (pMsgToNotif);
                }
                ranges.removeTSN (ui32MsgSeqIdTmp);
            }
            _pDataCacheInterface->release (pMessages);
        }

        // Unlock data cache
        _mAsynchronousNotify.unlock (270);
        _pDataCacheInterface->unlock();
    }

    uint32 ui32;
    int rc = ranges.getFirst (ui32, ui32, true);
    if (rc == 0) {
        RequestInfo reqInfo (ui16ClientId);
        reqInfo._pszGroupName = pszGroupName;
        reqInfo._pszSenderNodeId = pszSenderNodeId;
        reqInfo._ui64ExpirationTime = i64RequestTimeout;
        if (reqInfo._ui64ExpirationTime > 0) {
            reqInfo._ui64ExpirationTime += getTimeInMilliseconds();
        }
        reqInfo._pszQueryId = NULL;

        // Force sending a request message right away, if the message has not
        // already been requested
        const bool RESET_GET = true;
        uint32 ui32BeginEl, ui32EndEl;
        for (rc = ranges.getFirst (ui32BeginEl, ui32EndEl, RESET_GET);
                 rc == 0; rc = ranges.getNext (ui32BeginEl, ui32EndEl)) {
            for (uint32 ui32SeqId = ui32BeginEl; ui32SeqId <= ui32EndEl; ui32SeqId++) {
                PtrLList<Message> *pMsgs = _pDataCacheInterface->getMatchingFragments (reqInfo._pszGroupName, reqInfo._pszSenderNodeId, ui32SeqId);
                if (pMsgs == NULL) {
                    UInt32RangeDLList tmpRanges (true);
                    tmpRanges.addTSN (ui32SeqId, ui32SeqId);
                    _pMessageReassembler->sendRequest (reqInfo, &tmpRanges);
                }
                else {
                    for (Message *pCurr, *pNext = pMsgs->getFirst(); (pCurr = pNext) != NULL; pNext = pMsgs->getNext()) {
                        pNext = pMsgs->getNext();
                        delete pMsgs->remove (pCurr);
                    }
                    free (pMsgs);
                }
            }
        }
        _pMessageReassembler->addRequest (reqInfo, &ranges);
    }

    return 0;
}

int DisseminationService::historyRequest (uint16 ui16ClientId, const char *pszGroupName, const char *pszSenderNodeId,
                                          uint32 ui32MsgSeqId, uint8 ui8ChunkStart, uint8 ui8ChunkEnd,
                                          int64 i64RequestTimeout)
{
    if (pszGroupName == NULL || pszSenderNodeId == NULL) {
        return -1;
    }
    if (i64RequestTimeout < 0) {
        i64RequestTimeout = 0;
    }

    _pDataCacheInterface->lock();

    UInt8RangeDLList ranges (false);
    PtrLList<Message> *pMessages = _pDataCacheInterface->getCompleteMessages (pszGroupName, pszSenderNodeId, ui32MsgSeqId);
    ranges.addTSN (ui8ChunkStart, ui8ChunkEnd);
    if (pMessages != NULL) {
        Message *pMsg;
        Message *pTmpMsg = pMessages->getFirst();
        while ((pMsg = pTmpMsg) != NULL) {
            pTmpMsg = pMessages->getNext();
            MessageHeader *pMH = pMsg->getMessageHeader();
            uint8 ui8ChunkIdTmp = pMH->getChunkId();
            if (ui8ChunkIdTmp >= ui8ChunkStart && ui8ChunkIdTmp <= ui8ChunkEnd) {
                MessageToNotifyToClient *pMsgToNotif = new MessageToNotifyToClient;
                if (pMsgToNotif != NULL) {
                    pMsgToNotif->msgId = pMH->getMsgId();
                    pMsgToNotif->ui16ClientId = ui16ClientId;

                    _mAsynchronousNotify.lock (271);
                    _pMessagesToNotify->prepend (pMsgToNotif);
                    _mAsynchronousNotify.unlock (271);
                }
                ranges.removeTSN (ui8ChunkIdTmp);
            }
        }
        _pDataCacheInterface->release (pMessages);
    }

    _pDataCacheInterface->unlock();

    _pMessageReassembler->lock();
    _pDataCacheInterface->lock();
    
    RequestInfo reqInfo (ui16ClientId);
    reqInfo._pszGroupName = pszGroupName;
    reqInfo._pszSenderNodeId = pszSenderNodeId;
    reqInfo._ui64ExpirationTime = i64RequestTimeout;
    if (reqInfo._ui64ExpirationTime > 0) {
        reqInfo._ui64ExpirationTime += getTimeInMilliseconds();
    }
    reqInfo._pszQueryId = NULL;

    // Force sending a request message right away, if the message has not
    // already been requested
    const bool RESET_GET = true;
    uint8 ui8BeginEl, ui8EndEl;
    for (int rc = ranges.getFirst (ui8BeginEl, ui8EndEl, RESET_GET);
             rc == 0; rc = ranges.getNext (ui8BeginEl, ui8EndEl)) {
        for (uint8 ui8ChunkId = ui8BeginEl; ui8ChunkId <= ui8EndEl; ui8ChunkId++) {
            PtrLList<Message> *pMsgs = _pDataCacheInterface->getMatchingFragments (reqInfo._pszGroupName, reqInfo._pszSenderNodeId, ui32MsgSeqId, ui8ChunkId);
            if (pMsgs == NULL) {
                UInt8RangeDLList tmpRanges (false);
                tmpRanges.addTSN (ui8ChunkId, ui8ChunkId);
                _pMessageReassembler->sendRequest (reqInfo, ui32MsgSeqId, &tmpRanges);
            }
            else {
                for (Message *pCurr, *pNext = pMsgs->getFirst(); (pCurr = pNext) != NULL; pNext = pMsgs->getNext()) {
                    pNext = pMsgs->getNext();
                    delete pMsgs->remove (pCurr);
                }
                free (pMsgs);
            }
        }
    }
    _pMessageReassembler->addRequest (reqInfo, ui32MsgSeqId, &ranges);

    _pDataCacheInterface->unlock();
    _pMessageReassembler->unlock();

    return 0;
}

int DisseminationService::historyRequest (uint16 ui16ClientId, const char *pszGroupName, const char *pszSenderNodeId,
                                          uint16 ui16Tag, uint32 ui32StartSeqNo, uint32 ui32EndSeqNo, int64 i64RequestTimeout)
{
    History *pHistory = HistoryFactory::getHistory (ui32StartSeqNo, ui32EndSeqNo, i64RequestTimeout);
    if (pHistory == NULL) {
        return -1;
    }
    int rc;
    if (ui16Tag == 0) {
        rc = _pLocalNodeInfo->addHistory (ui16ClientId, pszGroupName, pHistory);
    }
    else {
        rc = _pLocalNodeInfo->addHistory (ui16ClientId, pszGroupName, ui16Tag, pHistory);
    }

    if (rc < 0) {
        return rc;
    }

    UInt32RangeDLList ranges (true);
    PtrLList<Message> *pMessages = _pDataCacheInterface->getMessages (pszGroupName, pszSenderNodeId, ui16Tag,
                                                                      ui32StartSeqNo, ui32EndSeqNo);
    if (pMessages != NULL) {
        Message *pMsg;
        Message *pTmpMsg = pMessages->getFirst();
        while ((pMsg = pTmpMsg) != NULL) {
            pTmpMsg = pMessages->getNext();
            notifyDisseminationServiceListener (ui16ClientId, pMsg, pMsg->getMessageInfo()->isMetaData(),
                                                pMsg->getMessageInfo()->isMetaDataWrappedInData(),
                                                NULL);
            ranges.addTSN (pMsg->getMessageHeader()->getMsgSeqId());
        }
        _pDataCacheInterface->release (pMessages);
    }

    DisServiceHistoryRequest hmsg (getNodeId(), NULL,
                                   new DisServiceHistoryRequest::HRangeReq (pszGroupName, pszSenderNodeId, ui16Tag, i64RequestTimeout,
                                                                            ui32StartSeqNo, ui32EndSeqNo, &ranges));

    return 0;
}

int DisseminationService::historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, int64 i64PublishTimeStart,
                                          int64 i64PublishTimeEnd, int64 i64RequestTimeout)
{
    History *pHistory = HistoryFactory::getHistory (i64PublishTimeStart, i64PublishTimeEnd, i64RequestTimeout);
    if (pHistory == NULL) {
        return -1;
    }
    int rc;
    if (ui16Tag == 0) {
        rc = _pLocalNodeInfo->addHistory (ui16ClientId, pszGroupName, pHistory);
    }
    else {
        rc = _pLocalNodeInfo->addHistory (ui16ClientId, pszGroupName, ui16Tag, pHistory);
    }
    return rc;
}

int DisseminationService::historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *pszPredicate,
                                          uint16 ui16HistoryLength, int64 i64RequestTimeout)
{
    History *pHistory = HistoryFactory::getHistory (ui16HistoryLength, i64RequestTimeout);
    if (pHistory == NULL) {
        return -1;
    }
    return _pLocalNodeInfo->addHistory (ui16ClientId, pszGroupName, ui8PredicateType, pszPredicate, pHistory);
}

int DisseminationService::historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *pszPredicate,
                                          int32 ui32StartSeqNo, uint32 ui32EndSeqNo, int64 i64RequestTimeout)
{
    History *pHistory = HistoryFactory::getHistory (ui32StartSeqNo, ui32EndSeqNo, i64RequestTimeout);
    if (pHistory == NULL) {
        return -1;
    }
    return _pLocalNodeInfo->addHistory (ui16ClientId, pszGroupName, ui8PredicateType, pszPredicate, pHistory);
}

int DisseminationService::historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *pszPredicate,
                                          int64 i64PublishTimeStart, int64 i64PublishTimeEnd, int64 i64RequestTimeout)
{
    History *pHistory = HistoryFactory::getHistory (i64PublishTimeStart, i64PublishTimeEnd, i64RequestTimeout);
    if (pHistory == NULL) {
        return -1;
    }
    return _pLocalNodeInfo->addHistory (ui16ClientId, pszGroupName, ui8PredicateType, pszPredicate, pHistory);
}

int DisseminationService::requestMoreChunks (uint16 ui16ClientId, const char *pszMsgId, int64 i64Timeout)
{
    if (pszMsgId == NULL) {
        return -1;
    }
    DArray2<NOMADSUtil::String> tokenizedKey;
    if (convertKeyToField (pszMsgId, tokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
        return -2;
    }
    return requestMoreChunks (ui16ClientId, (const char *) tokenizedKey[MSG_ID_GROUP],
                              (const char *) tokenizedKey[MSG_ID_SENDER],
                              (uint32) atoi (tokenizedKey[MSG_ID_SEQ_NUM]), i64Timeout);
}

int DisseminationService::requestMoreChunks (uint16, const char *pszGroupName,
                                             const char *pszSenderNodeId, uint32 ui32MsgSeqId,
                                             int64 i64Timeout)
{
    if (pszGroupName == NULL || pszSenderNodeId == NULL) {
        return -1;
    }
    char *pszOnDemandGrpName = isOnDemandGroupName (pszGroupName) ? strDup (pszGroupName) : getOnDemandDataGroupName (pszGroupName);

    unsigned int uiTotNChunks = 0;
    int iNChunks = _pDataCacheInterface->countChunks (pszOnDemandGrpName, pszSenderNodeId,
                                                      ui32MsgSeqId, uiTotNChunks);
    if ((iNChunks < 0) ||                // Error
        ((uiTotNChunks > 0) && (((unsigned int) iNChunks) == uiTotNChunks))) {  // All the chunks have already been received
        free (pszOnDemandGrpName);                                              // (if uiTotNChunks it means that no chunk has
        return 1;                                                               // been received)
    }

    // Request chunks
    _pDiscoveryCtrl->retrieve (pszOnDemandGrpName, pszSenderNodeId, ui32MsgSeqId, i64Timeout);
    free (pszOnDemandGrpName);
    return 0;
}

int DisseminationService::release (const char *pszMsgId, void *pData)
{
    return _pDataCacheInterface->release (pszMsgId, pData);
}

int DisseminationService::release (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, void *pData)
{
    return release (pszGroupName, pszSenderNodeId, ui32MsgSeqId, 0, pData);
}

int DisseminationService::release (const char *pszGroupName, const char *pszSenderNodeId,
                                   uint32 ui32MsgSeqId, uint32 ui32ChunkSeqId, void *pData)
{
    // TODO: implement this
    return 0;
}
            
void DisseminationService::run (void)
{
    setName ("DisseminationService::run");

    started();
    uint16 ui16Cycle = 0;
    int64 i64KeepAliveTime = getTimeInMilliseconds();
    char **ppszOldQuiscentInterfaces = NULL;
    char **ppszNewQuiscentInterfaces;
    while (!terminationRequested()) {
        // if bandwidth sharing is active, update the bandwidth very often
        // instead of just sleeping for seconds before the next keep alive
        if (bBandwidthSharingEnabled || bNetworkStateNotifierEnabled) {
            while (i64KeepAliveTime + _ui16KeepAliveInterval > getTimeInMilliseconds()) {
                if (bBandwidthSharingEnabled) {
                    _pBandwidthSharing->adjustBandwidthShare (_ui8NodeImportance);
                }
                if (bNetworkStateNotifierEnabled) {
                    ppszNewQuiscentInterfaces = _pTrSvc->getInterfacesByReceiveRateAndOutgoingQueueLenght (0.50f, 0.50f);
                    static bool bFirstTime = true;
                    if (!Utils::sameElements (ppszOldQuiscentInterfaces, ppszNewQuiscentInterfaces) || bFirstTime) {
                        _pNetworkStateNotifier->networkQuiescent ((const char **)ppszNewQuiscentInterfaces);
                        deallocateNullTerminatedPtrArray (ppszOldQuiscentInterfaces);
                        ppszOldQuiscentInterfaces = ppszNewQuiscentInterfaces;
                        if (bFirstTime) {
                            bFirstTime = false;
                        }
                    }
                }
                sleepForMilliseconds (500);
            }
            _pBandwidthSharing->adjustBandwidthShare (_ui8NodeImportance);
            i64KeepAliveTime = getTimeInMilliseconds();
        }
        else {
            sleepForMilliseconds (_ui16KeepAliveInterval);
        }

        // Data cache cleaning cycle
        if (((ui16Cycle++) % _ui16CacheCleanCycle == 0) &&
             (_pDataCacheInterface != NULL)) {
            _pDataCacheInterface->cacheCleanCycle();
        }

        // History message requesting cycle
        if ((ui16Cycle) % _ui16HistoryReqCycle == 0) {
            // For each client
            PtrLList<HistoryRequest> *pHistoryRequests;
            for (int i = 0; i <= _clients.getHighestIndex(); i++) {
                if (_clients.used(i)) {
                    pHistoryRequests = _pLocalNodeInfo->getHistoryRequests (i);
                    if (pHistoryRequests) {
                        HistoryRequestGroupTag *pRequest = (HistoryRequestGroupTag*) pHistoryRequests->getFirst();
                        HistoryRequestGroupTag *pRequestTmp;
                        while (pRequest) {
                            //request(i, pRequest);
                            pRequestTmp = (HistoryRequestGroupTag*) pHistoryRequests->getNext();
                            delete pRequest;
                            pRequest = pRequestTmp;
                        }
                    }
                }
            }
        }

        // Either send Subscription State OR the keep alive message
        if ((_pPeerState->getType() == PeerState::TOPOLOGY_STATE) &&
            _bSubscriptionStateExchangeEnabled &&
            ((ui16Cycle) % _ui16SubscriptionStatePeriod == 0)) {
            ((WorldState *) _pPeerState)->sendSubscriptionState();
        }
        else if (_bKeepAliveMsgEnabled) {
            DataCacheReplicationController *pRepCtrl = (_pDCRepCtlr ? _pDCRepCtlr : _pDefaultDCRepCtlr);
            ForwardingController *pFwdCtrl = (_pFwdCtlr ? _pFwdCtlr : _pDefaultFwdCtlr);
             // Send keep alive message
            char **ppszSilentInterfaces = _pTrSvc->getSilentInterfaces (_ui16KeepAliveInterval);
            if (ppszSilentInterfaces != NULL && ppszSilentInterfaces[0] != NULL) {
                DisServiceCtrlMsg *pWSSIMsg = _pPeerState->getKeepAliveMsg (pRepCtrl->getType(), pFwdCtrl->getType(), _ui8NodeImportance);
                if (pWSSIMsg != NULL) {
                    const String sessionId (getSessionId());
                    if (sessionId.length() > 0) {
                        pWSSIMsg->setSessionId (getSessionId());
                    }
                    _pTrSvc->broadcast (pWSSIMsg, (const char **) ppszSilentInterfaces, "Sending Keep Alive Msg", NULL, NULL);
                    delete pWSSIMsg;
                    pWSSIMsg = NULL;
                }
                for (unsigned int i = 0; ppszSilentInterfaces[i] != NULL; i++) {
                    free (ppszSilentInterfaces[i]);
                }
                free (ppszSilentInterfaces);
            }
            //_pStats->keepAliveMessageSent();
        }

        _pPeerState->updateNeighbors();
        sendStatus();

        _pDiscoveryCtrl->sendDiscoveryMessage();
        // Advertise Subscriptions if the Subscription Advertise Mechanism has been activated.
        if (_pSubFwdCtlr != NULL) {
            _pSubFwdCtlr->advertiseSubscriptions();
        }

        // Notify clients
        PtrLList<MessageToNotifyToClient> *pMessagesToNotify = NULL;

        _mAsynchronousNotify.lock (269);
        if (_pMessagesToNotify->getFirst() != NULL) {
            pMessagesToNotify = _pMessagesToNotify;
            _pMessagesToNotify = new PtrLList<MessageToNotifyToClient>();
        }
        _mAsynchronousNotify.unlock (269);

        if (pMessagesToNotify != NULL) {
            MessageToNotifyToClient *pCurr, *pNext;
            pNext = pMessagesToNotify->getFirst();
            while ((pCurr = pNext) != NULL) {
                pNext = pMessagesToNotify->getNext();
                MessageHeader *pMH = _pDataCacheInterface->getMessageInfo (pCurr->msgId.c_str());
                const void *pData = _pDataCacheInterface->getData (pCurr->msgId.c_str());
                if (pMH != NULL && pData != NULL) {
                    bool bIsMetadata;
                    bool bIsMetadataWrappedInData;
                    if (pMH->isChunk()) {
                        bIsMetadata = bIsMetadataWrappedInData = false;
                    }
                    else {
                        MessageInfo *pMI = (MessageInfo*) pMH;
                        bIsMetadata = pMI->isMetaData();
                        bIsMetadataWrappedInData = pMI->isMetaDataWrappedInData();
                    }
                    Message msg (pMH, pData);
                    _mToListeners.lock (267);
                    notifyDisseminationServiceListener (pCurr->ui16ClientId, &msg, bIsMetadata,
                                                        bIsMetadataWrappedInData, pCurr->searchId);
                    _mToListeners.unlock (267);
                }
                _pDataCacheInterface->release (pCurr->msgId.c_str(), pMH);
                _pDataCacheInterface->release (pCurr->msgId.c_str(), (void*)pData);
                pMessagesToNotify->remove (pCurr);
                delete pCurr;
            }
            delete pMessagesToNotify;
        }

        // Subscriptions and probabilities exchange
        if (_pPeerState->getType() == PeerState::IMPROVED_TOPOLOGY_STATE) {
            ((TopologyWorldState *) _pPeerState)->printWorldStateInfo();
            if (_bSubscriptionsExchangeEnabled && ((ui16Cycle) % _ui16SubscriptionsExchangePeriod == 0)) {
                ((TopologyWorldState *) _pPeerState)->sendSubscriptionState(); //send subscriptions updates or requests
            }
            if (_bTopologyExchangeEnabled && ((ui16Cycle) % _ui16TopologyExchangePeriod == 0)) {
                ((TopologyWorldState *) _pPeerState)->sendProbabilities(); //send probabilities updates
            }
        }
    }

    terminating();
}

/*
uint16 DisseminationService::getActualMTU (const char *pszGroupName, const char *pszTargetNode)
{
    BufferWriter bw (getMaxFragmentSize(), 1024);
    MessageInfo mi (pszGroupName,
                    (const char *) getNodeId(),
                     0,                          // ui32SeqId
                     0,                          // ui16Tag
                     0,                          // ui16ClientId
                     0,                          // ui8ClientType
                     0,                          // ui32TotalMessageLength
                     0,                          // ui32FragmentLength
                     0                           // ui32FragmentOffest
    );
    Message m (&mi, NULL);

    DisServiceDataMsg dsMsg (getNodeId(), &m, pszTargetNode);
    dsMsg.write (&bw, 0);
    uint32 ui32 = bw.getBufferLength();
    if (ui32 > 0x0000FFFF) {
        return 0;
    }
    uint16 ui16ActualMTU = getMaxFragmentSize() - ((uint16) bw.getBufferLength());
    return ui16ActualMTU;
}
*/

char ** DisseminationService::getPeerList()
{
    return _pPeerState->getAllNeighborNodeIds();
}

bool DisseminationService::isActiveNeighbor (const char *pszNodeId) 
{
    return _pPeerState->isActiveNeighbor (pszNodeId);
}

int DisseminationService::resetTransmissionHistory()
{
    return _pTransmissionHistoryInterface->reset();
}

int DisseminationService::reloadTransmissionService (void)
{
    // Get the Network Interfaces that should be used to send and/or receive
    ConfigFileReader cfgReader (_pCfgMgr);
    char **ppszOutgoingInterfaces = cfgReader.getNetIFs();
    char **ppszIgnoredInterfaces = cfgReader.getIgnoredNetIFs();
    checkAndLogMsg ("DisseminationService::reloadTransmissionService", Logger::L_Info,
                    "suspending DisseminationService::run\n");
    int rc = 0;
    _m.lock (65);
    if (_pTrSvc != NULL) {
        requestTerminationAndWait();
        checkAndLogMsg ("DisseminationService::reloadTransmissionService", Logger::L_Info,
                        "reloading transmission service\n");
        rc = _pTrSvc->init (_pCfgMgr, (const char **)ppszOutgoingInterfaces, (const char **)ppszIgnoredInterfaces);
        checkAndLogMsg ("DisseminationService::reloadTransmissionService", Logger::L_Info,
                        "restarting DisseminationService::run\n");
        start();
    }
    _m.unlock (65);
    return rc;
}

int DisseminationService::search (uint16 ui16ClientId, const char *pszGroupName, const char *pszQueryType,
                                  const char *pszQueryQualifiers, const void *pQuery,
                                  unsigned int uiQueryLen, char **ppszQueryId)
{
    if (ppszQueryId == NULL) {
        return -1;
    }

    *ppszQueryId = SearchService::getSearchId (pszGroupName, getNodeId(),
                                               _pDataCacheInterface->getStorageInterface()->getPropertyStore());
    if (*ppszQueryId == NULL) {
        return -2;
    }

    //_pSearchNotifier->searchArrived (*ppszQueryId, pszGroupName, pszQueryType, pszQueryQualifiers,
    //                                 pQuery, uiQueryLen);

    SearchMsg searchMsg (getNodeId(), NULL);
    searchMsg.setQuery (pQuery, uiQueryLen);
    searchMsg.setQuerier (getNodeId());
    searchMsg.setQueryId (*ppszQueryId);
    searchMsg.setGroupName (pszGroupName);
    searchMsg.setQueryType (pszQueryType);
    searchMsg.setQueryQualifier (pszQueryQualifiers);

    int rc = broadcastDisServiceCntrlMsg (&searchMsg, NULL, "sending search message", NULL, NULL);
    if (rc < 0) {
        return -3;
    }

    Searches::getSearches()->addSearchInfo (*ppszQueryId, pszQueryType, getNodeId(), ui16ClientId);

    return 0;
}

int DisseminationService::searchReply (const char *pszQueryId, const char **ppszMatchingMsgIds)
{
    if (pszQueryId == NULL || ppszMatchingMsgIds == NULL) {
        return -1;
    }
    uint16 ui16ClientId = 0;
    String queryType;
    String querier;
    if (Searches::getSearches()->getSearchInfo (pszQueryId, queryType, querier, ui16ClientId) < 0) {
        return -2;
    }

    if ((querier == getNodeId()) == 1) {
        // The node itself is the querier, it does not need to send the reply
        return -3;
    }

    // _pSearchNotifier->searchReplyArrived (pszQueryId, ppszMatchingMsgIds);

    SearchReplyMsg searchMsg (getNodeId(), querier);
    searchMsg.setQuerier (querier);
    searchMsg.setQueryId (pszQueryId);
    searchMsg.setMatchingMsgIds (ppszMatchingMsgIds);
    searchMsg.setQueryType (queryType);
    searchMsg.setMatchingNode (getNodeId());

    int rc = transmitDisServiceControllerMsg (&searchMsg, true, "sending search reply message");
    if (rc < 0) {
        return -4;
    }

    return 0;
}

uint16 DisseminationService::getMaxFragmentSize (void)
{
    if (_pTrSvc == NULL) {
        return 0;
    }
    return _pTrSvc->getMaxFragmentSize();
}

int DisseminationService::getNextPushId (const char *pszGroupName, char *pszIdBuf, uint32 ui32IdBufLen)
{
    if (pszGroupName == NULL) {
        return -1;
    }
    if (pszIdBuf == NULL) {
        return -2;
    }
    const char *pszNodeId = getNodeId();
    uint32 ui32SeqId = _pLocalNodeInfo->getGroupPubState (pszGroupName);
    if (ui32IdBufLen < (strlen (pszGroupName) + 1 + strlen (pszNodeId) + 1 + 10 + 1)) {
        return -3;
    }
    sprintf (pszIdBuf, "%s:%s:%u", pszGroupName, pszNodeId, ui32SeqId);
    return 0;
}

void DisseminationService::construct (ConfigManager *pCfgMgr)
{
    _m.lock (9);

    const char *pszMethodName = "DisseminationService::construct";
    _pTrSvc = NULL;
    _pTrSvcListener = NULL;
    _pDCExpCtlr = NULL;
    _pDefaultDCExpCtlr = NULL;
    _pDCRepCtlr = NULL;
    _pFwdCtlr = NULL;
    _pSearchCtrl = NULL;
    _pNetTrafficMemory = NULL;
    _pTransmissionHistoryInterface = NULL;
    _pReceivedMessagesInterface = NULL;
    _pSearchCtrl = NULL;
    _pMessagesToNotify = new PtrLList<MessageToNotifyToClient>();
    _pCfgMgr = pCfgMgr;

    // Initialize rand seed
    srand ((uint32)getTimeInMilliseconds());

    _ui16CacheCleanCycle = DataCacheInterface::DEFAULT_CACHE_CLEAN_CYCLE;
    setMissingFragmentTimeout ((uint32) pCfgMgr->getValueAsInt64 ("aci.disService.reassembler.request.time",
                                                                  DEFAULT_FRAGMENT_REQUEST_TIMEOUT));
    checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "Missing Fragment (and Message) request timeout set to %lu\n",
                    (uint32) getMissingFragmentTimeout());

    if (pCfgMgr->hasValue ("aci.disService.clientIdFiltering")) {
        _bClientIdFilteringEnabled = pCfgMgr->getValueAsBool ("aci.disService.clientIdFiltering");
    }
    else {
        _bClientIdFilteringEnabled = true;
    }

    // DisService Status Notifier
    _pStats = new DisServiceStats();
    _pSearchNotifier = new SearchNotifier();

    _bSubscriptionsExchangeEnabled = pCfgMgr->getValueAsBool ("aci.disService.worldstate.subscriptionsExchange.enabled", false);
    _bTopologyExchangeEnabled = pCfgMgr->getValueAsBool ("aci.disService.worldstate.topologyExchange.enabled", false);    
    // Create world state or topology world state
    if (_bSubscriptionsExchangeEnabled || _bTopologyExchangeEnabled) {   
        _pPeerState = PeerState::getInstance (this, PeerState::IMPROVED_TOPOLOGY_STATE);
    }
    else {
        _pPeerState = PeerState::getInstance (this, PeerState::TOPOLOGY_STATE);
    }
    _m.unlock (9);

    /*
     * NOTE: this is a HACK
    _bDataRequestReplyEnabled = true;
    */
}

int DisseminationService::sendStatus (void)
{
    int retStats, retTopology;
    retStats = retTopology = 0;
    if ((_pStatusNotifier) && (_pStats)) {
        retStats = _pStatusNotifier->sendSummaryStats (_pStats);
        //retTopology  = _pStatusNotifier->sendNeighborList ((const char **) _pPeerState->getAllNeighborIPs());
    }
    else {
        return -1;
    }
    if ((retStats == 0) && (retTopology == 0)) {
        return 0;
    }
    return -2;
}

DArray<uint16> * DisseminationService::getAllClients()
{
    DArray<uint16> *pClients = new DArray<uint16>();
    int i, j;
    for (i = j = 0; i <= _clients.getHighestIndex(); i++) {
        if (_clients.used(i)) {
            if (pClients == NULL) {
                pClients = new DArray<uint16>();
            }
            if (pClients) {
                (*pClients)[j] = i;
                j++;
            }
            else {
                checkAndLogMsg ("DisseminationService::getAllClients",
                                 memoryExhausted);
            }
        }
    }
    return pClients;
}

DArray<uint16> * DisseminationService::getSubscribingClients (Message *pMsg)
{
    return (_pLocalNodeInfo->getSubscribingClients (pMsg));
}

DArray<uint16> * DisseminationService::getAllSubscribingClients()
{
    return (_pLocalNodeInfo->getAllSubscribingClients());
}

uint16 DisseminationService::getNumberOfActiveNeighbors (void)
{
    return _pPeerState->getNumberOfActiveNeighbors();
}

void DisseminationService::addDataToCache (MessageHeader *pMsgHeader, const void *pData)
{
    _pDataCacheInterface->addData (pMsgHeader, (void*) pData);
}

int DisseminationService::handleDataRequestMessage (DisServiceDataReqMsg *pDSDRMsg, const char *pszIncomingInterface, int64 i64Timeout)
{
    return _pDataReqSvr->handleDataRequestMessage (pDSDRMsg, pszIncomingInterface, i64Timeout);
}

int DisseminationService::handleDataRequestMessage (const char *pszMsgId, DisServiceMsg::Range *pRange,
                                                    bool bIsChunk, const char *pszTarget,
                                                    unsigned int uiNumberOfActiveNeighbors,
                                                    int64 i64RequestArrivalTime,
                                                    const char **ppszOutgoingInterfaces, int64 i64Timeout)
{
    return _pDataReqSvr->handleDataRequestMessage (pszMsgId,pRange, bIsChunk, pszTarget, uiNumberOfActiveNeighbors,
                                                   i64RequestArrivalTime, ppszOutgoingInterfaces, i64Timeout);
}

int DisseminationService::handleWorldStateSequenceIdMessage (DisServiceWorldStateSeqIdMsg *, uint32)
{
    return -1;
}

int DisseminationService::handleSubscriptionStateMessage (DisServiceSubscriptionStateMsg *, uint32)
{
    return 0;
}

int DisseminationService::handleSubscriptionStateRequestMessage (DisServiceSubscriptionStateReqMsg *, uint32)
{
    return 0;
}

int DisseminationService::handleImprovedSubscriptionStateMessage (DisServiceImprovedSubscriptionStateMsg *, uint32)
{
    return 0;
}

int DisseminationService::handleProbabilitiesMessage (DisServiceProbabilitiesMsg *, uint32)
{
    return 0;
}

int DisseminationService::handleDataCacheQueryMessage (DisServiceDataCacheQueryMsg *pDSDCQMsg, uint32 ui32SourceIPAddress)
{
    if (_ui8QueryDataCacheReplyType != DISABLED) {
        DArray2<String> queryMatches;
        for (uint16 i = 0; i < pDSDCQMsg->getQueryCount(); i++) {
            if (NULL == pDSDCQMsg->getQuery (i)) {
                continue;
            }
            DArray2<String> *pIDs = _pDataCacheInterface->getMessageIDs (pDSDCQMsg->getQuery (i));
            if (pIDs == NULL) {
                // no matches for query "i"
                continue;
            }

            long lTmp = queryMatches.getHighestIndex();
            uint16 ui16Start;
            if (lTmp == -1) {
                ui16Start = 0;
            }
            else if (lTmp >= 0) {
                ui16Start = (uint16) lTmp + 1;
            }

            for (uint16 j = 0; j <= pIDs->getHighestIndex(); j++) {
                queryMatches [ui16Start + j] = (*pIDs)[j];
            }
        }
        if (_ui8QueryDataCacheReplyType == REPLY_DATA) {
            // Sends all the Messages not from the SenderNodeId
            PtrLList<MessageHeader> *pMsgInfos = new PtrLList<MessageHeader>();
            MessageHeader *pMI;
            const char *pszMsgId;
            const char *pszCacheNodeId;
            const char *pszSenderNodeId = pDSDCQMsg->getSenderNodeId();
            for (uint16 i = 0; (i <= queryMatches.getHighestIndex()); i++) {
                if (queryMatches.used (i)) {
                    pszMsgId = (const char *) queryMatches[i];
                    pMI = _pDataCacheInterface->getMessageInfo (pszMsgId);

                    if (pMI != NULL) {      // NOTE: This is temporary to avoid
                                            // lock where entries may have
                                            // been expired while in this method
                                            // TODO: investigate!
                        pszCacheNodeId = pMI->getPublisherNodeId();
                        if(strcmp(pszCacheNodeId, pszSenderNodeId) != 0) {
                            // If the publisher of the message is not the node that
                            // sent the DisServiceDataCacheQueryMsg message, add the
                            // MessageInfo to the list of the messages to replicate
                            pMsgInfos->append (pMI);
                        }
                    }
                }
            }
            if (pMsgInfos->getFirst()) {
                DataCacheReplicationController *pRepCtrl = (_pDCRepCtlr ? _pDCRepCtlr : _pDefaultDCRepCtlr);
                for (MessageHeader *pMITmp = pMsgInfos->getFirst(); pMITmp; pMITmp = pMsgInfos->getNext()) {
                    pRepCtrl->replicateMessage (pMITmp, pDSDCQMsg->getSenderNodeId(), 0);

                    // TODO: The following two lines are temporary - to be
                    // replaced by calling release on _pDataCacheInterface
                    delete pMITmp;
                    pMITmp = NULL;
                }
                return 0;
            }

            delete pMsgInfos;
            pMsgInfos = NULL;
        }
        else if (_ui8QueryDataCacheReplyType == REPLY_MSGID) {
            // Sends the content of the cache
            DisServiceDataCacheQueryReplyMsg dcqrMsg (getNodeId(), &queryMatches);
            broadcastDisServiceCntrlMsg (&dcqrMsg, NULL, "Sending DisServiceDataCacheQueryReplyMsg");
           _pStats->dataCacheQueryMessageReplySent(dcqrMsg.getSize());
           return 0;
        }
    }
    // The cache is empty or the replyQueryDataCache is disabled
    // sends the CacheEmptyMsg
    DisServiceCacheEmptyMsg emptyMsg (getNodeId());
    broadcastDisServiceCntrlMsg (&emptyMsg, NULL, "Sending DisServiceCacheEmptyMsg");
    return 0;
}

int DisseminationService::handleDataCacheQueryReplyMessage (DisServiceDataCacheQueryReplyMsg *pDSDCQRepMsg, uint32 ui32SourceIPAddress)
{
    selectMsgIDsToRequest (pDSDCQRepMsg->getSenderNodeId(), pDSDCQRepMsg->getIDs());

    return 0;
}

int DisseminationService::handleDataCacheMessagesRequestMessage (DisServiceDataCacheMessagesRequestMsg *pDSDCMReqMsg, uint32 ui32SourceIPAddress)
{
    DataCacheReplicationController * pRepCtrl = (_pDCRepCtlr ? _pDCRepCtlr : _pDefaultDCRepCtlr);
    for (uint16 i = 0; i < pDSDCMReqMsg->getIDsCount(); i++) {
        String id = pDSDCMReqMsg->getID (i);
        MessageHeader *pMH = _pDataCacheInterface->getMessageInfo (id);
        pRepCtrl->replicateMessage (pMH, pDSDCMReqMsg->getSenderNodeId(), 0);
    }
    return 0;
}

int DisseminationService::handleAcknowledgmentMessage (DisServiceAcknowledgmentMessage *pDSAM, uint32 ui32SourceIPAddress)
{
    if (_pTransmissionHistoryInterface != NULL) {
        _pTransmissionHistoryInterface->addMessageTarget (pDSAM->getAckedMsgId(), pDSAM->getSenderNodeId());
    }

    return 0;
}

int DisseminationService::handleCompleteMessageRequestMessage (DisServiceCompleteMessageReqMsg *pDSCMRMsg, uint32 ui32SourceIPAddress)
{ 
    const char *pszMsgId = pDSCMRMsg->getMsgId();
    if (_pDataCacheInterface->hasCompleteMessage (pszMsgId)) {  //TODO : CHECK THIS OUT
        MessageHeader *pMH = _pDataCacheInterface->getMessageInfo (pszMsgId);
        broadcastDisServiceCntrlMsg (new DisServiceAcknowledgmentMessage (getNodeId(), pMH->getMsgId()), NULL, "Sending DisServiceAcknowledgmentMessage");
        checkAndLogMsg ("DisseminationService::handleCompleteMessageRequestMessage",
                         Logger::L_Info, " Sended the acknowledge for the MsgId <%s>\n",
                         pMH->getMsgId());
    }
    return 0;
}

int DisseminationService::handleCacheEmptyMessage (DisServiceCacheEmptyMsg *pDSCEMsg, uint32 ui32SourceIPAddress)
{
    return 0;
}

int DisseminationService::handleCtrlToCtrlMessage (ControllerToControllerMsg *pCTCMsg, uint32 ui32SourceIPAddress)
{
    return 0;
}

int DisseminationService::handleHistoryRequestMessage (DisServiceHistoryRequest *pCTCMsg, uint32 ui32SourceIPAddress)
{
    DisServiceHistoryRequest::HReq * pHReq = pCTCMsg->getHistoryRequest();
    switch (pHReq->_ui8Type) {
        case DisServiceHistoryRequest::HT_WIN: {
            DisServiceHistoryRequest::HWinReq * pHWReq =  (DisServiceHistoryRequest::HWinReq*) pHReq;
            uint16 ui16HistoryLength = pHWReq->_ui16HistoryLength;
            const char *pszGroupName = pHWReq->_groupName;
            uint16 ui16Tag = pHWReq->_ui16Tag;
            char *pszOnDemandGroupName = getOnDemandDataGroupName (pszGroupName);
            const char *apszGroupNames[2];
            apszGroupNames[0] = pszGroupName;
            apszGroupNames[1] = pszOnDemandGroupName;
            const char *pszSenderNodeId = NULL;
            DArray2<String> *pQueryResult = _pDataCacheInterface->getSenderNodeIds (pszGroupName);
            if (pQueryResult == NULL) {
                // Nothing in the DB for the base group name - try the on-demand group name just in case
                // This is a fix to handle cases where data is made available with empty metadata, which
                // after replication, results in entries for the on-demand group but nothing for the
                // base group
                pQueryResult = _pDataCacheInterface->getSenderNodeIds (pszOnDemandGroupName);
            }
            if (pQueryResult != NULL) {
                for (uint8 i = 0; i < pQueryResult->size(); i++) {
                    uint32 ui32NextExpectedSeqNo;
                    pszSenderNodeId = (*pQueryResult)[i].c_str();
                    if (0 == stricmp (getNodeId(), pszSenderNodeId)) {
                        ui32NextExpectedSeqNo = _pLocalNodeInfo->getGroupPubState (pszGroupName);
                    }
                    else {
                        ui32NextExpectedSeqNo = _pDataCacheInterface->getNextExpectedSeqId (pszGroupName, pszSenderNodeId, ui16Tag);
                        if (ui32NextExpectedSeqNo == 0) {
                            // Try the on-demand group name also
                            ui32NextExpectedSeqNo = _pDataCacheInterface->getNextExpectedSeqId (pszOnDemandGroupName, pszSenderNodeId, ui16Tag);
                        }
                    }
                    uint16 ui16From = 0;
                    if (ui32NextExpectedSeqNo > ui16HistoryLength) {
                        ui16From = ui32NextExpectedSeqNo - ui16HistoryLength;
                    }
                    
                    for (uint32 ui32GroupNameIdx = 0; ui32GroupNameIdx < 2; ui32GroupNameIdx++) {
                        DisServiceDataCacheQuery *pQuery = new DisServiceDataCacheQuery();
                        pQuery->selectPrimaryKey();
                        pQuery->addConstraintOnGroupName (apszGroupNames[ui32GroupNameIdx]);
                        pQuery->addConstraintOnSenderNodeId (pszSenderNodeId);
                        pQuery->addConstraintOnMsgSeqIdGreaterThanOrEqual (ui16From);
                        pQuery->addConstraintOnMsgSeqIdSmallerThan (ui32NextExpectedSeqNo);
                        if (ui16Tag != 0) {
                            pQuery->addConstraintOnTag (ui16Tag);
                        }

                        PtrLList<MessageHeader> *pMHs = _pDataCacheInterface->getMessageInfos (pQuery);
                        if (pMHs != NULL) {
                            // I want the message with greater seq id first.
                            // TODO: implement ORDER BY in DisServiceDataCacheQuery
                            /*String tmp;
                            int x ,y; y = pMID->getHighestIndex();
                            for (x = 0; x < y; ) {
                                tmp = (*pMID)[y];
                                (*pMID)[y] = (*pMID)[x];
                                (*pMID)[x] = tmp;
                                x++; y--;
                            }*/

                            for (MessageHeader *pMH = pMHs->getFirst(); pMH != NULL; pMH = pMHs->getNext()) {
                                const void *pData = _pDataCacheInterface->getData (pMH->getMsgId());
                                Message *pMsg = new Message (pMH, pData);
                                DisServiceDataMsg *pDataMsg = new DisServiceDataMsg (getNodeId(), pMsg);
                                broadcastDisServiceDataMsg (pDataMsg, "Handling History Request");
                                delete pDataMsg;
                                delete pMsg;
                                pDataMsg = NULL;
                                pMsg = NULL;
                            }
                            //DisServiceHistoryRequestReplyMsg dhrr (getNodeId(), pMID);
                            //broadcastDisServiceCntrlMsg (&dhrr);
                            _pDataCacheInterface->release (pMHs);
                        }
                        delete pQuery;
                    }
                }
                delete pQueryResult;
                pQueryResult = NULL;
            }
            free (pszOnDemandGroupName);
            break;
        }
        case DisServiceHistoryRequest::HT_RANGE: {
            DisServiceHistoryRequest::HRangeReq *pReq = (DisServiceHistoryRequest::HRangeReq*) pCTCMsg->getHistoryRequest();
            PtrLList<Message> * pMessages = _pDataCacheInterface->getMessages (pReq->_groupName, pReq->_senderNodeId, pReq->_ui16Tag,
                                                                               pReq->_ui32Begin, pReq->_ui32End);
            Message *pMsg;
            Message *pTmp= pMessages->getFirst();
            while ((pMsg = pTmp) != NULL) {
                pTmp= pMessages->getNext();
                DisServiceDataMsg *pDataMsg = new DisServiceDataMsg (getNodeId(), pMsg);
                broadcastDisServiceDataMsg(pDataMsg, "Handling History Request");
            }
            _pDataCacheInterface->release (pMessages);
            break;
        }
        case DisServiceHistoryRequest::HT_TMP_RANGE: {
            break;
        }
    }
    return 0;
}

int DisseminationService::handleHistoryRequestReplyMessage (DisServiceHistoryRequestReplyMsg *pHistoryRequestReplyMsg, uint32)
{
    // TODO update the statistic
    selectMsgIDsToRequest (pHistoryRequestReplyMsg->getSenderNodeId(), pHistoryRequestReplyMsg->getIDs());
    return 0;
}

int DisseminationService::handleSearchMessage (SearchMsg *pSearchMsg, uint32)
{
    if (pSearchMsg == NULL || _pSearchNotifier == NULL) {
        return -1;
    }
    unsigned int uiQueryLen = 0;
    const void *pQuery = pSearchMsg->getQuery (uiQueryLen);
    _pSearchNotifier->searchArrived (pSearchMsg->getQueryId(), pSearchMsg->getGroupName(),
            pSearchMsg->getQuerier(), pSearchMsg->getQueryType(), pSearchMsg->getQueryQualifier(),
            pQuery, uiQueryLen);

    return 0;
}

int DisseminationService::handleSearchReplyMessage (SearchReplyMsg *pSearchReplyMsg, uint32)
{
    if (pSearchReplyMsg == NULL || _pSearchNotifier == NULL) {
        return -1;
    }

    _pSearchNotifier->searchReplyArrived (pSearchReplyMsg->getQueryId(),
            pSearchReplyMsg->getMatchingMsgIds(), pSearchReplyMsg->getMatchingNode());

    return 0;
}

int DisseminationService::selectMsgIDsToRequest (const char *pszSenderNodeId, DArray2<String> *pMsgIDs)
{
    DArray2<String> *pIDs = pMsgIDs;
    bool bAtLeastOneID = false;
    for (uint16 i = 0; i <= pIDs->getHighestIndex(); i++) {
        if (_pDataCacheInterface->getMessageInfo ((*pIDs)[i]) != NULL) {
            (*pIDs)[i] = NULL;
        }
        else {
            bAtLeastOneID = true;
        }
    }

    // TODO: should I add a new method in stats????
    if (bAtLeastOneID) {
        DisServiceDataCacheMessagesRequestMsg dcmrMsg (getNodeId(), NULL, pIDs);
        broadcastDisServiceCntrlMsg (&dcmrMsg, NULL, "Sending DisServiceDataCacheMessagesRequestMsg");
    }
    return 0;
}

bool DisseminationService::clearToSend (const char *pszInterface)
{
    return _pTrSvc->clearToSend (pszInterface);
}

bool DisseminationService::clearToSendOnAllInterfaces (void)
{
    return _pTrSvc->clearToSendOnAllInterfaces();
}

int DisseminationService::broadcastDisServiceDataMsg (DisServiceDataMsg *pDDMsg, const char *pszPurpose,
                                                      const char **ppszOutgoingInterfaces, const char *pszTargetAddr,
                                                      const char *pszHints)
{
    _mBrcast.lock (60);
    const char *pszMethodName = "DisseminationService::broadcastDisServiceDataMsg";
    uint32 ui32HeaderSize = _pTrSvcHelper->computeMessageHeaderSize (getNodeId(), pDDMsg->getTargetNodeId(),
                                                                     getSessionId(), pDDMsg->getMessageHeader());
    pDDMsg->flush();

    const MessageHeader *pMH = pDDMsg->getMessage()->getMessageHeader();  // !! // DO NOT MODIFY the MessageInfo in pDDMsg
    const MessageId msgId (pMH->getGroupName(), pMH->getPublisherNodeId(), pMH->getMsgSeqId(), pMH->getChunkId());

    uint16 ui16FragSize = minimum (_pTrSvc->getMTU(), _pTrSvc->getMaxFragmentSize());
    if (ui16FragSize == 0) {
        _mBrcast.unlock (60);
        return -1;
    }

    if (ui32HeaderSize >= ui16FragSize) {
        // In this case there is no room to add the actual data or even the
        // DisseminationService header is too long
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "the DisseminationService header "
                        "is longer than the max value allowed - message cannot be transmitted\n");
        _mBrcast.unlock (60);
        return -2;
    }

    int rc = 0;
    if (pMH->getFragmentLength() <=  (ui16FragSize - ui32HeaderSize)) {

        //-----------------------------------------------------------------
        // There is no need to fragment
        //-----------------------------------------------------------------
        pDDMsg->setSenderNodeId (getNodeId());
        if (pMH->isCompleteMessage()) {
            pDDMsg->setSendingCompleteMsg (true);
        }

        _pDataReqSvr->startedPublishingMessage (msgId.getId());
        TransmissionService::TransmissionResults res = _pTrSvc->broadcast (pDDMsg, ppszOutgoingInterfaces, pszPurpose, pszTargetAddr, pszHints);
        _pDataReqSvr->endedPublishingMessage (msgId.getId());
        rc = res.rc;
        res.reset();
    }
    else {
        //-----------------------------------------------------------------
        // Need to fragment message
        //-----------------------------------------------------------------

        DisServiceDataMsgFragmenter fragmenter (getNodeId());
        fragmenter.init (pDDMsg, ui16FragSize, ui32HeaderSize);
        
        _pDataReqSvr->startedPublishingMessage (msgId.getId ());
        for (DisServiceDataMsg *pFragDDMsg = NULL; (pFragDDMsg = fragmenter.getNextFragment()) != NULL;) {
            TransmissionService::TransmissionResults res = _pTrSvc->broadcast (pFragDDMsg, ppszOutgoingInterfaces, pszPurpose, pszTargetAddr, pszHints);
            delete pFragDDMsg;
            if (res.rc != 0) {
                rc = res.rc;
            }
            res.reset();
        }
        _pDataReqSvr->endedPublishingMessage (msgId.getId());
    }

    _mBrcast.unlock (60);
    return rc;
}

int DisseminationService::broadcastDisServiceCntrlMsg (DisServiceCtrlMsg *pDDCtrlMsg, const char **ppszOutgoingInterfaces,
                                                       const char *pszPurpose, const char *pszTargetAddr, const char *pszHints)
{
    _mBrcast.lock (61);
    TransmissionService::TransmissionResults res = _pTrSvc->broadcast (pDDCtrlMsg, ppszOutgoingInterfaces, pszPurpose, pszTargetAddr, pszHints);
    int rc = res.rc;
    res.reset();
    _mBrcast.unlock (61);
    return rc;
}

uint32 DisseminationService::getMinOutputQueueSizeForPeer (const char *pszTargetNodeId)
{
    if (pszTargetNodeId == NULL) {
        return 0;
    }
    uint32 ui32OutputQueueSize = 0;
    _pPeerState->lock();
    const char **ppszInterfaces = _pPeerState->getIPAddressesAsStrings (pszTargetNodeId);
    if (ppszInterfaces != NULL) {
        bool bOutputQueueSizeInitialized = false;
        uint32 ui32;
        for (uint8 i = 0; ppszInterfaces[i] != NULL; i++) {
            if (!bOutputQueueSizeInitialized) {
                ui32OutputQueueSize = _pTrSvc->getTransmissionQueueSize (ppszInterfaces[i]);
                bOutputQueueSizeInitialized = true;
            }
            else if ((ui32 = _pTrSvc->getTransmissionQueueSize (ppszInterfaces[i])) < ui32OutputQueueSize) {
                ui32OutputQueueSize = ui32;
            }
        }
        free (ppszInterfaces);
    }
    _pPeerState->unlock();
    return ui32OutputQueueSize;
}

int DisseminationService::transmitDisServiceControllerMsg (DisServiceCtrlMsg *pCtrlMsg, bool bReliable,
                                                           const char *pszPurpose)
{
    if (pCtrlMsg == NULL) {
        return -1;
    }
    _mControllers.lock (63);
    int rc = transmitDisServiceControllerMsgInternal (pCtrlMsg, pCtrlMsg->getTargetNodeId(), bReliable, pszPurpose);
    _mControllers.unlock (63);
    return rc;
}

int DisseminationService::transmitDisServiceControllerMsg (ControllerToControllerMsg *pCtrlMsg, bool bReliable,
                                                           const char *pszPurpose)
{
    if (pCtrlMsg == NULL) {
        return -1;
    }
    _mControllers.lock (63);
    int rc = transmitDisServiceControllerMsgInternal (pCtrlMsg, pCtrlMsg->getReceiverNodeID(), bReliable, pszPurpose);
    _mControllers.unlock (63);
    return rc;
}

int DisseminationService::transmitDisServiceControllerMsgInternal (DisServiceMsg *pCtrlMsg, const char *pszRecepientNodeId,
                                                                   bool bReliable, const char *pszPurpose)
{
    if (_pTrSvc == NULL) {
        // Not initialized
        return -1;
    }

    int rc = 0;
    _pPeerState->lock();
    const char **ppszPeerAddresses = _pPeerState->getIPAddressesAsStrings (pszRecepientNodeId);
    if (ppszPeerAddresses != NULL) {
        for (unsigned int i = 0; ppszPeerAddresses[i] != NULL; i++) {
            const char **ppszOutgoingInterfaces = (const char **) _pTrSvc->getInterfacesByDestinationAddress (ppszPeerAddresses[i]);
            TransmissionService::TransmissionResults rcTmp = _pTrSvc->unicast (pCtrlMsg, ppszOutgoingInterfaces, pszPurpose,
                                                                               ppszPeerAddresses[i], NULL, bReliable);
            if (rcTmp.rc != 0) {
                rc = rcTmp.rc;
            }
            rcTmp.reset();
            if (ppszOutgoingInterfaces != NULL) {
                for (uint8 ui8 = 0; ppszOutgoingInterfaces[ui8]; ui8++) {
                    free ((char*)ppszOutgoingInterfaces[ui8]);
                    ppszOutgoingInterfaces[ui8] = NULL;
                }
                free (ppszOutgoingInterfaces);
            }
        }
        free (ppszPeerAddresses);
    }
    _pPeerState->unlock();

    return rc;
}

void DisseminationService::messageArrived (Message *pMsg, RequestDetails *pDetails)
{
    bool bIsMetadata = false;
    bool bIsMetadataWrappedInData = false;
    MessageHeader *pMH = pMsg->getMessageHeader();
    if (!pMH->isChunk()) {
        MessageInfo *pMI = pMsg->getMessageInfo();
        bIsMetadata = pMI->isMetaData();
        bIsMetadataWrappedInData = pMI->isMetaDataWrappedInData();
    }
    if (pMH->getAcknowledgment() && pMH->isCompleteMessage()) {
        // Send Acknowledgment
        DisServiceAcknowledgmentMessage ack (getNodeId(), pMH->getMsgId());
        if (0 == broadcastDisServiceCntrlMsg (&ack, NULL, "Sending DisServiceAcknowledgmentMessage")) {
            checkAndLogMsg ("DisseminationService::messageArrived", Logger::L_Info,
                            "Sent acknowledgment for MsgId <%s>\n", pMH->getMsgId());
        }
        else {
            checkAndLogMsg ("DisseminationService::messageArrived", Logger::L_SevereError,
                            "Could not send acknowledgment for MsgId <%s>.\n", pMH->getMsgId());
        }
    }

    DArray<uint16> *pSubscribingClients = getSubscribingClients (pMsg);
    if (pSubscribingClients != NULL && pSubscribingClients->size() > 0) {
        notifyDisseminationServiceListeners (pSubscribingClients, pMsg,
                                             bIsMetadata, bIsMetadataWrappedInData,
                                             pDetails);
    }

    delete pSubscribingClients;
    pSubscribingClients = NULL;
}

void DisseminationService::notifyDisseminationServiceListeners (DArray<uint16> *pClientIdList, Message *pMsg,
                                                                bool bMetaData, bool bMetaDataWrappedInData,
                                                                RequestDetails *pDetails)
{
    _mToListeners.lock (64);
    for (unsigned int i = 0; i < pClientIdList->size(); i++) {
        const uint16 ui16ClientId = pClientIdList->get (i);
        bool bFound = false;
        if (pDetails != NULL) {
            for (RequestDetails::QueryDetails *pQD = pDetails->_details.getFirst(); pQD != NULL; pQD = pDetails->_details.getNext()) {
                if (ui16ClientId == pQD->_ui16ClientId) {
                    bFound = true;
                    notifyDisseminationServiceListener (ui16ClientId, pMsg, bMetaData,
                                                        bMetaDataWrappedInData, pQD->_queryId);
                    break;
                }
            }
        }
        if (!bFound) {
            notifyDisseminationServiceListener (ui16ClientId, pMsg, bMetaData,
                                                bMetaDataWrappedInData, NULL);
        }
        
    }
    _mToListeners.unlock (64);
}

void DisseminationService::notifyDisseminationServiceListener (uint16 ui16ClientId, Message *pMsg, bool bMetaData,
                                                               bool bMetaDataWrappedInData, const char *pszQueryId)
{
    const char *pszMethodName = "DisseminationService::notifyDisseminationServiceListener";
    MessageHeader *pMH = pMsg->getMessageHeader();
    if (pMH == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "null message header.\n");
        return;
    }

    const char *pszMyself = getNodeId();
    const char *pszSender = pMH->getPublisherNodeId();
    
    if (_bClientIdFilteringEnabled && (pszQueryId == NULL)) {
        // If I am the sender of the message I do not want to notify it to the
        // application, unless it matched a search of mine, in which case
        // pszQueryId is set
        if (0 == stricmp (pszSender, pszMyself) && (ui16ClientId == pMH->getClientId())) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "not notifying client %d of the arrival "
                            "of message %s because the client is the publisher of the message\n",
                            (int) ui16ClientId, pMH->getMsgId());
            return;
        }
    }
    if ((!_clients.used (ui16ClientId)) || _clients[ui16ClientId].pListener == NULL) {
        // After pClientIdList was retrieved, clients may have de-registered, so
        // I have to check whether the client identified by ui16ClientId is still
        // registered. registration/de-registration are assumed to be rare events,
        // therefore it in most cases, recomputing the list of the interested peer
        // (pClientIdList) in this method, is not worth it
        checkAndLogMsg (pszMethodName, Logger::L_Info, "not notifying client %d of the arrival "
                        "of message %s because the client has no registered listener\n",
                        (int) ui16ClientId, pMH->getMsgId());
        return;
    }

    if ((pszQueryId == NULL) && (!_pLocalNodeInfo->hasSubscription (ui16ClientId, pMsg))) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "not notifying client %d of the arrival "
                        "of message %s because the client does not subscribe to the message's group\n",
                        (int) ui16ClientId, pMH->getMsgId());
        return;
    }
    else if ((pszQueryId != NULL) && _pSearchCtrl->messageNotified (pMH->getMsgId(), ui16ClientId, pszQueryId)) {
        // The query ID is set, but the match was already notified
        checkAndLogMsg (pszMethodName, Logger::L_Info, "not notifying client %d of the arrival "
                        "of message %s because the client has already been notified as a result of the search %s\n",
                        (int) ui16ClientId, pMH->getMsgId(), pszQueryId);
        return;
    }
    else if (pszQueryId != NULL) {
        uint16 ui16SearchingClientId = 0;
        Searches::getSearches()->getSearchQueryId (pszQueryId, ui16SearchingClientId);
        if (ui16ClientId != ui16SearchingClientId) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "not notifying client %d of the arrival "
                            "of message %s because the client did not search it (it was %d who did)\n",
                            (int) ui16ClientId, pMH->getMsgId(), (int) ui16SearchingClientId);
            return;
        }
    }

    if (pMH->isChunk()) {
        ChunkMsgInfo *pCMI = (ChunkMsgInfo*) pMH;
        unsigned int uiTotNChunks = 0;
        uint8 ui8NChunks = 1;
        int rc = _pDataCacheInterface->countChunks (pMH->getGroupName(), pMH->getPublisherNodeId(),
                                                    pMH->getMsgSeqId(), uiTotNChunks);
        assert (uiTotNChunks <= 0xFF);
        assert (rc <= 0xFF);
        if (rc < 1) {
            checkAndLogMsg ("DisseminationService::notifyDisseminationServiceListeners", Logger::L_Warning,
                            "could not get the total number of chunks\n");
        }
        else {
            ui8NChunks = (uint8) rc;
        }

        char *pszGroupName = removeOnDemandSuffixFromGroupName (pMH->getGroupName());
        char *pszChunkedMsgId = convertFieldToKey (pszGroupName, pMH->getPublisherNodeId(), pMH->getMsgSeqId());
        _clients[ui16ClientId].pListener->chunkArrived (pMH->getClientId(), pMH->getPublisherNodeId(),
                                                        pszGroupName, pMH->getMsgSeqId(), pMH->getObjectId(),
                                                        pMH->getInstanceId(), pMH->getMimeType(), pMsg->getData(),
                                                        pMH->getTotalMessageLength(), ui8NChunks, pCMI->getTotalNumberOfChunks(),
                                                        pszChunkedMsgId, pMH->getTag(), pMH->getPriority(), pszQueryId);
        free (pszGroupName);
        free (pszChunkedMsgId);
    }
    else {
        MessageInfo *pMI = pMsg->getMessageInfo();
        if (bMetaData) {
            const char *pszReferrebObjectId = removeOnDemandSuffixFromId (pMI->getReferredObject());
            if (pszReferrebObjectId == NULL) {
                checkAndLogMsg ("DisseminationService::notifyDisseminationServiceListeners", Logger::L_Warning,
                                "pszReferrebObjectId is NULL\n");
            }
            _clients[ui16ClientId].pListener->dataAvailable (pMI->getClientId(), pMI->getPublisherNodeId(), pMI->getGroupName(), pMI->getMsgSeqId(),
                                                             pMI->getObjectId(), pMI->getInstanceId(), pMI->getMimeType(), pszReferrebObjectId,
                                                             pMsg->getData(), pMI->getTotalMessageLength(), pMI->getTag(), pMI->getPriority(), pszQueryId);
            free ((char*) pszReferrebObjectId);
        }
        else if (bMetaDataWrappedInData) {
            _clients[ui16ClientId].pListener->metadataArrived (pMI->getClientId(), pMI->getPublisherNodeId(), pMI->getGroupName(), pMI->getMsgSeqId(),
                                                               pMI->getObjectId(), pMI->getInstanceId(), pMI->getMimeType(), pMsg->getData(),
                                                               pMI->getMetaDataLength(), false, pMI->getTag(), pMI->getPriority(), pszQueryId);
        }
        else {
            _clients[ui16ClientId].pListener->dataArrived (pMI->getClientId(), pMI->getPublisherNodeId(), pMI->getGroupName(), pMI->getMsgSeqId(),
                                                           pMI->getObjectId(), pMI->getInstanceId(), pMI->getAnnotates(), pMI->getMimeType(), pMsg->getData(),
                                                           pMI->getTotalMessageLength(), pMI->getMetaDataLength(), pMI->getTag(), pMI->getPriority(),
                                                           pszQueryId);
        }
    }

    if (pszQueryId != NULL) {
        _pSearchCtrl->setNotifiedClients (pMH->getMsgId(), pszQueryId, ui16ClientId);
        checkAndLogMsg ("DisseminationService::notifyDisseminationServiceListeners",
                        Logger::L_Warning, "notified client %d of the arrival of message %s (searchId %s).\n",
                        (int) ui16ClientId, pMH->getMsgId(), pszQueryId);
    }
    else {
        checkAndLogMsg ("DisseminationService::notifyDisseminationServiceListeners",
                        Logger::L_Warning, "notified client %d of the arrival of message %s.\n",
                        (int) ui16ClientId, pMH->getMsgId());
    }
}

void DisseminationService::notifyPeerStatusListeners (const char *pszPeerNodeId, bool bIsNewPeer)
{
    _mToPeerListeners.lock (66);
    for (unsigned int i = 0; i < _peerStatusClients.size(); i++) {
        if (_peerStatusClients.used(i)) {
            if (bIsNewPeer) {
                _peerStatusClients[i].pListener->newPeer (pszPeerNodeId);
            }
            else {
                _peerStatusClients[i].pListener->deadPeer (pszPeerNodeId);
            }
        }
    }
    _mToPeerListeners.unlock (66);
}

Reader * DisseminationService::getData (const char *pszId, uint32 &ui32MessageLength)
{
    _mGetData.lock (67);
    DataCache::Result result;
    _pDataCacheInterface->getData (pszId, result);
    if (result.pData != NULL) {
        ui32MessageLength = result.ui32Length;
        switch (result.ui8StorageType) {
            case DataCacheInterface::MEMORY: {
                _mGetData.unlock (67);
                return (new BufferReader (result.pData, result.ui32Length));
            }
            case DataCacheInterface::FILE: {
                FILE *pFile = fopen ((const char *)result.pData , "r");
                if (pFile != NULL) {
                    _mGetData.unlock (67);
                    return (new FileReader(pFile, true));
                }
            }
            case DataCacheInterface::NOSTORAGETYPE: {
                _mGetData.unlock (67);
                return NULL;
            }
        }
    }
    _mGetData.unlock (67);
    return NULL;
}

int DisseminationService::getData (const char *pszGroupName, const char *pszSender,
                                   uint32 ui32SeqId, uint32 *ui32DataLength, void **pData)
{
    return getData (convertFieldToKey (pszGroupName, pszSender, ui32SeqId),
                    ui32DataLength, pData);
}

int DisseminationService::getData (const char *pszMsgId, uint32 *ui32DataLength, void **pData)
{
    if (pszMsgId == NULL) {
        return -1;
    }
    _mGetData.lock (68);
    const void *pBuffer = _pDataCacheInterface->getData (pszMsgId, *ui32DataLength);
    if (pBuffer == NULL) {
         checkAndLogMsg ("DisseminationService::getData", Logger::L_Info,
                         "no data has been found in the cache %s\n", pszMsgId);
         (*ui32DataLength) = 0;
         _mGetData.unlock (68);
         return 0;
    }
    else {
        checkAndLogMsg ("DisseminationService::getData", Logger::L_Info,
                        "data found in the cache %s\n", pszMsgId);
    }
    (*pData) = calloc ((*ui32DataLength), sizeof(char));
    memcpy ((*pData), pBuffer, (*ui32DataLength));

    _pDataCacheInterface->release (pszMsgId, (void*)pBuffer);

    _mGetData.unlock (68);
    return 0;
}

char ** DisseminationService::getDisseminationServiceIds (const char *pszObjectId)
{
    if (pszObjectId == NULL) {
        return NULL;
    }
    return getDisseminationServiceIds (pszObjectId, NULL);
}

char ** DisseminationService::getDisseminationServiceIds (const char *pszObjectId, const char *pszInstanceId)
{
    if (pszObjectId == NULL || _pDataCacheInterface == NULL) {
        return NULL;
    }

    return _pDataCacheInterface->getDisseminationServiceIds (pszObjectId, pszInstanceId);
}

int strcpyAndFill (char *pszDst, uint32 ui32DestLen, const char *pszSrc, uint32 ui32SrcLen)
{
    if (ui32SrcLen > ui32DestLen) {
        return -1;
    }
    strncpy (pszDst, pszSrc, ui32SrcLen);
    if (ui32DestLen > ui32SrcLen) {
        // fill the rest with NULL
        memset (pszDst+ui32SrcLen, 0, ui32DestLen-ui32SrcLen);
    }
    return 0;
}

