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

#include "UDPAdaptor.h"

#include "ConfigManager.h"

#include "UDPDatagramSocket.h"
#include "StringTokenizer.h"

#include <string>
#include <set>

using namespace NOMADSUtil;

namespace UDP_ADAPTOR
{
    struct UDPAdaptorImpl
    {
        UDPAdaptorImpl (uint16 ui16Port)
            : _ui16Port (ui16Port)
        {
        }

        ~UDPAdaptorImpl (void) {}

        int init (void)
        {
            return _socket.init();
        }

        void addPeer (const char *pszPeer)
        {
            if (pszPeer != nullptr) {
                std::string dst (pszPeer);
                _destinations.insert (dst);
            }
        }

        int send (const void *pBuf, uint32 ui32Len)
        {
            if ((pBuf == nullptr) || (ui32Len == 0U)) {
                return 0;
            }
            bool success = true;
            for (std::string dst : _destinations) {
                int rc = _socket.sendTo (dst.c_str(), _ui16Port, pBuf, (int)ui32Len);
                success = success && (rc == 0);
            }
            return success ? 0 : -2;
        }

    private:
        const uint16 _ui16Port;
        UDPDatagramSocket _socket;
        std::set<std::string> _destinations;
    };
}

using namespace IHMC_ACI;
using namespace UDP_ADAPTOR;

const unsigned short UDPAdaptor::DEFAULT_PORT = 2401;

UDPAdaptor::UDPAdaptor (unsigned int uiId, CommAdaptorListener *pListener,
                        const char *pszNodeId, uint16 ui16Port)
    : CommAdaptor (uiId, UDP, true, false, pszNodeId, pListener),
       _pImpl (new UDPAdaptorImpl (ui16Port))
{
}

UDPAdaptor::~UDPAdaptor (void)
{
    delete _pImpl;
}

int UDPAdaptor::init (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == nullptr) {
        return -1;
    }
    const char *pszAddr = pCfgMgr->getValue ("aci.dspro.adaptor.udp.peer.addr");
    if (pszAddr != nullptr) {
        StringTokenizer tokenizer (pszAddr, ',', ',');
        for (const char *pszToken; (pszToken = tokenizer.getNextToken()) != nullptr;) {
            _pImpl->addPeer (pszToken);
        }
    }
    if (_pImpl->init() < 0) {
        return -2;
    }
    return 0;
}

int UDPAdaptor::changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len)
{
    return 0;
}

int UDPAdaptor::startAdaptor (void)
{
    return 0;
}

int UDPAdaptor::stopAdaptor (void)
{
    return 0;
}

void UDPAdaptor::resetTransmissionCounters (void)
{
}

bool UDPAdaptor::supportsManycast (void)
{
    return true;
}

int UDPAdaptor::sendContextUpdateMessage (const void *pBuf, uint32 ui32Len,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendContextVersionMessage (const void *pBuf, uint32 ui32Len,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendDataMessage (Message *pMsg,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendChunkedMessage (Message *pMsg, const char *pszDataMimeType,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendMessageRequestMessage (const char *pszMsgId,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendChunkRequestMessage (const char *pszMsgId,
    NOMADSUtil::DArray<uint8> *pCachedChunks,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendPositionMessage (const void *pBuf, uint32 ui32Len,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendSearchMessage (SearchProperties &searchProp,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendSearchReplyMessage (const char *pszQueryId,
    const char **ppszMatchingMsgIds,
    const char *pszTarget,
    const char *pszMatchingNode,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendVolatileSearchReplyMessage (const char *pszQueryId,
    const void *pReply, uint16 ui16ReplyLen,
    const char *pszTarget,
    const char *pszMatchingNode,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendTopologyReplyMessage (const void *pBuf, uint32 ui32BufLen,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendTopologyRequestMessage (const void *pBuf, uint32 ui32Len,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendUpdateMessage (const void *pBuf, uint32 ui32Len,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendVersionMessage (const void *pBuf, uint32 ui32Len,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendWaypointMessage (const void *pBuf, uint32 ui32Len,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::sendWholeMessage (const void *pBuf, uint32 ui32Len,
    const char *pszPublisherNodeId,
    const char **ppszRecipientNodeIds,
    const char **ppszInterfaces)
{
    return 0;
}

int UDPAdaptor::notifyEvent (const void *pBuf, uint32 ui32Len,
                             const char *pszPublisherNodeId,
                             const char *pszTopic, const char **ppszInterfaces)
{
    _pImpl->send (pBuf, ui32Len);
    return 0;
}

int UDPAdaptor::subscribe (Subscription &sub)
{
    return 0;
}

void UDPAdaptor::messageArrived (const char *pszTopic, const void *pMsg, int iLen)
{
}
