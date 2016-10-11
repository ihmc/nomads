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
#include "MetadataConfiguration.h"
#include "NodePath.h"
#include "CommAdaptorManager.h"

#include "C45AVList.h"

#include "BufferWriter.h"
#include "BufferReader.h"
#include "InstrumentedReader.h"
#include "Logger.h"
#include "NLFLib.h"

#include <string.h>

using namespace IHMC_ACI;
using namespace IHMC_C45;
using namespace NOMADSUtil;

NodeContextManager::NodeContextManager (const char *pszNodeId,
                                        LocalNodeContext *pLocalNodeContext)
    : _mLocal (MutexId::NodeContextManager_mLocal, LOG_MUTEX),
      _mPeer (MutexId::NodeContextManager_mPeer, LOG_MUTEX)
{
    _pLocalNodeContext = pLocalNodeContext;
    _pPeerNodeContextList = NULL;
    _uiActivePeerNumber = 0;
    _pMetadataConf = NULL;
    _pTopology = NULL;
    _pszNodeId = strDup (pszNodeId);
    _pPeerNodeContextList = new PeerNodeContextList();
}

NodeContextManager::~NodeContextManager()
{
    if (_pPeerNodeContextList != NULL) {
        PeerNodeContext *pCurr, *pNext;
        pNext = _pPeerNodeContextList->getFirst();
        while ((pCurr = pNext) != NULL) {
            pNext = _pPeerNodeContextList->getNext();
            delete _pPeerNodeContextList->remove (pCurr);
        }
        delete _pPeerNodeContextList;
        _pPeerNodeContextList = NULL;
    }
    free ((char *)_pszNodeId);
}

int NodeContextManager::configure (CommAdaptorManager *pCommMgr, Topology *pTopology,
                                   MetadataConfiguration *pMetadataConf)
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
    if (pszPathId == NULL) {
        return -1;
    }
    _mLocal.lock (1062);
    const NodeContext::Versions versions (_pLocalNodeContext->getVersions());
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
    if (pszNodeId == NULL) {
        return -1;
    }

    BufferWriter bw (1024, 128);
    int rc = newPeer (pszNodeId, &bw);
    if (rc < 0) {
        return -2;
    }
    TargetPtr targets[2] = {_pTopology->getNextHopAsTarget (pszNodeId), NULL};
    if (targets[0] == NULL) {
        return 0;
    }
    if ((rc = _pCommMgr->sendVersionMessage (bw.getBuffer(), bw.getBufferLength(),
                                             _pszNodeId, targets)) < 0) {
        return -3;
    }
    delete targets[0];

    return 0;
}

int NodeContextManager::newPeer (const char *pszNodeId, Writer *pWriter)
{
    if ((pszNodeId == NULL) || (pWriter == NULL)) {
        return -1;
    }

    // Write the id of the node for which the versions are being written
    uint32 ui32NodeIdLen = strlen (pszNodeId);
    if (ui32NodeIdLen == 0) {
        return -2;
    }
    pWriter->write32 (&ui32NodeIdLen);
    pWriter->writeBytes (pszNodeId, ui32NodeIdLen);

    _mPeer.lock (1045);
    PeerNodeContext *pCurrPeer = getNodeContextInternal (pszNodeId);
    if (pCurrPeer != NULL) {
        if (!pCurrPeer->isPeerActive()) {
            _uiActivePeerNumber++;
        }
        pCurrPeer->setPeerPresence (true);
        // this is a peer already seen before
        checkAndLogMsg ("NodeContextManager::newPeer", Logger::L_Info,
                        "old peer with id = <%s> is again in contact\n", pszNodeId);
        if (pCurrPeer->writeVersions (pWriter, pCurrPeer->getWriteVersionsLength()) < 0) {
           checkAndLogMsg ("NodeContextManager::newPeer", Logger::L_SevereError,
                           "Could not write a versions message.\n");
           _mPeer.unlock (1045);
           return -1;
        }
    }
    else {
        // this is a new peer never seen before
        pCurrPeer = new PeerNodeContext (pszNodeId, _pMetadataConf->getMetadataAsStructure(),
                                         NodeContext::TOO_FAR_FROM_PATH_COEFF,
                                         NodeContext::APPROXIMABLE_TO_POINT_COEFF);
        if (pCurrPeer == NULL) {
            _mPeer.unlock (1045);
            checkAndLogMsg ("NodeContextManager::newPeer", memoryExhausted);
            return -3;
        }
        pCurrPeer->setPeerPresence (true);
        _uiActivePeerNumber++;
        _pPeerNodeContextList->append (pCurrPeer);
        checkAndLogMsg ("NodeContextManager::newPeer", Logger::L_Info,
                        "new peer with id = <%s> appeared\n", pszNodeId);
        if (pCurrPeer->writeEmptyVersions (pWriter) < 0) {
           checkAndLogMsg ("NodeContextManager::newPeer", Logger::L_SevereError,
                           "Could not write an empty versions message.\n");
           _mPeer.unlock (1045);
           return -1;
        }
    }

    _mPeer.unlock (1045);
    return 0;
}

void NodeContextManager::deadPeer (const char *pszNodeID)
{
    if (pszNodeID == NULL) {
        return;
    }
    _mPeer.lock (1046);
    PeerNodeContext *pCurrPeer = getNodeContextInternal (pszNodeID);
    if (pCurrPeer == NULL) {
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
        checkAndLogMsg ("NodeContextManager::deadPeer", Logger::L_SevereError,
                        "trying to decrement _uiActivePeerNumber which is already 0\n", pszNodeID);
    }
    
    checkAndLogMsg ("NodeContextManager::deadPeer", Logger::L_Info,
                    "peer with id = <%s> disappeared\n", pszNodeID);

    _mPeer.unlock (1046);
}

int NodeContextManager::versionsMessageArrived (const void *pData, uint32 ui32DataLength,
                                                const char *pszPublisherNodeId, char **ppszNodeToForwardTo)
{
    if (ppszNodeToForwardTo != NULL) {
        *ppszNodeToForwardTo = NULL;
    }
    if (pszPublisherNodeId == NULL || pData == NULL || ui32DataLength == 0) {
        return -1;
    }

    checkAndLogMsg ("NodeContextManager::versionsMessageArrived", Logger::L_Info,
                    "Versions message arrived.\n");

    BufferReader br (pData, ui32DataLength);
    // Get the peer's version of my info, path, waypoint and tree
    uint32 ui32VersionMessageTargetIdLen = 0;
    br.read32 (&ui32VersionMessageTargetIdLen);
    ui32DataLength -= 4;
    char *pszVersionMessageTargetId = (char *) calloc (ui32VersionMessageTargetIdLen+1, sizeof (char));
    br.readBytes (pszVersionMessageTargetId, ui32VersionMessageTargetIdLen);
    ui32DataLength -= ui32VersionMessageTargetIdLen;

    if (strcmp (pszVersionMessageTargetId, _pszNodeId) != 0 && ppszNodeToForwardTo != NULL) {
        *ppszNodeToForwardTo = pszVersionMessageTargetId;
        return 0;
    }

    TargetPtr targets[2] = {_pTopology->getNextHopAsTarget (pszPublisherNodeId), NULL};
    if (targets[0]== NULL) {
        return 0;
    }

    bool bContainsVersions = true;
    NodeContext::Versions v;
    int64 i = NodeContext::readVersions (&br, ui32DataLength, bContainsVersions, v);
    if (i < 0) {
        checkAndLogMsg ("NodeContextManager::versionsMessageArrived", Logger::L_SevereError,
                        "it is not possible to read the received message.\n");
        delete targets[0];
        return -2;
    }
    const bool bPeerHasNoVersions = !bContainsVersions;
    const NodeContext::Versions readVersions (v);

    BufferWriter bw (1024, 128);

    _mLocal.lock (1047);

    const uint32 ui32CurrStartTimeForPeer = (_pLocalNodeContext != NULL ? _pLocalNodeContext->getStartTime() : 0U);
    const bool bPeerHasRestarted = (ui32CurrStartTimeForPeer != readVersions._ui32StartingTime);
    if (bPeerHasRestarted) {
        checkAndLogMsg ("NodeContextManager::versionsMessageArrived", Logger::L_Info,
                        "peer has re-started. Current Start Time: <%u> Old Start Time <%u>.\n",
                        ui32CurrStartTimeForPeer, readVersions._ui32StartingTime);
    }

    int rc = 0;
    if (bPeerHasNoVersions || bPeerHasRestarted) {

        // Prepend message target
        uint32 ui32Len = (pszPublisherNodeId == NULL ? 0 : strlen (pszPublisherNodeId));
        bw.write32 (&ui32Len);
        if (ui32Len > 0) {
            bw.writeBytes (pszPublisherNodeId, ui32Len);
        }

        // Write actual "whole message"
        ui32Len = (uint32) _pLocalNodeContext->getWriteAllLength();
        i = _pLocalNodeContext->writeAll (&bw, ui32Len + 10);
        if (i < 0) {
            checkAndLogMsg ("NodeContextManager::versionsMessageArrived", Logger::L_SevereError,
                            "it is not possible to write a whole message.\n");
            _mLocal.unlock (1047);
            delete targets[0];
            return -3;
        }
        else if (bPeerHasNoVersions) {
            checkAndLogMsg ("NodeContextManager::versionsMessageArrived", Logger::L_Info,
                            "writing whole message because of empty version message.\n");
        }
        else if (bPeerHasRestarted) {
            checkAndLogMsg ("NodeContextManager::versionsMessageArrived", Logger::L_Info,
                            "writing whole message because peer has restarted.\n");
        }

        // Send "whole message"
        rc = _pCommMgr->sendWholeMessage (bw.getBuffer(), bw.getBufferLength(), _pszNodeId, targets);
    }
    else {
        const uint32 ui32Len = (uint32) _pLocalNodeContext->getWriteUpdatesLength (readVersions);
        if (ui32Len <= 0) {
            _mLocal.unlock (1047);
            delete targets[0];
            return 0;
        }

        i = _pLocalNodeContext->writeUpdates (&bw, ui32Len, readVersions);
        if (i < 0) {
            checkAndLogMsg ("NodeContextManager::versionsMessageArrived", Logger::L_SevereError,
                            "it is not possible to write update message.\n");
            _mLocal.unlock (1047);
            delete targets[0];
            return -4;
        }

        rc = _pCommMgr->sendUpdateMessage (bw.getBuffer(), bw.getBufferLength(), _pszNodeId, targets);
    }

    _mLocal.unlock (1047);             
    delete targets[0];
    return rc == 0 ? 0 : -5;
}

int NodeContextManager::updatesMessageArrived (const void *pData, uint32 ui32DataLength,
                                               const char *pszPublisherNodeID,
                                               bool &bContextUnsynchronized)
{
    bContextUnsynchronized = false;
    checkAndLogMsg ("NodeContextManager::updatesMessageArrived", Logger::L_Info,
                    "Updates message arrived from sender with id = <%s>.\n", pszPublisherNodeID);

    _mPeer.lock (1048);
    PeerNodeContext *pCurrPeer = getNodeContextInternal (pszPublisherNodeID);
    if (pCurrPeer == NULL) {
        _mPeer.unlock (1048);
        return -1;
    }

    BufferReader br (pData, ui32DataLength);
    int64 rc = pCurrPeer->readUpdates (&br, ui32DataLength,
                                       bContextUnsynchronized);
    if (rc < 0) {
        checkAndLogMsg ("NodeContextManager::updatesMessageArrived", Logger::L_SevereError,
                        "Could not read the update message arrived from sender with id = <%s>; rc = %d\n",
                        pszPublisherNodeID, (int) rc);
        _mPeer.unlock (1048);
        return -2;
    }
    else {
        // set this to 0, just in case it is never written anymore.
        rc = 0;
    }

    BufferWriter bw (1024, 128);
    if (bContextUnsynchronized) {
        // Write the id of the node for which the versions are being written
        uint32 ui32NodeIdLen = strlen (pszPublisherNodeID);
        if (pszPublisherNodeID == 0) {
            _mPeer.unlock (1048);
            return -3;
        }
        bw.write32 (&ui32NodeIdLen);
        bw.writeBytes (pszPublisherNodeID, ui32NodeIdLen);

        uint32 ui32 = pCurrPeer->getWriteVersionsLength();
        if (pCurrPeer->writeVersions (&bw, ui32) < 0) {
            _mPeer.unlock (1048);
            return -4;
        }
        TargetPtr targets[2];
        targets[0] = _pTopology->getNextHopAsTarget (pszPublisherNodeID);
        targets[1] = NULL;
        if (targets[0] != NULL) {
            rc = _pCommMgr->sendVersionMessage (bw.getBuffer(), bw.getBufferLength(), _pszNodeId, targets);
            if (rc < 0) {
                checkAndLogMsg ("NodeContextManager::updatesMessageArrived", Logger::L_Warning,
                                "Could not send version message to sender with id = <%s>; rc = %lld\n",
                                pszPublisherNodeID, (int) rc);
            }
        }
        delete targets[0];
    }
    const char *pszPeerNodeID = pCurrPeer->getNodeId();
    NodePath *pPeerNodePath = pCurrPeer->getPath();
    checkAndLogMsg ("NodeContextManager::updatesMessageArrived", Logger::L_Info,
                    "received a message from node <%s>; path length is %d\n",
                    pszPeerNodeID != NULL ? pszPeerNodeID : "null",
                    pPeerNodePath != NULL ? (int) pPeerNodePath->getPathLength() : -1);

    _mPeer.unlock (1048);
    return (rc == 0 ? 0 : -5);    
}

int NodeContextManager::wholeMessageArrived (const void *pData, uint32 ui32DataLength,
                                             const char *pszPublisherNodeID, char **ppszNodeToForwardTo)
{
    if (pData == NULL || ui32DataLength == 0 || ppszNodeToForwardTo == NULL) {
        return -1;
    }
    *ppszNodeToForwardTo = NULL;
    checkAndLogMsg ("NodeContextManager::wholeMessageArrived", Logger::L_Info,
                    "whole message arrived from sender with id = <%s>\n", pszPublisherNodeID);

    _mPeer.lock (1049);
    PeerNodeContext *pCurrPeer = getNodeContextInternal (pszPublisherNodeID);
    if (pCurrPeer == NULL) {
        pCurrPeer = new PeerNodeContext (pszPublisherNodeID,
                                         _pMetadataConf->getMetadataAsStructure(),
                                         NodeContext::TOO_FAR_FROM_PATH_COEFF,
                                         NodeContext::APPROXIMABLE_TO_POINT_COEFF);
        if (pCurrPeer == NULL) {
            _mPeer.unlock (1049);
            checkAndLogMsg ("NodeContextManager::wholeMessageArrived", memoryExhausted);
            return -3;
        }
        pCurrPeer->setPeerPresence (true);
        _uiActivePeerNumber++;
        _pPeerNodeContextList->prepend (pCurrPeer);
    }

    BufferReader ibr (pData, ui32DataLength);
    InstrumentedReader br (&ibr, false);
    uint32 ui32Len = 0;
    br.read32 (&ui32Len);
    if (ui32Len > 0) {
        *ppszNodeToForwardTo = (char *) calloc (ui32Len + 1, sizeof (char));
        br.readBytes (*ppszNodeToForwardTo, ui32Len);
        (*ppszNodeToForwardTo)[ui32Len] = '\0';
    }
    ui32DataLength -=  br.getBytesRead();
    int64 rc = pCurrPeer->readAll (&br, ui32DataLength);
    if (rc == 1) {
        // Old message - drop it
        _mPeer.unlock (1049);
        return 1;
    }
    if (rc < 0) {
        checkAndLogMsg ("NodeContextManager::wholeMessageArrived", Logger::L_SevereError,
                        "could not read the whole message arrived from sender with id = <%s>; rc = %d\n",
                        pszPublisherNodeID, (int) rc);
        _mPeer.unlock (1049);
        return -1;
    }
    const char *pszPeerNodeID = pCurrPeer->getNodeId();
    NodePath *pPeerNodePath = pCurrPeer->getPath();
    checkAndLogMsg ("NodeContextManager::wholeMessageArrived", Logger::L_Info,
                    "received a message from node <%s>; path length is %d\n",
                    pszPeerNodeID != NULL ? pszPeerNodeID : "null",
                    pPeerNodePath != NULL ? (int) pPeerNodePath->getPathLength() : -1);
    _mPeer.unlock (1049);
    return 0;
}

int NodeContextManager::wayPointMessageArrived (void *pData, uint32 ui32DataLength, const char *pszPublisherNodeId,
                                                bool &bPositionHasChanged)
{
    checkAndLogMsg ("NodeContextManager::wayPointMessageArrived", Logger::L_Info,
                    "Way Point message arrived from sender with id = <%s>.\n", pszPublisherNodeId);

    if (pszPublisherNodeId == NULL) {
        return -1;
    }

    _mPeer.lock (1050);
    PeerNodeContext *pCurrPeer = getNodeContextInternal (pszPublisherNodeId);
    if (pCurrPeer == NULL) {
        _mPeer.unlock (1050);
        return -2;
    }

    const NodeContext::Versions storedVersions (pCurrPeer->getVersions());

    // Found the PeerNodeContext for pszSenderNodeID
    BufferReader br (pData, ui32DataLength);
    NodeContext::Versions v;
    int rc = pCurrPeer->readCurrentWaypoint (&br, ui32DataLength, v, bPositionHasChanged);
    if (rc < 0) {
        checkAndLogMsg ("NodeContextManager::wayPointMessageArrived", Logger::L_MildError,
                        "Could not read the way point message arrived from sender with id = <%s>.\n",
                        pszPublisherNodeId);
        _mPeer.unlock (1050);
        bPositionHasChanged = false;
        return -3;
    }

    const NodeContext::Versions readVersions (v);

    if ((readVersions._ui32StartingTime < storedVersions._ui32StartingTime) || readVersions.lessThan (storedVersions, true)) {
        // Old waypoint message - it should be ignored
        _mPeer.unlock (1050);
        bPositionHasChanged = false;
        return 0;
    }

    checkAndLogMsg ("NodeContextManager::wayPointMessageArrived", Logger::L_Info,
                    "read new waypoint for peer %s; waypoint version is: %d, waypoint is: %d\n",
                    pszPublisherNodeId, (int) readVersions._ui16WaypointVersion, pCurrPeer->getCurrentWayPointInPath());

    BufferWriter bw (1024, 128);
    if ((readVersions._ui32StartingTime != storedVersions._ui32StartingTime) ||
        (readVersions.greaterThan (storedVersions, true))) {

        // Some part of the node context is not up-to-date, send my versions to
        // the peer, and it will reply with the updated parts.

        // Write the id of the node for which the versions are being written
        uint32 ui32NodeIdLen = strlen (pszPublisherNodeId);
        if (ui32NodeIdLen == 0) {
            _mPeer.unlock (1050);
            return -4;
        }
        bw.write32 (&ui32NodeIdLen);
        bw.writeBytes (pszPublisherNodeId, ui32NodeIdLen);

        int64 rc = pCurrPeer->writeVersions (&bw, pCurrPeer->getWriteVersionsLength());
        if (rc < 0) {
            checkAndLogMsg ("NodeContextManager::wayPointMessageArrived", Logger::L_SevereError,
                            "Could not write a versions message. Returned %d\n", (int) rc);
            _mPeer.unlock (1050);
            return -5;
        }
        else {
            rc = 0;
        }

        /*if (wayPointVersion > prevWayPointVersion) {
            _mPeer.unlock (1050);
            return 0;
        }
        else {*/
        _mPeer.unlock (1050);

        TargetPtr targets[2];
        targets[0] = _pTopology->getNextHopAsTarget (pszPublisherNodeId);
        targets[1] = NULL;
        if (targets[0] != NULL) {
            // The waypoint version decreased or it has not changed -
            // the nodes may not be synchronized, or it could just be an
            // out-of-order waypoint message
            rc = _pCommMgr->sendVersionMessage (bw.getBuffer(), bw.getBufferLength(),
                                                _pszNodeId, targets);
        }
        delete targets[0];
        return 0;
        //}
    }
    else if (readVersions._ui16WaypointVersion > storedVersions._ui16WaypointVersion) {
        // The node context is up-to-date, I only need to update the current waypoint
        _mPeer.unlock (1050);
        return 0;
    }

    // else, the version has not changed
    bPositionHasChanged = false;
    _mPeer.unlock (1050);
    return 0;
}

int NodeContextManager::localContextHasChanged (const NodeContext::Versions &versions)
{
    Targets **ppTargets = _pTopology->getNeighborsAsTargets();
    if ((ppTargets == NULL) || (ppTargets[0] == NULL)) {
        Targets::deallocateTargets (ppTargets);
        return 0;
    }

    _mLocal.lock (1051);
    int64 rc = _pLocalNodeContext->getWriteUpdatesLength (versions);
    if (rc == 0) { 
        checkAndLogMsg ("NodeContextManager::localContextHasChanged", Logger::L_Info,
                        "The local context is still the same.\n");
        _mLocal.unlock (1051);
        Targets::deallocateTargets (ppTargets);
        return -1;
    }
    checkAndLogMsg ("NodeContextManager::localContextHasChanged", Logger::L_Info,
                    "The local context has changed.\n");

    BufferWriter bw (1024, 128);
    rc = _pLocalNodeContext->writeUpdates (&bw, (uint32) rc, versions);
    if (rc < 0) {
        checkAndLogMsg ("NodeContextManager::localContextHasChanged", Logger::L_MildError,
                        "Could not write the update message. Returned %d\n", (int) rc);
        _mLocal.unlock (1051);
        Targets::deallocateTargets (ppTargets);
        return -2;
    }
    _mLocal.unlock (1051);

    rc = _pCommMgr->sendUpdateMessage (bw.getBuffer(), bw.getBufferLength(), _pszNodeId, ppTargets);
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
    NodeContext::Versions versions (_pLocalNodeContext->getVersions());
    versions._ui16ClassifierVersion = ui16PrevClassifierVersion;
    rc = localContextHasChanged (versions);
    return (rc == 0 ? 0 : -2);
}

int NodeContextManager::updatePosition (Writer *pWriter)
{
    if (pWriter == NULL) {
        return -1;
    }

    checkAndLogMsg ("NodeContextManager::updatePosition", Logger::L_Info,
                    "The actual way point in path has changed.\n");
    _mPeer.lock (1056);
    if ((_pPeerNodeContextList == NULL) || (_uiActivePeerNumber == 0)) {
        _mPeer.unlock (1056);
        return 0;
    }
    _mPeer.unlock (1056);

    _mLocal.lock (1057);
    int rc = _pLocalNodeContext->getWriteWaypointLength();
    if (rc == 0) {
        _mLocal.unlock (1057);
        return -2;
    }
    rc = _pLocalNodeContext->writeCurrentWaypoint (pWriter, (uint32) rc);
    if (rc < 0) {
        checkAndLogMsg ("NodeContextManager::actualWayPointIsChanged",
                        Logger::L_MildError, "Could not write the way point message.\n");
        _mLocal.unlock (1057);
        return -3;
    }
    _mLocal.unlock (1057);
    return 0;
}

LocalNodeContext * NodeContextManager::getLocalNodeContext()
{
    _mLocal.lock (1062);
    return _pLocalNodeContext;
}

void NodeContextManager::releaseLocalNodeContext()
{
    _mLocal.unlock (1062);
}

PeerNodeContextList * NodeContextManager::getPeerNodeContextList()
{
    _mPeer.lock (1063);
    return _pPeerNodeContextList;
}

PeerNodeContextList * NodeContextManager::getPeerNodeContextList (const char **ppszPeerNodeIDFilter)
{
    _mPeer.lock (1063);
    if (_pPeerNodeContextList == NULL || ppszPeerNodeIDFilter == NULL) {
        return NULL;
    }
    PeerNodeContextList *pFilteredPeerNodeContext = new PeerNodeContextList();
    if (pFilteredPeerNodeContext != NULL) {
        PeerNodeContext *pCurrPeerNodeID = _pPeerNodeContextList->getFirst();
        bool bFilter;
        while (pCurrPeerNodeID != NULL) {
            bFilter = false;
            for (unsigned int i = 0; ppszPeerNodeIDFilter[i] != NULL; i++) {
                if (0 == strcmp (ppszPeerNodeIDFilter[i], pCurrPeerNodeID->getNodeId())) {
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

char ** NodeContextManager::getPeerList (bool bNeighborsOnly)
{
    _mPeer.lock (1063);
    if (_pPeerNodeContextList == NULL) {
        _mPeer.unlock (1063);
        return NULL;
    }
    int iCount = _pPeerNodeContextList->getCount();
    if (iCount <= 0) {
        _mPeer.unlock (1063);
        return NULL;
    }

    char **ppszIds = (char **) calloc (iCount+1, sizeof (char *));
    if (ppszIds == NULL) {
        checkAndLogMsg ("NodeContextManager::getPeerList", memoryExhausted);
        _mPeer.unlock (1063);
        return NULL;
    }

    PeerNodeContext *pCurrPeerNodeContext = _pPeerNodeContextList->getFirst();
    for (unsigned int i = 0; pCurrPeerNodeContext != NULL;) {
        if (!bNeighborsOnly || _pTopology->isNeighbor (pCurrPeerNodeContext->getNodeId())) {
            ppszIds[i] = strDup (pCurrPeerNodeContext->getNodeId());
            if (ppszIds[i] == NULL) {
                checkAndLogMsg ("NodeContextManager::getPeerList", memoryExhausted);
                break;
            }
            i++;
        }
        pCurrPeerNodeContext = _pPeerNodeContextList->getNext();
    }

    _mPeer.unlock (1063);
    return ppszIds;
}

PeerNodeContext * NodeContextManager::getPeerNodeContext (const char *pszNodeID)
{
    _mPeer.lock (1063);
    return getNodeContextInternal (pszNodeID);
}

PeerNodeContext * NodeContextManager::getNodeContextInternal (const char *pszNodeID)
{
    if (_pPeerNodeContextList != NULL) {
        PeerNodeContext *pCurrPeer = _pPeerNodeContextList->getFirst();
        while (pCurrPeer != NULL) {
            if (0 == strcmp (pszNodeID, pCurrPeer->getNodeId())) {
                return pCurrPeer;
            }
            pCurrPeer = _pPeerNodeContextList->getNext();
        }
    }
    return NULL;
}

void NodeContextManager::releasePeerNodeContextList()
{
    _mPeer.unlock (1063);
}

unsigned int NodeContextManager::getActivePeerNumber()
{
    return _uiActivePeerNumber;
}

char ** NodeContextManager::getActivePeerList()
{
    _mPeer.lock (1064);
    if (getActivePeerNumber() == 0) {
        _mPeer.unlock (1064);
        return NULL;
    }
    char **ppActivePeersList = (char **) calloc (_uiActivePeerNumber + 1, sizeof(char *));
    int count = 0;
    for (PeerNodeContext *pCurrPeer = _pPeerNodeContextList->getFirst(); pCurrPeer != NULL;
         pCurrPeer = _pPeerNodeContextList->getNext()) {
        if (pCurrPeer->isPeerActive()) {
            ppActivePeersList[count] = (char *) pCurrPeer->getNodeId();
            count ++;
        }
    }
    ppActivePeersList[count] = NULL;
    _mPeer.unlock (1064);
    return ppActivePeersList;
}

char ** NodeContextManager::getReachablePeers (AdaptorId adaptorId, const char **ppszPeerNodeIds)
{
    if (ppszPeerNodeIds == NULL) {
        return NULL;
    }
    _mPeer.lock (1065);
    if (getActivePeerNumber() == 0) {
        _mPeer.unlock (1065);
        return NULL;
    }
    char **ppActivePeersList = (char **) calloc (_uiActivePeerNumber + 1, sizeof(char *));
    int count = 0;
    for (PeerNodeContext *pCurrPeer = _pPeerNodeContextList->getFirst(); pCurrPeer != NULL;
            pCurrPeer = _pPeerNodeContextList->getNext()) {
        for (unsigned int i = 0; ppszPeerNodeIds[i] != NULL; i++) {
            if (strcmp (pCurrPeer->getNodeId(), ppszPeerNodeIds[i]) == 0) {
                if (pCurrPeer->isReacheableThrough (adaptorId)) {
                    ppActivePeersList[count] = (char *) pCurrPeer->getNodeId();
                    count++;
                    break;
                }
            }
        }
    }
    ppActivePeersList[count] = NULL;
    _mPeer.unlock (1065);
    return ppActivePeersList;
}


