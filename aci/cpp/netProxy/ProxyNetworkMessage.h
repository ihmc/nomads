#ifndef INCL_PROXY_NETWORK_MESSAGE_H
#define INCL_PROXY_NETWORK_MESSAGE_H

/*
 * ProxyNetworkMessage.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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
 * Class that handles a ProxyMessage and its payload, if any.
 */

#include "FTypes.h"
#include "InetAddr.h"

#include "ProxyMessages.h"


namespace ACMNetProxy
{
    class ProxyNetworkMessage
    {
    public:
        ProxyNetworkMessage (const NOMADSUtil::InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage, uint32 ui32SourceIPAddr,
                             uint32 ui32DestIPAddr, const uint8 * const pui8MessagePayload = nullptr, const uint16 ui16PayloadLen = 0);
        ProxyNetworkMessage (ProxyNetworkMessage && pnm);
        ~ProxyNetworkMessage (void);

        ProxyNetworkMessage & operator= (ProxyNetworkMessage && pnm);

        bool operator == (const ProxyNetworkMessage &rProxyNetworkMessage) const;
        bool operator != (const ProxyNetworkMessage &rProxyNetworkMessage) const;

        PacketType getMessageType (void) const;
        uint16 getMessageTotalSize (void) const;
        uint16 getMessageHeaderSize (void) const;
        uint16 getMessagePayloadLength (void) const;

        uint32 getSourceIPAddr (void) const;
        uint32 getDestVirtualIPAddr (void) const;
        const NOMADSUtil::InetAddr * const getDestInetAddr (void) const;
        const char * const getDestIPAddrAsString (void) const;
        uint16 getDestPortNumber (void) const;
        const uint8 * const getMessageAsByteStream (void) const;
        const ProxyMessage * const getProxyMessage (void) const;
        const uint8 * const getPointerToPayload (void) const;

        int setNewPacketPayload (const uint8 * const pNewBuf, uint16 ui16PayloadLen);


    private:
        NOMADSUtil::InetAddr _iaProxyAddr;
        uint8 * _pui8Buf;

        ProxyMessage * _pProxyMessage;
        uint32 _ui32SourceIPAddr;
        uint32 _ui32DestIPAddr;
        uint16 _ui16PayloadLen;
    };


    inline ProxyNetworkMessage::~ProxyNetworkMessage (void)
    {
        if (_pui8Buf) {
            delete[] _pui8Buf;
        }
    }

    inline bool ProxyNetworkMessage::operator == (const ProxyNetworkMessage &rProxyNetworkMessage) const
    {
        return this->_pui8Buf == rProxyNetworkMessage._pui8Buf;
    }

    inline bool ProxyNetworkMessage::operator != (const ProxyNetworkMessage &rProxyNetworkMessage) const
    {
        return this->_pui8Buf != rProxyNetworkMessage._pui8Buf;
    }

    inline PacketType ProxyNetworkMessage::getMessageType (void) const
    {
        return _pProxyMessage->getMessageType();
    }

    inline uint16 ProxyNetworkMessage::getMessageTotalSize (void) const
    {
        return (_pProxyMessage->getMessageHeaderSize() + _ui16PayloadLen);
    }

    inline uint16 ProxyNetworkMessage::getMessageHeaderSize (void) const
    {
        return _pProxyMessage->getMessageHeaderSize();
    }

    inline uint16 ProxyNetworkMessage::getMessagePayloadLength (void) const
    {
        return _ui16PayloadLen;
    }

    inline uint32 ProxyNetworkMessage::getSourceIPAddr (void) const
    {
        return _ui32SourceIPAddr;
    }

    inline uint32 ProxyNetworkMessage::getDestVirtualIPAddr (void) const
    {
        return _ui32DestIPAddr;
    }

    inline const NOMADSUtil::InetAddr * const ProxyNetworkMessage::getDestInetAddr (void) const
    {
        return &_iaProxyAddr;
    }

    inline const char * const ProxyNetworkMessage::getDestIPAddrAsString (void) const
    {
        return _iaProxyAddr.getIPAsString();
    }

    inline uint16 ProxyNetworkMessage::getDestPortNumber (void) const
    {
        return _iaProxyAddr.getPort();
    }

    inline const uint8 * const ProxyNetworkMessage::getMessageAsByteStream (void) const
    {
        return _pui8Buf;
    }

    inline const ProxyMessage * const ProxyNetworkMessage::getProxyMessage (void) const
    {
        return _pProxyMessage;
    }

    inline const uint8 * const ProxyNetworkMessage::getPointerToPayload (void) const
    {
        return (_ui16PayloadLen > 0) ? _pui8Buf + getMessageHeaderSize() : nullptr;
    }

}

#endif  // INCL_PROXY_NETWORK_MESSAGE_H
