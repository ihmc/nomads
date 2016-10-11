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
#define	INCL_DSPRO_H

#include "CommAdaptorManager.h"

namespace NOMADSUtil
{
    class AVList;
    class ConfigManager;
}

namespace IHMC_ACI
{
    class DSProImpl;

    class ControlMessageListener;
    class DisseminationService;
    class DSProListener;
    class MatchmakingLogListener;
    class MetadataConfiguration;
    class MetadataInterface;
    class NodePath;
    class SearchListener;
    class SQLAVList;

    class DSPro
    {
        public:
            static const char * const NODE_NAME;
            static const char * const ENABLE_DISSERVICE_ADAPTOR;
            static const char * const ENABLE_MOCKETS_ADAPTOR;
            static const char * const ENABLE_TCP_ADAPTOR;
            static const char * const ENABLE_TOPOPLOGY_EXCHANGE;
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

            int addCustumPoliciesAsXML (const char **ppszCustomPoliciesXML);

            /**
             * Add a new path to the node context.
             */
            int registerPath (NodePath *pPath);

            int addUserId (const char *pszUserName);
            int setMissionId (const char *pszMissionName);

            /**
             * Choose what path the node is following now.
             */
            int setCurrentPath (const char *pszPathID);

            /**
             * A time stamp is set automatically by DisServicePro.
             */
            int setCurrentPosition (float fLatitude, float fLongitude, float Altitude,
                                    const char *pszLocation, const char *pszNote);

            int setBatteryLevel (unsigned int uiBatteryLevel);
            int setMemoryAvailable (unsigned int uiMemoryAvailable);

            /**
             * Connect to a peer that is listening pszRemoteAddr:ui16Port,
             * using an adaptor of type "type.
             *
             * NOTE: currently, the only supporter adaptor is AdaptorType::MOCKETS
	         * and AdaptorType::TCP.
             */
            int addPeer (AdaptorType protocol, const char *pszNetworkInterface,
                         const char *pszRemoteAddress, uint16 ui16Port);

            int getAdaptorType (AdaptorId adaptorId, AdaptorType &adaptorType);

            /**
             * Return the current path of the local peer
             */
            NodePath * getCurrentPath (void);

            /**
             * Returns the list of the known peer node IDs
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
             * Returns the list of DSPro identifiers that correspond to the
             * object id in pszObjectId, and to the instance id in pszInstanceId.
             * If pszInstanceId is set to NULL, then all the DSPro ids that
             * correspond to pszObjectId are returned.
             * NOTE: the returned DSPro identifiers are the IDs of the metadata part
             *       of the message.
             */
            char ** getDSProIds (const char *pszObjectId, const char *pszInstanceId);

            /**
             * Returns the metadata messages whose metadata attributes match
             * the ones specified in pAVQueryList, and whose SOURCE_TIME is
             * within i64BeginArrivalTimestamp and i64EndArrivalTimestamp.
             * If i64BeginArrivalTimestamp and i64EndArrivalTimestamp are both
             * set to 0, then no constraint is specified on SOURCE_TIME.
             *
             * NOTE: the returned string, representing the XML document, should
             * be freed by the caller after use.
             */
            char ** getMatchingMetadataAsXML (NOMADSUtil::AVList *pAVQueryList,
                                              int64 i64BeginArrivalTimestamp = 0,
                                              int64 i64EndArrivalTimestamp = 0);

            /**
             * Method that lets the application explicitly give a feedback about
             * the usefulness of the data identified by pszMessageID.
             */
            int notUseful (const char *pszMessageID);

            /**
             * Requests a portion of a certain object identified by pszBaseObjectMsgId
             */
            int requestCustomAreaChunk (const char *pszChunkedObjMsgId, const char *pszMIMEType,
                                        uint32 ui32StartXPixel, uint32 ui32EndXPixel, uint32 ui32StartYPixel,
                                        uint32 ui32EndYPixel, uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds);

            /**
             * Requests a portion of a certain object identified by pszBaseObjectMsgId
             */
            int requestCustomTimeChunk (const char *pszChunkedObjMsgId, const char *pszMIMEType,
                                        int64 i64StartTimeInMillisec, int64 i64EndTimeInMillisec,
                                        uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds);

            /**
             * Requests one more chunk, for the chunked data message identified
             * by pszChunkedMsgId
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
             * pszQueryQualifiers -
             * pQuery - the query itself
             * uiQueryLen - the length of the query
             * i64TimeoutInMilliseconds - the time after which will stop searching
             *                            regardless on whether a search repyly was
             *                            received or not.  If set to 0 DSPro keeps
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
             * by pszQueryId.
             *
             * pszQueryId -
             * ppszMatchingMsgIds - a null-terminated array of string representing
             *                      the IDs of the message that match the query.
             */
            int searchReply (const char *pszQueryId, const char **ppszMatchingMsgIds);

            int volatileSearchReply (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen);

            /**
             * The message is stored and its metadata is sent to the nodes which
             * node context matches the metadata describing the data.
             *
             * - pszGroupName: the group of the message.
             * - pszObejctId
             * - pszInstanceId
             * - pszXmlMedatada: null-terminated string that contains and XML
             *                   document describing the data
             * - pMedatadaAttrList: as an alternative to pszXmlMedatada, an
             *                      instance of pMedatadaAttrList can be used to
             *                      specify the values of the metadata.
             * - pData: the actual data to be sent
             * - ui32DataLen: the length of the actual data to be sent
             * - i64ExpirationTime: the expiration time of the message.
             *                      (if set to 0, the message never expires)
             * - ppszId: the ID assigned to the data part of the message.
             *           (it should be deallocated by the caller).
             */
            int addMessage (const char *pszGroupName, const char *pszObjectId,
                            const char *pszInstanceId, const char *pszXmlMedatada,
                            const void *pData, uint32 ui32DataLen,
                            int64 i64ExpirationTime, char **ppszId);
            int addMessage (const char *pszGroupName, const char *pszObjectId,
                            const char *pszInstanceId, SQLAVList *pMedatadaAttrList,
                            const void *pData, uint32 ui32DataLen,
                            int64 i64ExpirationTime, char **ppszId);

            /**
             * If the data of the message is of one of the supported MIME types,
             * the message is chunked in smaller, individually intelligible, but
             * containing lower resolution or incomplete data, messages.  These
             * messages are then stored and the metadata is sent to the nodes which
             * node context matches the metadata describing the data.
             * The node receiving the metadata can retrieve individual chunks.
             * If the MIME type of the data is not supported, then chunkAndAddMessage()
             * behaves as addMessage().
             *
             * - pszGroupName: the group of the message.
             * - pszObejctId
             * - pszInstanceId
             * - pszXmlMedatada: null-terminated string that contains and XML
             *                   document describing the data
             * - pMedatadaAttrList: as an alternative to pszXmlMedatada, an
             *                      instance of pMedatadaAttrList can be used to
             *                      specify the values of the metadata.
             * - pData: the actual data to be sent
             * - ui32DataLen: the length of the actual data to be sent
             * - pszDataMimeType: the MIME type of the message
             * - i64ExpirationTime: the expiration time of the message.
             *                      (if set to 0, the message never expires)
             * - ppszId: the ID assigned to the data part of the message.
             *           (it should be deallocated by the caller).
             */
            int chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                                    const char *pszInstanceId, const char *pszXmlMedatada,
                                    const void *pData, uint32 ui32DataLen,
                                    const char *pszDataMimeType,
                                    int64 i64ExpirationTime, char **ppszId);
            int chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                                    const char *pszInstanceId, SQLAVList *pMedatadaAttrList,
                                    const void *pData, uint32 ui32DataLen,
                                    const char *pszDataMimeType,
                                    int64 i64ExpirationTime, char **ppszId);

            /**
             * Add metadata annotation to the object identified by pszReferredObject.
             * This metadata annotation is sent to the nodes which node context
             * matches the metadata annotation.
             *
             * - pszGroupName: the group of the message.
             * - pszObejctId
             * - pszInstanceId
             * - pszXmlMedatada: null-terminated string that contains and XML
             *                   document describing the data
             * - pMedatadaAttrList: as an alternative to pszXmlMedatada, an
             *                      instance of pMedatadaAttrList can be used to
             *                      specify the values of the metadata.
             * - pszReferredObject: the id of the messsage that the annotation
             *                      refers to.
             * - i64ExpirationTime: the expiration time of the message.
             *                      (if set to 0, the message never expires)
             * - ppszId: the ID assigned to the data part of the message.
             *           (it should be deallocated by the caller). 
             */
            int addAnnotation (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, const char *pszXmlMedatada,
                               const char *pszReferredObject, int64 i64ExpirationTime,
                               char **ppszId);
            int addAnnotation (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, SQLAVList *pszMedatada,
                               const char *pszReferredObject, int64 i64ExpirationTime,
                               char **ppszId);

            /**
             * Cancel the message with ID pszId from DSPro. (It may still be
             * kept in the cache though, just for serving requests. It will
             * no longer be considered for matchmaking though).
             */
            int cancel (const char *pszId);

            DisseminationService * getDisService (void);
            const char * getNodeId (void) const;
            NOMADSUtil::String getSessionId (void) const;

            // DSPro Listener

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

            // Search Listener
            int registerSearchListener (uint16 ui16ClientId, SearchListener *pListener, uint16 &ui16AssignedClientId);
            int deregisterSearchListener (uint16 ui16ClientId, SearchListener *pListener);

            int reloadCommAdaptors (void);

            /**
             * When the behavior of a node is to be connected for some time,
             * then disconnected for some time and then connected again and so on,
             * the application may wish to reset the transmission counters upon
             * reconnection so the communication won't suffer from the period of
             * unreachability.
             * This feature is currently only supported for mockets adaptors.
             */
            void resetTransmissionCounters (void);

            /**
             * The transmission history maintains a list of the replicated
             * messages for each peer, in order to avoid replicating the same
             * message multiple times.
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
            MetadataConfiguration *_pMetadataConf;
    };
}

#endif	/* INCL_DSPRO_H */

