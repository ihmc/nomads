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

#include "CommAdaptor.h"
#include "Defs.h"
#include "PtrLList.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class DSProListener;
    class MatchmakingLogListener;
    class MetaData;
    class NodePath;

    class DSProProxy
    {
        public:
            DSProProxy (uint16 ui16ApplicationId = 0);
            virtual ~DSProProxy (void);

            int init (const char *pszHost, uint16 ui16Port);

            int setRankingWeigths (float coordRankWeight, float timeRankWeight,
                                   float expirationRankWeight, float impRankWeight,
                                   float predRankWeight, float targetWeight,
                                   bool bStrictTarget,
                                   bool bConsiderFuturePathSegmentForMatchmacking);

            /**
             * Add a new path to the node context.
             */
            int registerPath (NodePath *pPath);

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
            int getData (const char *pszId, void **pData, uint32 &ui32DataLen);

            /**
             * Method that lets the application explicitly give a feedback about
             * the usefulness of the data identified by pszMessageID.
             */
            int notUseful (const char *pszMessageID);

            /**
             * Search data based on the given groupName and a query on the
             * metadata fields. The data is searched both in the local cache,
             * and on the ones of neighboring nodes.
             */
            NOMADSUtil::PtrLList<const char> * search (const char *pszGroupName,
                                                       const char *pszQuery);

            /**
             * The message is stored and sent to the nodes which node context
             * matches the metadata (in  pszXmlMedatada) describing the data.
             *
             * - pszXmlMedatada: null-terminated string that contains and XML
             *                   document describing the data
             * - pData: the actual data to be sent
             * - ui32DataLen: the length of the actual data to be sent
             */
            int addMessage (const char *pszGroupName, const char *pszXmlMedatada,
                            const void *pData, uint32 ui32DataLen,
                            int64 i64ExpirationTime, char **ppszId);
            int chunkAndAddMessage (const char *pszGroupName, const char *pszXmlMedatada,
                                    const void *pData, uint32 ui32DataLen,
                                    const char *pszDataMimeType,
                                    int64 i64ExpirationTime, char **ppszId);
            int addAnnotation (const char *pszGroupName, const char *pszXmlMedatada,
                               const char *pszReferredObject,
                               int64 i64ExpirationTime, char **ppszId);
            int addAnnotation (const char *pszGroupName, MetaData *pMetadata,
                               int64 i64ExpirationTime, char **ppszId);

            const char * getNodeId (void);

            // DSPro Listener

            int registerDSProListener (uint16 ui16ClientId, DSProListener *pListener);
            int deregisterDSProListener (uint16 ui16ClientId, DSProListener *pListener);

            // Matchmaking Log Listener

            int registerMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener);
            int deregisterMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener);

            int resetTransmissionCounters (void);
    };
}

#endif // INCL_DISSERVICE_PRO_PROXY_H
