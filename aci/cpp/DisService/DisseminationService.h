/*
 * DisseminationService.h
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

#ifndef INCL_DISSEMINATION_SERVICE_H
#define INCL_DISSEMINATION_SERVICE_H

#include "DisServiceMsg.h"

#include "ManageableThread.h"
#include "LoggingMutex.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class Reader;
}

namespace IHMC_ACI
{
    class BandwidthSharing;
    class ChunkingConfiguration;
    class ChunkDiscoveryController;
    class ChunkRetrievalController;
    class DisServiceMsg;
    class DisServiceCtrlMsg;
    class DisServiceDataMsg;
    class DisServiceDataReqMsg;
    class DisServiceWorldStateSeqIdMsg;
    class DisServiceSubscriptionStateMsg;
    class DisServiceSubscriptionStateReqMsg;
    class DisServiceDataCacheQueryMsg;
    class DisServiceDataCacheQueryReplyMsg;
    class DisServiceDataCacheMessagesRequestMsg;
    class DisServiceCacheEmptyMsg;
    class DisServiceAcknowledgmentMessage;
    class DisServiceCompleteMessageReqMsg;
    class DisServiceHistoryRequest;
    class DisServiceHistoryRequestReplyMsg;
    class DisServiceMsg;
    struct HistoryRequest;
    class ControllerToControllerMsg;
    class DataCacheInterface;
    class DataCache;
    class DataCacheService;
    class DataCacheListener;
    class DataCacheReplicationController;
    class DataCacheExpirationController;
    class DataRequestHandler;
    class DataRequestServer;
    class DisseminationServiceListener;
    class DisServiceCommandProcessor;
    class DisServiceStats;
    class DisServiceStatusNotifier;
    class ForwardingController;
    class GroupMembershipListener;
    class LocalNodeInfo;
    class Message;
    class MessageHeader;
    class MessageInfo;
    class MessageListener;
    class MessageReassembler;
    class NetworkStateListener;
    class NetworkStateListenerNotifier;
    class NetworkTrafficMemory;
    class PeerState;
    class PeerStateListener;
    class PeerStatusListener;
    class ReceivedMessagesInterface;
    struct RequestDetails;
    class SearchController;
    class SearchListener;
    class SearchNotifier;
    class SubscriptionState;
    class SubscriptionForwardingController;
    class TransmissionHistoryInterface;
    class TransmissionService;
    class TransmissionServiceHelper;
    class TransmissionServiceListener;
    class TopologyWorldState;
    class WorldState;

    class DisseminationService : public NOMADSUtil::ManageableThread
    {
        public:
            enum SubscriptionPredicateType {
                XPATH_PREDICATE = 0x00
            };

            DisseminationService (const char *pszNodeUID = NULL);

            #if defined (USE_XLAYER)
                DisseminationService (const char *pszXLayerIP, uint16 ui16XLayerPort, const char *pszSenderId = NULL);
            #else
                DisseminationService (uint16 ui16Port, const char *pszSenderId = NULL);
            #endif

            DisseminationService (NOMADSUtil::ConfigManager *pConfigManager);

            virtual ~DisseminationService (void);

            /**
             * Initializes DisService, but does not start threads to receive / handle any incoming data
             * Application must call start() when it is ready to start receiving data
             * NOTE: start() should not be called until the controllers have been initialized 
             */ 
            int init (void);

            int start (void);

            // Overridden functions from ManageableThread
            void requestTermination (void);
            void requestTerminationAndWait (void);

            /**
             * Get the id of the node
             * By default, a random UUID is generated for each node during initialization
             */
            const char * getNodeId (void);

            /**
             * Get the id of the current session.
             */
            const char * getSessionId (void);

            /**
             * Set the maximum size of each fragment
             * Messages larger than this specified value will be fragmented as necessary
             */
            int setMaxFragmentSize (uint16 ui16MaxFragmentSize);

            /**
             * Get the current maximum size for each fragment
             */
            uint16 getMaxFragmentSize (void);

            /**
             * Set the timeout for requesting missing fragments in a message
             */
            int setMissingFragmentTimeout (uint32 ui32Milliseconds);

            /**
             * Get the current timeout value for requesting missing fragments in a message
             */
            int64 getMissingFragmentTimeout (void);

            /**
             * Get the current timeout value for messages that are missing the end part.
             * NOTE: This timeout is different (and longer) than the missing fragment timeout
             *       to allow for packets that are still enroute that contain subsequent fragments.
             */
            int64 getMissingTailFragmentTimeout (void);

            /**
             * For every node that published messages that belong the subscription
             * <pszGroupName ui16Tag>, retrieve the latest message received and
             * update the next sequence id expected for the sender.
             */
            void setInitialSubscriptionStatus (const char *pszGroupName, uint16 ui16Tag);

            /**
             * Stores a message containing the specified data and metadata into
             * the Dissemination Service cache.
             *
             * Returns 0 if successful or a negative value in case of error.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszGroupName - specifies the group to which this data should be disseminated.
             *                QUESTION: If we use hierarchical groups (eg. a.b.c), would it be useful to
             *                          be able to use wildcards (e.g., send it to a.* or *.*.c)?
             *
             * pszObjectId - an application-defined id that uniquely identify the object.
             *
             * pszInstanceId - an application-defined id that uniquely identify the version of the object.
             *
             * pszMimeType - the MIME type of the data being stored.
             *
             * pMetadata - the metadata describing the data
             *
             * ui32MetadataLength - the length of the metadata
             *
             * pData - the data itself.
             *
             * ui32DataLength - the size of the data buffer in bytes.
             *
             * i64ExpirationTime - the expiration time for the data specified in milliseconds.
             *                     Beyond this time, the data will no longer be disseminated to any new nodes.
             *                     A value of 0 indicates there is no expiration time.
             *
             * ui16HistoryWindow - the number of messages previously transmitted that are recommended to be
             *                     retrieved before processing the current message.
             *
             * ui16Tag - the tag value identifying the type (application defined) for the data.
             *           NOTE: a tag value of 0 implies that this message has not been tagged.
             *
             * ui8Priority - the priority value for the data, with higher values indicated higher priority
             *
             * pszIdBuf - the buffer into which the id of this message will be copied.
             *            NOTE: This parameter may be NULL if the caller does not need to receive the id of the message.
             *
             * ui32IdBufLen - the length of the buffer to receive the message id.
             *                NOTE: if the buffer is too small to hold the id, the id will not be copied into the buffer.
             */
            int store (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                       const char *pszMimeType, const void *pMetadata, uint32 ui32MetadataLength, const void *pData,
                       uint32 ui32DataLength, int64 i64ExpirationTime, uint16 ui16HistoryWindow, uint16 ui16Tag,
                       uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen);

            /**
             * Disseminates data identified by the ID pszIdBuf. (The data must
             * have been stored in the data cache in order to be pushed through
             * this method)
             */
            int push (uint16 ui16ClientId, char *pszMsgId);

            /**
             * Disseminates data to nodes belonging to the specified group that are reachable.
             * Returns 0 if successful or a negative value in case of error.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszGroupName - specifies the group to which this data should be disseminated.
             *                QUESTION: If we use hierarchical groups (eg. a.b.c), would it be useful to
             *                          be able to use wildcards (e.g., send it to a.* or *.*.c)?
             *
             * pszObjectId - an application-defined id that uniquely identify the object.
             *
             * pszInstanceId - an application-defined id that uniquely identify the version of the object.
             *
             * pszMimeType - the MIME type of the data being stored.
             *
             * pMetadata - the metadata describing the data
             *
             * ui32MetadataLength - the length of the metadata
             *
             * pData - the data itself.
             *
             * ui32DataLength - the size of the data buffer in bytes.
             *
             * i64ExpirationTime - the expiration time for the data specified in milliseconds.
             *                     Beyond this time, the data will no longer be disseminated to any new nodes.
             *                     A value of 0 indicates there is no expiration time.
             *
             * ui16HistoryWindow - the number of messages previously transmitted that are recommended to be
             *                     retrieved before processing the current message.
             *
             * ui16Tag - the tag value identifying the type (application defined) for the data.
             *           NOTE: a tag value of 0 implies that this message has not been tagged.
             *
             * ui8Priority - the priority value for the data, with higher values indicated higher priority
             *
             * pszIdBuf - the buffer into which the id of this message will be copied.
             *            NOTE: This parameter may be NULL if the caller does not need to receive the id of the message.
             *
             * ui32IdBufLen - the length of the buffer to receive the message id.
             *                NOTE: if the buffer is too small to hold the id, the id will not be copied into the buffer.
             */
            int push (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                      const char *pszMimeType, const void *pMetadata, uint32 ui32MetadataLength, const void *pData,
                      uint32 ui32DataLength, int64 i64ExpirationTime, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority,
                      char *pszIdBuf, uint32 ui32IdBufLen);

            /**
             * Updates the expiration time of the message identified by pszId.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszId - the id of the message whose expiration time must be modified
             *
             * i64NewExpirationTime - the new value for the expiration time
             *
             */        
            int modify (uint16 ui16ClientId, const char *pszId, int64 i64NewExpirationTime);

            /**
             * Updates the priority of the message identified by pszId.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszId - the id of the message whose priority must be modified
             *
             * ui8NewPriority - the new value of priority
             *
             */
            int modify (uint16 ui16ClientId, const char *pszId, uint8 ui8NewPriority);

            /**
             * Makes the specified data available and disseminates the associated metadata to nodes belonging to
             * the specified group that are reachable. The data is stored in the network until the specified
             * expiration time, given storage constraints. The data may be split into chunks that are distributed
             * and replicated among several nodes.
             * Returns 0 if successful or a negative value in case of error
             *
             * ui16ClientId - see comments for the push() method.
             *
             * pszGroupName - see comments for the push() method.
             *
             * pszObjectId - an application-defined id that uniquely identify the object.
             *
             * pszInstanceId - an application-defined id that uniquely identify the version of the object.
             *
             * pMetadata - any associated metadata for this data. May be null.
             *
             * ui32MetadataLength - the length of the metadata.
             *
             * pData - the data itself.
             *
             * ui32Length - the size of the data buffer in bytes.
             *
             * pszDataMimeType - the MIME type of the data
             *
             * ui32Expiration - see comments for the push() method. The expiration time applies to both the
             *                  metadata and the data itself.
             *
             * ui16HistoryWindow - the number of messages previously transmitted that are recommended to be
             *                     retrieved before processing the current message.
             *
             * ui16Tag - see comments for the push() method.
             *
             * ui8Priority - see comments for the push() method.
             *
             * pszIdBuf - see comments for the push() method.
             *
             * ui32IdBufLen - see comments for the push() method.
             */
            int makeAvailable (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                               const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32Length,
                               const char *pszDataMimeType, int64 i64Expiration, uint16 ui16HistoryWindow, uint16 ui16Tag,
                               uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen);
            int makeAvailable (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                               const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32Length, int64 i64Expiration,
                               uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen);

            /**
             * Same as the previous version of makeAvailable, except that the data is read from a file.
             */
            int makeAvailable (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                               const void *pMetadata, uint32 ui32MetadataLength, const char *pszFilePath, const char *pszDataMimeType,
                               int64 i64Expiration, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf,
                               uint32 ui32IdBufLen);
            int makeAvailable (uint16 ui16ClientId, const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                               const void *pMetadata, uint32 ui32MetadataLength, const char *pszFilePath, int64 i64Expiration,
                               uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen);

            /**
             * Cancel a data message that has been pushed or made available in the past
             * Returns 0 if successful or a negative value in case of error
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszId - the identifier of the message to cancel
             */
            int cancel (uint16 ui16ClientId, const char *pszId);

            /**
             * Cancel all messages tagged with the specified tag
             * Returns 0 if successful or a negative value in case of error
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * ui16Tag - the tag marking the set of messages that should be canceled.
             */
            int cancel (uint16 ui16ClientId, uint16 ui16Tag);

            /**
             * Cancel all messages tagged with the specified tag
             * Returns 0 if successful or a negative value in case of error
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * ui16Tag - the tag marking the set of messages that should be canceled.
             */
            int cancelAll (uint16 ui16ClientId);

            /**
             * Subscribe to the specified group. The application will subsequently receive any messages
             * addressed to matching groups that are received.
             * Returns 0 if successful or a negative value in case of error.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszGroupName - the name of the group to subscribe / join
             *                Wildcards are allowed in the subscription (e.g., "a.b.*")
             *
             * bReliable - used to indicate that the data transmission should be reliable and not just best effort.
             *             If this is true, the sender will retransmit the data until acknowledged by the recipient.
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
             * ui8Priority -
             */
            int subscribe (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8Priority, bool bGroupReliable, bool bMsgReliable, bool bSequenced);
    
         /**
             * Subscribe with a predicate. The application will subsequently receive any messages
             * with a positive evaluation of the predicate.
             * Returns 0 if successful or a negative value in case of error.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszGroupName - the name of the group to subscribe / join
             *                QUESTION: Should wildcards be allowed in the subscription (e.g., "a.b.*")?
             *
             * ui8PredicateType - the type of the predicate to be evaluated.
             *                    The supported type are listed in DisseminationService::SubscriptionPredicateType.
             * 
             * pszPredicate - the predicate to evaluate
             * 
             * bGroupReliable - used to indicate that the data transmission of all the message in the grup should be reliable
             *                  and not just best effort.
             * 
             * bMsgReliable - used to indicate that the data transmission of a single message should be reliable and not
             *                just best effort.
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
             * ui8Priority -
             */
            int subscribe (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *pszPredicate,
                           uint8 ui8Priority, bool bGroupReliable, bool bMsgReliable, bool bSequenced);

            /**
             * Filter (i.e., prevent delivery) of any incoming messages that match the specified tag.
             * Returns 0 if successful or a negative value in case of error.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszGroupName - the name - may be null if the filter should apply to all subscribed groups.
             *
             * ui16Tag - the tag that identifies messages that should be filtered.
             *           NOTE: The tag cannot be 0 (which is used to indicate that there is no tag)
             */
            int addFilter (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag);

            /**
             * Remove a previously specified filter.
             * Returns 0 if successful or a negative value in case of error.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszGroupName - the name - may be null if the filter was for all subscribed groups.
             *
             * ui16Tag - the tag that was specified earlier as part of a filter.
             *           NOTE: The tag cannot be 0 (which is used to indicate that there is no tag)
             */
            int removeFilter (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag);

            /**
             * Remove all the previously specified filters.
             * Returns 0 if successful or a negative value in case of error.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszGroupName - the name - may be null if the filter was for all subscribed groups.
             */
            int removeAllFilters (uint16 ui16ClientId, const char *pszGroupName);

            /**
             * Unsubscribe to the previously subscribed group.
             * NOTE: This method may also be used to clear ALL subscriptions made with the group name + tag option
             * Returns 0 if successful or a negative value in case of error.
             *
             * NOTE: This method also remove subscriptions that have been added by the method
             * subscribe (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint8 ui8Priority)
             * For further information about this method look at the comment related to the method itself.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszGroupName - the name of the group to unsubscribe.
             *                QUESTION: Should wildcards be allowed in the unsubscribe request?
             */
            int unsubscribe (uint16 ui16ClientId, const char *pszGroupName);

            /**
             * Subscribe to the specified group and the specified tag. Only messages addressed to that group
             * and that have the specified tag will be delivered.
             * Returns 0 if successful or a negative value in case of error.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients.
             *
             * pszGroupName - the name of the group to subscribe / join.
             *                QUESTION: Should wildcards be allowed in the unsubscribe request?
             *
             * ui16Tag - the tag specifying data the node wants to receive.
             *           NOTE: The tag cannot be 0.
             *
             * bReliable - see comments for the previous subscribe() method.
             *
             * bSequenced - see comments for the previous subscribe() method.
             *
             * ui8Priority - see comments for the previous subscribe() method.
             *
             */
            int subscribe (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint8 ui8Priority,
                           bool bGroupReliable, bool bMsgReliable, bool bSequenced);

            /**
             * Unsubscribe to the specified group and the specified tag.
             * NOTE: The first version of unsubscribe(), which only takes the group name as an argument, can also be used
             *       if the goal is to unsubscribe enmasse to all subscribed groups and tags.
             * Returns 0 if successful or a negative value in case of error
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pszGroupName - the name of the group to unsubscribe
             *                QUESTION: Should wildcards be allowed in the unsubscribe request?
             *
             * ui16Tag - the tag specifiying data the node does not want to receive.
             *           NOTE: The tag value cannot be 0.
             */
            int unsubscribe (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag);

            /**
             * Register a listener that is used to notify the application about the arrival of new data
             * that has been pushed or metadata about a message that has been made available.
             * Returns 0 if successful or a negative value in case of error.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pListener - the listener that will be invoked upon arrival of data.
             */     
            int registerDisseminationServiceListener (uint16 ui16ClientId, DisseminationServiceListener *pListener);

            /**
             * NOTE: pListener is not deallocated bt DisseminationService.
             * The deallocation of pListener is up to the application.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pListener - the listener that will be invoked upon arrival of data.
             */
            int deregisterDisseminationServiceListener (uint16 ui16ClientId, DisseminationServiceListener *pListener);

            /**
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pListener - the listener that will be invoked upon arrival/leave of a peer
             */
            int registerPeerStatusListener (uint16 ui16ClientId, PeerStatusListener *pListener);

            /**
             * NOTE: pListener is not deallocated bt DisseminationService.
             * The deallocation of pListener is up to the application.
             *
             * ui16ClientId - the id of the client (or application) invoking this operation.
             *                Used to multiplex requests from multiple clients
             *
             * pListener - the listener that will be invoked upon arrival of data.
             */
            int deregisterPeerStatusListener (uint16 ui16ClientId, PeerStatusListener *pListener);

            /**
             * Registering methods set a unique index for the listener in uiIndex.
             * The unique index is used to deregister the listener.
             */
            int deregisterAllDataCacheListeners (void);
            int deregisterDataCacheListener (unsigned int uiIndex);
            int registerDataCacheListener (DataCacheListener *pListener, unsigned int &uiIndex);

            int deregisterAllMessageListeners (void);
            int deregisterMessageListener (unsigned int uiIndex);
            int registerMessageListener (MessageListener *pListener, unsigned int &uiIndex);

            int deregisterAllGroupMembershipListeners (void);
            int deregisterGroupMembershipListener (unsigned int uiIndex);
            int registerGroupMembershiListener (GroupMembershipListener *pListener, unsigned int &uiIndex);

            int deregisterAllNetworkStateListeners (void);
            int deregisterNetworkStateListener (unsigned int uiIndex);
            int registerNetworkStateListener (NetworkStateListener *pListener, unsigned int &uiIndex);

            int deregisterAllPeerStateListeners (void);
            int deregisterPeerStateListener (unsigned int uiIndex);
            int registerPeerStateListener (PeerStateListener *pListener, unsigned int &uiIndex);

            int deregisterSearchListeners (void);
            int deregisterSearchListener (unsigned int uiIndex);
            int registerSearchListener (SearchListener *pListener, unsigned int &uiIndex);

            /**
             * Retrieve data that is identified by the specified id. The data must have been made available
             * by some (other) node and the local node must have received the metadata associated with the data.
             * 
             * If the retrieved data, or complete chunks are already available in the cache, they
             * are returned. If the data is not found, then it will be searched on neighboring nodes,
             * and if it is found it will be requested.
             *
             * Returns a negative number if an error occurred
             *
             * pszId - the id of the message whose data is to be retrieved.
             *
             * pBuf - the buffer into which the data should be stored.
             *
             * ui32BufSize - the size of the buffer.
             *
             * i64Timeout - the maximum time the data must be retrieved.
             */
            int retrieve (const char *pszId, void **ppBuf, uint32 *pui32BufSize, int64 i64Timeout);

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

            // Need some approach to examine the set of locally available data (or query the data available in the network) which can be retrieved

            /*
             * The method returns a copy of the data in the data cache.
             * NOTE: the copy should be deallocated by the caller after use.
             */
            int getData (const char *pszGroupName, const char *pszSender, uint32 ui32SeqId,
                         uint32 *ui32DataLength, void **pData);
            int getData (const char *pszMsgId, uint32 *ui32DataLength, void **pData);

            /**
             * Get the disservice mesage id that corresponds to the pszObjectId and pszInstanceId
             */
            char ** getDisseminationServiceIds (const char *pszObjectId);
            char ** getDisseminationServiceIds (const char *pszObjectId, const char *pszInstanceId);

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
            int historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag,
                                uint16 ui16HistoryLength, int64 i64RequestTimeout);

            /**
             * Same as the previous request except that this is a specific range
             * of messages that are requested.
             *
             * ui32StartSeqNo -
             *
             * ui32EndSeqNo -
             */
            int historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag,
                                uint32 ui32StartSeqNo, uint32 ui32EndSeqNo, int64 i64RequestTimeout);
            int historyRequest (uint16 ui16ClientId, const char *pszGroupName, const char *pszSenderNodeId, uint16 ui16Tag,
                                uint32 ui32StartSeqNo, uint32 ui32EndSeqNo, int64 i64RequestTimeout);
            int historyRequest (uint16 ui16ClientId, const char *pszGroupName, const char *pszSenderNodeId,
                                uint32 ui32StartSeqNo, uint32 ui32EndSeqNo, int64 i64RequestTimeout);
            int historyRequest (uint16 ui16ClientId, const char *pszGroupName, const char *pszSenderNodeId,
                                uint32 ui32MsgSeqId, uint8 ui8ChunkStart, uint8 ui8ChunkEnd, int64 i64RequestTimeout);
            int historyRequest (uint16 ui16ClientId, const char *pszMsgId, int64 i64RequestTimeout);

            /**
             * Same as the previous request except that this time a specific
             * range of messages
             *
             * i64PublishTimeStart -
             *
             * i64RequestTimeout -
             */
            int historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, int64 i64PublishTimeStart,
                                int64 i64PublishTimeEnd, int64 i64RequestTimeout);

            int historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *pszPredicate,
                                uint16 ui16HistoryLength, int64 i64RequestTimeout);
            int historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *pszPredicate,
                                int32 ui32StartSeqNo, uint32 ui32EndSeqNo, int64 i64RequestTimeout);
            int historyRequest (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *pszPredicate,
                                int64 i64PublishTimeStart, int64 i64PublishTimeEnd, int64 i64RequestTimeout);

            /**
             * Request more chunks for the "chunked" message identified by
             *
             * Returns 0, if successful, a negative number if an error occurred,
             * or a positive number if all the chunks have already been retrieved.
             */
            int requestMoreChunks (uint16 ui16ClientId, const char *pszMsgId, int64 i64Timeout = 0);
            int requestMoreChunks (uint16 ui16ClientId, const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, int64 i64Timeout = 0);

            /**
             * Releases a message that the client obtained by mean of
             * DisseminationserviceListener,
             * or by the get function.
             * The message will not be deleted from the data cache until it is
             * released by all the clients that obtained it.
             * Once the object has been released DisseminationService may delete
             * the object to free up memory to cache messages that are newer or
             * that are considered to be more useful.
             */
            int release (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, void *pData);
            int release (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, uint32 ui32ChunkSeqId,
                         void *pData);
            int release (const char *pszMsgId, void *pData);

            void run (void);

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
             * ppszQueryId - a unique identifier that is generated for the query.
             */
            int search (uint16 ui16ClientId, const char *pszGroupName, const char *pszQueryType,
                        const char *pszQueryQualifiers, const void *pQuery,
                        unsigned int uiQueryLen, char **ppszQueryId);

            /**
             * Method to return the list of IDs that match the query identified
             * by pszQueryId.
             *
             * pszQueryId -
             * ppszMatchingMsgIds - a null-terminated array of string representing
             *                      the IDs of the message that match the query.
             */
            int searchReply (const char *pszQueryId, const char **ppszMatchingMsgIds);

            /**
             * Returns the maximum amount of bytes that, with the current
             * configuration, will be sent without need of fragmentation
             */
            // uint16 getActualMTU (const char * pszGroupName, const char * pszTargetNode=NULL);

            /**
             * Returns the list of the peers that are currently in communications range
             * NOTE: Caller must delete the array, but NOT the strings within the array.
             */
            char ** getPeerList (void);

            bool isActiveNeighbor (const char *pszNodeId);

            int resetTransmissionHistory (void);
            int reloadTransmissionService (void);

            /**
             * Returns the ID that will be assigned to the next message that will be pushed to the specified group
             */
            int getNextPushId (const char *pszGroupName, char *pszIdBuf, uint32 ui32IdBufLen);

        public:
            static const uint16 DEFAULT_MAX_FRAGMENT_SIZE = 1400;   // Value is in Bytes
            static const uint8 MPSMT_DisService = 100;   // Message type to be used for DisseminationService messages sent/received
                                                               // via the MessagePropagationService

            // Node configuration parameters
            static const uint32 DEFAULT_MAX_MEMORY_SPACE = 1048575; //Value in Kbytes
            static const uint32 DEFAULT_MAX_BANDWIDTH = 1024;       //Max value in Kbits/s
            static const uint32 DEFAULT_MIN_BANDWIDTH = 0;       //Min value in Kbits/s
            static const int64 DEFAULT_CONNECTIVITY_HISTORY_WIN_SIZE = 40000;   // time in milliseconds

            static const uint16 DEFAULT_KEEP_ALIVE_INTERVAL = 2000;
            static const uint32 DEFAULT_DEAD_PEER_INTERVAL = 30000;

            // A Subscription State message is sent instead of the keep alive
            // message every SUBSCRIPTION_STATE_TIME_SLOTS loops of the run method.
            static const bool DEFAULT_SUBSCRIPTION_STATE_EXCHANGE_ENABLED = false;
            static const uint8 DEFAULT_SUBSCRIPTION_STATE_PERIOD = 3;

            ////////////////// Missing Fragments/Messages Config /////////////////

            // Specifies the interval of time from the last time the message/fragment has
            // been broadcasted (by whatever neighbors) in which  a request for the same
            // message/fragment must not be served
            // (NOTE: the request is dropped, not queued)
            // Setting the following property to 0 will disable this feature.
            static const uint16 DEFAULT_IGNORE_REQUEST_TIME = 0;

            enum TransmissionMode {
                TRANSM_MODE_UNDEFINED = 0x00,
                PUSH_DISABLED         = 0x01,
                PUSH_ENABLED          = 0x02,
            };

            enum DataCacheQueryReplyType {
                DISABLED    = 0x00,
                REPLY_DATA  = 0x01,
                REPLY_MSGID = 0x02,
            };

            enum ControllerType {
                DCReplicationCtrl = 0x00,
                ForwardingCtrl    = 0x01,
                DCExpirationCtrl  = 0x02
            };

            // DisService Status Monitoring ////////////////////////////////////
            static const uint16 DEFAULT_DIS_SERVICE_STATUS_PORT = 2400;

            // Large Object Dissemination //////////////////////////////////////
            static const uint16 MOCKET_SERVER_PORT = 9999;
            static const int64 MOCKET_RECEIVER_TIMEOUT = 3000;                                              //Time in milliseconds
            static const int64 MOCKET_MAX_BUFFER_SIZE = 1024;

            static const uint8 DEFAULT_CHUNKS_SERVING_REQUEST_NUM = 3;              // It's the default number of chunks request served in a cycle.
            static const uint32 DEFAULT_CHUNK_SIZE = 16384;                         // Values in Bytes
            static const uint32 DEFAULT_MIN_FREE_MEMORY_THRESHOLD = 128;            // Value is in KB
            static const uint32 DEFAULT_MIN_BANDWIDTH_THRESHOLD = 43;               // Value is in Kb/s. This means that nodes unable to download a
                                                                                    // chunk of 16KB in a time lower than 2.977 seconds, are not considered                                                                      // to be target node for chunk replication
            static const uint8 DEFAULT_NUM_OF_CLASSES = 3;
            static const uint8 DEFAULT_BANDWIDTH_FACTOR = 1;
            static const uint8 DEFAULT_MEMORY_FACTOR = 1;
            static const uint8 DEFAULT_ACTIVES_NEIGHBORS_FACTOR = 1;
            static const uint8 DEFAULT_NODES_IN_CONNECTIVITY_HISTORY_FACTOR = 1;
            // end of chunk replication parameters
            static const int64 DEFAULT_CHUNK_SEARCH_TIMEOUT = 30000;        // In milliseconds
            static const uint32 DEFAULT_FRAGMENT_REQUEST_TIMEOUT = 5000;    // In milliseconds

            // Multimedia Types supported by the DisService
            enum MultimediaTypes {
                NO_TYPE = 0x00,
                BITMAP_TYPE = 0x01,
                JPEG_TYPE = 0x02,
                MPEG_TYPE = 0x03
            };

            // Topology //////////////////
            static const bool DEFAULT_SUBSCRIPTIONS_EXCHANGE_ENABLED = false;
            static const uint32 DEFAULT_SUBSCRIPTIONS_EXCHANGE_PERIOD = 3;
            static const bool DEFAULT_TOPOLOGY_EXCHANGE_ENABLED = false;
            static const uint32 DEFAULT_TOPOLOGY_EXCHANGE_PERIOD = 3;

            static const float DEFAULT_PROB_CONTACT;   // Probability when two nodes are in contact
            static const float DEFAULT_PROB_THRESHOLD; // Probabilities lower than the threshold are discarded
            static const float DEFAULT_ADD_PARAM;      // Parameter used when adding a probability
            static const float DEFAULT_AGE_PARAM;      // Parameter used when aging a probability

        protected:
            NOMADSUtil::DArray<uint16> * getAllClients (void);
            NOMADSUtil::DArray<uint16> * getSubscribingClients (Message *pMsg);
            NOMADSUtil::DArray<uint16> * getAllSubscribingClients (void);

            void notifyDisseminationServiceListener (uint16 ui16ClientId, Message *pMsg,
                                                     bool bMetaData, bool bMetaDataWrappedInData, const char *pszQueryId);
            void notifyDisseminationServiceListeners (NOMADSUtil::DArray<uint16> *pClientIdList, Message *pMsg,
                                                      bool bMetaData, bool bMetaDataWrappedInData, RequestDetails *pDetails);
            void notifyPeerStatusListeners (const char *pszPeerNodeId, bool bIsNewPeer);

            DataCacheInterface * getDataCacheInterface (void) {return _pDataCacheInterface;}
            TransmissionHistoryInterface * getTrasmissionHistoryInterface (void) {return _pTransmissionHistoryInterface;};

            PeerState * getPeerState (void);

            /**
             * Returns the output queue size (transmission queue size) for the specified target node id,
             * provided the target node id maps to a specific interface and the interface has
             * asynchronous transmission enabled.
             */
            virtual uint32 getMinOutputQueueSizeForPeer (const char *pszTargetNodeId);

            bool doBroadcastDataMsg (DisServiceDataMsg *pDDMsg);

            int pushInternal (uint16 ui16ClientId, char *pszIdBuf);
            int pushInternal (uint16 ui16ClientId, Message *pMsg);
            int storeInternal (uint16 ui16ClientId, const char *pszGroupName,
                               const char *pszObjectId, const char *pszInstanceId,
                               const void *pMetadata, uint32 ui32MetadataLength,
                               const void *pData, uint32 ui32DataLength,
                               const char *pszDataMimeType, bool bChunkData,
                               int64 i64ExpirationTime, uint16 ui16HistoryWindow,
                               uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf,
                               uint32 ui32IdBufLen, const char *pszRefObj=NULL);
            virtual int storeInternalNoNotify (uint16 ui16ClientId, const char *pszGroupName,
                                               const char *pszObjectId, const char *pszInstanceId,
                                               const void *pMetadata, uint32 ui32MetadataLength,
                                               const void *pData, uint32 ui32DataLength,
                                               const char *pszDataMimeType, bool bChunkData,
                                               int64 i64ExpirationTime, uint16 ui16HistoryWindow,
                                               uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf,
                                               uint32 ui32IdBufLen, const char *pszRefObj,
                                               unsigned int uiListener);
            int storeInternalNoNotify (MessageHeader *pMH, const void *pData, unsigned int uiListener);

        private:
            // Friend all the controllers
            friend class DataCacheService;
            friend class GroupMembershipService;
            friend class MessagingService;
            friend class TopologyService;

            friend class MessageReassembler;
            friend class BasicWorldState;           
            friend class DataCache;
            friend class PersistentDataCache;
            friend class PullReplicationController;
            friend class ForwardingController;
            friend class DataCacheExpirationController;
            friend class DataCacheReplicationController;
            friend class SubscriptionForwardingController;
            friend class TransmissionServiceListener;
            friend class ConnHandler;
            friend class DataRequestHandler;
            friend class DataRequestServer;
            friend class SearchService;

            friend class WorldState;

            friend class KAoSCacheController;
            friend class KAoSPullReplicationController;

            friend class SubscriptionState;
            friend class DisServiceCommandProcessor;
            friend class TopologyWorldState;

            struct ClientInfo
            {
                enum Type { DIRECTLY_CONNECTED, PROXY_CONNECTED };

                ClientInfo (void);
                ~ClientInfo (void);

                DisseminationServiceListener *pListener;
                Type type;
            };
            
            struct PeerStatusClientInfo
            {
                PeerStatusClientInfo (void);
                ~PeerStatusClientInfo (void);
                PeerStatusListener *pListener;
                unsigned int uiIndex;
            };

            uint16 getNumberOfActiveNeighbors (void);            

            NetworkStateListenerNotifier *_pNetworkStateNotifier;

        private:
            int registerDisseminationServiceProxyAdaptor (uint16 ui16ClientId, DisseminationServiceListener *pListener);
            int registerDisseminationServiceListenerInternal (uint16 ui16ClientId, DisseminationServiceListener *pListener, ClientInfo::Type type);

            void construct (NOMADSUtil::ConfigManager *_pConfigManager);
            DisServiceStats * getStats (void);
            int sendStatus (void);

            LocalNodeInfo * getLocalNodeInfo (void);
            SubscriptionState * getSubscriptionState (void);
            int inizializeSubscriptionState (const char * pszGroupName, const char * pszSenderNodeID, uint32 ui32NewNextExpectedSeqId);

            void addDataToCache (MessageHeader *pMsgHeader, const void *pData);

            NOMADSUtil::Reader * getData (const char *pszId, uint32 &ui32MessageLength);

            void messageArrived (Message *pMsg, RequestDetails *pDetails);

            int handleDataRequestMessage (DisServiceDataReqMsg *pDSDRMsg, const char *pszIncomingInterface, int64 i64Timeout=0);
            int handleDataRequestMessage (const char *pszMsgId, DisServiceMsg::Range *pRange,
                                          bool bIsChunk, const char *pszSenderNodeId,unsigned int uiNumberOfActiveNeighbors,
                                          int64 i64RequestArrivalTime, const char **ppszOutgoingInterfaces, int64 i64Timeout=0);

            /**********************************************************************
             * WORLD STATE                                                        *
             **********************************************************************/
            int handleWorldStateSequenceIdMessage (DisServiceWorldStateSeqIdMsg *pDSWSMsg, uint32 ui32SourceIPAddress);
            // Subscription state
            int handleSubscriptionStateMessage (DisServiceSubscriptionStateMsg *pDSSM, uint32 ui32SourceIPAddress);
            int handleSubscriptionStateRequestMessage (DisServiceSubscriptionStateReqMsg *pDSSRMsg, uint32 ui32SourceIPAddress);

            int handleImprovedSubscriptionStateMessage (DisServiceImprovedSubscriptionStateMsg *pDSISSMsg, uint32 ui32SourceIPAddress);
            int handleProbabilitiesMessage (DisServiceProbabilitiesMsg *pDSPMsg, uint32 ui32SourceIPAddress);

            /**********************************************************************
             * DATA CACHE SYNCHRONIZATION AMONG THE NEIGHBOURS                    *
             *                                                                    *
             * READ THE NOTES ABOUT OVERLAPPING MANAGMENT.                        *
             * NOTE: actually, in order to reduce the delivery of unwanted data   *
             * it's necessary to implement only one of the two policies described *
             * in the 2 following notes about the overlapping management          *
             *                                                                    *
             * NOTE: For now, fragments are of FIXED SIZE                         *
             **********************************************************************/

            /**
             * DisServiceDataCacheQueryMsg contains a set of queries. Every query
             * may match a set of messages in the data cache.
             * handleDataCacheQueryMessage processes the query and sends a message
             * containing all the MessageIds of the cached messages (fragments)
             * matching the query.
             *
             * NOTE ABOUT OVERLAPPING MANAGEMENT: if a message (fragment) matches
             * the query partially (in case of overlap) only the overlapping message
             * should be sent
             */
            int handleDataCacheQueryMessage (DisServiceDataCacheQueryMsg *pDSDCQMsg, uint32 ui32SourceIPAddress);

            /**
             * DisServiceDataCacheQueryReplyMsg contains all the messages (fragments) that a certain peer has in its cache.
             * handleDataCacheQueryReplyMessage select and request a subset of them.
             *
             * NOTE: look into a method to split the requests among the neighbours, in order to minimize multiple requests
             * for the same data.
             * (In order to achieve this result the different neighbour's caches can be compared and then the field "target"
             * in DisServiceDataCacheQueryReplyMsg can be used to specify the node asked for a specific subset)
             *
             * NOTE ABOUT OVERLAPPING MANAGMENT: if a message (fragment) overlaps partially with an already received message,
             * only the non overlapping fragment should be requested.
             */
            int handleDataCacheQueryReplyMessage (DisServiceDataCacheQueryReplyMsg *pDSDCQRMsg, uint32 ui32SourceIPAddress);

            /**
             * For each id specified in the DisServiceDataCacheMessagesRequestMsg, retrieve the message ans broadcast it.
             */
            int handleDataCacheMessagesRequestMessage (DisServiceDataCacheMessagesRequestMsg *pDSDCMRMsg, uint32 ui32SourceIPAddress);

            int handleCacheEmptyMessage (DisServiceCacheEmptyMsg *pDSCEMsg, uint32 ui32SourceIPAddress);

            /************************************************************************************************
             * ACKNOLEDGEMENT                                                                               *
             ************************************************************************************************/
            int handleAcknowledgmentMessage (DisServiceAcknowledgmentMessage *pDSAMsg, uint32 ui32SourceIPAddress);
            int handleCompleteMessageRequestMessage (DisServiceCompleteMessageReqMsg *pDSCMRMsg, uint32 ui32SourceIPAddress);

            /************************************************************************************************
             * CONTROLLER TO CONTROLLER COMUNICATION                                                        *
             ************************************************************************************************/
            int handleCtrlToCtrlMessage (ControllerToControllerMsg *pCTCMsg, uint32 ui32SourceIPAddress);

            /************************************************************************************************
             * HISTORY REQUEST                                                                              *
             ************************************************************************************************/
            int handleHistoryRequestMessage (DisServiceHistoryRequest *pHistoryReq, uint32 ui32SourceIPAddress);
            int handleHistoryRequestReplyMessage (DisServiceHistoryRequestReplyMsg *pDSHRRMsg, uint32 ui32SourceIPAddress);

            int handleSearchMessage (SearchMsg *pSearchMsg, uint32 ui32SourceIPAddress);
            int handleSearchReplyMessage (SearchReplyMsg *pSearchReplyMsg, uint32 ui32SourceIPAddress);

            /*
             * selectMsgIDsToRequest is called from handleHistoryRequestReplyMessage or handleDataCacheQueryReplyMessage
             * and send a DisServiceDataCacheMessagesRequestMsg that contain the MsgIds that i not already have in my cache
             */
            int selectMsgIDsToRequest (const char *pszSenderNodeId, NOMADSUtil::DArray2<NOMADSUtil::String> *pMsgIDs);

            /************************************************************************************************
             * FLOW CONTROL                                                                                 *
             ************************************************************************************************/
            bool clearToSend (const char *pszInterface);
            bool clearToSendOnAllInterfaces (void);

            /*
             * BroadcastDisServiceDataMsg broadcasts a messages containing data from the clients 
             * whereas broadcastDisServiceCntrlMsg created by the dissemination service.
             * Data created by the application maybe be bigger than the DEFAULT_MAX_FRAGMENT_SIZE,
             * thus they may need to be fragmented.
             *
             * ASSUMPTION 1: size of broadcastDisServiceCntrlMsg <= MAX_FRAGMENT_SIZE
             * ASSUMPTION 2: MessageInfo < MAX_FRAGMENT_SIZE (otherwise fragments wont contain any piece of the actual data)
             *
             * NOTE: broadcastDisServiceDataMsg writes (overwrites) _nodeId
             */
            int broadcastDisServiceDataMsg (DisServiceDataMsg *pDDMsg,
                                            const char *pszPurpose,
                                            const char **pszOutgoingInterfaces = NULL,
                                            const char *pszTargetAddr = NULL,
                                            const char *pszHints = NULL);

            int broadcastDisServiceCntrlMsg (DisServiceCtrlMsg *pDDCtrlMsg,
                                             const char **ppszOutgoingInterfaces,
                                             const char *pszPurpose,
                                             const char *pszTargetAddr = NULL,
                                             const char *pszHints = NULL);

            int transmitDisServiceControllerMsg (DisServiceCtrlMsg *pCtrlMsg,
                                                 bool bReliable, const char *pszPurpose);

            int transmitDisServiceControllerMsg (ControllerToControllerMsg *pCtrlMsg,
                                                 bool bReliable, const char *pszPurpose);

            int transmitDisServiceControllerMsgInternal (DisServiceMsg *pCtrlMsg,
                                                         const char *pszRecepientNodeId, bool bReliable, const char *pszPurpose);

        private:
            int historyRequest (uint16 ui16ClientId, HistoryRequest *pRequest);

            NOMADSUtil::ConfigManager *_pCfgMgr;
            NOMADSUtil::String _nodeId;
            NOMADSUtil::String _sessionId;

            bool _bKeepAliveMsgEnabled;

            uint8 _ui8TransmissionMode;
            uint8 _ui8ReplicationMode;
            bool _bReplicationAcked;

            bool _bTargetFilteringEnabled;
            bool _bOppListeningEnabled;
 
            bool _bQueryDataCacheEnabled;
            uint8 _ui8QueryDataCacheReplyType;

            bool _bSkipKeepAliveMsg;

            bool _bClientIdFilteringEnabled;

            bool _bSubscriptionStateExchangeEnabled;
            uint16 _ui16SubscriptionStatePeriod;

            int64 _i64FragmentRequestTimeout;
            int64 _i64TailTimeout;

            uint16 _ui16KeepAliveInterval;
            uint32 _ui32DeadPeerInterval;

            uint8 _ui8NodeImportance;

            uint16 _ignoreMissingFragReqTime;

            uint16 _ui16CacheCleanCycle;		// Set by the DataCacheExpirationController
            uint16 _ui16HistoryReqCycle;

            DisServiceStats *_pStats;
            DisServiceStatusNotifier *_pStatusNotifier;

            NOMADSUtil::DArray2<ClientInfo> _clients;
            NOMADSUtil::DArray2<PeerStatusClientInfo> _peerStatusClients;
            ChunkingConfiguration *_pChunkingConf;

            LocalNodeInfo *_pLocalNodeInfo;
            PeerState *_pPeerState;
            SubscriptionState *_pSubscriptionState;
            SearchNotifier *_pSearchNotifier;
            DataRequestServer *_pDataReqSvr;

            DataCacheInterface *_pDataCacheInterface;
            DataCacheReplicationController *_pDCRepCtlr;
            DataCacheReplicationController *_pDefaultDCRepCtlr;

            ForwardingController *_pFwdCtlr;
            ForwardingController *_pDefaultFwdCtlr;

            DataCacheExpirationController *_pDCExpCtlr;
            DataCacheExpirationController *_pDefaultDCExpCtlr;

            SubscriptionForwardingController *_pSubFwdCtlr;

            MessageReassembler *_pMessageReassembler;
            NetworkTrafficMemory *_pNetTrafficMemory;
            TransmissionService *_pTrSvc;
            TransmissionServiceHelper *_pTrSvcHelper;
            TransmissionServiceListener *_pTrSvcListener;

            TransmissionHistoryInterface *_pTransmissionHistoryInterface;

            ReceivedMessagesInterface *_pReceivedMessagesInterface;

            DataRequestHandler *_pDataRequestHandler;

            BandwidthSharing *_pBandwidthSharing;

            ChunkDiscoveryController *_pDiscoveryCtrl;
            ChunkRetrievalController *_pChunkRetrCtrl;
            SearchController *_pSearchCtrl;

            struct MessageToNotifyToClient
            {
                uint16 ui16ClientId;
                NOMADSUtil::String msgId;
                NOMADSUtil::String searchId;

                bool operator == (const MessageToNotifyToClient &mtn);
            };

            NOMADSUtil::PtrLList<MessageToNotifyToClient> *_pMessagesToNotify;
            NOMADSUtil::LoggingMutex _m;
            NOMADSUtil::LoggingMutex _mBrcast;
            NOMADSUtil::LoggingMutex _mKeepAlive;
            NOMADSUtil::LoggingMutex _mGetData;
            NOMADSUtil::LoggingMutex _mControllers;
            NOMADSUtil::LoggingMutex _mToListeners;
            NOMADSUtil::LoggingMutex _mToPeerListeners;
            NOMADSUtil::LoggingMutex _mAsynchronousNotify;

            bool _bSubscriptionsExchangeEnabled;
            uint16 _ui16SubscriptionsExchangePeriod;
            bool _bTopologyExchangeEnabled;
            uint16 _ui16TopologyExchangePeriod;
    };

    inline int64 DisseminationService::getMissingFragmentTimeout (void)
    {
        return _i64FragmentRequestTimeout;
    }

    inline int64 DisseminationService::getMissingTailFragmentTimeout (void)
    {
        //return _i64FragmentRequestTimeout * 10;
        return _i64TailTimeout;
    }

    inline DisServiceStats * DisseminationService::getStats (void)
    {
        return _pStats;
    }

    inline LocalNodeInfo * DisseminationService::getLocalNodeInfo (void)
    {
        return _pLocalNodeInfo;
    }

    inline SubscriptionState * DisseminationService::getSubscriptionState (void)
    {
        return _pSubscriptionState;
    }

    inline DisseminationService::ClientInfo::ClientInfo (void)
    {
        pListener = NULL;
    }

    inline DisseminationService::ClientInfo::~ClientInfo (void)
    {
        pListener = NULL; 
    }

    inline DisseminationService::PeerStatusClientInfo::PeerStatusClientInfo (void)
    {
        pListener = NULL;
    }

    inline DisseminationService::PeerStatusClientInfo::~PeerStatusClientInfo (void)
    {
        pListener = NULL;
    }

    inline bool DisseminationService::MessageToNotifyToClient::operator == (const DisseminationService::MessageToNotifyToClient &mtn)
    {
        return (ui16ClientId == mtn.ui16ClientId) && (msgId == mtn.msgId) && (searchId == mtn.searchId);
    }
}

#endif   // #ifndef INCL_DISSEMINATION_SERVICE_H
