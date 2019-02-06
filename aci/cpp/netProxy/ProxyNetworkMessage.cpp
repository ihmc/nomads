/*
 * ProxyNetworkMessage.cpp
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
 */

#include "ProxyNetworkMessage.h"
#include "Utilities.h"


namespace ACMNetProxy
{
    ProxyNetworkMessage::ProxyNetworkMessage (const NOMADSUtil::InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage, uint32 ui32SourceIPAddr,
                                              uint32 ui32DestIPAddr, const uint8 * const pui8MessagePayload, const uint16 ui16PayloadLen) :
        _iaProxyAddr{*pProxyAddr}, _pui8Buf{((ui16PayloadLen > 0) && pui8MessagePayload) ? new uint8[pProxyMessage->getMessageHeaderSize() + ui16PayloadLen] :
                                                                                      new uint8[pProxyMessage->getMessageHeaderSize()]},
        _pProxyMessage{reinterpret_cast<ProxyMessage *> (_pui8Buf)}, _ui32SourceIPAddr{ui32SourceIPAddr}, _ui32DestIPAddr{ui32DestIPAddr},
        _ui16PayloadLen{((ui16PayloadLen > 0) && pui8MessagePayload) ? ui16PayloadLen : 0_us}
    {
        memcpy ((void *) _pui8Buf, (void *) pProxyMessage, pProxyMessage->getMessageHeaderSize());
        if ((ui16PayloadLen > 0) && pui8MessagePayload) {
            memcpy ((void *) (getPointerToPayload()), (void *) pui8MessagePayload, ui16PayloadLen);
        }
    }

    ProxyNetworkMessage::ProxyNetworkMessage (ProxyNetworkMessage && pnm) :
        _iaProxyAddr{std::move (pnm._iaProxyAddr)}, _pui8Buf{std::move (pnm._pui8Buf)}, _pProxyMessage{reinterpret_cast<ProxyMessage *> (_pui8Buf)},
        _ui32SourceIPAddr{std::move (pnm._ui32SourceIPAddr)}, _ui32DestIPAddr{std::move (pnm._ui32DestIPAddr)},
        _ui16PayloadLen{std::move (pnm._ui16PayloadLen)}
    {
        pnm._pui8Buf = nullptr;
    }

    ProxyNetworkMessage & ProxyNetworkMessage::operator= (ProxyNetworkMessage && pnm)
    {
        _iaProxyAddr = std::move (pnm._iaProxyAddr);
        _pui8Buf = std::move (pnm._pui8Buf);
        _pProxyMessage = reinterpret_cast<ProxyMessage *> (_pui8Buf);
        _ui32SourceIPAddr = std::move (pnm._ui32SourceIPAddr);
        _ui32DestIPAddr = std::move (pnm._ui32DestIPAddr);
        _ui16PayloadLen = std::move (pnm._ui16PayloadLen);

        pnm._pui8Buf = nullptr;

        return *this;
    }

    int ProxyNetworkMessage::setNewPacketPayload (const uint8 * const pNewBuf, uint16 ui16PayloadLen)
    {
        if (!pNewBuf || (ui16PayloadLen == 0)) {
            return -1;
        }

        if (ui16PayloadLen > _ui16PayloadLen) {
            // Need to resize buffer, keeping message header (realloc of the whole buffer is not necessary)
            auto * const pTempBuf = new uint8[_pProxyMessage->getMessageHeaderSize() + ui16PayloadLen];
            memcpy (pTempBuf, _pui8Buf, _pProxyMessage->getMessageHeaderSize());
            _pProxyMessage = reinterpret_cast<ProxyMessage *> (pTempBuf);
            delete[] _pui8Buf;
            _pui8Buf = pTempBuf;
        }

        memcpy ((void *) (getPointerToPayload()), pNewBuf, ui16PayloadLen);
        _ui16PayloadLen = ui16PayloadLen;

        return _ui16PayloadLen;
    }
}
