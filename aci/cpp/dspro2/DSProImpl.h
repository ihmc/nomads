/*
 * DSProImpl.h
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
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#ifndef INCL_DSPRO_IMPLEMENTATION_H
#define INCL_DSPRO_IMPLEMENTATION_H

#include "FTypes.h"
#include "LoggingMutex.h"
#include "PtrLList.h"

#include "BoundingBox.h"
#include "CallbackHandler.h"
#include "BaseController.h"
#include "CommAdaptorManager.h"
#include "Publisher.h"
#include "UserRequests.h"
#include "Voi.h"

namespace IHMC_MISC
{
    class ChunkReassemblerInterface;
}

namespace NOMADSUtil
{
    class AVList;
}

namespace IHMC_VOI
{
    class MetadataInterface;
    class NodePath;
}

namespace IHMC_ACI
{
    class AMTDictator;
    class Controller;
    class CustomPolicyImpl;
    class DataStore;
    class InformationStore;
    class LocalNodeContext;
    class MetadataConfigurationImpl;
    class MetaData;
    class NodeContextManager;
    class PositionUpdater;
    class Publisher;
    class Scheduler;
    class Topology;

    class DSProImpl
    {
        public:
            DSProImpl (const char *pszNodeId, const char *pszVersion);
            ~DSProImpl (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr,
                      MetadataConfigurationImpl *pMetadataConf);

            int changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len);

            /*
             * Get DSPro Components
             * NB: the returned components could be accessed concurrently,
             *     therefore they must be thread-safe
             */
            CallbackHandler * getCallbackHandler (void);
            CommAdaptorManager * getCommAdaptorManager (void);
            DataStore * getDataStore (void);
            InformationStore * getInformationStore (void);
            MetadataConfigurationImpl * getMetadataConf (void);
            NodeContextManager * getNodeContextManager (void);
            ThreadSafePublisher * getPublisher (void);
            Topology * getTopology (void);

            // Message Publication
            int addAnnotation (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, const char *pszAnnotationTargetObjId,
                               MetaData *pMetadata, const void *pData, uint32 ui32DataLen,
                               int64 i64ExpirationTime, char **ppszId);
            int addMessage (const char *pszGroupName, const char *pszObjectId,
                            const char *pszInstanceId, MetaData *pMetadata,
                            const void *pData, uint32 ui32DataLen,
                            int64 i64ExpirationTime, char **ppszId);

            int addChunkedMessage (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                   MetaData *pMetadata, NOMADSUtil::PtrLList<IHMC_MISC::Chunker::Fragment> *pChunks,
                                   const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId);
            int addAdditionalChunk (const char *pszMetadataId, const char *pszReferredObjectId,
                                    const char *pszObjectId, const char *pszInstanceId,
                                    IHMC_MISC::Chunker::Fragment *pChunk, int64 i64ExpirationTime);
            int chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                                    const char *pszInstanceId, MetaData *pMetadata,
                                    const void *pData, uint32 ui32DataLen,
                                    const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId,
                                    bool bPush=true);
            int chunkAndAddMessageInternal (const char *pszGroupName, const char *pszObjectId,
                                            const char *pszInstanceId, MetaData *pMetadata,
                                            const void *pData, uint32 ui32DataLen,
                                            const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId,
                                            bool bPush=true);

            int disseminateMessage (const char *pszGroupName, const char *pszObjectId,
                                    const char *pszInstanceId, const void *pData, uint32 ui32DataLen,
                                    int64 i64ExpirationTime, char **ppszId);
            int disseminateMessageMetadata (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                            const void *pMetadata, uint32 ui32MetadataLen,
                                            const void *pData, uint32 ui32DataLen, const char *pszMimeType,
                                            int64 i64ExpirationTime, char **ppszId);

            int subscribe (CommAdaptor::Subscription &sub);

            /*
             * Add metadata annotation to the object identified by pszReferredObject.
             * This metadata annotation is inserted "as-it-is", and it is not
             * matched.
             */
            int addAnnotationNoPrestage (const char *pszGroupName, const char *pszObjectId,
                                         const char *pszInstanceId, IHMC_VOI::MetadataInterface *pMetadata,
                                         const char *pszReferredObject, int64 i64ExpirationTime,
                                         char **ppszId);

            /* Add data object to DSPro. The data is not matched. */
            int addData (const char *pszGroupName, const char *pszObjectId,
                         const char *pszInstanceId, const char *pszAnnotatedObjMsgId,
                         const char *pszAnnotationMetadata, uint32 ui32AnnotationMetdataLen,
                         const void *pData, uint32 ui32DataLen, const char *pszDataMimeType,
                         int64 i64ExpirationTime, char **ppszId);

            /**
            * It adds a metadata message into InformationStore and DataStore
            *
            * NOTE: it modifies pMetadata!
            */
            int setAndAddMetadata (Publisher::PublicationInfo &pubInfo, IHMC_VOI::MetadataInterface *pMetadata,
                                   NOMADSUtil::String &msgId, bool bStoreInInfoStore);

            // Misc
            int addPeer (AdaptorType protocol, const char *pszNetworkInterface,
                         const char *pszRemoteAddress, uint16 ui16Port);
            int addRequestedMessageToUserRequests (const char *pszId, const char *pszQueryId);
            int addUserId (const char *pszUserName);
            int addAreaOfInterest (const char *pszAreaName, NOMADSUtil::BoundingBox &bb, int64 i64StatTime, int64 i64EndTime);
            int addCustomPolicy (CustomPolicyImpl *pPolicy);
            void asynchronouslyNotifyMatchingMetadata (const char *pszQueryId, const char **ppszMsgIds);
            void asynchronouslyNotifyMatchingSearch (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen);
            int getData (const char *pszId, const char *pszCallbackParameter, void **ppData, uint32 &ui32DataLen,
                         bool &bHasMoreChunks);
            char ** getMatchingMetadataAsJson (NOMADSUtil::AVList *pAVQueryList, int64 i64BeginArrivalTimestamp,
                                               int64 i64EndArrivalTimestamp = 0);
            const char * getNodeId (void) const;
            NOMADSUtil::DArray2<NOMADSUtil::String> * getPeerList (void);
            const char * getVersion (void) const;
            bool isTopologyExchangedEnabled (void);
            int notUseful (const char *pszMessageID);
            void sendWaypointMessage (const void *pBuf, uint32 ui32BufLen);
            int updateUsage (const char *pszMessageId);
            int updateLearning (const char *pszMessageId, uint8 ui8Usage);
            int search (SearchProperties &searchProp, char **ppszQueryId);
            int sendAsynchronousRequestMessage (const char *pszId);

            // Configure node Context
            int setMissionId (const char *pszMissionName);
            int setRole (const char *pszRole);
            int setTeamId (const char *pszTeamId);
            int setNodeType (const char *pszType);
            int setCurrentPath (const char *pszPathID);
            int setCurrentPosition (float fLatitude, float fLongitude, float Altitude,
                                    const char *pszLocation, const char *pszNote);
            int setRangeOfInfluence (const char *pszNodeType, uint32 ui32RangeOfInfluenceInMeters);
            int setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters);
            int setUsefulDistance (const char *pszMIMEType, uint32 ui32UsefulDistanceInMeters);
            int setMatchingThreshold (float fMatchmakingThreshold);
            int registerPath (IHMC_VOI::NodePath *pPath);

            int removeAsynchronousRequestMessage (const char *pszId);
            int requestMoreChunks (const char *pszChunkedMsgId, const char *pszCallbackParameter);

            // Communication Adaptor Listener
            int registerCommAdaptorListener (uint16 ui16ClientId, CommAdaptorListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterCommAdaptorListener (uint16 ui16ClientId, CommAdaptorListener *pListener);

            // Chunking Plugin Registration
            int registerChunkFragmenter (const char *pszMimeType, IHMC_MISC::ChunkerInterface *pChunker);
            int registerChunkReassembler (const char *pszMimeType, IHMC_MISC::ChunkReassemblerInterface *pReassembler);

            int deregisterChunkFragmenter (const char *pszMimeType);
            int deregisterChunkReassembler (const char *pszMimeType);

            // Callbacks
            int dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                             const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                             const void *pBuf, uint32 ui32Len, uint8 ui8ChunkIndex, uint8 ui8TotNChunks,
                             const char *pszQueryId);
            int metadataArrived (const char *pszId, const char *pszGroupName,
                                 const char *pszObjectId, const char *pszInstanceId,
                                 const void *pBuf, uint32 ui32Len, const char *pszReferredDataId,
                                 const char *pszQueryId);
            int metadataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                                 const char *pszInstanceId, const MetaData *pMetadata, const char *pszReferredDataId,
                                 const char *pszQueryId);
            int dataAvailable (const char *pszId, const char *pszGroupName,
                               const char *pszObjectId, const char *pszInstanceId,
                               const char *pszRefObjId, const char *pszMimeType,
                               const void *pMetadata, uint32 ui32MetadataLength,
                               const char *pszQueryId);

            int newPeer (const char *pszNewPeerId);
            int deadPeer (const char *pszDeadPeerId);

        private:
            // friend class DSPro;
            friend class DSProProxyAdaptor;
            friend class PositionUpdater;

            bool _bEnableLoopbackNotifications;
            bool _bEnableTopologyExchange;

            MetadataConfigurationImpl *_pMetadataConf;
            LocalNodeContext *_pLocalNodeContext;
            DataStore *_pDataStore;
            InformationStore *_pInfoStore;

            Controller *_pController;
            Publisher *_pPublisher;
            NodeContextManager *_pNodeContextMgr;
            Topology *_pTopology;
            Scheduler *_pScheduler;
            PositionUpdater *_pPositionUpdater;
            IHMC_VOI::Voi *_pVoi;
            AMTDictator *_pAMTDict;

            const NOMADSUtil::String _nodeId;
            const NOMADSUtil::String _version;
            mutable NOMADSUtil::LoggingMutex _m;

            ChunkingConfiguration _chunkingConf;
            CommAdaptorManager _adaptMgr;
            CallbackHandler _cbackHandler;
            UserRequests _userReqs;

            NOMADSUtil::PtrLList<BaseController> _controllers;
    };
}

#endif  /* INCL_DSPRO_IMPLEMENTATION_H */
