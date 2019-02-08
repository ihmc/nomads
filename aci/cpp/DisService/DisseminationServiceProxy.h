/*
 * DisseminationServiceProxy.h
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

#ifndef INCL_DISSEMINATION_SERVICE_PROXY_H
#define INCL_DISSEMINATION_SERVICE_PROXY_H

#include "CommHelper2.h"
#include "FTypes.h"
#include "LoggingMutex.h"
#include "Socket.h"
#include "TCPSocket.h"
#include "Thread.h"
#include "ManageableThread.h"
#include "StrClass.h"
#include "SemClass.h"
#include "LList.h"
#include <stddef.h>

namespace IHMC_ACI
{
    class DisseminationServiceProxyCallbackHandler;
    class DisseminationServiceProxyListener;
    class PeerStatusListener;
    class SearchListener;

    class DisseminationServiceProxy: public NOMADSUtil::ManageableThread
    {
        public:
            DisseminationServiceProxy (uint16 ui16ApplicationId = 0);
            virtual ~DisseminationServiceProxy (void);

            /**
             * Initialize the proxy by connecting it to the DisseminationService Proxy Server
             * By default, connects to the proxy on localhost (127.0.0.1)
             *
             * Returns 0 if successful or a negative value in case of error
             */
            int init (const char *pszHost = NULL, uint16 ui16Port = 0, bool bUseBackgroundReconnect = false);

            int getNodeId (char *&pszNodeId);
            int getPeerList (char **&ppszPeerList);

            int store (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                       const char *pszMimeType, const void *pMetadata, uint32 ui32MetadataLength,
                       const void *pData, uint32 ui32DataLength, int64 i64ExpirationTime,
                       uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf,
                       uint32 ui32IdBufLen);

            int push (char *pszMsgId);

            /**
             * Disseminates data to nodes belonging to the specified group that are reachable.
             * Returns 0 if successful or a negative value in case of error.
             *
             * pszGroupName - specifies the group to which this data should be disseminated.
             *                QUESTION: If we use hierarchical groups (eg. a.b.c), would it be useful to
             *                          be able to use wildcards (e.g., send it to a.* or *.*.c)?
             *
             * pData - the data itself.
             *
             * ui32Length - the size of the data buffer in bytes.
             *
             * ui32Expiration - the expiration time for the data specified in milliseconds.
             *                  Beyond this time, the data will no longer be disseminated to any new nodes.
             *                  A value of 0 indicates there is no expiration time.
             *
             * bReliable - used to indicate that the data transmission should be reliable and not just best effort.
             *             If this is true, the sender will retransmit the data until acknowledged by the recipient.
             *             QUESTION: What are the semantics of reliable transmission? Is it point to point or end to end?
             *
             * bSequenced - used to indicate that the data delivery should be sequential and in the same order
             *              that it was pushed.
             *              NOTE: If sequencing is requested but not reliability, the delivery of message n will
             *                    prevent message n-1 from being delivered later.
             *              NOTE: If reliable delivery is requested and an expiration time is specified,
             *                    that implies that once message n-1 has expired, message n can be delivered
             *                    even if n-1 was never delivered to a node.
             *              ISSUE: Think about how this can be realized efficiently.
             *
             * ui16Tag - the tag value identifying the type (application defined) for the data
             *
             * ui8Priority - the priority value for the data, with higher values indicated higher priority
             *
             * pszIdBuf - the buffer into which the id of this message will be copied.
             *            The complete message id will be a string of the form "<nodeId>.<clientId>.<groupName>.<msgSeqNo>".
             *            NOTE: This parameter may be NULL if the caller does not need to receive the id of the message.
             *
             * ui32IdBufLen - the length of the buffer to receive the message id.
             *                NOTE: if the buffer is too small to hold the id, the id will not be copied into the buffer.
             */
            int push (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType,
                      const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32Length, int64 i64ExpirationTime,
                      uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen);

            /**
             * Makes the specified data available and disseminates the associated metadata to nodes belonging to
             * the specified group that are reachable. The data is stored in the network until the specified
             * expiration time, given storage constraints. The data may be split into chunks that are distributed
             * and replicated among several nodes.
             * Returns 0 if successful or a negative value in case of error
             *
             * pszGroupName - see comments for the push() method.
             *
             * pMetadata - any associated metadata for this data. May be null.
             *
             * ui32MetadataLength - the length of the metadata.
             *
             * pData - the data itself.
             *
             * ui32Length - the size of the data buffer in bytes.
             *
             * ui32Expiration - see comments for the push() method. The expiration time applies to both the
             *                  metadata and the data itself.
             *
             * bReliable - see comments for the push() method.
             *
             * bSequenced - see comments for the push() method.
             *
             * ui16Tag - see comments for the push() method.
             *
             * ui8Priority - see comments for the push() method.
             */
            int makeAvailable (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                               const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32Length,
                               const char *pszDataMimeType, int64 i64Expiration, uint16 ui16HistoryWindow,
                               uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen);

            /**
             * Same as the previous version of makeAvailable, except that the data is read from a file.
             */
            int makeAvailable (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                               const void *pMetadata, uint32 ui32MetadataLength, const char *pszFilePath,
                               const char *pszDataMimeType, int64 i64Expiration, uint16 ui16HistoryWindow,
                               uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen);

            /**
             * Cancel a data message that has been pushed or made available in the past
             * Returns 0 if successful or a negative value in case of error
             *
             * pszId - the identifier of the message to cancel
             */
            int cancel (const char *pszId);

            /**
             * Cancel all messages tagged with the specified tag
             * Returns 0 if successful or a negative value in case of error
             *
             * ui16Tag - the tag marking the set of messages that should be cancelled.
             */
            int cancel (uint16 ui16Tag);

            /**
             * Filter (i.e., prevent delivery) of any incoming messages that match the specified tag.
             * Returns 0 if successful or a negative value in case of error.
             *
             * pszGroupName - the name - may be null if the filter should apply to all subscribed groups.
             *
             * ui16Tag - the tag that identifies messages that should be filtered.
             */
            int addFilter (const char *pszGroupName, uint16 ui16Tag);

            /**
             * Remove a previously specified filter.
             * Returns 0 if successful or a negative value in case of error.
             *
             * pszGroupName - the name - may be null if the filter was for all subscribed groups.
             *
             * ui16Tag - the tag that was specified earlier as part of a filter.
             */
            int removeFilter (const char *pszGroupName, uint16 ui16Tag);

             /**
             * Subscribe to the specified group. The application will subsequently receive any messages
             * addressed to matching groups that are received.
             * Returns 0 if successful or a negative value in case of error.
             *
             * pszGroupName - the name of the group to subscribe / join
             *                QUESTION: Should wildcards be allowed in the subscription (e.g., "a.b.*")?
             */
            int subscribe (const char *pszGroupName, uint8 ui8Priority, bool bGroupReliable, bool MsgReliable, bool bSequenced);

            /**
             * Unsubscribe to the previously subscribed group.
             * NOTE: This method may also be used to clear ALL subscriptions made with the group name + tag option
             * Returns 0 if successful or a negative value in case of error.
             *
             * pszGroupName - the name of the group to unsubscribe.
             *                QUESTION: Should wildcards be allowed in the unsubscribe request?
             */
            int unsubscribe (const char *pszGroupName);

            /**
             * Subscribe to the specified group and the specified tag. Only messages addressed to that group
             * and that have the specified tag will be delivered.
             * Returns 0 if successful or a negative value in case of error.
             *
             * pszGroupName - the name of the group to subscribe / join.
             *                QUESTION: Should wildcards be allowed in the unsubscribe request?
             *
             * ui16Tag - the tag specifiying data the node wants to receive
             */
            int subscribe (const char *pszGroupName, uint16 ui16Tag, uint8 ui8Priority, bool bGroupReliable, bool MsgReliable, bool bSequenced);

            int subscribeWithXPathPredicate (const char *pszGroupName, const char *pszXPathPredicate, uint8 ui8Priority, bool bGroupReliable, bool MsgReliable, bool bSequenced);

            /**
             * Unsubscribe to the specified group and the specified tag.
             * NOTE: The first version of unsubscribe(), which only takes the group name as an argument, can also be used
             *       if the goal is to unsubscribe enmasse to all subscribed groups and tags.
             * Returns 0 if successful or a negative value in case of error
             *
             * pszGroupName - the name of the group to unsubscribe
             *                QUESTION: Should wildcards be allowed in the unsubscribe request?
             *
             * ui16Tag - the tag specifiying data the node does not want to receive
             */
            int unsubscribe (const char *pszGroupName, uint16 ui16Tag);

            /**
             * Register a listener that is used to notify the application about the arrival of new data
             * that has been pushed or metadata about a message that has been made available.
             *
             * Returns 0 if successful or a negative value in case of error.
             *
             * pListener - the listener that will be invoked upon arrival of data.
             */
            int registerDisseminationServiceProxyListener (DisseminationServiceProxyListener *pListener);

            /**
             * Register a listener that is used to notify the application about the arrival/leave
             * of peers
             *
             * @return
             */
            int registerPeerStatusListener (PeerStatusListener *pListener);

            /**
             * Register a listener that is used to notify the application about the arrival
             * of search queries.
             *
             * @return
             */
            int registerSearchListener (SearchListener *pListener);

            /**
             * Retrieve data that is identified by the specified id. The data must have been made available
             * by some (other) node and the local node must have received the metadata associated with the data.
             * The retrieved data is stored in the specified buffer.
             * Returns a positive value with the size of the data that has been retrieved or
             * a negative value in case of error.
             *
             * pszId - the id of the message whose data is to be retrieved.
             *
             * pBuf - the buffer into which the data should be stored.
             *
             * ui32BufSize - the size of the buffer.
             */
            int retrieve (const char *pszId, void **ppBuf, uint32 *ui32BufSize, int64 i64Timeout);

            /**
             * Same as the previous retrieve except that the data is stored in a file instead of an internal buffer.
             *
             * pszId - the id of the message whose data is to be retrieved.
             *
             * pszFilePath - the path to the file that should be created in order to store the data being retrieved.
             *               If the file exists, the contents will be overwritten.
             *
             * i64Timeout - the maximum time the data must be retrieved.
             */
            int retrieve (const char *pszId, const char *pszFilePath, int64 i64Timeout);

            /**
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients.
             *
             * pszGroupName - the name of the group to subscribe / join.
             *                QUESTION: Should wildcards be allowed in the request invocation?
             *
             * ui16Tag - the tag that identifies messages that should be requested.
             *           QUESTION: How should tags be used with respect to sequencing?
             *
             * ui16HistoryLength - the number of messages previous the first one
             *                     received
             *
             * i64RequestTimeout - the maximum length of time, in milliseconds, for which the request operation will be active
             */
            int request (const char *pszGroupName, uint16 ui16Tag, uint16 ui16HistoryLength, int64 i64RequestTimeout);

            int requestMoreChunks (const char *pszMsgId);

            int requestMoreChunks (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId);

            int search (const char *pszGroupName, const char *pszQueryType,
                        const char *pszQueryQualifiers, const void *pszQuery,
                        unsigned int uiQueryLen, char **pszQueryId);
            int searchReply (const char *pszQueryId, const char **ppszMatchingQueryIds);

            /**
             * Returns true if we are currently connected to the DisService proxy server, false otherwise.
             */
            bool isConnected();

            // Need some approach to discard (or not retrieve) queued up data (which might have been published while this node was disconnected)

            // Need some approach to examine the set of available data which can be retrieved

            // Handles background reconnects to the DisService server
            void run();

        public:    // The following should actually be private
            NOMADSUtil::CommHelper2 * connectToServer (const char *pszHost, uint16 ui16Port);

            int registerProxy (NOMADSUtil::CommHelper2 *pch, NOMADSUtil::CommHelper2 *pchCallback,
                               uint16 ui16DesiredApplicationId);

            bool dataArrived (const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                              const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType,
                              const void *pData, uint32 ui32Length, uint32 ui32MetadataLength,
                              uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

            bool chunkArrived (const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                               const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType,
                               const void *pChunk, uint32 ui32Length, uint8 ui8NChunks, uint8 ui8TotNChunks,
                               const char *pszChunkedMsgId, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

            bool metadataArrived (const char *pszSender, const char *pszGroupName,  uint32 ui32SeqId,
                                  const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType,
                                  const void *pMetadata, uint32 ui32MetadataLength, bool bDataChunked,
                                  uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

            bool dataAvailable (const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                                const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType,
                                const char *pszId, const void *pMetadata, uint32 ui32MetadataLength,
                                uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

            bool newPeer (const char *pszPeerId);
            bool deadPeer (const char *pszPeerId);

            bool searchArrived (const char *pszQueryId, const char *pszGroupName,
                                const char *pszQuerier, const char *pszQueryType,
                                const char *pszQueryQualifiers,
                                const void *pszQuery, unsigned int uiQueryLen);

        protected:
            // used to store subscription info for later reconnect
            class SubscriptionInfo
            {
                public:
                    const NOMADSUtil::String _sGroupName;
                    const uint16 _ui16Tag;
                    const uint8 ui8Priority;
                    const bool bUsingTag;
                    const bool bReliable;
                    const bool bMsgReliable;
                    const bool bSequenced;

                    SubscriptionInfo (const char *pszGroupName, uint16 ui16Tag, uint8 ui8Priority,
                                      bool bUsingTag, bool bReliable, bool bMsgReliable, bool bSequenced)
                        : _sGroupName (pszGroupName),
                          _ui16Tag (ui16Tag),
                            ui8Priority (ui8Priority),
                            bUsingTag (bUsingTag),
                            bReliable (bReliable),
                            bMsgReliable (bMsgReliable),
                            bSequenced (bSequenced)
                    {
                    }
            };

            NOMADSUtil::CommHelper2 *_pCommHelper;
            DisseminationServiceProxyListener *_pListener;
            PeerStatusListener *_pPeerStatusListener;
            SearchListener *_pSearchListener;
            DisseminationServiceProxyCallbackHandler *_pHandler;
            NOMADSUtil::LoggingMutex _mutex;
            NOMADSUtil::LoggingMutex _mutexReconnect;
            uint16 _ui16ApplicationId;
            NOMADSUtil::String _sHost;
            uint16 _ui16Port;
            bool _bUsingBackgroundReconnect;
            bool _bReconnectStarted;
            NOMADSUtil::Semaphore *_pReconnectSemaphore;

            NOMADSUtil::LList<SubscriptionInfo *> _llSubscriptionInfoList;

            int tryConnect (void);
            bool startReconnect (void);
            bool checkConnection (void);
            int registerSubscriptionWithServer (SubscriptionInfo *sub);
            void registerDisseminationServiceProxyListenerWithServer (void);
            void registerPeerStatusListenerWithServer (void);
            void registerSearchListenerWithServer (void);

            friend class DisseminationServiceProxyCallbackHandler;
    };
}

#endif   // #ifndef INCL_DISSEMINATION_SERVICE_PROXY_H
