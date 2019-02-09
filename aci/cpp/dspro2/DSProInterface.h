/*
 * DSProInterface.h
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
 * Created on September 29, 2016, 3:25 PM
 */

#ifndef INCL_DSPRO_INTERFACE_H
#define INCL_DSPRO_INTERFACE_H

#include "CommAdaptorListener.h"

#include "StrClass.h"

namespace NOMADSUtil
{
    class AVList;
    class ConfigManager;
}

namespace IHMC_VOI
{
    class NodePath;
}

namespace IHMC_ACI
{
    class ControlMessageListener;
    class DSProListener;
    class MatchmakingLogListener;
    class SearchListener;

    class DSProInterface
    {
        public:
            DSProInterface (void);
            virtual ~DSProInterface (void);

            virtual const char * getVersion (void) const = 0;

            virtual int configureProperties (NOMADSUtil::ConfigManager *pCfgMgr) = 0;
            virtual int setRankingWeigths (float coordRankWeight, float timeRankWeight,
                                           float expirationRankWeight, float impRankWeight,
                                           float sourceReliabilityRankWeigth,
                                           float informationContentRankWeigth, float predRankWeight,
                                           float targetWeight, bool bStrictTarget,
                                           bool bConsiderFuturePathSegmentForMatchmacking) = 0;

            virtual int setSelectivity (float matchingThreshold) = 0;
            virtual int setRangeOfInfluence (const char *pszMilStd2525Symbol, uint32 ui32RangeInMeters) = 0;
            virtual int setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters) = 0;
            virtual int setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters) = 0;

            virtual int addCustumPoliciesAsXML (const char **ppszCustomPoliciesXML) = 0;

            virtual int registerPath (IHMC_VOI::NodePath *pPath) = 0;

            virtual int addUserId (const char *pszUserName) = 0;
            virtual int addAreaOfInterest (const char *pszAreaName, float fUpperLeftLat, float fUpperLeftLon,
                                           float fLowerRightLat, float fLowerRightLon,
                                           int64 i64StatTime, int64 i64EndTime) = 0;
            virtual int setMissionId (const char *pszMissionName) = 0;
            virtual int setRole (const char *pszRole) = 0;
            virtual int setCurrentPath (const char *pszPathID) = 0;

            virtual int setCurrentPosition (float fLatitude, float fLongitude, float Altitude,
                                            const char *pszLocation, const char *pszNote) = 0;

            virtual int setBatteryLevel (unsigned int uiBatteryLevel) = 0;

            virtual int setMemoryAvailable (unsigned int uiMemoryAvailable) = 0;

            virtual int addPeer (AdaptorType protocol, const char *pszNetworkInterface,
                                 const char *pszRemoteAddress, uint16 ui16Port) = 0;

            virtual int getAdaptorType (AdaptorId adaptorId, AdaptorType &adaptorType) = 0;

            virtual IHMC_VOI::NodePath * getCurrentPath (void) = 0;

            virtual char ** getPeerList (void) = 0;

            virtual int getData (const char *pszId, const char *pszCallbackParameter,
                                 void **ppData, uint32 &ui32DataLen,
                                 bool &bHasMoreChunks) = 0;
            virtual int release (const char *pszMessageID, void *pData) = 0;

            virtual char ** getDSProIds (const char *pszObjectId, const char *pszInstanceId) = 0;

            virtual char ** getMatchingMetadata (NOMADSUtil::AVList *pAVQueryList,
                                                 int64 i64BeginArrivalTimestamp = 0,
                                                 int64 i64EndArrivalTimestamp = 0) = 0;

            virtual int notUseful (const char *pszMessageID) = 0;

            virtual int requestCustomAreaChunk (const char *pszChunkedObjMsgId, const char *pszMIMEType,
                                        uint32 ui32StartXPixel, uint32 ui32EndXPixel, uint32 ui32StartYPixel,
                                        uint32 ui32EndYPixel, uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds) = 0;

            virtual int requestCustomTimeChunk (const char *pszChunkedObjMsgId, const char *pszMIMEType,
                                                int64 i64StartTimeInMillisec, int64 i64EndTimeInMillisec,
                                                uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds) = 0;

            virtual int requestMoreChunks (const char *pszChunkedMsgId, const char *pszCallbackParameter) = 0;

            virtual int search (const char *pszGroupName, const char *pszQueryType,
                        const char *pszQueryQualifiers, const void *pszQuery,
                        unsigned int uiQueryLen, int64 i64TimeoutInMilliseconds,
                        char **ppszQueryId) = 0;

            virtual int searchReply (const char *pszQueryId, const char **ppszMatchingMsgIds) = 0;

            virtual int volatileSearchReply (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen) = 0;

            virtual int addMessage (const char *pszGroupName, const char *pszObjectId,
                            const char *pszInstanceId, const char *pszJsonMetadata,
                            const void *pData, uint32 ui32DataLen,
                            int64 i64ExpirationTime, char **ppszId) = 0;
            virtual int addMessage (const char *pszGroupName, const char *pszObjectId,
                            const char *pszInstanceId, NOMADSUtil::AVList *pMetadataAttrList,
                            const void *pData, uint32 ui32DataLen,
                            int64 i64ExpirationTime, char **ppszId) = 0;

            virtual int chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                                    const char *pszInstanceId, const char *pszJsonMetadata,
                                    const void *pData, uint32 ui32DataLen,
                                    const char *pszDataMimeType,
                                    int64 i64ExpirationTime, char **ppszId) = 0;
            virtual int chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                                    const char *pszInstanceId, NOMADSUtil::AVList *pMetadataAttrList,
                                    const void *pData, uint32 ui32DataLen,
                                    const char *pszDataMimeType,
                                    int64 i64ExpirationTime, char **ppszId) = 0;
            virtual int disseminateMessage (const char *pszGroupName, const char *pszObjectId,
                                            const char *pszInstanceId, const void *pData, uint32 ui32DataLen,
                                            int64 i64ExpirationTime, char **ppszId) = 0;

            virtual int addAnnotation (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, const char *pszJsonMetadata,
                               const char *pszReferredObject, int64 i64ExpirationTime,
                               char **ppszId) = 0;
            virtual int addAnnotation (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, NOMADSUtil::AVList *pszMetadata,
                               const char *pszReferredObject, int64 i64ExpirationTime,
                               char **ppszId) = 0;

            virtual int subscribe (const char *pszGroupName, uint8 ui8Priority, bool bGroupReliable, bool bMsgReliable, bool bSequenced) = 0;

            virtual int cancel (const char *pszId) = 0;
            virtual int cancelByObjectAndInstanceId (const char *pszObjectId, const char *pszInstanceId) = 0;

            virtual NOMADSUtil::String getSessionId (void) const  = 0;
            virtual NOMADSUtil::String getNodeContext (const char *pszNodeId) const = 0;

            // DSPro Listener
            virtual int registerDSProListener (uint16 ui16ClientId, DSProListener *pListener, uint16 &ui16AssignedClientId) = 0;
            virtual int deregisterDSProListener (uint16 ui16ClientId, DSProListener *pListener) = 0;

            // Matchmaking Log Listener

            virtual int registerMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener, uint16 &ui16AssignedClientId) = 0;
            virtual int deregisterMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener) = 0;

            // Search Listener
            virtual int registerSearchListener (uint16 ui16ClientId, SearchListener *pListener, uint16 &ui16AssignedClientId) = 0;
            virtual int deregisterSearchListener (uint16 ui16ClientId, SearchListener *pListener) = 0;

            //virtual int reloadCommAdaptors (void) = 0;
            virtual void resetTransmissionCounters (void) = 0;
            virtual void resetTransmissionHistory (void) = 0;
    };
}

#endif    /* INCL_DSPRO_INTERFACE_H */
