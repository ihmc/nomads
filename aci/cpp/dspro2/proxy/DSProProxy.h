/**
 * DSProProxy.h
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
 */

#ifndef INCL_DISSERVICE_PRO_PROXY_H
#define INCL_DISSERVICE_PRO_PROXY_H


#include"DSProInterface.h"

#include "Defs.h"

#include "Stub.h"
#include "UInt32Hashtable.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_VOI
{
    class NodePath;
}

namespace IHMC_ACI
{
    class DSProListener;
    class MatchmakingLogListener;
    class MetaData;

    class DSProProxy : public NOMADSUtil::Stub, public DSProInterface
    {
        public:
            explicit DSProProxy (uint16 ui16DesiredApplicationId, bool bUseBackgroundReconnect = false);
            virtual ~DSProProxy (void);

            int subscribe (const char *pszGroupName, uint8 ui8Priority, bool bGroupReliable, bool bMsgReliable, bool bSequenced);

            //int init (const char *pszHost, uint16 ui16Port);

            int addUserId (const char *pszUserId);

            int addAreaOfInterest (const char *pszAreaName, float fUpperLeftLat, float fUpperLeftLon,
                                   float fLowerRightLat, float fLowerRightLon,
                                   int64 i64StatTime, int64 i64EndTime);

            /**
             * Add a new path to the node context.
             */
            int registerPath (IHMC_VOI::NodePath *pPath);

            /**
             * Choose what path the node is following now.
             */
            int setCurrentPath (const char *pszPathID);

            /**
             * A time stamp is set automatically by DisServicePro.
             */
            int setCurrentPosition (float fLatitude, float fLongitude, float Altitude,
                                    const char *pszLocation, const char *pszNote);

            int setBatteryLevel(unsigned int uiBatteryLevel);
            int setMemoryAvailable(unsigned int uiMemoryAvailable);

            int addPeer (AdaptorType protocol, const char *pszNetworkInterface,
                         const char *pszRemoteAddress, uint16 ui16Port);

            int getAdaptorType (AdaptorId adaptorId, AdaptorType &adaptorType);
           // int getData (const char *pszId, void **pData, uint32 &ui32DataLen);

            /**
             * Method that lets the application explicitly give a feedback about
             * the usefulness of the data identified by pszMessageID.
             */
            int notUseful (const char *pszMessageID);

            /**
             * Search data based on the given groupName and a query on the
             * metadata fields. The data is searched both in the local cache,
             * and on the ones of neighboring nodes.

            NOMADSUtil::PtrLList<const char> * search (const char *pszGroupName, const char *pszQueryType,
                const char *pszQueryQualifiers, const void *pQuery,
                unsigned int uiQueryLen, int64 i64TimeoutInMilliseconds,
                char **ppszQueryId);*/

            void resetTransmissionCounters (void);

            const char * getVersion (void) const;
            int configureProperties (NOMADSUtil::ConfigManager* pCfgMgr);
            int setRankingWeigths (float coordRankWeight, float timeRankWeight, float expirationRankWeight, float impRankWeight,
                                   float sourceReliabilityRankWeigth, float informationContentRankWeigth, float predRankWeight,
                                   float targetWeight, bool bStrictTarget, bool bConsiderFuturePathSegmentForMatchmacking);
            int setSelectivity (float matchingThreshold);
            int setRangeOfInfluence (const char *pszMilStd2525Symbol, uint32 ui32RangeInMeters);
            int setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters);
            int setUsefulDistance (const char* pszDataMIMEType, uint32 ui32UsefulDistanceInMeters);
            int addCustumPoliciesAsXML (const char **ppszCustomPoliciesXML);
            int setMissionId (const char* pszMissionName);
            int setRole (const char* pszRole);
            IHMC_VOI::NodePath * getCurrentPath (void);
            char** getPeerList (void);
            int getData (const char* pszId, const char* pszCallbackParameter, void** ppData, uint32& ui32DataLen, bool& bHasMoreChunks);
            int release (const char* pszMessageID, void* pData);
            char** getDSProIds (const char* pszObjectId, const char* pszInstanceId);
            char ** getMatchingMetadata (NOMADSUtil::AVList *pAVQueryList, int64 i64BeginArrivalTimestamp, int64 i64EndArrivalTimestamp);
            int requestCustomAreaChunk (const char* pszChunkedObjMsgId, const char* pszMIMEType, uint32 ui32StartXPixel, uint32 ui32EndXPixel, uint32 ui32StartYPixel, uint32 ui32EndYPixel, uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds);
            int requestCustomTimeChunk (const char* pszChunkedObjMsgId, const char* pszMIMEType, int64 i64StartTimeInMillisec, int64 i64EndTimeInMillisec, uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds);
            int requestMoreChunks (const char* pszChunkedMsgId, const char* pszCallbackParameter);
            int search (const char* pszGroupName, const char* pszQueryType, const char* pszQueryQualifiers, const void* pszQuery, unsigned uiQueryLen, int64 i64TimeoutInMilliseconds, char** ppszQueryId);
            int searchReply (const char* pszQueryId, const char** ppszMatchingMsgIds);
            int volatileSearchReply (const char* pszQueryId, const void* pReply, uint16 ui162ReplyLen);
            int addMessage (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, const char* pszJsonMetadata, const void* pData, uint32 ui32DataLen, int64 i64ExpirationTime, char** ppszId);
            int addMessage (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, NOMADSUtil::AVList* pMetadataAttrList, const void* pData, uint32 ui32DataLen, int64 i64ExpirationTime, char** ppszId);
            int chunkAndAddMessage (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, const char* pszJsonMetadata, const void* pData, uint32 ui32DataLen, const char* pszDataMimeType, int64 i64ExpirationTime, char** ppszId);
            int chunkAndAddMessage (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, NOMADSUtil::AVList *pMetadataAttrList, const void* pData, uint32 ui32DataLen, const char* pszDataMimeType, int64 i64ExpirationTime, char** ppszId);
            int disseminateMessage (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, const void *pData, uint32 ui32DataLen, int64 i64ExpirationTime, char **ppszId);
            int addAnnotation (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, const char* pszJsonMetadata, const char* pszReferredObject, int64 i64ExpirationTime, char** ppszId);
            int addAnnotation (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, NOMADSUtil::AVList* pszMetadata, const char* pszReferredObject, int64 i64ExpirationTime, char** ppszId);
            int cancel (const char* pszId);
            int cancelByObjectAndInstanceId (const char *pszObjectId, const char *pszInstanceId);
            NOMADSUtil::String getNodeId (void);
            NOMADSUtil::String getSessionId (void) const;
            NOMADSUtil::String getNodeContext (const char *pszNodeId) const;
            //TO-DO implement this method
            int addAreaOfInterest (const char *pszAreaName, float fUpperLeftLat, float fUpperLeftLon,
                float fLowerRightLat, int fLowerRightLon,
                int64 i64StatTime, int64 i64EndTime);
            int registerDSProListener (uint16 ui16ClientId, DSProListener *pListener, uint16& ui16AssignedClientId);
            int deregisterDSProListener (uint16 ui16ClientId, DSProListener *pListener);
            int registerMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener* pListener, uint16& ui16AssignedClientId);
            int deregisterMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener* pListener);
            int registerSearchListener (uint16 ui16ClientId, SearchListener* pListener, uint16& ui16AssignedClientId);
            int deregisterSearchListener (uint16 ui16ClientId, SearchListener* pListener);
           // int reloadCommAdaptors (void);
            void resetTransmissionHistory (void);

            //begin ---- methods regarding the encryption
            int push (const char* pszGroupName, const void* pData, uint32 ui32DataLen, int64 i64ExpirationTime);
            int changeEncryptionKey (const void* pEncrtionKey);
            //end --- methods regarding encryption
            // Callbacks
            int dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                             const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                             const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks,
                             const char *pszCallbackParameter);
            int metadataArrived (const char *pszId, const char *pszGroupName, const char *pszReferredDataObjectId,
                                 const char *pszReferredDataInstanceId, const char *pszXMLMetadada,
                                 const char *pszReferredDataId, const char *pszQueryId);
            int searchArrived (const char *pszQueryId, const char *pszGroupName,
                               const char *pszQuerier, const char *pszQueryType,
                               const char *pszQueryQualifiers,
                               const void *pszQuery, unsigned int uiQueryLen);
            int searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds,
                                    const char *pszMatchingNodeId);
            int volatileSearchReplyArrived (const char *pszQueryId, const void *pReply,
                                            uint16 ui162ReplyLen, const char *pszMatchingNodeId);

        private:
            NOMADSUtil::UInt32Hashtable<DSProListener> _dSProListeners;
            NOMADSUtil::UInt32Hashtable<MatchmakingLogListener> _matchmakingLogListeners;
            NOMADSUtil::UInt32Hashtable<SearchListener> _searchListeners;
    };
}

#endif // INCL_DISSERVICE_PRO_PROXY_H

