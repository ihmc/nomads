/*
 * Topology.h
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

#ifndef INCL_TOPOLOGY_H
#define INCL_TOPOLOGY_H

#include <stdio.h>

#include "CommAdaptor.h"
#include "NodeIdSet.h"
#include "Targets.h"

#include "DArray2.h"
#include "HTGraph.h"
#include "Mutex.h"
#include "PtrLList.h"

namespace NOMADSUtil
{
    class Logger;
    class Writer;
    class Reader;
}

namespace IHMC_ACI
{
    class CommAdaptorManager;
    class NodeContextManager;

    class Topology
    {
        public:
            static const uint16 _TOPOLOGY_SIZE;
            static const char * DEFAULT_INTERFACE;

            Topology (const char *pszNodeId);
            virtual ~Topology (void);

            int configure (CommAdaptorManager *pCommAdaptorMgr,
                           NodeContextManager *pNodeContextMgr);

            void display (FILE *pOutput);

            int addLink (const char *pszDstPeerId, const char *pszInterface,
                         AdaptorId adaptorId, AdaptorType type);
            virtual int addLink (const char *pszSrcPeerId, const char *pszDstPeerId,
                                 const char *pszInterface, AdaptorId adaptorId, AdaptorType type);
            int removeLink (const char *pszDstPeerId, AdaptorId adaptorId);
            int removeLink (const char *pszDstPeerId, const char *pszInterface, AdaptorId adaptorId);
            int removeAllLinksFromPeer (const char *pszPeerId);
            int replaceAllLinksForPeer (const char *pszPeerId, AdaptorId adaptorId, NOMADSUtil::PtrLList<NOMADSUtil::String> &neighbors);
            int topologyRequestArrived (const char *pszSenderNodeId, const void *pBuf, uint32 ui32Len);
            int topologyReplyArrived (const char *pszSenderNodeId, const void *pBuf, uint32 ui32Len);

            /**
             * NOTE: the list and its elements must be deallocated by the caller
             */
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getNeighbors (void);
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getNeighbors (const char *pszNodeId);

            /**
             * Returns an array of Targets, the last one is a nullptr pointer.
             */
            Targets ** getNeighborsAsTargets (void);

            /**
             * Returns the relative complement of the set of the peers of
             * pszCurrPeerId in the set of peers of pszPrevPeerId.
             */
            Targets ** getForwardingTargets (const char *pszCurrPeerId, const char *pszPrevPeerId);

            Targets ** getNextHopsAsTarget (IHMC_VOI::NodeIdSet &nodeIdSet);
            Targets * getNextHopAsTarget (const char *pszDstPeerId);

            Targets ** getRoutes (const char *pszRecipientPeerId);
            Targets ** getRoutes (const char **ppszRecipientsPeerIds);

            /**
             * Returns true if pszNodeId is a neighbor (a peer within 1-hop
             * distance), false otherwise.
             */
            bool isNeighbor (const char *pszNodeId);

            /**
             * Returns true if the node identified by pszNodeId is on the
             * given route.
             */
            bool isOnShortestRoute (const char *pszNodeId, const char *pszDstPeerId);

            int read (NOMADSUtil::Reader *pReader, NOMADSUtil::PtrLList<NOMADSUtil::String> &neighbors);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxLen);

        protected:
            struct NodeInfo
            {
                NodeInfo (void);
                ~NodeInfo (void);

                NodeInfo * operator = (NodeInfo *pNodeInfo);
                bool operator != (NodeInfo &rhsNodeInfo);

                int write (NOMADSUtil::Writer *pWriter, uint32 *pBytesWritten);
                int read (NOMADSUtil::Reader *pReader, uint32 *pBytesRead);

                int64 i64TimeStamp;
                bool bReply;
            };

            struct AdaptorStats
            {
                AdaptorStats (void) = default;
                AdaptorStats (const AdaptorStats & as) = delete;
                AdaptorStats (AdaptorStats && as) = delete;
                AdaptorStats & operator= (const AdaptorStats & as) = delete;
                AdaptorStats & operator= (AdaptorStats && as) = delete;

                AdaptorId adaptorId;
                int64 i64LastMsgRcvdTime;
                AdaptorType type;
            };

            struct EdgeInfo
            {
                EdgeInfo (void) = default;
                EdgeInfo (const EdgeInfo & ei) = delete;
                EdgeInfo (EdgeInfo && ei) = delete;
                EdgeInfo & operator= (const EdgeInfo & ei) = delete;
                EdgeInfo & operator= (EdgeInfo && ei) = delete;

                int getCost (void) { return 1; }
                void setCost (float fCost) {}
                NOMADSUtil::DArray2<AdaptorStats> stats;
            };

            const NOMADSUtil::String _nodeId;
            CommAdaptorManager *_pAdaptorMgr;
            NodeContextManager *_pNodeContextMgr;
            NOMADSUtil::HTGraph<NodeInfo, EdgeInfo> _graph;
            NOMADSUtil::Mutex _m;
            NOMADSUtil::StringHashset _availableInterfaces;

        private:
            int notifyNewNeighbor (const char *pszDstPeerId);       // Completely new neighbor
            int notifyLinkChanges (const char *pszDstPeerId);       // new/dead link to a known neighbor
            int notifyReply (const char *pszTargetNodeId);          // send a reply message to the target node

            int addLinkInternal (const char *pszSrcPeerId, const char *pszDstPeerId,
                                 const char *pszInterface, AdaptorId adaptorId, AdaptorType type);
            // Returns targets for forwarding that are not 1-hop neighbors of the sender node
            Targets ** getForwardingTargetsInternal (const char *pszCurrPeerId, const char *pszPrevPeerId);
            Targets ** getNodesAsTargets (NOMADSUtil::PtrLList<NOMADSUtil::Edge<EdgeInfo> > *pEdgeList);
            Targets * getNextHopAsTargetInternal (const char *pszDstPeerId);
    };

    class StaticTopology : public Topology
    {
        public:
            static const char * USE_STATIC_TOPOLOGY_PROPERTY;
            static const bool USE_STATIC_TOPOLOGY_DEFAULT;

            StaticTopology (const char *pszNodeId);
            ~StaticTopology (void);

            int configure (CommAdaptorManager *pCommAdaptorMgr, NodeContextManager *pNodeContextMgr,
                           NOMADSUtil::ConfigManager *pCfgMgr);

        private:
            struct StaticLink
            {
                StaticLink (void);
                ~StaticLink (void);

                NOMADSUtil::String srcPeerId;
                NOMADSUtil::String dstPeerId;
                NOMADSUtil::String srcInterfaceToDst;
                AdaptorType adaptorType;
            };

            int readStaticTopologyFile (const char *pszFileName, NOMADSUtil::DArray2<StaticLink> &links);

        private:
            static const char * USE_STATIC_TOPOLOGY_FILE;
    };

    extern NOMADSUtil::Logger *pTopoLog;

    inline Topology::NodeInfo * Topology::NodeInfo::operator = (Topology::NodeInfo *pNodeInfo)
    {
        (*this) = (*pNodeInfo);
        return this;
    }

    inline bool Topology::NodeInfo::operator != (Topology::NodeInfo &rhsNodeInfo)
    {
        if ((i64TimeStamp == rhsNodeInfo.i64TimeStamp) && (bReply == rhsNodeInfo.bReply)) {
            return false;
        }
        return true;
    }

    inline NOMADSUtil::PtrLList<NOMADSUtil::String> * Topology::getNeighbors()
    {
        return getNeighbors (_nodeId);
    }
}

#endif // INCL_TOPOLOGY_H
