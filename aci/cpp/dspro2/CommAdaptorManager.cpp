/*
 * CommAdaptorManager.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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

#include "CommAdaptorManager.h"

#include "Controller.h"
#include "Defs.h"
#include "DisServiceAdaptor.h"
#include "DSPro.h"
#include "MocketsAdaptor.h"
#include "NatsAdaptor.h"
#include "Targets.h"
#include "TCPAdaptor.h"
#include "UDPAdaptor.h"

#include "ConfigManager.h"
#include "SearchProperties.h"
#include "SessionId.h"

#include "Logger.h"
#include "UInt32Hashtable.h"

#define unconfiguredAdaptor Logger::L_Warning, "could not configure adaptor %s. Returned error code %d.\n"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    void mergeTargetsByAdaptorId (Targets **ppTargets, UInt32Hashtable<Targets> &ht)
    {
        if (ppTargets == nullptr) {
            return;
        }
        for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
            Targets *pTarget = ht.get (ppTargets[i]->adaptorId);
            if (pTarget == nullptr) {
                Targets *pTarget = new Targets (*(ppTargets[i]));
                ht.put (ppTargets[i]->adaptorId, pTarget);
            }
            else {
                pTarget->subsume (*(ppTargets[i]));
            }
        }
    }
}

const AdaptorId CommAdaptorManager::DISSERVICE_ADAPTOR_ID = 0;

CommAdaptorManager::CommAdaptorManager (const char *pszNodeId)
    : _m (MutexId::CommAdaptoManager_m, LOG_MUTEX),
      _nodeId (pszNodeId),
      _pPropertyStore (nullptr)
{
}

CommAdaptorManager::~CommAdaptorManager (void)
{
    stopAdaptors();
    for (unsigned int uiIndex = 0; uiIndex < _adaptors.size(); uiIndex++) {
        if (_adaptors.used (uiIndex) && _adaptors[uiIndex].pAdaptor != nullptr) {
            delete _adaptors[uiIndex].pAdaptor;
            _adaptors[uiIndex].pAdaptor = nullptr;
        }
    }
}

int CommAdaptorManager::init (NOMADSUtil::ConfigManager *pCfgMgr, const char *pszSessionId, Controller *pController, PropertyStoreInterface *pPropertyStore)
{
    if (pCfgMgr == nullptr || pController == nullptr || pPropertyStore == nullptr) {
        return -1;
    }

    _pPropertyStore = pPropertyStore;

    const char *pszMethodName = "CommAdaptorManager::init";

    unsigned int uiId = 0;
    if (_commListenerNotifier.registerCommAdaptorListener (pController, uiId) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "could not register Controller with _commListenerNotifier\n");
        return -2;
    }

    // Instantiate adaptors

    bool bUseDisservice = pCfgMgr->getValueAsBool (DSPro::ENABLE_DISSERVICE_ADAPTOR, false);
    const bool bUseMockets = pCfgMgr->getValueAsBool (DSPro::ENABLE_MOCKETS_ADAPTOR, false);
    const bool bUseTCP = pCfgMgr->getValueAsBool (DSPro::ENABLE_TCP_ADAPTOR, false);
    const bool bUseUDP = pCfgMgr->getValueAsBool (DSPro::ENABLE_UDP_ADAPTOR, false);
    const bool bUseNats = pCfgMgr->getValueAsBool (DSPro::ENABLE_NATS_ADAPTOR, false);

    if (!(bUseMockets || bUseTCP || bUseNats || bUseUDP)) {
        // Make sure that Disservice is used as default, if no other adaptor is selected
        bUseDisservice = true;
    }

    _mSessionId.lock();

    const String sessionId (SessionId::getSessionId (pCfgMgr));

    // -------------------------------------------------------------------------
    // DisService
    // -------------------------------------------------------------------------

    if (bUseDisservice) {

        // Enable DisService
        _adaptors[DISSERVICE_ADAPTOR_ID].uiId = DISSERVICE_ADAPTOR_ID;
        _adaptors[DISSERVICE_ADAPTOR_ID].pAdaptor = DisServiceAdaptor::getDisServiceAdaptor (DISSERVICE_ADAPTOR_ID, _nodeId,
                                                                                             &_commListenerNotifier, pCfgMgr);
        if (_adaptors[DISSERVICE_ADAPTOR_ID].pAdaptor == nullptr) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "could not create DisServiceAdaptor.\n");
            _mSessionId.unlock();
            return -3;
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info, "successfully created DisServiceAdaptor: "
                        "nodeId: <%s> sessionKey: <%s>.\n", _adaptors[DISSERVICE_ADAPTOR_ID].pAdaptor->getNodeId().c_str(),
                        sessionId.c_str());
    }
    else {
        SessionId::getInstance()->setSessionId (sessionId, 0);
    }

    // -------------------------------------------------------------------------
    // Mockets
    // -------------------------------------------------------------------------

    if (bUseMockets) {

        // Enable mockets
        unsigned int uiIndex = _adaptors.firstFree();
        _adaptors[uiIndex].uiId = uiIndex;
        uint16 ui16MocketsPort = (uint16) pCfgMgr->getValueAsInt ("aci.dspro.adaptor.mockets.port",
                                                                  MocketsAdaptor::DEFAULT_PORT);
        _adaptors[uiIndex].pAdaptor = new MocketsAdaptor (uiIndex, _nodeId, &_commListenerNotifier, ui16MocketsPort);
        if (_adaptors[uiIndex].pAdaptor == nullptr) {
            checkAndLogMsg (pszMethodName, memoryExhausted);
            _mSessionId.unlock();
            return -4;
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info, "successfully created MocketsAdaptor: "
                        "nodeId: <%s> sessionKey: <%s>.\n", _adaptors[uiIndex].pAdaptor->getNodeId().c_str(),
                        sessionId.c_str());
    }

    // -------------------------------------------------------------------------
    // TCP
    // -------------------------------------------------------------------------

    if (bUseTCP) {

        // Enable TCP
        unsigned int uiIndex = _adaptors.firstFree();
        _adaptors[uiIndex].uiId = uiIndex;
        uint16 ui16TCPPort = (uint16) pCfgMgr->getValueAsInt ("aci.dspro.adaptor.tcp.port",
                                                              TCPAdaptor::DEFAULT_PORT);
        _adaptors[uiIndex].pAdaptor = new TCPAdaptor (uiIndex, _nodeId,
                                                      &_commListenerNotifier, ui16TCPPort);
        if (_adaptors[uiIndex].pAdaptor == nullptr) {
            checkAndLogMsg (pszMethodName, memoryExhausted);
            _mSessionId.unlock();
            return -5;
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info, "successfully created TCPAdaptor: "
                        "nodeId: <%s> sessionKey: <%s>", _adaptors[uiIndex].pAdaptor->getNodeId().c_str(),
                        sessionId.c_str());
    }

    // -------------------------------------------------------------------------
    // UDP
    // -------------------------------------------------------------------------

    if (bUseUDP) {

        // Enable UDP
        unsigned int uiIndex = _adaptors.firstFree();
        _adaptors[uiIndex].uiId = uiIndex;
        uint16 ui16UDPPort = (uint16)pCfgMgr->getValueAsInt ("aci.dspro.adaptor.udp.port",
                                                             UDPAdaptor::DEFAULT_PORT);
        _adaptors[uiIndex].pAdaptor = new UDPAdaptor (uiIndex, &_commListenerNotifier, _nodeId, ui16UDPPort);
        if (_adaptors[uiIndex].pAdaptor == nullptr) {
            checkAndLogMsg (pszMethodName, memoryExhausted);
            _mSessionId.unlock();
            return -6;
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info, "successfully created UDPAdaptor: "
                        "nodeId: <%s> sessionKey: <%s>", _adaptors[uiIndex].pAdaptor->getNodeId().c_str(),
                        sessionId.c_str());
    }

    // -------------------------------------------------------------------------
    // NATS
    // -------------------------------------------------------------------------

    if (bUseNats) {

        // Enable NATS
        unsigned int uiIndex = _adaptors.firstFree();
        _adaptors[uiIndex].uiId = uiIndex;
        _adaptors[uiIndex].pAdaptor = new NatsAdaptor (uiIndex, &_commListenerNotifier, _nodeId);
        if (_adaptors[uiIndex].pAdaptor == nullptr) {
            checkAndLogMsg (pszMethodName, memoryExhausted);
            _mSessionId.unlock();
            return -7;
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info, "successfully created NatsAdaptor: "
                        "nodeId: <%s> sessionKey: <%s>", _adaptors[uiIndex].pAdaptor->getNodeId().c_str(),
                        sessionId.c_str());
    }

    _mSessionId.unlock();

    // Initialize all the adaptors
    int rc = 0;
    for (unsigned int uiIndex = 0; uiIndex < _adaptors.size(); uiIndex++) {
        if (_adaptors.used (uiIndex) && (_adaptors[uiIndex].pAdaptor != nullptr)) {
            int rcTmp = _adaptors[uiIndex].pAdaptor->init (pCfgMgr);
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiIndex].pAdaptor->getAdaptorAsString(), rc);
                if (rc == 0) {
                    rc = -8;
                }
            }
        }
    }

    return rc;
}

int CommAdaptorManager::changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len)
{
    _m.lock (2061);
    int rc = 0;
    for (unsigned int i = 0; i < _adaptors.size(); i++) {
        if (_adaptors.used (i) && (_adaptors[i].pAdaptor != nullptr)) {
            rc += _adaptors[i].pAdaptor->changeEncryptionKey (pchKey, ui32Len);
        }
    }
    _m.unlock (2061);
    return rc;
}

CommAdaptor * CommAdaptorManager::getAdaptorByType (AdaptorType type)
{
    _m.lock (2061);
    for (unsigned int i = 0; i < _adaptors.size(); i++) {
        if (_adaptors.used (i) && _adaptors[i].pAdaptor->getAdaptorType() == type) {
            _m.unlock (2061);
            return _adaptors[i].pAdaptor;
        }
    }
    _m.unlock (2061);
    return nullptr;
}

int CommAdaptorManager::registerCommAdaptorListener (CommAdaptorListener *pListener, unsigned int &uiListenerId)
{
    if (pListener == nullptr) {
        return -1;
    }
    _m.lock (2040);
    int rc = _commListenerNotifier.registerCommAdaptorListener (pListener, uiListenerId);
    _m.unlock (2040);
    return rc;
}

int CommAdaptorManager::deregisterCommAdaptorListener (unsigned int uiListenerId)
{
    _m.lock (2041);
    int rc = _commListenerNotifier.deregisterCommAdaptorListener (uiListenerId);
    _m.unlock (2041);
    return rc;
}

int CommAdaptorManager::startAdaptors (void)
{
    const char *pszMethodName = "CommAdaptorManager::startAdaptors";

    // Start all the adaptors
    int rc = 0;
    _m.lock (2042);
    for (unsigned int uiIndex = 0; uiIndex < _adaptors.size(); uiIndex++) {
        if (_adaptors.used (uiIndex) && _adaptors[uiIndex].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiIndex].pAdaptor->startAdaptor();
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not start adaptor\n",
                                _adaptors[uiIndex].pAdaptor->getAdaptorAsString(), rc);
                if (rc == 0) {
                    rc = -2;
                }
            }
        }
    }

    _m.unlock (2042);
    return rc;
}

int CommAdaptorManager::stopAdaptors (void)
{
    const char *pszMethodName = "CommAdaptorManager::stopAdaptors";

    // Stop all the adaptors
    int rc = 0;
    _m.lock (2042);
    for (unsigned int uiIndex = 0; uiIndex < _adaptors.size(); uiIndex++) {
        if (_adaptors.used (uiIndex) && _adaptors[uiIndex].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiIndex].pAdaptor->stopAdaptor();
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not stop adaptor\n",
                                _adaptors[uiIndex].pAdaptor->getAdaptorAsString(), rc);
                if (rc == 0) {
                    rc = -2;
                }
            }
        }
    }

    _m.unlock (2042);
    return rc;
}

AdaptorId CommAdaptorManager::addAdptor (void)
{
    // TODO: implement this
    return 0;
}

int CommAdaptorManager::connectToPeer (AdaptorType type, const char *pszRemoteAddr, uint16 ui16Port)
{
    const char *pszMethodName = "CommAdaptorManager::connectToPeer";

    if (pszRemoteAddr == nullptr) {
        return -1;
    }
    switch (type) {
        case MOCKETS:
        case TCP:
            break;

        case UDP:
            return 0;

        default:
            checkAndLogMsg (pszMethodName, Logger::L_Warning,
                            "only MOCKETS and TCP adaptors support direct connections\n");
            return -2;
    }

    for (unsigned int uiIndex = 0; uiIndex < _adaptors.size(); ++uiIndex) {
        if (_adaptors.used (uiIndex) && (_adaptors[uiIndex].pAdaptor != nullptr) &&
            (_adaptors[uiIndex].pAdaptor->getAdaptorType() == type) &&
            _adaptors[uiIndex].pAdaptor->supportsDirectConnection()) {

            ConnCommAdaptor *pConnCommAdpt = (ConnCommAdaptor*)_adaptors[uiIndex].pAdaptor;
            int rc = pConnCommAdpt->connectToPeer (pszRemoteAddr, ui16Port);
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not connect to %s:%u. Returned %d.\n",
                                pszRemoteAddr, ui16Port, rc);
                return -3;
            }
        }
    }

    return 0;
}

void CommAdaptorManager::resetTransmissionCounters (void)
{
    _m.lock (2043);
    for (unsigned int uiId = 0; uiId < _adaptors.size(); uiId++) {
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            _adaptors[uiId].pAdaptor->resetTransmissionCounters();
        }
    }
    _m.unlock (2043);
}

int CommAdaptorManager::sendContextUpdateMessage (const void *pBuf, uint32 ui32Len, Targets **ppTargets)
{
    if (pBuf == nullptr || ui32Len == 0) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendContextUpdateMessage";

    _m.lock (2044);
    int rc = 0;
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendContextUpdateMessage (pBuf, ui32Len,
                                                                           (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                           (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2044);
    return rc;
}

int CommAdaptorManager::sendContextVersionMessage (const void *pBuf, uint32 ui32Len, Targets **ppTargets)
{
    if (pBuf == nullptr || ui32Len == 0) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendContextVersionMessage";

    int rc = 0;
    _m.lock (2045);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendContextVersionMessage (pBuf, ui32Len,
                                                                            (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                            (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2045);
    return rc;
}

int CommAdaptorManager::broadcastDataMessage (Message *pMsg)
{
    const char *pszMethodName = "CommAdaptorManager::broadcastDataMessage";
    int rc = 0;
    _m.lock (2046);
    const unsigned int uiLen = _adaptors.size();
    for (unsigned int uiId = 0; uiId < uiLen; uiId++) {
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendDataMessage (pMsg, nullptr, nullptr);
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }
    _m.unlock (2046);
    return rc;
}

int CommAdaptorManager::sendDataMessage (Message *pMsg, Targets **ppTargets)
{
    if (pMsg == nullptr) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendDataMessage";

    UInt32Hashtable<Targets> ht (true);
    mergeTargetsByAdaptorId (ppTargets, ht);

    int rc = 0;
    _m.lock (2046);
    for (auto iter = ht.getAllElements(); !iter.end(); iter.nextElement()) {
        const AdaptorId uiId = iter.getValue()->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendDataMessage (pMsg,
                                                                   const_cast<const char **> (iter.getValue()->getTargetNodeIds()),
                                                                   const_cast<const char **> (iter.getValue()->getInterfaces()));
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2046);
    return rc;
}

int CommAdaptorManager::sendChunkedMessage (Message *pMsg, const char *pszDataMimeType, Targets **ppTargets)
{
    if (pMsg == nullptr || pszDataMimeType == nullptr) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendChunkedMessage";

    int rc = 0;
    _m.lock (2047);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendChunkedMessage (pMsg, pszDataMimeType,
                                                                      (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                      (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2047);
    return rc;
}

int CommAdaptorManager::sendMessageRequestMessage (const char *pszMsgId, const char *pszPublisherNodeId, Targets **ppTargets)
{
    if (pszMsgId == nullptr) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendMessageRequestMessage";

    int rc = 0;
    _m.lock (2048);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendMessageRequestMessage (pszMsgId, pszPublisherNodeId,
                                                                             (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                             (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2048);
    return rc;
}

int CommAdaptorManager::sendChunkRequestMessage (const char *pszMsgId, DArray<uint8> *pCachedChunks,
                                                 const char *pszPublisherNodeId, Targets **ppTargets)
{
    if (pszMsgId == nullptr) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendChunkRequestMessage";

    int rc = 0;
    _m.lock (2049);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendChunkRequestMessage (pszMsgId, pCachedChunks, pszPublisherNodeId,
                                                                           (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                           (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2049);
    return rc;
}

int CommAdaptorManager::sendSearchMessage (SearchProperties &searchProp, Targets **ppTargets)
{
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendSearchMessage";

    int rc = 0;

    _m.lock (2050);

    // If DisService is running, DisService should be called first, so that it
    // can generate the ID for the search query
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr &&
            _adaptors[uiId].pAdaptor->getAdaptorType() == DISSERVICE) {

            int rcTmp = _adaptors[uiId].pAdaptor->sendSearchMessage (searchProp,
                                                                     (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                     (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
            if (searchProp.pszQueryId == nullptr) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning,
                                "the disservice adaptor did not generate the search query id");
            }
            break;  // There can be only one DisService adaptor
        }
    }

    if (searchProp.pszQueryId == nullptr) {
        searchProp.pszQueryId = SearchService::getSearchId (searchProp.pszGroupName, _nodeId.c_str(), _pPropertyStore);
        if (searchProp.pszQueryId == nullptr) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not generate the search query id");
            _m.unlock (2050);
            return -2;
        }
    }

    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr &&
            _adaptors[uiId].pAdaptor->getAdaptorType() != DISSERVICE) {

            int rcTmp = _adaptors[uiId].pAdaptor->sendSearchMessage (searchProp,
                                                                     (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                     (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -3;
                }
            }
        }
    }

    _m.unlock (2050);
    return rc;
}

int CommAdaptorManager::sendSearchReplyMessage (const char *pszQueryId, const char **ppszMatchingMsgIds,
                                                const char *pszTarget,  const char *pszMatchingNode,
                                                Targets **ppTargets)
{
    if (pszQueryId == nullptr || ppszMatchingMsgIds == nullptr || pszMatchingNode == nullptr) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendSearchReplyMessage (1)";

    int rc = 0;
    _m.lock (2051);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendSearchReplyMessage (pszQueryId, ppszMatchingMsgIds, pszTarget, pszMatchingNode,
                                                                          (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                          (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2051);
    return rc;
}

int CommAdaptorManager::sendSearchReplyMessage (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen,
                                                const char *pszTarget, const char *pszMatchingNode,
                                                Targets **ppTargets)
{
    if (pszQueryId == nullptr || pReply == nullptr || ui16ReplyLen == 0 || pszMatchingNode == nullptr) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendSearchReplyMessage (2)";

    int rc = 0;
    _m.lock (2051);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendVolatileSearchReplyMessage (pszQueryId, pReply, ui16ReplyLen,
                                                                                  pszTarget, pszMatchingNode,
                                                                                  (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                                  (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2051);
    return rc;
}

int CommAdaptorManager::sendPositionMessage (const void *pBuf, uint32 ui32Len, Targets **ppTargets)
{
    if (pBuf == nullptr || ui32Len == 0) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendPositionMessage";

    int rc = 0;
    _m.lock (2052);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendPositionMessage (pBuf, ui32Len,
                                                                      (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                      (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2052);
    return rc;
}

int CommAdaptorManager::sendTopologyReplyMessage (const void *pBuf, uint32 ui32Len, Targets **ppTargets)
{
    if (pBuf == nullptr || ui32Len == 0) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendTopologyReplyMessage";

    int rc = 0;
    _m.lock (2053);
    // TODO: check if possible to optimize the process in case of the DisService adaptor to send only one TopologyReplyMessage
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendTopologyReplyMessage (pBuf, ui32Len,
                                                                            (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                            (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2053);
    return rc;
}

int CommAdaptorManager::sendTopologyRequestMessage (const void *pBuf, uint32 ui32Len, Targets **ppTargets)
{
    if (pBuf == nullptr || ui32Len == 0) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendTopologyRequestMessage";

    int rc = 0;
    _m.lock (2054);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendTopologyRequestMessage (pBuf, ui32Len,
                                                                              (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                              (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2054);
    return rc;
}

int CommAdaptorManager::sendUpdateMessage (const void *pBuf, uint32 ui32Len, const char *pszPublisherNodeId, Targets **ppTargets)
{
    if (pBuf == nullptr || ui32Len == 0) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendUpdateMessage";

    int rc = 0;
    _m.lock (2055);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendUpdateMessage (pBuf, ui32Len, pszPublisherNodeId,
                                                                     (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                     (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2055);
    return rc;
}

int CommAdaptorManager::sendVersionMessage (const void *pBuf, uint32 ui32Len, const char *pszPublisherNodeId, Targets **ppTargets)
{
    if (pBuf == nullptr || ui32Len == 0) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendVersionMessage";

    int rc = 0;
    _m.lock (2056);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendVersionMessage (pBuf, ui32Len, pszPublisherNodeId,
                                                                      (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                      (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2056);
    return rc;
}

int CommAdaptorManager::sendWaypointMessage (const void *pBuf, uint32 ui32Len, const char *pszPublisherNodeId, Targets **ppTargets)
{
    if (pBuf == nullptr || ui32Len == 0) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendWaypointMessage";

    UInt32Hashtable<Targets> ht (true);
    mergeTargetsByAdaptorId (ppTargets, ht);

    int rc = 0;
    _m.lock (2057);
    UInt32Hashtable<Targets>::Iterator iter = ht.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        AdaptorId uiId = iter.getValue()->adaptorId;
        if (_adaptors.used (uiId) && (_adaptors[uiId].pAdaptor != nullptr)) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendWaypointMessage (pBuf, ui32Len, pszPublisherNodeId,
                                                                       (const char **) iter.getValue()->getTargetNodeIds(),
                                                                       (const char **) iter.getValue()->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2057);
    return rc;
}

int CommAdaptorManager::sendWholeMessage (const void *pBuf, uint32 ui32Len, const char *pszPublisherNodeId, Targets **ppTargets)
{
    if (pBuf == nullptr || ui32Len == 0) {
        return -1;
    }
    if (ppTargets == nullptr) {
        return 0;
    }

    const char *pszMethodName = "CommAdaptorManager::sendWholeMessage";

    int rc = 0;
    _m.lock (2058);
    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        AdaptorId uiId = ppTargets[i]->adaptorId;
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->sendWholeMessage (pBuf, ui32Len, pszPublisherNodeId,
                                                                    (const char **) ppTargets[i]->getTargetNodeIds(),
                                                                    (const char **) ppTargets[i]->getInterfaces());
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2058);
    return rc;
}

int CommAdaptorManager::notifyEvent (const void *pBuf, uint32 ui32Len, const char *pszPublisherNodeId, const char *pszTopic)
{
    if ((pBuf == nullptr) || (ui32Len == 0)) {
        return -1;
    }

    const char *pszMethodName = "CommAdaptorManager::notifyEvent";

    int rc = 0;
    _m.lock (2063);
    for (unsigned int uiId = 0; uiId < _adaptors.size (); uiId++) {

        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            int rcTmp = _adaptors[uiId].pAdaptor->notifyEvent (pBuf, ui32Len, pszPublisherNodeId, pszTopic, nullptr);
            if (rcTmp < 0) {
                checkAndLogMsg (pszMethodName, unconfiguredAdaptor,
                                _adaptors[uiId].pAdaptor->getAdaptorAsString(), rcTmp);
                if (rc == 0) {
                    rc = -1;
                }
            }
        }
    }

    _m.unlock (2063);
    return rc;
}

int CommAdaptorManager::subscribe (CommAdaptor::Subscription &sub)
{
    unsigned int uiCount = 0;
    _m.lock (2062);
    const unsigned int uiLen = _adaptors.size();
    for (unsigned int uiId = 0; uiId < uiLen; uiId++) {
        if (_adaptors.used (uiId) && _adaptors[uiId].pAdaptor != nullptr) {
            _adaptors[uiId].pAdaptor->subscribe (sub);
            uiCount++;
        }
    }
    _m.unlock (2062);
    return (uiCount > 0 ? 0 : -1);
}

bool CommAdaptorManager::adaptorSupportsCaching (AdaptorId adaptorId)
{
    _m.lock (2059);
    long lHighestIndex = _adaptors.getHighestIndex();
    if (lHighestIndex < 0) {
        _m.unlock (2059);
        return false;
    }
    if (adaptorId > (unsigned int) lHighestIndex) {
        _m.unlock (2059);
        return false;
    }
    bool bSupportsCaching = _adaptors[adaptorId].pAdaptor->supportsCaching();
    _m.unlock (2059);
    return bSupportsCaching;
}

AdaptorType CommAdaptorManager::getAdaptorType (AdaptorId adaptorId)
{
    _m.lock (2060);
    long lHighestIndex = _adaptors.getHighestIndex();
    if (lHighestIndex < 0) {
        _m.unlock (2060);
        return UNKNOWN;
    }
    if (adaptorId > (unsigned int) lHighestIndex) {
        _m.unlock (2060);
        return UNKNOWN;
    }
    AdaptorType type = _adaptors[adaptorId].pAdaptor->getAdaptorType();
    _m.unlock (2060);
    return type;
}

int CommAdaptorManager::getDisServiceAdaptorId (AdaptorId &adaptorId)
{
    adaptorId = DISSERVICE_ADAPTOR_ID;
    return 0;
}
