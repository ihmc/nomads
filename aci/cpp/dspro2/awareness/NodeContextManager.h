/*
 * NodeContextManager.h
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

#ifndef INCL_NODE_CONTEXT_MANAGER_H
#define INCL_NODE_CONTEXT_MANAGER_H

#include "DArray2.h"
#include "PeerNodeContext.h"
#include "Topology.h"

#include "LoggingMutex.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class Writer;
}

namespace IHMC_C45
{
    class C45AVList;
}

namespace IHMC_VOI
{
    class NodePath;
}

namespace IHMC_ACI
{
    class LocalNodeContext;
    class MetadataConfigurationImpl;

    class NodeContextManager
    {
        public:
            NodeContextManager (const char *pszNodeId, LocalNodeContext *pLocalNodeContext);
            virtual ~NodeContextManager (void);

            int configure (CommAdaptorManager *pCommMgr, Topology *pTopology,
                           MetadataConfigurationImpl *pMetadataConf);

            void setBatteryLevel (unsigned int uiBattery);
            void setMemoryAvailable (unsigned int uiMemory);
            int registerPath (IHMC_VOI::NodePath *pPath);
            int setCurrentPath (const char *pszPathId);

            /*
             * Updates peer list and sends a VersionsMessage to the new peer
             */
            int newPeer (const char *pszNodeId);

            void deadPeer (const char *pszNodeId);

            /**
             * A peer node has sent its own versions of the local node context.
             * Uses the given Writer to write the payload of the reply.
             * Return values:
             * 1 send a whole message,
             * 2 send an updates message,
             * 0 do not send a reply,
             * -1 an error occurred
             */
            int versionsMessageArrived (const void *pData, uint32 ui32DataLength,
                                        const char *pszPublisherNodeId,
                                        NOMADSUtil::String &nodeToForwardTo);

            /**
             * A peer node has sent an update of its node context.
             * Returns -1 in case of errors, returns 0 otherwise.
             *
             * If bContextUnsynchronized is set on true, it means that part of
             * the node context, or part of it, is not upto date, therefore a
             * version message should be sent in order to synchronize it.
             */
            int nodeContextMessageArrived (const void *pData, uint32 ui32DataLength,
                                           const char *pszPublisherNodeId,
                                           bool &bContextUnsynchronized,
                                           bool &bNodeContextUpdated,
                                           NOMADSUtil::String &nodeToForwardTo);

            int updateClassifier (IHMC_C45::C45AVList *pDataset);

            /**
             * Writes updates when the actual way point in path changes.
             * Returns 0 if successful, a negative number otherwise.s
             */
            int updatePosition (NOMADSUtil::Writer *pWriter);

            // get methods

            /**
             * After use "releaseLocalNodeContext()" must be called to
             * unlock the "_mLocal" mutex.
             */
            LocalNodeContext * getLocalNodeContext (void);

            /**
             * Releases the mutex "_mLocal".
             */
            void releaseLocalNodeContext (void);

            /**
             * After use "releasePeerNodeContextList()" must be called
             * to unlock the mutex "_mPeer".
             * The caller MUST NOT delete the returned object.
             */
            PeerNodeContextList * getPeerNodeContextList (void);

            /**
             * After use "releasePeerNodeContextList()" must be called
             * to unlock the mutex "_mPeer".
             *
             * NOTE: The caller MUST deallocate the returned object (but it
             * MUST NOT deallocate the contained elements).
             * if ppszPeerNodeIDFilter == nullptr the method does not work. If no
             * filtering needs to be applied, use the above method.
             */
            PeerNodeContextList * getPeerNodeContextList (const char **ppszPeerNodeIDFilter);

            /**
             * Returns the list of the known peers' ids
             * NOTE: the list must be deallocated by the caller
             */
            NOMADSUtil::DArray2<NOMADSUtil::String> * getPeerList (bool bNeighborsOnly);

            /**
             * After use "releasePeerNodeContextList()" must be called
             * to unlock the mutex "_mPeer".
             * The caller MUST NOT delete the returned object.
             */
            PeerNodeContext * getPeerNodeContext (const char *pszNodeId);

            /**
             * Releases the mutex "_mPeer".
             */
            void releasePeerNodeContextList (void);

            /**
             * Call this method between "getPeerNodeContextList()" and
             * "releasePeerNodeContextList()" to ensure that the returned
             * number of active peers correspond to the list of peers
             * returned by "getPeerNodeContextList()".
             *
             * NOTE: the caller should deallocate the returned array, but NOT
             *       it elements.
             */
            unsigned int getActivePeerNumber (void);

            NOMADSUtil::DArray2<NOMADSUtil::String> * getActivePeerList (void);

            /**
             * Returns the sublist of peers in ppszPeerNodeIds that are reachable
             * via the adaptor identified by adaptorId.
             *
             * NOTE: the caller should deallocate the returned array, but NOT
             *       it elements.
             */
            //char ** getReachablePeers (AdaptorId adaptorId, const char **ppszPeerNodeIds);

        private:
            PeerNodeContext * getNodeContextInternal (const char *pszNodeId);

            /**
             * Write updates when the local context is changed. Returns
             * the list of active peers who are interested in the updates.
             */
            int localContextHasChanged (const Versions &versions);

            /**
             * Uses the given Writer to write the payload of the message to send
             * to the new peer. Returns -1 in case of errors, returns 0 otherwise.
             */
            int newPeer (const char *pszNodeId, NOMADSUtil::Writer *pWriter);

        private:
            LocalNodeContext *_pLocalNodeContext;
            NOMADSUtil::PtrLList<PeerNodeContext> *_pPeerNodeContextList;
            unsigned int _uiActivePeerNumber;

            CommAdaptorManager *_pCommMgr;
            MetadataConfigurationImpl *_pMetadataConf;
            Topology *_pTopology;
            const NOMADSUtil::String _nodeId;

            NOMADSUtil::LoggingMutex _mLocal; // Recursive mutexes.
            NOMADSUtil::LoggingMutex _mPeer;  // Recursive mutexes.
    };
}

#endif // INCL_NODE_CONTEXT_MANAGER_H
