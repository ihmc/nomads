#ifndef INCL_PROXY_NETWORK_MESSAGE_H
#define INCL_PROXY_NETWORK_MESSAGE_H

/*
 * ProxyNetworkMessage.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
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
 * Class that handles a Proxy Message and its payload, if any.
 */

#include "FTypes.h"
#include "InetAddr.h"

#include "ProxyMessages.h"


namespace ACMNetProxy
{
    class ProxyNetworkMessage
    {
    public:
        ProxyNetworkMessage (const NOMADSUtil::InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage, uint32 ui32DestVirtualIPAddr,
                             const uint8 * const pui8MessagePayload = NULL, const uint16 ui16PayloadLen = 0);
        ~ProxyNetworkMessage (void);

        bool operator == (const ProxyNetworkMessage &rProxyNetworkMessage) const;
        bool operator != (const ProxyNetworkMessage &rProxyNetworkMessage) const;

        ProxyMessage::PacketType getMessageType (void) const;
        uint16 getMessageTotalLength (void) const;
        uint16 getMessageHeaderLength (void) const;
        uint16 getMessagePayloadLength (void) const;

        uint32 getDestVirtualIPAddr (void) const;
        const NOMADSUtil::InetAddr * const getDestInetAddr (void) const;
        uint32 getDestIPAddr (void) const;
        const char * const getDestIPAddrAsString (void) const;
        uint16 getDestPortNumber (void) const;
        const uint8 * const getMessageAsByteStream (void) const;
        const ProxyMessage * const getProxyMessage (void) const;
        const uint8 * const getMessagePayload (void) const;

        int setNewPacketPayload (const uint8 * const pNewBuf, uint16 ui16PayloadLen);

    private:
        const NOMADSUtil::InetAddr _ProxyAddr;
        uint8 *_pBuf;

        ProxyMessage *_pProxyMessage;
        const uint32 _ui32DestVirtualIPAddr;
        uint8 *_pui8MessagePayload;
        uint16 _ui16PayloadLen;
    };


    inline ProxyNetworkMessage::~ProxyNetworkMessage (void)
    {
        if (_pBuf) {
            delete[] _pBuf;
        }

        _pui8MessagePayload = NULL;
        _ui16PayloadLen = 0;
    }

    inline bool ProxyNetworkMessage::operator == (const ProxyNetworkMessage &rProxyNetworkMessage) const
    {
        return this->_pBuf == rProxyNetworkMessage._pBuf;
    }

    inline bool ProxyNetworkMessage::operator != (const ProxyNetworkMessage &rProxyNetworkMessage) const
    {
        return this->_pBuf != rProxyNetworkMessage._pBuf;
    }

    inline ProxyMessage::PacketType ProxyNetworkMessage::getMessageType (void) const
    {
        return _pProxyMessage->getMessageType();
    }

    inline uint16 ProxyNetworkMessage::getMessageTotalLength (void) const
    {
        return (_pProxyMessage->getMessageHeaderSize() + _ui16PayloadLen);
    }

    inline uint16 ProxyNetworkMessage::getMessageHeaderLength (void) const
    {
        return _pProxyMessage->getMessageHeaderSize();
    }

    inline uint16 ProxyNetworkMessage::getMessagePayloadLength (void) const
    {
        return _ui16PayloadLen;
    }

    inline uint32 ProxyNetworkMessage::getDestVirtualIPAddr (void) const
    {
        return _ui32DestVirtualIPAddr;
    }

    inline const NOMADSUtil::InetAddr * const ProxyNetworkMessage::getDestInetAddr (void) const
    {
        return &_ProxyAddr;
    }

    inline uint32 ProxyNetworkMessage::getDestIPAddr (void) const
    {
        return _ProxyAddr.getIPAddress();
    }

    inline const char * const ProxyNetworkMessage::getDestIPAddrAsString (void) const
    {
        return _ProxyAddr.getIPAsString();
    }

    inline uint16 ProxyNetworkMessage::getDestPortNumber (void) const
    {
        return _ProxyAddr.getPort();
    }

    inline const uint8 * const ProxyNetworkMessage::getMessageAsByteStream (void) const
    {
        return _pBuf;
    }

    inline const ProxyMessage * const ProxyNetworkMessage::getProxyMessage (void) const
    {
        return _pProxyMessage;
    }

    inline const uint8 * const ProxyNetworkMessage::getMessagePayload (void) const
    {
        return _pui8MessagePayload;
    }

}

#endif  // INCL_PROXY_NETWORK_MESSAGE_H
