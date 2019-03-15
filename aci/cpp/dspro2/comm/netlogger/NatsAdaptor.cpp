/*
 * NatsAdaptor.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
 *
 * This program is free software you can redistribute it and/or
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
 * Created on February 20, 2017, 9:45 PM
 */

#include "NatsAdaptor.h"

using namespace IHMC_MISC_NATS;

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const unsigned short NatsAdaptor::DEFAULT_PORT = 4222;

NatsAdaptor::NatsAdaptor (unsigned int uiId, CommAdaptorListener *pListener,
                          const char *pszNodeId)
    : CommAdaptor (uiId, NATS, true, false, pszNodeId, pListener), _nats (true)
{ }

NatsAdaptor::~NatsAdaptor (void)
{ }

int NatsAdaptor::init (ConfigManager *pCfgMgr)
{
    return _nats.init (pCfgMgr);
}

int NatsAdaptor::changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len)
{
    return 0;
}

int NatsAdaptor::startAdaptor (void)
{
    return 0;
}

int NatsAdaptor::stopAdaptor (void)
{
    return 0;
}

void NatsAdaptor::resetTransmissionCounters (void)
{
}

bool NatsAdaptor::supportsManycast (void)
{
    return true;
}

int NatsAdaptor::sendContextUpdateMessage (const void *pBuf, uint32 ui32Len,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendContextVersionMessage (const void *pBuf, uint32 ui32Len,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendDataMessage (Message *pMsg,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendChunkedMessage (Message *pMsg, const char *pszDataMimeType,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendMessageRequestMessage (const char *pszMsgId,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendChunkRequestMessage (const char *pszMsgId,
    NOMADSUtil::DArray<uint8> *pCachedChunks,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendPositionMessage (const void *pBuf, uint32 ui32Len,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendSearchMessage (SearchProperties &searchProp,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendSearchReplyMessage (const char *pszQueryId,
    const char **ppszMatchingMsgIds,
    const char *pszTarget,
    const char *pszMatchingNode,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendVolatileSearchReplyMessage (const char *pszQueryId,
    const void *pReply, uint16 ui16ReplyLen,
    const char *pszTarget,
    const char *pszMatchingNode,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendTopologyReplyMessage (const void *pBuf, uint32 ui32BufLen,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendTopologyRequestMessage (const void *pBuf, uint32 ui32Len,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendUpdateMessage (const void *pBuf, uint32 ui32Len,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendVersionMessage (const void *pBuf, uint32 ui32Len,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendWaypointMessage (const void *pBuf, uint32 ui32Len,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::sendWholeMessage (const void *pBuf, uint32 ui32Len,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int NatsAdaptor::notifyEvent (const void *pBuf, uint32 ui32Len,
                              const char *, const char *pszTopic, const char **)
{
    if ((pszTopic == nullptr) || (pBuf == nullptr) || (ui32Len == 0) || (ui32Len > 0xFFFF)) {
        return -1;
    }
    if (_nats.publish (pszTopic, pBuf, (int)ui32Len) > 0) {
        return -2;
    }
    return 0;
}

int NatsAdaptor::subscribe (Subscription &sub)
{
    _nats.subscribe (sub.groupName, this);
    return 0;
}

void NatsAdaptor::messageArrived (const char *pszTopic, const void *pMsg, int iLen)
{
}
