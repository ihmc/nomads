/*
 * NodeContextManager.cpp
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

#include "NodeContextManager.h"

#include "Defs.h"
#include "LocalNodeContext.h"
#include "MetadataConfigurationImpl.h"
#include "NodePath.h"
#include "CommAdaptorManager.h"
#include "Versions.h"

#include "C45AVList.h"

#include "BufferWriter.h"
#include "BufferReader.h"
#include "InstrumentedReader.h"
#include "Json.h"
#include "Logger.h"


#include <string.h>

using namespace NOMADSUtil;
using namespace IHMC_VOI;

namespace NODE_CONTEXT_MANAGER
{
    static const char * NODE_TO_FORWARD_TO = "recipient";

    int writeVersions (Writer *pWriter, const JsonObject *pVersions, const char *pszRecepient)
    {
        if (pVersions == nullptr) {
            return -1;
        }
        JsonObject *pVersionsMsg = (JsonObject*)pVersions->clone();
        if (pVersionsMsg == nullptr) {
            return -2;
        }
        if (pVersionsMsg->setString (NODE_CONTEXT_MANAGER::NODE_TO_FORWARD_TO, pszRecepient) < 0) {
            delete pVersionsMsg;
            return -3;
        }
        if (pVersionsMsg->write (pWriter, true) < 0) {
            delete pVersionsMsg;
            return -4;
        }
        delete pVersionsMsg;
        return 0;
    }
}

using namespace IHMC_ACI;
using namespace IHMC_C45;

NodeContextManager::NodeContextManager (const char *pszNodeId, LocalNodeContext *pLocalNodeContext)
    : _pLocalNodeContext (pLocalNodeContext),
      _pPeerNodeContextList (new PeerNodeContextList()),
      _uiActivePeerNumber (0U),
      _pCommMgr (nullptr),
      _pMetadataConf (nullptr),
      _pTopology (nullptr),
      _nodeId (pszNodeId),
      _mLocal (MutexId::NodeContextManager_mLocal, LOG_MUTEX),
      _mPeer (MutexId::NodeContextManager_mPeer, LOG_MUTEX)
{
}

NodeContextManager::~NodeContextManager (void)
{
    if (_pPeerNodeContextList != nullptr) {
        PeerNodeContext *pCurr, *pNext;
        pNext = _pPeerNodeContextList->getFirst();
        while ((pCurr = pNext) != nullptr) {
            pNext = _pPeerNodeContextList->getNext();
            delete _pPeerNodeContextList->remove (pCurr);
        }
        delete _pPeerNodeContextList;
        _pPeerNodeContextList = nullptr;
    }
}

int NodeContextManager::configure (CommAdaptorManager *pCommMgr, Topology *pTopology,
                                   MetadataConfigurationImpl *pMetadataConf)
{
    _pCommMgr = pCommMgr;
    _pTopology = pTopology;
    _pMetadataConf = pMetadataConf;
    return 0;
}

void NodeContextManager::setBatteryLevel (unsigned int uiBattery)
{
    _mLocal.lock (1087);
    _pLocalNodeContext->setBatteryLevel (uiBattery);
    _mLocal.unlock (1087);
}

void NodeContextManager::setMemoryAvailable (unsigned int uiMemory)
{
    _mLocal.lock (1088);
    _pLocalNodeContext->setMemoryAvailable (uiMemory);
    _mLocal.unlock (1088);
}

int NodeContextManager::registerPath (NodePath *pPath)
{
    _mLocal.lock (1089);
    int rc = _pLocalNodeContext->addPath (pPath);
    _mLocal.unlock (1089);
    return rc;
}

int NodeContextManager::setCurrentPath (const char *pszPathId)
{
    if (pszPathId == nullptr) {
        return -1;
    }
    _mLocal.lock (1062);
    const Versions versions (_pLocalNodeContext->getVersions());
    int rc = _pLocalNodeContext->setCurrentPath (pszPathId);
    if (rc != 0) {
        _mLocal.unlock (1062);
        return -2;
    }
    _mLocal.unlock (1062);
    return localContextHasChanged (versions);
}

int NodeContextManager::newPeer (const char *pszNodeId)
{
    if (pszNodeId == nullptr) {
        return -1;
    }

    BufferWriter bw (1024, 128);
    int rc = newPeer (pszNodeId, &bw);
    if (rc < 0) {
        return -2;
    }
    TargetPtr targets[2] = {_pTopology->getNextHopAsTarget (pszNodeId), nullptr};
    if (targets[0] == nullptr) {
        return 0;
    }
    if ((rc = _pCommMgr->sendVersionMessage (bw.getBuffer(), bw.getBufferLength(),
                                             _nodeId, targets)) < 0) {
        return -3;
    }
    delete targets[0];

    return 0;
}

int NodeContextManager::newPeer (const char *pszNodeId, Writer *pWriter)
{
    const char *pszMethodName = "NodeContextManager::newPeer";
    if ((pszNodeId == nullptr) || (pWriter == nullptr)) {
        return -1;
    }

    _mPeer.lock (1045);
    PeerNodeContext *pCurrPeer = getNodeContextInternal (pszNodeId);
    if (pCurrPeer == nullptr) {
        // this is a new peer never seen before
        pCurrPeer = new PeerNodeContext (pszNodeId, _pMetadataConf->getMetadataAsStructure(),
            NodeContext::TOO_FAR_FROM_PATH_COEFF,
            NodeContext::APPROXIMABLE_TO_POINT_COEFF);
        if (pCurrPeer == nullptr) {
            _mPeer.unlock (1045);
            checkAndLogMsg (pszMethodName, memoryExhausted);
            return -2;
        }
        _uiActivePeerNumber++;
        _pPeerNodeContextList->append (pCurrPeer);
        checkAndLogMsg (pszMethodName, Logger::L_Info, "new peer with id = <%s> appeared\n",
                        pszNodeId);
    }
    else {
        if (!pCurrPeer->isPeerActive()) {
            _uiActivePeerNumber++;
        }
        // this is a peer already seen before
        checkAndLogMsg (pszMethodName, Logger::L_Info, "old peer with id = <%s> is again in contact\n",
                        pszNodeId);
    }

    pCurrPeer->setPeerPresence (true);
    Versions currVersions (pCurrPeer->getVersions());
    _mPeer.unlock (1045);

    if (NODE_CONTEXT_MANAGER::writeVersions (pWriter, currVersions.toJson(), pszNodeId) < 0) {
        return -3;
    }
    return 0;
}

void NodeContextManager::deadPeer (const char *pszNodeID)
{
    const char *pszMethodName = "NodeContextManager::deadPeer";
    if (pszNodeID == nullptr) {
        return;
    }
    _mPeer.lock (1046);
    PeerNodeContext *pCurrPeer = getNodeContextInternal (pszNodeID);
    if (pCurrPeer == nullptr) {
        _mPeer.unlock (1046);
        return;
    }

    if (!pCurrPeer->isPeerActive()) {
        // The peer has already been deactivated - no need to do it again
        _mPeer.unlock (1046);
        return;
    }

    pCurrPeer->setPeerPresence (false);
    if (_uiActivePeerNumber > 0) {
        _uiActivePeerNumber--;
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "trying to decrement "
                        "_uiActivePeerNumber which is already 0\n", pszNodeID);
    }
    _mPeer.unlock (1046);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "peer with id = <%s> disappeared\n",
                    pszNodeID);
}

int NodeContextManager::versionsMessageArrived (const void *pData, uint32 ui32DataLength,
                                                const char *pszPublisherNodeId, String &nodeToForwardTo)
{
    const char *pszMethodName = "NodeContextManager::versionsMessageArrived";
    if (pszPublisherNodeId == nullptr || pData == nullptr || ui32DataLength == 0) {
        return -1;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Versions message arrived.\n");

    // Get the peer's version of my info, path, waypoint and tree
    BufferReader br (pData, ui32DataLength);
    InstrumentedReader ir (&br);
    JsonObject json;
    int rc = json.read (&ir, true);
    ui32DataLength -= ir.getBytesRead();
    if (rc < 0) {
        return -2;
    }

    String s;
    json.getString (NODE_CONTEXT_MANAGER::NODE_TO_FORWARD_TO, s);
    if (s != _nodeId) {
        nodeToForwardTo = s;
        return 0;
    }

    TargetPtr targets[2] = {_pTopology->getNextHopAsTarget (pszPublisherNodeId), nullptr};
    if (targets[0]== nullptr) {
        return 0;
    }

    bool bContainsVersions = true;
    Versions v;
    if (v.fromJson (&json) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "it is not possible to read the received message.\n");
        delete targets[0];
        return -2;
    }
    const bool bPeerHasNoVersions = !bContainsVersions;
    const Versions readVersions (v);

    _mLocal.lock (1047);

    const int64 i64CurrStartTimeForPeer = (_pLocalNodeContext != nullptr ? _pLocalNodeContext->getStartTime() : 0U);
    const bool bPeerHasRestarted = (i64CurrStartTimeForPeer != readVersions._i64StartingTime);
    if (bPeerHasRestarted) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "peer has re-started. Current Start Time: <%lld>"
                        "Old Start Time <%lld>.\n", i64CurrStartTimeForPeer, readVersions._i64StartingTime);
    }

    const Versions *pVersions = (bPeerHasNoVersions || bPeerHasRestarted) ? nullptr : &readVersions;
    JsonObject *pJson = _pLocalNodeContext->toJson (pVersions);
    if (pJson == nullptr) {
        delete targets[0];
        _mLocal.unlock (1047);
        return -3;
    }
    if (bPeerHasNoVersions || bPeerHasRestarted) {
        // Prepend message target
        pJson->setString (NODE_CONTEXT_MANAGER::NODE_TO_FORWARD_TO, pszPublisherNodeId);
    }
    BufferWriter bw (1024, 128);
    rc = pJson->write (&bw, true);
    delete pJson;
    if (rc < 0) {
        _mLocal.unlock (1047);
        delete targets[0];
        return -4;
    }

    rc = 0;
    if (bPeerHasNoVersions || bPeerHasRestarted) {
        if (bPeerHasNoVersions) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "writing whole message because of empty version message.\n");
        }
        else if (bPeerHasRestarted) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "writing whole message because peer has restarted.\n");
        }
        // Send "whole message"
        rc = _pCommMgr->sendWholeMessage (bw.getBuffer(), bw.getBufferLength(), _nodeId, targets);
    }
    else {
        rc = _pCommMgr->sendUpdateMessage (bw.getBuffer(), bw.getBufferLength(), _nodeId, targets);
    }

    _mLocal.unlock (1047);
    delete targets[0];
    return rc == 0 ? 0 : -5;
}

int NodeContextManager::nodeContextMessageArrived (const void *pData, uint32 ui32DataLength,
                                                   const char *pszPublisherNodeId,
                                                   bool &bContextUnsynchronized, bool &bNodeContextUpdated,
                                                   String &nodeToForwardTo)
{
    const char *pszMethodName = "NodeContextManager::updatesMessageArrived";
    bContextUnsynchronized = bNodeContextUpdated = false;
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Updates message arrived from sender with id = <%s>.\n", pszPublisherNodeId);

    _mPeer.lock (1048);
    PeerNodeContext *pCurrPeer = getNodeContextInternal (pszPublisherNodeId);
    if (pCurrPeer == nullptr) {
        pCurrPeer = new PeerNodeContext (pszPublisherNodeId, _pMetadataConf->getMetadataAsStructure(),
                                         NodeContext::TOO_FAR_FROM_PATH_COEFF,
                                         NodeContext::APPROXIMABLE_TO_POINT_COEFF);
        if (pCurrPeer == nullptr) {
            _mPeer.unlock (1048);
            checkAndLogMsg (pszMethodName, memoryExhausted);
            return -3;
        }
        pCurrPeer->setPeerPresence (true);
        _uiActivePeerNumber++;
        _pPeerNodeContextList->prepend (pCurrPeer);
        bNodeContextUpdated = true;
    }
    Versions oldVersions (pCurrPeer->getVersions());
    BufferReader br (pData, ui32DataLength);
    JsonObject json;
    if (json.read (&br, true) < 0) {
        _mPeer.unlock (1048);
        return -2;
    }
    json.getString (NODE_CONTEXT_MANAGER::NODE_TO_FORWARD_TO, nodeToForwardTo);
    JsonObject *pIncomingVersions = json.getObject (Versions::VERSIONS_OBJECT_NAME);
    Versions incomingVersions;
    int rc = incomingVersions.fromJson (pIncomingVersions);
    delete pIncomingVersions;
    if (rc < 0) {
        _mPeer.unlock (1048);
        return -3;
    }
    bool bPeerRestarted = incomingVersions._i64StartingTime > oldVersions._i64StartingTime;
    if (bPeerRestarted) {
        pCurrPeer->reset();
        oldVersions.reset();
    }
    // set this to 0, just in case it is never written anymore.
    rc = 0;
    BufferWriter bw (1024, 128);

    if (bPeerRestarted || incomingVersions.greaterThan (oldVersions, false)) {
        // Read new versions if newer
        if (pCurrPeer->fromJson (&json)) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not read the update message "
                "arrived from sender with id = <%s>;\n", pszPublisherNodeId);
            _mPeer.unlock (1048);
            return -4;
        }
        bNodeContextUpdated = true;
        Versions currVersions (pCurrPeer->getVersions());

        // If there still is missing context, send versions
        if (incomingVersions.greaterThan (currVersions, true)) {
            if (NODE_CONTEXT_MANAGER::writeVersions (&bw, currVersions.toJson(), pszPublisherNodeId) < 0) {
                _mPeer.unlock (1048);
                return -5;
            }
            TargetPtr targets[2];
            targets[0] = _pTopology->getNextHopAsTarget (pszPublisherNodeId);
            targets[1] = nullptr;
            if (targets[0] != nullptr) {
                rc = _pCommMgr->sendVersionMessage (bw.getBuffer(), bw.getBufferLength(), _nodeId, targets);
                if (rc < 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_Warning, "Could not send version message "
                                    "to sender with id = <%s>; rc = %lld\n", pszPublisherNodeId,
                                    static_cast<int>(rc));
                }
            }
            delete targets[0];
        }

        const String peerNodeId (pCurrPeer->getNodeId());
        NodePath *pPeerNodePath = pCurrPeer->getPath();
        const int iPathLen = (pPeerNodePath == nullptr ? -1 : (pPeerNodePath->getPathLength()));
        JsonObject *pjson = (pPeerNodePath == nullptr ? nullptr : pPeerNodePath->toJson());
        String p;
        if (pjson != nullptr) {
            p = pjson->toString();
        }
        _mPeer.unlock (1048);
        checkAndLogMsg (pszMethodName, Logger::L_Info, "received a message from node <%s>; path length is %d\n",
                        peerNodeId.length() > 0 ? peerNodeId.c_str() : "NULL", iPathLen);
        checkAndLogMsg (pszMethodName, Logger::L_Info, "path: %s\n", p.c_str());
        return (rc == 0 ? 0 : -6);
    }
    if ((incomingVersions._i64StartingTime == oldVersions._i64StartingTime) &&
             oldVersions.greaterThan (incomingVersions, true)) {
        _mPeer.unlock (1048);
        // Stale versions
        return -5;
    }

    _mPeer.unlock (1048);
    return 0;
}

int NodeContextManager::localContextHasChanged (const Versions &versions)
{
    const char *pszMethodName = "NodeContextManager::localContextHasChanged";
    Targets **ppTargets = _pTopology->getNeighborsAsTargets();
    if ((ppTargets == nullptr) || (ppTargets[0] == nullptr)) {
        Targets::deallocateTargets (ppTargets);
        return 0;
    }

    _mLocal.lock (1051);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "The local context has changed.\n");

    BufferWriter bw (1024, 128);
    JsonObject *pJson = _pLocalNodeContext->toJson (&versions);
    if (pJson == nullptr) {
        _mLocal.unlock (1051);
        Targets::deallocateTargets (ppTargets);
        return -1;
    }
    int rc = pJson->write (&bw, true);
    delete pJson;
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "Could not write the update message. Returned %d\n", (int) rc);
        _mLocal.unlock (1051);
        Targets::deallocateTargets (ppTargets);
        return -2;
    }
    _mLocal.unlock (1051);

    rc = _pCommMgr->sendUpdateMessage (bw.getBuffer(), bw.getBufferLength(), _nodeId, ppTargets);
    Targets::deallocateTargets (ppTargets);
    return (rc == 0 ? 0 : -3);
}

int NodeContextManager::updateClassifier (IHMC_C45::C45AVList *pDataset)
{
    uint16 ui16PrevClassifierVersion = _pLocalNodeContext->getClassifierVersion();
    int rc = _pLocalNodeContext->updateClassifier(pDataset);
    if (rc < 0) {
        return -1;
    }
    if (rc <= ui16PrevClassifierVersion) {
        return 0;
    }
    Versions versions (_pLocalNodeContext->getVersions());
    versions._ui16ClassifierVersion = ui16PrevClassifierVersion;
    rc = localContextHasChanged (versions);
    return (rc == 0 ? 0 : -2);
}

int NodeContextManager::updatePosition (Writer *pWriter)
{
    const char *pszMethodName = "NodeContextManager::updatePosition";
    if (pWriter == nullptr) {
        return -1;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "The actual way point in path has changed.\n");
    _mPeer.lock (1056);
    if ((_pPeerNodeContextList == nullptr) || (_uiActivePeerNumber == 0)) {
        _mPeer.unlock (1056);
        return 0;
    }
    _mPeer.unlock (1056);

    _mLocal.lock (1057);
    JsonObject *pJson = _pLocalNodeContext->toJson (nullptr, true);
    if (pJson == nullptr) {
        delete pJson;
        _mLocal.unlock (1057);
        return -3;
    }
    int rc = pJson->write (pWriter, true);
    delete pJson;
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "Could not write the way point message.\n");
        _mLocal.unlock (1057);
        return -4;
    }
    _mLocal.unlock (1057);
    return 0;
}

LocalNodeContext * NodeContextManager::getLocalNodeContext (void)
{
    _mLocal.lock (1062);
    return _pLocalNodeContext;
}

void NodeContextManager::releaseLocalNodeContext()
{
    _mLocal.unlock (1062);
}

PeerNodeContextList * NodeContextManager::getPeerNodeContextList (void)
{
    _mPeer.lock (1063);
    return _pPeerNodeContextList;
}

PeerNodeContextList * NodeContextManager::getPeerNodeContextList (const char **ppszPeerNodeIDFilter)
{
    _mPeer.lock (1063);
    if (_pPeerNodeContextList == nullptr || ppszPeerNodeIDFilter == nullptr) {
        return nullptr;
    }
    PeerNodeContextList *pFilteredPeerNodeContext = new PeerNodeContextList();
    if (pFilteredPeerNodeContext != nullptr) {
        PeerNodeContext *pCurrPeerNodeID = _pPeerNodeContextList->getFirst();
        bool bFilter;
        while (pCurrPeerNodeID != nullptr) {
            bFilter = false;
            const String nodeId (pCurrPeerNodeID->getNodeId());
            for (unsigned int i = 0; ppszPeerNodeIDFilter[i] != nullptr; i++) {
                if (0 == strcmp (ppszPeerNodeIDFilter[i], nodeId)) {
                    bFilter = true;
                    break;
                }
            }
            if (!bFilter) {
                pFilteredPeerNodeContext->prepend (pCurrPeerNodeID);
            }
            pCurrPeerNodeID = _pPeerNodeContextList->getNext();
        }
    }
    return pFilteredPeerNodeContext;
}

DArray2<String> * NodeContextManager::getPeerList (bool bNeighborsOnly)
{
    _mPeer.lock (1063);
    if (_pPeerNodeContextList == nullptr) {
        _mPeer.unlock (1063);
        return nullptr;
    }
    int iCount = _pPeerNodeContextList->getCount();
    if (iCount <= 0) {
        _mPeer.unlock (1063);
        return nullptr;
    }

    DArray2<String> *pNeighbors = new DArray2<String> (iCount);
    if (pNeighbors == nullptr) {
        checkAndLogMsg ("NodeContextManager::getPeerList", memoryExhausted);
        _mPeer.unlock (1063);
        return nullptr;
    }

    PeerNodeContext *pCurrPeerNodeContext = _pPeerNodeContextList->getFirst();
    for (unsigned int i = 0; pCurrPeerNodeContext != nullptr;) {
        const String nodeId (pCurrPeerNodeContext->getNodeId());
        if (!bNeighborsOnly || _pTopology->isNeighbor (nodeId)) {
            (*pNeighbors)[i] = nodeId;
            i++;
        }
        pCurrPeerNodeContext = _pPeerNodeContextList->getNext();
    }

    _mPeer.unlock (1063);
    return pNeighbors;
}

PeerNodeContext * NodeContextManager::getPeerNodeContext (const char *pszNodeID)
{
    _mPeer.lock (1063);
    return getNodeContextInternal (pszNodeID);
}

PeerNodeContext * NodeContextManager::getNodeContextInternal (const char *pszNodeId)
{
    if (_pPeerNodeContextList != nullptr) {
        PeerNodeContext *pCurrPeer = _pPeerNodeContextList->getFirst();
        while (pCurrPeer != nullptr) {
            const String nodeId (pCurrPeer->getNodeId());
            if (nodeId == pszNodeId) {
                return pCurrPeer;
            }
            pCurrPeer = _pPeerNodeContextList->getNext();
        }
    }
    return nullptr;
}

void NodeContextManager::releasePeerNodeContextList()
{
    _mPeer.unlock (1063);
}

unsigned int NodeContextManager::getActivePeerNumber()
{
    return _uiActivePeerNumber;
}

DArray2<String> * NodeContextManager::getActivePeerList()
{
    _mPeer.lock (1064);
    if (getActivePeerNumber() == 0) {
        _mPeer.unlock (1064);
        return nullptr;
    }
    DArray2<String> *pPeerList = new DArray2<String> (_uiActivePeerNumber);
    if (pPeerList == nullptr) {
        _mPeer.unlock (1064);
        return nullptr;
    }
    unsigned int count = 0;
    for (PeerNodeContext *pCurrPeer = _pPeerNodeContextList->getFirst(); (pCurrPeer != nullptr)
        && (count < _uiActivePeerNumber); pCurrPeer = _pPeerNodeContextList->getNext()) {
        if (pCurrPeer->isPeerActive()) {
            (*pPeerList)[count] = pCurrPeer->getNodeId();
            count++;
        }
    }
    _mPeer.unlock (1064);
    return pPeerList;
}

/*
char ** NodeContextManager::getReachablePeers (AdaptorId adaptorId, const char **ppszPeerNodeIds)
{
    if (ppszPeerNodeIds == nullptr) {
        return nullptr;
    }
    _mPeer.lock (1065);
    if (getActivePeerNumber() == 0) {
        _mPeer.unlock (1065);
        return nullptr;
    }
    char **ppActivePeersList = (char **) calloc (_uiActivePeerNumber + 1, sizeof(char *));
    int count = 0;
    for (PeerNodeContext *pCurrPeer = _pPeerNodeContextList->getFirst(); pCurrPeer != nullptr;
            pCurrPeer = _pPeerNodeContextList->getNext()) {
        for (unsigned int i = 0; ppszPeerNodeIds[i] != nullptr; i++) {
            const String peerNodeId (ppszPeerNodeIds[i]);
            if (strcmp (pCurrPeer->getNodeId(), peerNodeId) == 0) {
                if (pCurrPeer->isReacheableThrough (adaptorId)) {
                    ppActivePeersList[count] = (char *) pCurrPeer->getNodeId();
                    count++;
                    break;
                }
            }
        }
    }
    ppActivePeersList[count] = nullptr;
    _mPeer.unlock (1065);
    return ppActivePeersList;
}
*/

