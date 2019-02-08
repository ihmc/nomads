/*
 * DSPro.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on June 26, 2012, 10:13 PM
 */

#ifndef INCL_DSPRO_H
#define INCL_DSPRO_H

#include "DSProInterface.h"
#include "CommAdaptorManager.h"
#include "ChunkReassembler.h"

namespace IHMC_MISC
{
    class ChunkReassemblerInterface;
}

namespace NOMADSUtil
{
    class AVList;
    class ConfigManager;
}

namespace IHMC_VOI
{
    class MetadataInterface;
    class NodePath;
}

namespace IHMC_ACI
{
    class DSProImpl;

    class ControlMessageListener;
    class DisseminationService;
    class DSProListener;
    class MatchmakingLogListener;
    class MetadataConfigurationImpl;
    class Reset;
    class SearchListener;
    class Stats;


    class DSPro : public DSProInterface
    {
        public:
            static const char * const NODE_NAME;
            static const char * const ENABLE_DISSERVICE_ADAPTOR;
            static const char * const ENABLE_MOCKETS_ADAPTOR;
            static const char * const ENABLE_TCP_ADAPTOR;
            static const char * const ENABLE_UDP_ADAPTOR;
            static const char * const ENABLE_NATS_ADAPTOR;
            static const char * const ENABLE_TOPOPLOGY_EXCHANGE;
            static const char * const PEER_ID_NAME;
            static const char * const PEER_MSG_COUNTS_JSON_ARRAY_NAME;
            static const int64 DEFAULT_UPDATE_TIMEOUT;     // Timeout value used by NodeContextManager::run() to send
                                                           // periodically way point update messages.
            static const int64 DEFAULT_REPLICATE_TIMEOUT;
            static const char * const SQL_QUERY_TYPE;

            /**
             * DSPro contructor:
             * - pszNodeId: null-terminated string that uniquely identifies
             *              the node in the network, it can't be null.
             * - pszVersion: a string identifying the version of DSPro.
             *               It may be null.
             */
            DSPro (const char *pszNodeId, const char *pszVersion);
            virtual ~DSPro (void);

            const char * getVersion (void) const;

            /**
             * NOTE: init() should be called after the instantiation of a DSPro
             * object, before calling any other method.
             *
             * It returns 0, if the initialization was successful, a negative
             * number otherwise.
             */
            int init (NOMADSUtil::ConfigManager *pCfgMgr,
                      const char *pszMetadataExtraAttributes,
                      const char *pszMetadataValues);

            int configureProperties (NOMADSUtil::ConfigManager *pCfgMgr);
            int changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len);
            int changeSessionId (unsigned char *pszSessionId);

            /**
             * - Coordinate Ranking Weight
             * - Time Ranking Weight
             * - Expiration Ranking Weight
             * - Importance Ranking Weight
             * - Prediction Ranking Weight
             * - Target Ranking Weight
             * - bStrictTarget: if set to true, when the target is specified,
             *   all the peer node context of peer != target will have rank 0
             *
             * NOTE: dspro must have been initialized before calling
             */
            int setRankingWeigths (float coordRankWeight, float timeRankWeight,
                                   float expirationRankWeight, float impRankWeight,
                                   float sourceReliabilityRankWeigth,
                                   float informationContentRankWeigth, float predRankWeight,
                                   float targetWeight, bool bStrictTarget,
                                   bool bConsiderFuturePathSegmentForMatchmacking);

            /**
             * - matchingThreshold: within 0.0 and 10.0.  0.0 means no selection (every message
             *   will match), 10.0 maximum selectiviness
             */
            int setSelectivity (float matchingThreshold);

            /**
             * Sets the range of relevance (bounding box) of an object of type NodeType
             */
            int setRangeOfInfluence (const char *pszNodeType, uint32 ui32RangeInMeters);

            /**
             * Set the distance from this node's path after which an object will never match
             */
            int setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters);

            /**
             * Set the distance from this node's path after which an object of the
             * specified MIME type will never match
             */
            int setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters);

            /**
             * Add new policies to the VoI computation
             */
            int addCustumPoliciesAsXML (const char **ppszCustomPoliciesXML);

            /**
             * Add a new path to the node context.
             */
            int registerPath (IHMC_VOI::NodePath *pPath);

            /**
             * Choose what path the node is following now.
             */
            int setCurrentPath(const char *pszPathID);

            /**
             * Adds a new User ID to this instance (will be used to match Target).
             */
            int addUserId (const char *pszUserName);

            /**
             * Adds a new Mission ID to this instance (will be used to match Target).
             */
            int setMissionId (const char *pszMissionName);

            /**
             * Adds a new Role to this instance (will be used to match Target).
             */
            int setRole (const char *pszRole);

            /**
             * Adds a new Team ID to this instance (will be used to match Target).
             */
            int setTeamId (const char *pszTeamId);

            // Whether the node is a ground, sea, air vehicle (usually specified using milstd2525)
            int setNodeType (const char *pszType);

            /**
             * Specifies a new area of interest to do matchmaking
             */
            int addAreaOfInterest (const char *pszAreaName, float fUpperLeftLat, float fUpperLeftLon,
                                   float fLowerRightLat, float fLowerRightLon,
                                   int64 i64StatTime, int64 i64EndTime);


            /**
             * Updates the current position in the NodeContext.
             * A timestamp is set automatically by DisServicePro.
             */
            int setCurrentPosition (float fLatitude, float fLongitude, float Altitude,
                                    const char *pszLocation, const char *pszNote);

            /**
             * Updates battery level info in the NodeContext.
             */
            int setBatteryLevel (unsigned int uiBatteryLevel);

            /**
             * Updates free memory (storage?) info in the NodeContext.
             */
            int setMemoryAvailable (unsigned int uiMemoryAvailable);

            /**
             * Connect to a peer that is listening pszRemoteAddr:ui16Port,
             * using an adaptor of type "type".
             *
             * NOTE: currently, the only supporter adaptor is AdaptorType::MOCKETS
             * and AdaptorType::TCP.
             */
            int addPeer (AdaptorType protocol, const char *pszNetworkInterface,
                         const char *pszRemoteAddress, uint16 ui16Port);

            /**
             * Updates free memory (storage?) info in the NodeContext.
             */
            int getAdaptorType (AdaptorId adaptorId, AdaptorType &adaptorType);

            /**
             * Return the current path of the local peer
             */
            IHMC_VOI::NodePath * getCurrentPath (void);

            /**
             * Returns the null-terminated list of the known peer node IDs.
             * NOTE: the list must be deallocated by the caller
             */
            char ** getPeerList (void);

            /**
             * Returns the data identified by pszId. If the data is not
             * currently available in the local data cache, null is returned.
             * If the data ever arrives at a later point, it is asynchronously
             * returned via the dataArrived() callback in DSProListener.
             *
             * NOTE: the returned data must be deallocated by the caller by
             * using release()
             */
            int getData (const char *pszId, const char *pszCallbackParameter,
                         void **ppData, uint32 &ui32DataLen,
                         bool &bHasMoreChunks);
            int release (const char *pszMessageID, void *pData);

            /**
             * Returns the list of DSPro message identifiers that correspond to the
             * object id in pszObjectId, and to the instance id in pszInstanceId.
             * If pszInstanceId is set to nullptr, then all the DSPro message ids
             * that correspond to pszObjectId are returned.
             * NOTE: the returned DSPro identifiers are the IDs of the metadata part
             *       of the message.
             */
            char ** getDSProIds (const char *pszObjectId, const char *pszInstanceId);

            /**
             * Returns the metadata messages whose metadata attributes match
             * the ones specified in pAVQueryList, and whose SOURCE_TIME
             * (time of object creation) iswithin i64BeginArrivalTimestamp
             * and i64EndArrivalTimestamp.
             * If i64BeginArrivalTimestamp and i64EndArrivalTimestamp are both
             * set to 0, then no constraint is specified on SOURCE_TIME.
             *
             * NOTE: the returned string, representing the JSON document,
             * should be freed by the caller after use.
             */
            char ** getMatchingMetadata (NOMADSUtil::AVList *pAVQueryList,
                                         int64 i64BeginArrivalTimestamp = 0,
                                         int64 i64EndArrivalTimestamp = 0);

            /**
             * Method that lets the application explicitly give a feedback about
             * the usefulness of the data identified by pszMessageID.
             *
             * NOTE: used only by the C45 classifier.
             */
            int notUseful (const char *pszMessageID);

            /**
             * Requests a portion (identified by: ui32StartXPixel, ui32EndXPixel,
             * ui32StartYPixel, ui32EndYPixelof) of a certain object identified
             * by pszBaseObjectMsgId
             */
            int requestCustomAreaChunk (const char *pszChunkedObjMsgId, const char *pszMIMEType,
                                        uint32 ui32StartXPixel, uint32 ui32EndXPixel, uint32 ui32StartYPixel,
                                        uint32 ui32EndYPixel, uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds);

            /**
             * Requests a portion (limited by: i64StartTimeInMillisec, i64EndTimeInMillisec)
             * of a certain object identified by pszBaseObjectMsgId.
             *
             * NOTE: for videos only; probably will not work
             */
            int requestCustomTimeChunk (const char *pszChunkedObjMsgId, const char *pszMIMEType,
                                        int64 i64StartTimeInMillisec, int64 i64EndTimeInMillisec,
                                        uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds);

            /**
             * Requests one more chunk, for the chunked data message identified
             * by pszChunkedMsgId.
             *
             * pszCallbackParameter will be returned as a parameter in the callback;
             * it can be use as an ID to identify the request.
             */
            int requestMoreChunks (const char *pszChunkedMsgId, const char *pszCallbackParameter);

            /**
             * Search data based on the given groupName and a query on the
             * metadata fields. The method returns the query ID, or null in case
             * of error.
             *
             * pszGroupName - a group name for the search
             * pszQueryType - the type of the query. It is used to identify
             *                the proper search controller to handle the query
             * pszQueryQualifiers - additional parameters beside the query
             *                      (potentially only used by an ApplicationController)
             * pQuery - the query itself
             * uiQueryLen - the length of the query
             * i64TimeoutInMilliseconds - the time after which will stop searching
             *                            regardless on whether a search reply was
             *                            received or not. If set to 0, DSPro keeps
             *                            searching indefinitely until a search reply
             *                            is received.
             * ppszQueryId - a unique identifier that is generated for the query.
             */
            int search (const char *pszGroupName, const char *pszQueryType,
                        const char *pszQueryQualifiers, const void *pszQuery,
                        unsigned int uiQueryLen, int64 i64TimeoutInMilliseconds,
                        char **ppszQueryId);

            /**
             * Method to return the list of IDs that match the query identified
             * by pszQueryId. Registered as a callback.
             *
             * pszQueryId - query identifier
             * ppszMatchingMsgIds - a null-terminated array of string representing
             *                      the IDs of the message that match the query.
             */
            int searchReply (const char *pszQueryId, const char **ppszMatchingMsgIds);

            /**
             * Method to return the answer of a query identified by pszQueryId that
             * does not contain a message. The answer is contained in pReply.
             * Registered as a callback.
             *
             * pszQueryId - query identifier
             * pReply - query result
             * ppszMatchingMsgIds - a null-terminated array of string representing
             *                      the IDs of the message that match the query.
             */
            int volatileSearchReply (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen);

            /**
             * The message is stored and its metadata is sent to the nodes whose
             * node context matches the metadata describing the data.
             *
             * - pszGroupName: the group of the message.
             * - pszObejctId: an application-defined ID for the object being added.
             * - pszInstanceId: an application-defined instance ID for the object.
             * - pszJsonMetadata: null-terminated string that contains a JSON
             *                    document describing the data.
             * - pMetadataAttrList: as an alternative to pszJsonMetadata, an
             *                      instance of pMetadataAttrList can be used to
             *                      specify the values of the metadata.
             * - pData: the actual data to be sent.
             * - ui32DataLen: the length of the actual data to be sent.
             * - i64ExpirationTime: the expiration time of the message
             *                      (if set to 0, the message never expires).
             * - ppszId: the ID assigned to the data part of the message
             *           (it should be deallocated by the caller).
             *
             * NOTE: second overload will be removed when we will move to LinguaFranca.
             */
            int addMessage (const char *pszGroupName, const char *pszObjectId,
                            const char *pszInstanceId, const char *pszJsonMetadata,
                            const void *pData, uint32 ui32DataLen,
                            int64 i64ExpirationTime, char **ppszId);
            int addMessage (const char *pszGroupName, const char *pszObjectId,
                            const char *pszInstanceId, NOMADSUtil::AVList *pMetadataAttrList,
                            const void *pData, uint32 ui32DataLen,
                            int64 i64ExpirationTime, char **ppszId);

            struct Chunk
            {
                uint32 ui32ChunkLen;
                void *pChunkData;
            };


            /**
             * The chunked message is stored and its metadata is sent to the nodes whose
             * node context matches the metadata describing the data.
             *
             * - pszGroupName: the group of the message.
             * - pszObejctId: an application-defined ID for the whole object being added.
             * - pszInstanceId: an application-defined instance ID for the whole object.
             * - pszJsonMetadata: null-terminated string that contains a JSON
             *                    document describing the data.
             * - pMetadataAttrList: as an alternative to pszJsonMetadata, an
             *                      instance of pMetadataAttrList can be used to
             *                      specify the values of the metadata.
             * - pChunkedData: the actual data of the chunk to be sent.
             * - ui8ChunkId: the ID of the chunk being added.
             * - ui8NChunks: the total number of chunks for the object.
             * - pszDataMimeType: the MIME type of the object being added.
             * - i64ExpirationTime: the expiration time of the message
             *                      (if set to 0, the message never expires).
             * - ppszId: the ID assigned to the data part of the message
             *           (it should be deallocated by the caller).
             *
             * NOTE: second overload will be removed when we will move to LinguaFranca.
             */
            int addChunkedData (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                const char *pszJsonMetadata, const Chunk *pChunkedData, uint8 ui8ChunkId, uint8 ui8NChunks,
                                const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId);
            int addChunkedData (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                NOMADSUtil::AVList *pMetadataAttrList, const Chunk *pChunkedData, uint8 ui8ChunkId, uint8 ui8NChunks,
                                const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId);
            int addAdditionalChunkedData (const char *pszId, const Chunk *pChunkedData, uint8 ui8ChunkId, const char *pszDataMimeType);


            /**
             * The chunked message is stored and its metadata is sent to the nodes whose
             * node context matches the metadata describing the data.
             *
             * - pszGroupName: the group of the message.
             * - pszObejctId: an application-defined ID for the whole object being added.
             * - pszInstanceId: an application-defined instance ID for the whole object.
             * - pszJsonMetadata: null-terminated string that contains a JSON
             *                    document describing the data.
             * - pMetadataAttrList: as an alternative to pszJsonMetadata, an
             *                      instance of pMetadataAttrList can be used to
             *                      specify the values of the metadata.
             * - ppChunkedData: the null-terminated list of all chunks of the object.
             * - ui8NChunks: the total number of chunks for the object.
             * - pszDataMimeType: the MIME type of the object being added.
             * - i64ExpirationTime: the expiration time of the message
             *                      (if set to 0, the message never expires).
             * - ppszId: the ID assigned to the data part of the message
             *           (it should be deallocated by the caller).
             *
             * NOTE: second overload will be removed when we will move to LinguaFranca.
             */
            int addChunkedData (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                const char *pszJsonMetadata, const Chunk **ppChunkedData, uint8 ui8NChunks,
                                const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId);
            int addChunkedData (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                NOMADSUtil::AVList *pMetadataAttrList, const Chunk **ppChunkedData, uint8 ui8NChunks,
                                const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId);

            /**
             * If the data of the message is of one of the supported MIME types,
             * the message is chunked in smaller, individually intelligible, but
             * containing lower resolution or incomplete data, messages. These
             * messages are then stored and the metadata is sent to the nodes whose
             * node context matches the metadata describing the data.
             * The node receiving the metadata can retrieve individual chunks.
             * If the MIME type of the data is not supported, then chunkAndAddMessage()
             * behaves as addMessage().
             *
             * - pszGroupName: the group of the message.
             * - pszObejctId: an application-defined ID for the object being added.
             * - pszInstanceId: an application-defined instance ID for the object.
             * - pszJsonMetadata: null-terminated string that contains and XML
             *                    document describing the data
             * - pMetadataAttrList: as an alternative to pszJsonMetadata, an
             *                      instance of pMetadataAttrList can be used to
             *                      specify the values of the metadata.
             * - pData: the actual data to be sent.
             * - ui32DataLen: the length of the actual data to be sent.
             * - pszDataMimeType: the MIME type of the message.
             * - i64ExpirationTime: the expiration time of the message
             *                      (if set to 0, the message never expires).
             * - ppszId: the ID assigned to the data part of the message
             *           (it should be deallocated by the caller).
             */
            int chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                                    const char *pszInstanceId, const char *pszJsonMetadata,
                                    const void *pData, uint32 ui32DataLen,
                                    const char *pszDataMimeType,
                                    int64 i64ExpirationTime, char **ppszId);
            int chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                                    const char *pszInstanceId, NOMADSUtil::AVList *pMetadataAttrList,
                                    const void *pData, uint32 ui32DataLen,
                                    const char *pszDataMimeType,
                                    int64 i64ExpirationTime, char **ppszId);

            /**
             * The message is disseminated (pushed within DisService) within a group
             * as a standard pub/sub message.
             * Nodes must subscribe to pszGroupName in order to receive the message.
             *
             * - pszGroupName: the group of the message.
             * - pszObejctId: an application-defined ID for the object being added.
             * - pszInstanceId: an application-defined instance ID for the object.
             * - pMetadata: a buffer contaning application-defined metadata.
             * - ui32MetadataLength: the length of the metadata.
             * - pData: the actual data to be sent.
             * - ui32Len: the length of the actual data to be sent.
             * - i64ExpirationTime: the expiration time of the message
             *                      (if set to 0, the message never expires).
             * - ppszId: the ID assigned to the data part of the message
             *           (it should be deallocated by the caller).
             */
            int disseminateMessage (const char *pszGroupName, const char *pszObjectId,
                                    const char *pszInstanceId, const void *pData, uint32 ui32Len,
                                    int64 i64ExpirationTime, char **ppszId);
            int disseminatedMessageMetadata (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                             const void *pMetadata, uint32 ui32MetadataLength,
                                             const void *pData, uint32 ui32Len, const char *pszMimeType,
                                             int64 i64ExpirationTime, char **ppszId);

            /**
             * The client can use this method to subscribe to the pszGroupName group.
             *
             * - pszGroupName: the group of the message.
             * - ui8Priority: a value in the [0, 255] range; the higher the value
             *                the greater the priority.
             * - bGroupReliable: true if the client wants to receive all messages
             *                   published within the group.
             * - bMsgReliable: true if the client wants to retrieve all fragments of a message.
             * - bSequenced: true if the client needs messages to be dispatched in order.
             */
            int subscribe (const char *pszGroupName, uint8 ui8Priority, bool bGroupReliable,
                           bool bMsgReliable, bool bSequenced);

            /**
             * Add metadata annotation to the object identified by pszReferredObject.
             * This metadata annotation is sent to the nodes which node context
             * matches the metadata annotation.
             *
             * - pszGroupName: the group of the message.
             * - pszObejctId: an application-defined ID for the object being annotated.
             * - pszInstanceId: an application-defined instance ID for the object
             *                  being annotated.
             * - pszJsonMetadata: null-terminated string that contains and XML
             *                   document describing the data.
             * - pMetadataAttrList: as an alternative to pszJsonMetadata, an
             *                      instance of pMetadataAttrList can be used to
             *                      specify the values of the metadata.
             * - pszReferredObject: the id of the message that the annotation
             *                      refers to.
             * - i64ExpirationTime: the expiration time of the message
             *                      (if set to 0, the message never expires).
             * - ppszId: the ID assigned to the new metadata message
             *           (it should be deallocated by the caller).
             */
            int addAnnotation (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, const char *pszJsonMetadata,
                               const char *pszReferredObject, int64 i64ExpirationTime,
                               char **ppszId);
            int addAnnotation (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, NOMADSUtil::AVList *pszMetadata,
                               const char *pszReferredObject, int64 i64ExpirationTime,
                               char **ppszId);

            /**
             * Cancel the message with ID pszId from DSPro.
             * The message may still be kept in the cache,
             * just for serving requests. It will no longer
             * be considered for matchmaking.
             *
             * - pszId: the ID assigned to the message to cancel.
             * - pszObjectId: the object ID assigned to the object to cancel.
             * - pszInstanceId: the instance ID assigned to the object to cancel.
             */
            int cancel (const char *pszId);
            int cancelByObjectId (const char *pszObjectId);
            int cancelByObjectAndInstanceId (const char *pszObjectId, const char *pszInstanceId);

            DisseminationService * getDisService (void);
            const char * getNodeId (void) const;

            /**
             * It returns the NodeContext for the node with ID pszNodeId.
             * It returns the NodeContext of the current node if szNodeId == nullptr.
             *
             * - pszId: the ID of the node whose NodeContext needs to be retrieved.
             */
            NOMADSUtil::String getNodeContext (const char * pszNodeId) const;

            /**
             * It returns the numer of messages received from the node with ID pszNodeId, grouped by interface ID.
             *
             * - pszId: the ID of the node whose message counts need to be retrieved.
             */
            NOMADSUtil::String getPeerMsgCounts (const char * pszNodeId) const;
            NOMADSUtil::String getSessionId (void) const;

            // DSPro Listener --> to register callbacks for dataArrived and MetadataArrived
            int registerDSProListener (uint16 ui16ClientId, DSProListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterDSProListener (uint16 ui16ClientId, DSProListener *pListener);

            // Matchmaking Log Listener
            int registerMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener);

            // Communication Adaptor Listener
            int registerCommAdaptorListener (uint16 ui16ClientId, CommAdaptorListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterCommAdaptorListener (uint16 ui16ClientId, CommAdaptorListener *pListener);

            // Control Message Listener
            int registerControlMessageListener (uint16 ui16ClientId, ControlMessageListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterControlMessageListener (uint16 ui16ClientId, ControlMessageListener *pListener);

            // Search Listener --> to register callbacks for searchArrived and searchReplyArrived
            int registerSearchListener (uint16 ui16ClientId, SearchListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterSearchListener (uint16 ui16ClientId, SearchListener *pListener);

            // Chunking Plugin Registration
            int registerChunkFragmenter (const char *pszMimeType, IHMC_MISC::ChunkerInterface *pChunker);
            int registerChunkReassembler (const char *pszMimeType, IHMC_MISC::ChunkReassemblerInterface *pReassembler);

            int deregisterChunkFragmenter (const char *pszMimeType);
            int deregisterChunkReassembler (const char *pszMimeType);

            int reloadCommAdaptors (void);

            /**
             * When the behavior of a node is to be connected for some time, then
             * disconnected for some time, and then connected again and so on,
             * the application may wish to reset the transmission counters upon
             * reconnection so the communication won't suffer from the period of
             * unreachability.
             *
             * NOTE: This feature is currently only supported for mockets adaptors.
             */
            void resetTransmissionCounters (void);

            /**
             * The transmission history maintains a list of the replicated
             * messages for each peer, in order to avoid replicating the same
             * message multiple times.
             * Replicated messages generally are metadata for which matchmaking
             * was successful and they were pushed to matched peers.
             * resetTransmissionHistory() can be used to reset it, if ever
             * necessary.
             *
             * NOTE: this method is not yet implemented.
             */
            void resetTransmissionHistory (void);

        private:
            friend class DSProCmdProcessor;
            friend class DSProProxyAdaptor;

            bool _bInitialized;
            DSProImpl *_pImpl;
            Stats *_pStats;
            Reset *_pReset;
            MetadataConfigurationImpl *_pMetadataConf;
    };
}

#endif    /* INCL_DSPRO_H */
