/*
 * Topology.cpp
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

#include "Topology.h"

#include <stdlib.h>
#include <string.h>

#include "ConfigManager.h"
#include "FileReader.h"
#include "Logger.h"
#include "NLFLib.h"
#include "UInt32Hashtable.h"
#include "BufferWriter.h"
#include "BufferReader.h"
#include "LineOrientedReader.h"
#include "NetUtils.h"
#include "NICInfo.h"
#include "StringTokenizer.h"

#include "CommAdaptorManager.h"
#include "NodeContextManager.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

const uint16 Topology::_TOPOLOGY_SIZE = 1;
const char * Topology::DEFAULT_INTERFACE = Edge<EdgeInfo>::DEFAULT_EDGE_LABEL;

const char * StaticTopology::USE_STATIC_TOPOLOGY_PROPERTY = "aci.dspro2.topology.static";
const bool StaticTopology::USE_STATIC_TOPOLOGY_DEFAULT = false;
const char * StaticTopology::USE_STATIC_TOPOLOGY_FILE = "aci.dspro2.topology.static.file";

namespace IHMC_ACI
{
    Logger *pTopoLog;
}

Topology::Topology (const char *pszNodeId)
    : _nodeId (pszNodeId),
      _pAdaptorMgr (nullptr),
      _pNodeContextMgr (nullptr),
      //_graph (true,   // bDeleteElements=true,
      //        false), // bDirected
      _availableInterfaces (true, // bCaseSensitiveKeys
                            true, // bCloneKeys
                            true) // bDeleteKeys
{
    NodeInfo *pLocalNodeInfo = new NodeInfo;
    pLocalNodeInfo->bReply = false;        // local node has the complete flag
    pLocalNodeInfo->i64TimeStamp = 0;
    _graph.addVertex (_nodeId, pLocalNodeInfo);
}

Topology::~Topology (void)
{
}

int Topology::configure (CommAdaptorManager *pCommAdaptorManager,
                         NodeContextManager *pNodeContextManager)
{
    if (pCommAdaptorManager == nullptr || pNodeContextManager == nullptr) {
        return -1;
    }

    _pAdaptorMgr = pCommAdaptorManager;
    _pNodeContextMgr = pNodeContextManager;

    NICInfo **ppNICInterfaces = NetUtils::getNICsInfo (false, false);
    for (unsigned int i = 0; ppNICInterfaces[i] != nullptr; i++) {
        _availableInterfaces.put (ppNICInterfaces[i]->getIPAddrAsString().c_str());
    }
    StringHashset::Iterator iter = _availableInterfaces.getAllElements();
    while (!iter.end()) {
        checkAndLogMsg ("Topology::configure", Logger::L_Info, "found interface %s\n",
                        iter.getKey());
        iter.nextElement();
    }

    return 0;
}

void Topology::display (FILE *pOutput)
{
    if (pOutput == nullptr) {
        return;
    }
    _m.lock();
    _graph.display (pOutput);
    _m.unlock();
}

int Topology::addLink (const char *pszDstPeerId, const char *pszInterface,
                       AdaptorId adaptorId, AdaptorType type)
{
    if (pszDstPeerId == nullptr || pszInterface == nullptr) {
        return -1;
    }

    _m.lock();
    if (!_availableInterfaces.containsKey (pszInterface)) {
        pszInterface = DEFAULT_INTERFACE;
    }

    logTopology ("Topology::addLink 1", Logger::L_Info,
                 "adding link from myself to %s\n", pszDstPeerId);

    NodeInfo *pNodeInfo = _graph.getVertex (pszDstPeerId);
    if (pNodeInfo == nullptr) {
        // New neighbor previously not in the topology
        pNodeInfo = new NodeInfo();
        if (pNodeInfo == nullptr) {
            checkAndLogMsg ("Topology::addLink", memoryExhausted);
        }
        pNodeInfo->i64TimeStamp = 0;
        pNodeInfo->bReply = true;
        _graph.addVertex (pszDstPeerId, pNodeInfo);
        EdgeInfo *pEdgeInfo = new EdgeInfo{};
        if (pEdgeInfo == nullptr) {
            checkAndLogMsg ("Topology::addLink", memoryExhausted);
        }
        pEdgeInfo->stats[0].adaptorId = adaptorId;
        pEdgeInfo->stats[0].i64LastMsgRcvdTime = getTimeInMilliseconds();
        pEdgeInfo->stats[0].type = type;
        _graph.addEdge (_nodeId, pszDstPeerId, pszInterface, pEdgeInfo);

        int rc = notifyNewNeighbor (pszDstPeerId);
        if (rc != 0) {
            _m.unlock();
            return rc;
        }

        _m.unlock();
        if (_pNodeContextMgr != nullptr) {
            return _pNodeContextMgr->newPeer (pszDstPeerId);
        }
        return -2;
    }

    PtrLList<Edge<EdgeInfo> > edgeList;
    _graph.getEdgeList (_nodeId, pszDstPeerId, &edgeList);
    if (edgeList.isEmpty()) {
        // New neighbor that was previously in the topology but not a neighbor
        EdgeInfo *pEdgeInfo = new EdgeInfo{};
        if (pEdgeInfo == nullptr) {
            checkAndLogMsg ("Topology::addLink", memoryExhausted);
        }
        pEdgeInfo->stats[0].adaptorId = adaptorId;
        pEdgeInfo->stats[0].i64LastMsgRcvdTime = getTimeInMilliseconds();
        pEdgeInfo->stats[0].type = type;
        _graph.addEdge (_nodeId, pszDstPeerId, pszInterface, pEdgeInfo);

        int rc = notifyLinkChanges (pszDstPeerId);
        _m.unlock();
        return rc;
    }

    Edge<EdgeInfo> *pEdge = _graph.getEdge (_nodeId, pszDstPeerId, pszInterface);
    if (pEdge == nullptr) {
        // New interface for a known neighbor
        EdgeInfo *pEdgeInfo = new EdgeInfo{};
        if (pEdgeInfo == nullptr) {
            checkAndLogMsg ("Topology::addLink", memoryExhausted);
        }
        pEdgeInfo->stats[0].adaptorId = adaptorId;
        pEdgeInfo->stats[0].i64LastMsgRcvdTime = getTimeInMilliseconds();
        pEdgeInfo->stats[0].type = type;

        int rc = _graph.addEdge (_nodeId, pszDstPeerId, pszInterface, pEdgeInfo);
        _m.unlock();
        return rc;
    }

    EdgeInfo *pEdgeInfo = pEdge->_pEdgeElement;
    if (pEdgeInfo == nullptr) {
        _m.unlock();
        return -3;
    }

    for (unsigned int i = 0; i < pEdgeInfo->stats.size(); i++) {
        if (pEdgeInfo->stats.used (i) && (pEdgeInfo->stats[i].adaptorId == adaptorId)) {
            // Known neighbor, known interface, known adaptor
            pEdgeInfo->stats[i].i64LastMsgRcvdTime = getTimeInMilliseconds();
            _m.unlock();
            return 0;
        }
    }

    // New adaptor connected to a known interface to a known neighbor
    int index = pEdgeInfo->stats.firstFree();
    pEdgeInfo->stats[index].adaptorId = adaptorId;
    pEdgeInfo->stats[index].i64LastMsgRcvdTime = getTimeInMilliseconds();
    pEdgeInfo->stats[index].type = type;

    _m.unlock();
    return 0;
}

int Topology::addLink (const char *pszSrcPeerId, const char *pszDstPeerId,
                       const char *pszInterface, AdaptorId adaptorId, AdaptorType type)
{
    if (pszSrcPeerId == nullptr || pszDstPeerId == nullptr || pszInterface == nullptr) {
        return -1;
    }

    _m.lock();
    int rc = addLinkInternal (pszSrcPeerId, pszDstPeerId, pszInterface, adaptorId, type);
    _m.unlock();
    return rc;
}

int Topology::addLinkInternal (const char *pszSrcPeerId, const char *pszDstPeerId,
                               const char *pszInterface, AdaptorId adaptorId, AdaptorType type)
{
    if (pszSrcPeerId == nullptr || pszDstPeerId == nullptr || pszInterface == nullptr) {
        return -1;
    }

    if (!_availableInterfaces.containsKey (pszInterface)) {
        pszInterface = DEFAULT_INTERFACE;
    }

    logTopology ("Topology::addLinkInternal", Logger::L_Info,
                 "adding link from %s to %s\n", pszSrcPeerId, pszDstPeerId);

    // Add Source Vertex
    NodeInfo *pNodeInfo = _graph.getVertex (pszSrcPeerId);
    if (pNodeInfo == nullptr) { // new neighbor previously not in the topology
        pNodeInfo = new NodeInfo();
        if (pNodeInfo == nullptr) {
            checkAndLogMsg ("Topology::addLinkInternal", memoryExhausted);
        }
        pNodeInfo->i64TimeStamp = 0;
        pNodeInfo->bReply = true;
        _graph.addVertex (pszSrcPeerId, pNodeInfo);
    }

    // Add Destination Vertex
    pNodeInfo = _graph.getVertex (pszDstPeerId);
    if (pNodeInfo == nullptr) {
        // New neighbor previously not in the topology
        pNodeInfo = new NodeInfo();
        if (pNodeInfo == nullptr) {
            checkAndLogMsg ("Topology::addLinkInternal", memoryExhausted);
        }
        pNodeInfo->i64TimeStamp = 0;
        pNodeInfo->bReply = true;
        _graph.addVertex (pszDstPeerId, pNodeInfo);
    }

    // Add source - destination edge
    PtrLList<Edge<EdgeInfo> > edgeList;
    _graph.getEdgeList (pszSrcPeerId, pszDstPeerId, &edgeList);
    if (edgeList.isEmpty()) {
        // New neighbor that was previously in the topology, but not a neighbor
        EdgeInfo *pEdgeInfo = new EdgeInfo{};
        if (pEdgeInfo == nullptr) {
            checkAndLogMsg ("Topology::addLinkInternal", memoryExhausted);
        }
        pEdgeInfo->stats[0].adaptorId = adaptorId;
        pEdgeInfo->stats[0].i64LastMsgRcvdTime = getTimeInMilliseconds();
        pEdgeInfo->stats[0].type = type;

        int rc = _graph.addEdge (pszSrcPeerId, pszDstPeerId, pszInterface, pEdgeInfo);
        return rc;
    }

    // Add source - destination edge
    Edge<EdgeInfo> *pEdge = _graph.getEdge (pszSrcPeerId, pszDstPeerId, pszInterface);
    if (pEdge == nullptr) {
        // New interface to a known neighbor
        EdgeInfo *pEdgeInfo = new EdgeInfo{};
        if (pEdgeInfo == nullptr) {
            checkAndLogMsg ("Topology::addLinkInternal", memoryExhausted);
        }
        pEdgeInfo->stats[0].adaptorId = adaptorId;
        pEdgeInfo->stats[0].i64LastMsgRcvdTime = getTimeInMilliseconds();
        pEdgeInfo->stats[0].type = type;

        int rc = _graph.addEdge (pszSrcPeerId, pszDstPeerId, pszInterface, pEdgeInfo);
        return rc;
    }

    EdgeInfo *pEdgeInfo = pEdge->_pEdgeElement;
    if (pEdgeInfo == nullptr) {
        return -3;
    }

    for (unsigned int i = 0; i < pEdgeInfo->stats.size(); i++) {
        if (pEdgeInfo->stats.used (i) && pEdgeInfo->stats[i].adaptorId == adaptorId) {
            logTopology ("Topology::addLinkInternal", Logger::L_Info,
                         "edge %s -> %s already exists, only updating time\n",
                         pszSrcPeerId, pszDstPeerId);
            // Existing adaptor to reach a known neighbor via a known interface
            pEdgeInfo->stats[i].i64LastMsgRcvdTime = getTimeInMilliseconds();
            return 0;
        }
    }


    // New adaptor connected to a known neighbor via a known interface
    int index = pEdgeInfo->stats.firstFree();
    pEdgeInfo->stats[index].adaptorId = adaptorId;
    pEdgeInfo->stats[index].i64LastMsgRcvdTime = getTimeInMilliseconds();
    pEdgeInfo->stats[index].type = type;
    logTopology ("Topology::addLinkInternal", Logger::L_Info,
                 "edge %s -> %s already exists, but new adaptor.\n",
                 pszSrcPeerId, pszDstPeerId);

    return 0;
}

int Topology::removeAllLinksFromPeer (const char *pszPeerId)
{
    if (pszPeerId == nullptr) {
        return -1;
    }

    logTopology ("Topology::removeLink", Logger::L_Info,
                 "removing all edges from peer %s\n", pszPeerId);

    _m.lock();
    _graph.removeAllEdgesFromVertex (pszPeerId);
    _m.unlock();
    return 0;
}

int Topology::replaceAllLinksForPeer (const char *pszPeerId, AdaptorId adaptorId, NOMADSUtil::PtrLList<NOMADSUtil::String> &neighbors)
{
    if (pszPeerId == nullptr) {
        return -1;
    }

    logTopology ("Topology::replaceAllLinksForPeer", Logger::L_Info,
                 "removing all edges from peer %s\n", pszPeerId);
    checkAndLogMsg ("Topology::replaceAllLinksForPeer", Logger::L_Info,
                    "removing all edges from peer %s\n", pszPeerId);

    _m.lock();
    _graph.removeAllEdgesFromVertex (pszPeerId);
    String *pNext = neighbors.getFirst();
    for (String *pCurr; (pCurr = pNext) != nullptr;) {
        pNext = neighbors.getNext();
        addLinkInternal (pszPeerId, pCurr->c_str(), DEFAULT_INTERFACE, adaptorId, UNKNOWN);
        delete neighbors.remove (pCurr);
    }
    _m.unlock();
    return 0;
}

int Topology::removeLink (const char *pszDstPeerId, AdaptorId adaptorId)
{
    if (pszDstPeerId == nullptr) {
        return -1;
    }

    logTopology ("Topology::removeLink", Logger::L_Info,
                 "removing link from myself to %s\n", pszDstPeerId);

    PtrLList<Edge<EdgeInfo> > edges;
    _m.lock();
    if (_graph.getEdgeList (_nodeId, pszDstPeerId, &edges) < 0) {
        _m.unlock();
        return -2;
    }

    /* TO-DO: remove link should only remove the link with a specific label,
     * to support multiple interfaces connected via the same AdaptorId */
    DArray2<String> interfacesToRemove (2U);
    unsigned int index = 0;
    for (Edge<EdgeInfo> *pEdge = edges.getFirst(); pEdge != nullptr; pEdge = edges.getNext()) {
        for (unsigned int i = 0; i < pEdge->_pEdgeElement->stats.size(); i++) {
            if (pEdge->_pEdgeElement->stats.used (i) &&
                (pEdge->_pEdgeElement->stats[i].adaptorId == adaptorId)) {
                interfacesToRemove[index] = pEdge->_pszEdgeLabel;
            }
        }
    }

    for (unsigned int i = 0; i < interfacesToRemove.size(); i++) {
        if (removeLink (pszDstPeerId, interfacesToRemove[i].c_str(), adaptorId) < 0) {
            _m.unlock();
            return -3;
        }
    }

    _m.unlock();
    return 0;
}

int Topology::removeLink (const char *pszDstPeerId, const char *pszInterface, AdaptorId adaptorId)
{
    if (pszDstPeerId == nullptr || pszInterface == nullptr) {
        return -1;
    }

    _m.lock();
    Edge<EdgeInfo> *pEdge = _graph.getEdge (_nodeId, pszDstPeerId, pszInterface);
    if (pEdge == nullptr) {
        _m.unlock();
        return 0;
    }

    EdgeInfo *pEdgeInfo = pEdge->_pEdgeElement;
    if (pEdgeInfo == nullptr) {
        _m.unlock();
        return -2;
    }

    // Count the remanining adaptors after removing adaptorId
    unsigned int uiAdaptorCounts = 0;
    for (unsigned int i = 0; i < pEdgeInfo->stats.size(); i++) {
        if (pEdgeInfo->stats.used (i)) {
            if (pEdgeInfo->stats[i].adaptorId == adaptorId) {
                pEdgeInfo->stats.clear (i);
            }
            else {
                uiAdaptorCounts++;
            }
        }
    }

    int rc = 0;
    if (uiAdaptorCounts == 0) {
        rc = _graph.removeEdge (_nodeId, pszDstPeerId, pszInterface);
        PtrLList<Edge<EdgeInfo> > *pEdges = _graph.getEdgeList (pszDstPeerId);
        if ((pEdges == nullptr) || (pEdges->getFirst() == nullptr)) {
            checkAndLogMsg ("Topology::removeLink", Logger::L_Info, "removed link %s to peer %s. "
                            "It was the only link. Removing peer.\n", pszInterface, pszDstPeerId);
            _graph.deleteElements (_nodeId, _TOPOLOGY_SIZE);
            rc = notifyLinkChanges (pszDstPeerId);
        }
        delete pEdges;
    }

    _m.unlock();
    return rc;
}

int Topology::topologyRequestArrived (const char * pszSenderNodeId, const void *pBuf, uint32 ui32Len)
{
    if ((pBuf == nullptr) || (ui32Len <= 0)) {
        return -1;
    }

    _m.lock();

    BufferReader *pBr = new BufferReader (pBuf,ui32Len);
    uint32 ui32BuffLen = 0;
    uint16 ui16Temp = 0;
    int rc;
    char *pszOriginalSenderId = nullptr;
    int64 i64Time;
    uint16 ui16ttl;
    uint16 ui16ReplyNum = 0;
    char **ppszReplyList = nullptr;
    uint32 ui32Position = 11;
    if ((rc = pBr->read16 (& ui16Temp)) < 0) {        // node id
        _m.unlock();
        return rc;
    }
    pszOriginalSenderId = (char *) calloc (ui16Temp + 1, sizeof(char));
    if ((rc = pBr->readBytes (&pszOriginalSenderId, (long int) ui16Temp)) < 0) {
        _m.unlock();
        return rc;
    }
    pszOriginalSenderId[ui16Temp] = '\0';
    ui32Position += ui16Temp;
    if ((rc = pBr->read64 (&i64Time)) < 0) {          // timestamp
        _m.unlock();
        return rc;
    }
    if (!_graph.hasVertex (pszOriginalSenderId)) {
        if (i64Time <= _graph.getVertex (pszOriginalSenderId)->i64TimeStamp) {
            _m.unlock();
            return 0; // discard message if old
        }
    }
    if ((rc = pBr->read16 (&ui16ttl)) < 0) {
        // TTL
        _m.unlock();
        return rc;
    }
    if ((rc = pBr->read16 (&ui16ReplyNum)) < 0) {
        // reply list
        _m.unlock();
        return rc;
    }
    if(ui16ReplyNum > 0) {
        ppszReplyList = (char **) calloc (ui16ReplyNum, sizeof(char **));
        for(int i = 0; i < ui16ReplyNum; i ++) {
            if ((rc = pBr->read16 (&ui16Temp)) < 0) {
                _m.unlock();
                return rc;
            }
            ui32Position += 2;
            ppszReplyList[i] = (char *) calloc (ui16Temp + 1, sizeof(char));
            if ((rc = pBr->readBytes(ppszReplyList[i], (long int) ui16Temp)) < 0) {
                _m.unlock();
                return rc;
            }
            ppszReplyList[i][ui16Temp] = '\0';
            ui32Position += ui16Temp;
        }
    }
    bool bFlag;  // complete flag
    if ((rc = pBr->read8(& bFlag)) < 0) {
        _m.unlock();
        return rc;
    }

    PtrLList<const char> *pUpdates = _graph.updateGraph (pBr, &ui32BuffLen);        // topology
    if ((!bFlag) && (pUpdates != nullptr)) {                                                // set reply flags
        const char *pszUpNode = (char *) pUpdates->getFirst();
        for (; pszUpNode != nullptr; pszUpNode = pUpdates->getNext()) {
            PtrLList<String> neighList;
            PtrLList<const char> pathList;
            int numNeigh = _graph.getNeighborsList (pszUpNode, &neighList);
            String *pCurrNeigh = neighList.getFirst();
            for (int j = 0; j < numNeigh; j ++) {
                int path = _graph.getShortestPath (_nodeId, pCurrNeigh->c_str(), &pathList, false);
                if ((path > 0) && (path < (_TOPOLOGY_SIZE - 1))) {
                    _graph.getVertex (pCurrNeigh->c_str())->bReply = false;
                }
                pathList.removeAll();
            }
        }
    }

    _graph.deletePartitions (_nodeId);
    _graph.deleteElements (_nodeId, _TOPOLOGY_SIZE);
    pUpdates = _graph.getVertexKeys(); // check if the graph is now complete
    bFlag = true;
    char *pszUpNode = (char *) pUpdates->getFirst();
    for (int i = 0; i < pUpdates->getCount(); i ++) {
        if (!_graph.getVertex (pszUpNode)->bReply) {
            bFlag = false;
            break;
        }
        pszUpNode = (char *) pUpdates->getNext();
    }
    _graph.getVertex (_nodeId)->bReply = bFlag;
    rc = 0;
    for (int i = 0; i < ui16ReplyNum; i ++) {
        if (strcmp (ppszReplyList[i], _nodeId) == 0) {
            rc = notifyReply (pszOriginalSenderId); // send reply message if requested
            if (rc < 0) {
                checkAndLogMsg ("Topology::topologyRequestArrived", memoryExhausted);
            }
            break;
        }
    }
    ui16ttl--;
    if(ui16ttl <= 1) {
        _m.unlock();
        return rc; // do not forward the message if TTL is expired
    }
    Targets **ppTargets = getForwardingTargetsInternal (_nodeId, pszSenderNodeId);
    if ((ppTargets == nullptr) || (ppTargets[0] == nullptr)) {
        _m.unlock();
        Targets::deallocateTargets (ppTargets);
        return rc;        // return if there are no targets
    }
    Targets::deallocateTargets (ppTargets);

    pBr->setPosition (ui32Position);
    HTGraph<NodeInfo, EdgeInfo> tempGraph;
    BufferWriter bw (0,5);
    tempGraph.updateGraph (pBr, & ui32BuffLen);
    ui16Temp = strlen (pszOriginalSenderId); // node id
    if ((rc = bw.write16 (& ui16Temp)) < 0) {
        _m.unlock();
        return rc;
    }
    if ((rc = bw.writeBytes (pszOriginalSenderId, (long int) ui16Temp)) < 0) {
        _m.unlock();
        return rc;
    }
    if ((rc = bw.write64 (& i64Time)) < 0) { // timestamp
        _m.unlock();
        return rc;
    }
    if ((rc = bw.write16 (& ui16ttl)) < 0) { // TTL
        _m.unlock();
        return rc;
    }

    ui16Temp = 0; // reply list
    ui16ReplyNum = 0;
    do {
        rc = 0;
        do {
            ppszReplyList[ui16ReplyNum] = ppTargets[ui16Temp]->aTargetNodeIds[rc];
            rc ++;
            ui16ReplyNum ++;
        } while(ppTargets[ui16Temp]->aTargetNodeIds[rc]);
        ui16Temp++;
    } while (ppTargets[ui16Temp]);

    if ((rc = bw.write16 (&ui16ReplyNum)) < 0) {
        _m.unlock();
        return rc;
    }
    for (int i = 0; i < ui16ReplyNum; i++) {
        ui16Temp = strlen (ppszReplyList[i]);
        if ((rc = bw.write16 (& ui16Temp)) < 0) {
            _m.unlock();
            return rc;
        }
        if ((rc = bw.writeBytes (ppszReplyList[i], (long int) ui16Temp)) < 0) {
            _m.unlock();
            return rc;
        }
    }
    if ((rc = bw.write8 (&bFlag)) < 0) { // complete flag
        _m.unlock();
        return rc;
    }
    BufferWriter tempBw (0,5);
    if ((rc = _graph.writeNeighborhood (&tempBw, &ui32BuffLen, _nodeId)) < 0) {
        _m.unlock();
        return rc;
    }
    BufferReader *pTempBr = new BufferReader (tempBw.getBuffer(), tempBw.getBufferLength());
    ui32BuffLen = tempBw.getBufferLength();
    tempGraph.updateGraph (pTempBr, &ui32BuffLen);
    if ((rc = tempGraph.writeGraph (&bw, &ui32BuffLen)) < 0) {    // topology
        _m.unlock();
        return rc;
    }

    _m.unlock();
    if (_pAdaptorMgr != nullptr) {
        _pAdaptorMgr->sendTopologyRequestMessage ((const void *) bw.getBuffer(),
                                                  (unsigned int) bw.getBufferLength(), ppTargets);
    }
    // TODO: delete pBr and pTempBr and pszReplyList and pszOriginalSenderNodeId
    return 0;
}

int Topology::topologyReplyArrived (const char *pszSenderNodeId, const void *pBuf, uint32 ui32Len)
{
    if ((pBuf == nullptr) || (ui32Len <= 0)) {
        return -1;
    }
    BufferReader *pBr = new BufferReader (pBuf,ui32Len);
    uint32 ui32BuffLen = 0;
    uint16 ui16Temp = 0;
    int rc;
    char *pszTargetNodeId = nullptr;
    int64 i64Time;
    uint32 ui32Position = 7;
    if ((rc = pBr->read16 (&ui16Temp)) < 0) {        // node id
        return rc;
    }
    pszTargetNodeId = (char *) calloc (ui16Temp + 1, sizeof(char));
    if ((rc = pBr->readBytes (&pszTargetNodeId, (long int) ui16Temp)) < 0) {
        return rc;
    }
    pszTargetNodeId[ui16Temp] = '\0';
    ui32Position += ui16Temp;
    if ((rc = pBr->read64(& i64Time)) < 0) {          // timestamp
        return rc;
    }

    _m.lock();
    if (! _graph.hasVertex (pszTargetNodeId)) {
        if (i64Time <= _graph.getVertex (pszTargetNodeId)->i64TimeStamp) {
            _m.unlock();
            return 0; // discard message if old
        }
    }
    bool flag;                    // complete flag
    if ((rc = pBr->read8 (&flag)) < 0) {
        _m.unlock();
        return rc;
    }

    PtrLList<const char> *pUpdates = _graph.updateGraph (pBr, &ui32BuffLen); // topology
    if ((!flag) && (pUpdates != nullptr)) {                                                // set reply flags
        char *pszUpNode = (char *) pUpdates->getFirst();
        for(int i = 0; i < pUpdates->getCount(); i ++) {
            PtrLList<String> neighList;
            PtrLList<const char> pathList;
            int numNeigh = _graph.getNeighborsList (pszUpNode, &neighList);
            String *pCurrNeigh = neighList.getFirst();
            for(int j = 0; j < numNeigh; j ++) {
                int path = _graph.getShortestPath (_nodeId, pCurrNeigh->c_str(), &pathList, false);
                if ((path > 0) && (path < _TOPOLOGY_SIZE -1)) {
                    _graph.getVertex (pCurrNeigh->c_str())->bReply = false;
                }
                pathList.removeAll (true);
            }
            pszUpNode = (char *) pUpdates->getNext();
        }
    }
    _graph.deletePartitions (_nodeId);
    _graph.deleteElements (_nodeId, _TOPOLOGY_SIZE);
    pUpdates = _graph.getVertexKeys(); // check if the graph is now complete
    flag = true;
    char *pszUpNode = (char *) pUpdates->getFirst();
    for(int i = 0; i < pUpdates->getCount(); i ++) {
        if(!_graph.getVertex (pszUpNode)->bReply) {
            flag = false;
            break;
        }
        pszUpNode = (char *) pUpdates->getNext();
    }
    _graph.getVertex (_nodeId)->bReply = flag;
    if (!strcmp (_nodeId, pszTargetNodeId)) {
        _m.unlock();
        return 0; // check if the node was the target
    }
    PtrLList<const char> pathList; // forward the reply message
    _graph.getShortestPath (_nodeId, pszTargetNodeId, &pathList, false);
    Targets *pNextHop = getNextHopAsTargetInternal (pathList.getFirst());
    pathList.removeAll (true);
    if (pNextHop == nullptr) {
        _m.unlock();
        return rc; // return if there are no targets
    }
    pBr->setPosition (ui32Position);
    HTGraph<NodeInfo, EdgeInfo> tempGraph;
    BufferWriter bw(0,5);
    tempGraph.updateGraph (pBr, &ui32BuffLen);
    ui16Temp = strlen (pszTargetNodeId); // node id
    if ((rc = bw.write16 (&ui16Temp)) < 0) {
        _m.unlock();
        delete pNextHop;
        return rc;
    }
    if ((rc = bw.writeBytes (pszTargetNodeId, (long int) ui16Temp)) < 0) {
        _m.unlock();
        delete pNextHop;
        return rc;
    }
    if ((rc = bw.write64 (&i64Time)) < 0) { // timestamp
        _m.unlock();
        delete pNextHop;
        return rc;
    }
    if ((rc = bw.write8 (&flag)) < 0) { // complete flag
        _m.unlock();
        delete pNextHop;
        return rc;
    }
    BufferWriter tempBw(0,5);
    if ((rc = _graph.writeNeighborhood (&tempBw, &ui32BuffLen, _nodeId)) < 0) {
        _m.unlock();
        delete pNextHop;
        return rc;
    }
    BufferReader * pTempBr = new BufferReader(tempBw.getBuffer(), tempBw.getBufferLength());
    ui32BuffLen = tempBw.getBufferLength();
    tempGraph.updateGraph (pTempBr, &ui32BuffLen);
    if ((rc = tempGraph.writeGraph (&bw, &ui32BuffLen)) < 0) {    // topology
        _m.unlock();
        delete pNextHop;
        return rc;
    }

    _m.unlock();

    if (_pAdaptorMgr != nullptr) {
        TargetPtr targets[2] = {pNextHop, nullptr};
        _pAdaptorMgr->sendTopologyReplyMessage ((const void *) bw.getBuffer(),
                                                (unsigned int) bw.getBufferLength(), targets);
    }
    // TODO: delete pBr and pTempBr and pszTargetNodeId
    delete pNextHop;

    return 0;
}

Targets ** Topology::getRoutes (const char *pszRecipientPeerId)
{
    char **ppszRecipientsPeerIds = (char **) calloc (2, sizeof (char*));
    if (ppszRecipientsPeerIds == nullptr) {
        checkAndLogMsg ("Topology::getRoutes", memoryExhausted);
        return nullptr;
    }
    ppszRecipientsPeerIds[0] = (char *) pszRecipientPeerId;
    Targets **ppTargets = getRoutes ((const char **) ppszRecipientsPeerIds);
    free (ppszRecipientsPeerIds);
    return ppTargets;
}

Targets ** Topology::getRoutes (const char **ppszRecipientsPeerIds)
{
    _m.lock();
    PtrLList<Edge<EdgeInfo> > *pEdges = _graph.getEdgeList (_nodeId);
    if (pEdges == nullptr) {
        _m.unlock();
        return nullptr;
    }

    UInt32Hashtable<Targets> ht;
    Edge<EdgeInfo> *pEdge = pEdges->getFirst();
    while (pEdge != nullptr) {
        EdgeInfo *pEdgeInfo = pEdge->_pEdgeElement;
        if (pEdgeInfo != nullptr) {
            bool bAddPeerRoute = (ppszRecipientsPeerIds == nullptr);
            for (unsigned i = 0; !bAddPeerRoute && ppszRecipientsPeerIds[i] != nullptr; i++) {
                if (strcmp (pEdge->_pszDstVertexKey, ppszRecipientsPeerIds[i]) == 0) {
                    bAddPeerRoute = true;
                }
            }
            if (bAddPeerRoute) {
                for (unsigned int i = 0; i < pEdgeInfo->stats.size(); i++) {
                    if (pEdgeInfo->stats.used (i)) {
                        Targets *pTarget = ht.get (pEdgeInfo->stats[i].adaptorId);
                        if (pTarget == nullptr) {
                            pTarget = new Targets();
                            pTarget->adaptorId = pEdgeInfo->stats[i].adaptorId;
                            ht.put (pEdgeInfo->stats[i].adaptorId, pTarget);
                        }

                        bool bFound = false;
                        for (unsigned k = 0; k < pTarget->aTargetNodeIds.size(); k++) {
                            if (strcmp (pTarget->aTargetNodeIds[k], pEdge->_pszDstVertexKey) == 0) {
                               bFound = true;
                            }
                        }
                        if (!bFound) {
                            unsigned int uiIndex = pTarget->firstFreeTargetNodeId();
                            pTarget->aTargetNodeIds[uiIndex] = strDup (pEdge->_pszDstVertexKey);
                        }

                        bFound = false;
                        for (unsigned k = 0; k < pTarget->aInterfaces.size(); k++) {
                            if (strcmp (pTarget->aInterfaces[k], pEdge->_pszEdgeLabel) == 0) {
                               bFound = true;
                            }
                        }
                        if (!bFound) {
                            unsigned int uiIndex = pTarget->firstFreeInterface();
                            pTarget->aInterfaces[uiIndex] = strDup (pEdge->_pszEdgeLabel);
                        }
                    }
                }
            }
        }

        pEdge = pEdges->getNext();
    }
    delete pEdges;

    Targets **ppTargets = (Targets **) calloc (ht.getCount(), sizeof (Targets*));
    if (ppTargets == nullptr) {
        _m.unlock();
        return nullptr;
    }
    UInt32Hashtable<Targets>::Iterator iter = ht.getAllElements();
    for (unsigned int i = 0; iter.nextElement(); i++) {
        ppTargets[i] = iter.getValue();
    }
    ht.removeAll();

    _m.unlock();
    return ppTargets;
}

bool Topology::isNeighbor (const char *pszNodeId)
{
    _m.lock();
    bool bRet = _graph.hasNeighbor (_nodeId, pszNodeId);
    _m.unlock();
    return bRet;
}

bool Topology::isOnShortestRoute (const char *pszNodeId, const char *pszDstPeerId)
{
    if (pszNodeId == nullptr || pszDstPeerId == nullptr) {
        return false;
    }

    _m.lock();
    if (_graph.hasNeighbor (_nodeId, pszDstPeerId)) {
        Targets **ppTargets = getForwardingTargetsInternal (_nodeId, pszDstPeerId);
        for (unsigned int i =0;  ppTargets[i] != nullptr; i++) {
            for (unsigned int j = 0; j < ppTargets[i]->aTargetNodeIds.size(); j++) {
                if (ppTargets[i]->aTargetNodeIds[j] != nullptr && strcmp (ppTargets[i]->aTargetNodeIds[j], pszNodeId) == 0) {
                    Targets::deallocateTargets (ppTargets);
                    _m.unlock();
                    return true;
                }
            }
        }
        Targets::deallocateTargets (ppTargets);
    }
    else if (_graph.hasVertex (pszDstPeerId)) {
        PtrLList<const char> path;
        _graph.getShortestPath (_nodeId, pszDstPeerId, &path, false);
        for (const char *pszCurr = path.getFirst(); pszCurr != nullptr; pszCurr = path.getNext()) {
            if (strcmp (pszNodeId, pszCurr) == 0) {
                _m.unlock();
                path.removeAll (true);
                return true;
            }
        }
        path.removeAll (true);
    }

    _m.unlock();
    return false;
}

int Topology::read (Reader *pReader, PtrLList<String> &neighbors)
{
    if (pReader == nullptr) {
        return -1;
    }
    uint16 ui16Len = 0;

    for (int rc = pReader->read16 (&ui16Len); rc >= 0 && ui16Len > 0;
         rc = pReader->read16 (&ui16Len)) {
        char *pszNeighbor = (char *) calloc (ui16Len+1, sizeof (char));
        if (pszNeighbor != nullptr) {
            pReader->readBytes (pszNeighbor, ui16Len);
            pszNeighbor[ui16Len] = '\0';
            String *pNeighbor = new String (pszNeighbor);
            if (pNeighbor != nullptr) {
                neighbors.prepend (pNeighbor);
            }
            free (pszNeighbor);
        }
    }

    return 0;
}

int Topology::write (Writer *pWriter, uint32 ui32MaxLen)
{
    if (pWriter == nullptr) {
        return -1;
    }
    PtrLList<String> *pNeighbors = getNeighbors();
    uint16 ui16Len;
    String *pNext = pNeighbors->getFirst();
    for (String *pCurr; (pCurr = pNext) != nullptr;) {
        pNext = pNeighbors->getNext();
        ui16Len = pCurr->length();
        if (ui16Len > 0) {
            pWriter->write16 (&ui16Len);
            pWriter->writeBytes (pCurr->c_str(), ui16Len);
        }
        delete pNeighbors->remove (pCurr);
    }
    delete pNeighbors;
    pNeighbors = nullptr;
    ui16Len = 0;
    pWriter->write16 (&ui16Len);
    return 0;
}

int Topology::notifyNewNeighbor (const char *pszDstPeerId)
{
    if (_TOPOLOGY_SIZE <= 1) {
        return 0;
    }

    NodeInfo *pNI = _graph.getVertex (_nodeId);
    if (pNI == nullptr) {
        return -1;
    }
    pNI->i64TimeStamp = getTimeInMilliseconds();
    BufferWriter bw (0,5);
    uint32 ui32BuffLen = 0;
    uint16 ui16Temp = 0;
    int rc;
    ui16Temp = strlen (_nodeId);        // node id
    if ((rc = bw.write16(& ui16Temp)) < 0) {
        return rc;
    }
    if ((rc = bw.writeBytes (_nodeId, (long int) ui16Temp)) < 0) {
        return rc;
    }
    if ((rc = bw.write64 (&_graph.getVertex (_nodeId)->i64TimeStamp)) < 0) {  // timestamp
        return rc;
    }
    ui16Temp = _TOPOLOGY_SIZE; // TTL
    if ((rc = bw.write16 (&ui16Temp)) < 0) {
        return rc;
    }
    ui16Temp = 1;        // reply list: new neighbor
    if ((rc = bw.write16 (&ui16Temp)) < 0) {
        return rc;
    }
    ui16Temp = strlen(pszDstPeerId);
    if ((rc = bw.write16 (&ui16Temp)) < 0) {
        return rc;
    }
    if ((rc = bw.writeBytes (pszDstPeerId, (long int) ui16Temp)) < 0) {
        return rc;
    }
    bool flag = _graph.getVertex (_nodeId)->bReply; // complete flag
    if ((rc = bw.write8(& flag)) < 0) {
           return rc;
    }
    if (flag) {
        if ((rc = _graph.writeGraph (&bw, & ui32BuffLen)) < 0) {
            return rc;
        }
    }
    else if ((rc = _graph.writeNeighborhood (&bw, & ui32BuffLen, _nodeId)) < 0) {
        return rc;
    }
    Targets **ppTargets = getNeighborsAsTargets();
    if ((ppTargets == nullptr) || (ppTargets[0] == nullptr)) {
        Targets::deallocateTargets (ppTargets);
        return -1;
    }
    if (_pAdaptorMgr != nullptr) {
        _pAdaptorMgr->sendTopologyRequestMessage ((const void *) bw.getBuffer(),
                                                  (unsigned int) bw.getBufferLength(), ppTargets);
    }
    Targets::deallocateTargets (ppTargets);
    return 0;
}

int Topology::notifyLinkChanges (const char *pszDstPeerId)
{
    if (_TOPOLOGY_SIZE <= 1) {
        return 0;
    }

    NodeInfo *pNI = _graph.getVertex (_nodeId);
    if (pNI == nullptr) {
        return -1;
    }
    _graph.getVertex (_nodeId)->i64TimeStamp = getTimeInMilliseconds();
    BufferWriter bw (0,5);
    uint32 ui32BuffLen = 0;
    uint16 ui16Temp = 0;
    int rc;
    ui16Temp = strlen (_nodeId); // node id
    if ((rc = bw.write16 (&ui16Temp)) < 0) {
        return rc;
    }
    if ((rc = bw.writeBytes (_nodeId, (long int) ui16Temp)) < 0) {
        return rc;
    }

    if ((rc = bw.write64 (&_graph.getVertex (_nodeId)->i64TimeStamp)) < 0) {  // timestamp
        return rc;
    }
    ui16Temp = _TOPOLOGY_SIZE; // TTL
    if ((rc = bw.write16 (&ui16Temp)) < 0) {
        return rc;
    }
    ui16Temp = 0; // reply list empty
    if ((rc = bw.write16 (&ui16Temp)) < 0) {
        return rc;
    }
    bool bFlag = false; // complete flag
    if ((rc = bw.write8 (&bFlag)) < 0) {
           return rc;
    }
    if ((rc = _graph.writeNeighborhood (&bw, &ui32BuffLen, _nodeId)) < 0) {
        return rc;
    }
    Targets **ppTargets = getNeighborsAsTargets();
    if ((ppTargets == nullptr) || (ppTargets[0] == nullptr)) {
        Targets::deallocateTargets (ppTargets);
        return 0;
    }
    if (_pAdaptorMgr != nullptr) {
        _pAdaptorMgr->sendTopologyRequestMessage ((const void *) bw.getBuffer(),
                                                  (unsigned int) bw.getBufferLength(), ppTargets);
    }
    Targets::deallocateTargets (ppTargets);
    return 0;
}

int Topology::notifyReply (const char *pszTargetNodeId)
{
    if (_TOPOLOGY_SIZE <= 1) {
        return 0;
    }

    _graph.getVertex (_nodeId)->i64TimeStamp = getTimeInMilliseconds(); // increase timestamp
    PtrLList<const char> pathList; // forward the reply message
    _graph.getShortestPath (_nodeId, pszTargetNodeId, &pathList, false);
    Targets *pNextHop = getNextHopAsTargetInternal (pathList.getFirst());
    if (pNextHop == nullptr) {
        return 0; // return if there are no targets
    }

    uint16 ui16Temp;
    BufferWriter bw(0,5);
    uint32 ui32BuffLen = 0;
    ui16Temp = strlen (pathList.getFirst());
    int rc = bw.write16 (& ui16Temp);
    if (rc < 0) {
        delete pNextHop;
        return rc;
    }
    if ((rc = bw.writeBytes (pathList.getFirst(), (long int) ui16Temp)) < 0) {
        delete pNextHop;
        return rc;
    }
    if ((rc = bw.write64 (&_graph.getVertex (_nodeId)->i64TimeStamp)) < 0) {      // timestamp
        delete pNextHop;
        return rc;
    }
    if ((rc = bw.write8 (&_graph.getVertex (_nodeId)->bReply)) < 0) {     // complete flag
        delete pNextHop;
        return rc;
    }
    if (_graph.getVertex (_nodeId)->bReply) { // topology
        if ((rc = _graph.writeGraph (&bw, &ui32BuffLen)) < 0) {
            delete pNextHop;
            return rc;
        }
    }
    else if ((rc = _graph.writeNeighborhood (&bw, &ui32BuffLen, _nodeId)) < 0) {
        delete pNextHop;
        return rc;
    }

    if (_pAdaptorMgr != nullptr) {
        TargetPtr targets[2] = {pNextHop, nullptr};
        _pAdaptorMgr->sendTopologyReplyMessage ((const void *) bw.getBuffer(),
                                                (unsigned int) bw.getBufferLength(), targets);
    }
    delete pNextHop;
    pathList.removeAll (true);

    return 0;
}

PtrLList<String> * Topology::getNeighbors (const char *pszNodeId)
{
    PtrLList<String> *pList = new PtrLList<String>();
    if (pList == nullptr) {
        return nullptr;
    }
    _m.lock();
    _graph.getNeighborsList (pszNodeId, pList);
    _m.unlock();
    return pList;
}

Targets ** Topology::getNeighborsAsTargets()
{
    _m.lock();
    PtrLList<Edge<EdgeInfo> > *pEdgeList = _graph.getEdgeList (_nodeId);
    Targets **ppTargets = getNodesAsTargets (pEdgeList);
    _m.unlock();
    delete pEdgeList;
    return ppTargets;
}

Targets ** Topology::getNextHopsAsTarget (NodeIdSet &nodeIdSet)
{
    StringHashtable<Targets> targets;
    for (NodeIdIterator iter = nodeIdSet.getIterator(); !iter.end(); iter.nextElement()) {
        Targets *pNextHop = getNextHopAsTarget (iter.getKey());
        if (pNextHop != nullptr) {
            if (!targets.containsKey (pNextHop->aTargetNodeIds[0])) {
                targets.put (pNextHop->aTargetNodeIds[0], pNextHop);
            }
            else {
                delete pNextHop;
            }
        }
    }
    if (targets.getCount() == 0) {
        return nullptr;
    }
    Targets **ppTargets = (Targets **) calloc (targets.getCount() + 1, sizeof (Targets*));
    if (ppTargets != nullptr) {
        StringHashtable<Targets>::Iterator iter = targets.getAllElements();
        for (unsigned int i = 0; !iter.end(); iter.nextElement(), i++) {
            ppTargets[i] = iter.getValue();
        }
    }
    return ppTargets;
}

Targets * Topology::getNextHopAsTarget (const char *pszDstPeerId)
{
    _m.lock();
    Targets *pTargets = getNextHopAsTargetInternal (pszDstPeerId);
    _m.unlock();
    return pTargets;
}

Targets * Topology::getNextHopAsTargetInternal (const char *pszDstPeerId)
{
    Targets **ppTargets = nullptr;
    if (_graph.hasNeighbor (_nodeId, pszDstPeerId)) {
        PtrLList<Edge<EdgeInfo> > edgeList;
        _graph.getEdgeList (_nodeId, pszDstPeerId, &edgeList);
        ppTargets = getNodesAsTargets (&edgeList);
    }
    else if (_graph.hasVertex (pszDstPeerId)) {
        PtrLList<Edge<EdgeInfo> > edgeList;
        PtrLList<const char> path;
        _graph.getShortestPath (_nodeId, pszDstPeerId, &path, false);
        // Get the next hop
        const char *pszCurr = path.getFirst();
        // Find the first peer that it's not myself
        while ((pszCurr != nullptr) && (strcmp (_nodeId, pszCurr) == 0)) {
            pszCurr = path.getNext();
        }
        path.removeAll (true);
        if (pszCurr == nullptr) {
            return nullptr;
        }
        _graph.getEdgeList (_nodeId, pszCurr, &edgeList);
        ppTargets = getNodesAsTargets (&edgeList);
    }

    // Sanity check
    if (ppTargets != nullptr) {
        unsigned int i = 0;
        for (; ppTargets[i] != nullptr; i++);
        assert (i < 2);
        Targets *pNextHop = ppTargets[0];
        free (ppTargets);
        return pNextHop;
    }
    return nullptr;
}

Targets ** Topology::getForwardingTargets (const char *pszCurrPeerId,
                                           const char *pszPrevPeerId)
{
    _m.lock();
    Targets **ppTargets = getForwardingTargetsInternal (pszCurrPeerId, pszPrevPeerId);
    _m.unlock();
    return ppTargets;
}

Targets ** Topology:: getForwardingTargetsInternal (const char *pszCurrPeerId,
                                                    const char *pszPrevPeerId)
{
    PtrLList<Edge<EdgeInfo> > *pCurrEdgeList = _graph.getEdgeList (pszCurrPeerId);
    if (pCurrEdgeList == nullptr) {
        return nullptr;
    }

    PtrLList<String> prevNodeList;
    _graph.getNeighborsList (pszPrevPeerId, &prevNodeList);
    prevNodeList.prepend (new String (pszPrevPeerId));
    for (Edge<EdgeInfo> *pEdge = pCurrEdgeList->getFirst(); pEdge != nullptr;) {
        Edge<EdgeInfo> *pNextEdge = pCurrEdgeList->getNext();
        for (String *pPrevNeighbor = prevNodeList.getFirst(); pPrevNeighbor != nullptr;) {
            if (!strcmp (pEdge->_pszDstVertexKey, pPrevNeighbor->c_str())) {
                pCurrEdgeList->remove (pEdge);
                break;
            }
            pPrevNeighbor = prevNodeList.getNext();
        }
        pEdge = pNextEdge;
    }

    // Deallocate prevNodeList's elements
    String *pCurr, *pTmp;
    pTmp = prevNodeList.getFirst();
    while ((pCurr = pTmp) != nullptr) {
        pTmp = prevNodeList.getNext();
        delete prevNodeList.remove (pCurr);
    }

    Targets **ppTargets = getNodesAsTargets (pCurrEdgeList);
    delete pCurrEdgeList;

    return ppTargets;
}

Targets ** Topology::getNodesAsTargets (PtrLList<Edge<Topology::EdgeInfo> > *pEdgeList)
{
    const int iNumEdges = pEdgeList->getCount();
    if (iNumEdges <= 0) {
        return nullptr;
    }

    int iInterfaces = 0;
    auto pTableCount = new int[iNumEdges] {0};
    Edge<EdgeInfo> ***pppEdgeTable;
    pppEdgeTable = (Edge<EdgeInfo> ***) calloc (iNumEdges, sizeof (Edge<EdgeInfo> **));
    for (int i = 0; i < iNumEdges; i ++) {
        pppEdgeTable[i] = (Edge<EdgeInfo> **) calloc (iNumEdges, sizeof (Edge<EdgeInfo> *));
    }
    Edge<EdgeInfo> *pEdge = pEdgeList->getFirst();        // group edges by interface
    pppEdgeTable[0][0] = pEdge;
    pTableCount[0]++;
    iInterfaces++;
    bool bFlag;
    for (int i = 1; i < iNumEdges; i ++) {
        bFlag = false;
        pEdge = pEdgeList->getNext();
        for (int j = 0; j < iInterfaces; j ++) {
            if (strcmp (pppEdgeTable[j][0]->_pszEdgeLabel, pEdge->_pszEdgeLabel) == 0) {
                pppEdgeTable[j][pTableCount[j]] = pEdge;
                pTableCount[j]++;
                bFlag = true;
                break;
            }
        }
        if (!bFlag) {
            pppEdgeTable[iInterfaces][0] = pEdge;
            pTableCount[iInterfaces]++;
            iInterfaces ++;
        }
    }

    int iMax, iMaxInt;
    Edge<EdgeInfo> **ppTemp;
    int iTemp;
    PtrLList<String> neighList;
    int iNumVertex = 0;  // Count the number of nodes
    DArray2<char*> aNodeIDs;
    for (pEdge = pEdgeList->getFirst(); pEdge != nullptr; pEdge = pEdgeList->getNext()) {
        bFlag = true;
        for (unsigned int j = 0; j < aNodeIDs.size(); j++) {
            if (aNodeIDs.used (j) && (!strcmp (aNodeIDs[j], pEdge->_pszDstVertexKey))) {
                bFlag = false;
                break;
            }
        }
        if (bFlag) {
            aNodeIDs[aNodeIDs.firstFree()] = pEdge->_pszDstVertexKey;
            iNumVertex ++;
        }
    }

    int iOutgoingNodes = 0;
    for (int k = 0; k < iInterfaces; k++) {
        iMax = pTableCount[k];
        iMaxInt = k;
        for (int i = k+1; i < iInterfaces; i++) { // select the interface that covers the highest
            if (pTableCount[i] > iMax) { // number of ppTargets and set it as first interface
                iMax = pTableCount[i];
                iMaxInt = i;
            }
        }
        ppTemp = pppEdgeTable[k];
        pppEdgeTable[k] = pppEdgeTable[iMaxInt];
        pppEdgeTable[iMaxInt] = ppTemp;
        iTemp = pTableCount[k];
        pTableCount[k] = pTableCount[iMaxInt];
        pTableCount[iMaxInt] = iTemp;
        iOutgoingNodes += pTableCount[k];
        if (iOutgoingNodes == iNumVertex) {
            // all ppTargets covered by an interface: stop and select the adaptors
            break;
        }
        for (int i = k+1; i < iInterfaces; i++) { // delete from the table all ppTargets already
            for (int j = 0; j < pTableCount[i]; j++) {    // covered by an interface
                for(int h = 0; h < pTableCount[k]; h++) {
                    if(strcmp(pppEdgeTable[k][h]->_pszDstVertexKey, pppEdgeTable[i][j]->_pszDstVertexKey) == 0) {
                        pppEdgeTable[i][j] = nullptr;
                        pppEdgeTable[i][j] = pppEdgeTable[i][pTableCount[i]-1];
                        pppEdgeTable[i][pTableCount[i]-1] = nullptr;
                        pTableCount[i]--;
                        j--;
                        break;
                    }
                }
            }
            if (pTableCount[i] == 0) {
                free (pppEdgeTable[i]);
                pppEdgeTable[i] = nullptr;
                pppEdgeTable[i] = pppEdgeTable[iInterfaces-1];
                pppEdgeTable[iInterfaces-1] = nullptr;
                iInterfaces--;
                i--;
            }
        }
    }

    DArray2<int> adaptorCount;          // count the adaptors
    DArray2<unsigned int> adaptorId;
    for (int i = 0; i < iInterfaces; i++) {
           for (int j = 0; j < pTableCount[i]; j++) {
            for (unsigned int k = 0; k < pppEdgeTable[i][j]->_pEdgeElement->stats.size(); k++) {
                if (pppEdgeTable[i][j]->_pEdgeElement->stats.used (k)) {
                    bFlag = false;
                    for (unsigned int h = 0; h < adaptorId.size(); h++) {
                        if (pppEdgeTable[i][j]->_pEdgeElement->stats[k].adaptorId == adaptorId[h]) {
                            adaptorCount[h]++;
                            bFlag = true;
                            break;
                        }
                    }
                    if (!bFlag) {
                        iTemp = adaptorId.firstFree();
                        adaptorId[iTemp] = pppEdgeTable[i][j]->_pEdgeElement->stats[k].adaptorId;
                        adaptorCount[iTemp] = 1;
                    }
                }
            }
           }
    }

    Targets **ppTargets = (Targets **) calloc (1, sizeof (Targets *));
    ppTargets[0] = nullptr;
    iOutgoingNodes = 0;
    iTemp = 1;
    int nodes;
    do { // compose the output
        iMax = 0;
        iMaxInt = 0;
        nodes = 0;
        for (unsigned int j = 0; j < adaptorCount.size(); j++) {   // select the adaptor that covers the highest
            if (adaptorCount.used (j) && (adaptorCount[j] > iMax)) {  // number of ppTargets
                iMax = adaptorCount[j];
                iMaxInt = j;
            }
        }
        ppTargets = (Targets **) realloc (ppTargets, (iTemp+1) * sizeof (Targets *));
        ppTargets[iTemp-1] = new Targets (adaptorCount[iMaxInt]);
        ppTargets[iTemp-1]->adaptorId = adaptorId[iMaxInt];
        for (int i = 0; i < iInterfaces; i++) {
            for (int j = 0; j < pTableCount[i]; j++) {
                for (unsigned int k = 0; k < pppEdgeTable[i][j]->_pEdgeElement->stats.size(); k++) {
                    if (pppEdgeTable[i][j]->_pEdgeElement->stats.used (k) &&
                        (pppEdgeTable[i][j]->_pEdgeElement->stats[k].adaptorId == adaptorId[iMaxInt])) {
                        ppTargets[iTemp-1]->aTargetNodeIds[nodes] = strDup (pppEdgeTable[i][j]->_pszDstVertexKey);
                        ppTargets[iTemp-1]->aInterfaces[nodes] = strDup (pppEdgeTable[i][j]->_pszEdgeLabel);
                        nodes++;
                        pppEdgeTable[i][j] = nullptr;
                        pTableCount[i]--;
                        if(pTableCount[i] > 0) {
                            pppEdgeTable[i][j] = pppEdgeTable[i][pTableCount[i]];
                            pppEdgeTable[i][pTableCount[i]] = nullptr;
                            j--;
                        }
                        break;
                    }
                }
            }
            if (pTableCount[i] == 0) {
                free (pppEdgeTable[i]);
                pppEdgeTable[i] = nullptr;
                iInterfaces--;
                if (iInterfaces > 0) {
                    pppEdgeTable[i] = pppEdgeTable[iInterfaces];
                    pppEdgeTable[iInterfaces] = nullptr;
                    pTableCount[i] = pTableCount[iInterfaces];
                    pTableCount[iInterfaces] = 0;
                    i--;
                }
            }
        }
        ppTargets[iTemp] = nullptr;
        iTemp++;
        iOutgoingNodes += nodes;
        adaptorCount.clear (iMaxInt);
        adaptorId.clear (iMaxInt);
    } while (iOutgoingNodes < iNumVertex);

    for (int i = 0; i < iNumEdges; i++) {
        if (pppEdgeTable[i] != nullptr) {
            free (pppEdgeTable[i]);
        }
    }
    free (pppEdgeTable);
    delete pTableCount;

    return ppTargets;
}

Topology::NodeInfo::NodeInfo()
{
    bReply = true;
    i64TimeStamp = 0;
}

Topology::NodeInfo::~NodeInfo()
{
}

int Topology::NodeInfo::read (Reader *pReader, uint32 *pBytesWritten)
{
    return 0;
}

int Topology::NodeInfo::write (Writer *pWriter, uint32 *pBytesRead)
{
    return 0;
}

StaticTopology::StaticTopology (const char *pszNodeId)
    : Topology (pszNodeId)
{
}

StaticTopology::~StaticTopology()
{
}

int StaticTopology::configure (CommAdaptorManager *pCommAdaptorMgr,
                               NodeContextManager *pNodeContextMgr,
                               NOMADSUtil::ConfigManager *pCfgMgr)
{
    if (pCommAdaptorMgr == nullptr || pNodeContextMgr == nullptr || pCfgMgr == nullptr) {
        return -1;
    }

    if (Topology::configure (pCommAdaptorMgr, pNodeContextMgr) < 0) {
        return -2;
    }

    const char *pszFileName = pCfgMgr->getValue (USE_STATIC_TOPOLOGY_FILE, "topology.conf");
    DArray2<StaticLink> links;
    if (readStaticTopologyFile (pszFileName, links) < 0) {
        return -3;
    }

    int rc = 0;
    for (unsigned int i = 0; i < links.size() && rc == 0; i++) {
        if (!links.used (i)) {
            continue;
        }
        AdaptorId adaptorId = 0;
        if (links[i].adaptorType == DISSERVICE) {
            if (pCommAdaptorMgr->getDisServiceAdaptorId (adaptorId) < 0) {
                rc = -4;
                break;
            }
        }

        checkAndLogMsg ("StaticTopology::configure", Logger::L_Info, "Adding link to static topology: "
                        "src <%s> dst <%s> iface <%s> adaptor id <%u> adaptor type <%s>\n",
                        links[i].srcPeerId.c_str(), links[i].dstPeerId.c_str(),
                        links[i].srcInterfaceToDst.c_str(), adaptorId,
                        (links[i].adaptorType == DISSERVICE ? "DISSERVICE" : (links[i].adaptorType == MOCKETS ? "MOCKETS" : "UNKNOWN")));

        rc = addLink (links[i].srcPeerId.c_str(), links[i].dstPeerId.c_str(),
                      links[i].srcInterfaceToDst.c_str(), adaptorId,
                      links[i].adaptorType);
    }

    return (rc < 0 ? -5 : 0);
}

int StaticTopology::readStaticTopologyFile (const char *pszFileName, DArray2<StaticLink> &links)
{
    if (pszFileName == nullptr) {
        return -1;
    }
    FileReader fr (pszFileName, "r");
    LineOrientedReader lr (&fr);
    int iBytesRead = 0;
    unsigned int uiLineNumber = 0;
    for (char buf[256]; (iBytesRead = lr.readLine (buf, 256)) >= 0;) {
        if (iBytesRead > 0) {
            String line (buf, (unsigned short) iBytesRead);
            StringTokenizer tokenizer (line.c_str(), ' ', ' ');
            const char *pszToken = tokenizer.getNextToken();
            for (unsigned int tokenId = 0; pszToken != nullptr; tokenId++) {
                switch (tokenId) {
                    case 0:
                        links[uiLineNumber].srcPeerId = pszToken;
                        break;

                    case 1:
                        links[uiLineNumber].dstPeerId = pszToken;
                        break;

                    case 2:
                        links[uiLineNumber].srcInterfaceToDst = pszToken;
                        break;

                    case 3:
                    {
                        String sAdaptorType = pszToken;
                        sAdaptorType.convertToUpperCase();
                        if (sAdaptorType == "DISSERVICE") {
                            links[uiLineNumber].adaptorType = DISSERVICE;
                        }
                        else if (sAdaptorType == "MOCKETS") {
                            links[uiLineNumber].adaptorType = MOCKETS;
                        }
                        else {
                            return -2;
                        }
                        break;
                    }

                    default:
                        return -3;
                }

                pszToken = tokenizer.getNextToken();
            }
        }
        uiLineNumber++;
    }

    return 0;
}

StaticTopology::StaticLink::StaticLink()
{
    adaptorType = UNKNOWN;
}

StaticTopology::StaticLink::~StaticLink()
{
}
