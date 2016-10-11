/**
 * DSProProxy.cpp
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

#include "DSProProxy.h"

#include "DisseminationServiceProxyCallbackHandler.h"
#include "DisseminationServiceProxyListener.h"

#include "Logger.h"
#include "PtrLList.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define DISPRO_SVC_PROXY_SERVER_PORT_NUMBER 56487   // Also see DisServiceProProxyServer.h

DSProProxy::DSProProxy (uint16 ui16ApplicationId)
{
    //TODO: implement this
}

DSProProxy::~DSProProxy()
{
    //TODO: implement this
}

int DSProProxy::init (const char*, uint16)
{
    //TODO: implement this
    return 0;
}

int DSProProxy::setRankingWeigths (float coordRankWeight, float timeRankWeight,
                                   float expirationRankWeight, float impRankWeight,
                                   float predRankWeight, float targetWeight,
                                   bool bStrictTarget,
                                   bool bConsiderFuturePathSegmentForMatchmacking)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::registerPath (NodePath *pPath)
{
    // TODO: implement this
    return 0;
}

/**
 * Choose what path the node is following now.
 */
int DSProProxy::setCurrentPath (const char *pszPathID)
{
    // TODO: implement this
    return 0;
}

/**
 * A time stamp is set automatically by DisServicePro.
 */
int DSProProxy::setCurrentPosition (float fLatitude, float fLongitude, float Altitude,
                                            const char *pszLocation, const char *pszNote)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::setBatteryLevel(unsigned int uiBatteryLevel)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::setMemoryAvailable(unsigned int uiMemoryAvailable)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::addPeer (AdaptorType protocol, const char *pszNetworkInterface,
             const char *pszRemoteAddress, uint16 ui16Port)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::getAdaptorType (AdaptorId adaptorId, AdaptorType &adaptorType)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::getData (const char *pszId, void **pData, uint32 &ui32DataLen)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::notUseful (const char *pszMessageID)
{
    // TODO: implement this
    return 0;
}

PtrLList<const char> * DSProProxy::search (const char *pszGroupName,
                                           const char *pszQuery)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::addMessage (const char *pszGroupName, const char *pszXmlMedatada,
                const void *pData, uint32 ui32DataLen,
                int64 i64ExpirationTime, char **ppszId)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::chunkAndAddMessage (const char *pszGroupName, const char *pszXmlMedatada,
                                    const void *pData, uint32 ui32DataLen,
                                    const char *pszDataMimeType,
                                    int64 i64ExpirationTime, char **ppszId)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::addAnnotation (const char *pszGroupName, const char *pszXmlMedatada,
                               const char *pszReferredObject,
                               int64 i64ExpirationTime, char **ppszId)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::addAnnotation (const char *pszGroupName, MetaData *pMetadata,
                               int64 i64ExpirationTime, char **ppszId)
{
    // TODO: implement this
    return 0;
}

const char * DSProProxy::getNodeId (void)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::registerDSProListener (uint16 ui16ClientId, DSProListener *pListener)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::deregisterDSProListener (uint16 ui16ClientId, DSProListener *pListener)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::registerMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::deregisterMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::resetTransmissionCounters (void)
{
    return 0;
}
