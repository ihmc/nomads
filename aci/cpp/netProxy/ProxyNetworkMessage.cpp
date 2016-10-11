/*
 * ProxyNetworkMessage.cpp
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
 */

#include "ProxyNetworkMessage.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    ProxyNetworkMessage::ProxyNetworkMessage (const InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage, uint32 ui32DestVirtualIPAddr,
                                              const uint8 * const pui8MessagePayload, const uint16 ui16PayloadLen) :
        _ProxyAddr(pProxyAddr->getIPAddress(), pProxyAddr->getPort()), _ui32DestVirtualIPAddr (ui32DestVirtualIPAddr)
    {
        if ((ui16PayloadLen > 0) && pui8MessagePayload) {
            _pBuf = new uint8[pProxyMessage->getMessageHeaderSize() + ui16PayloadLen];
        }
        else {
            _pBuf = new uint8[pProxyMessage->getMessageHeaderSize()];
        }
        memcpy ((void*) _pBuf, (void*) pProxyMessage, pProxyMessage->getMessageHeaderSize());
        _pProxyMessage = (ProxyMessage *) _pBuf;

        if ((ui16PayloadLen > 0) && pui8MessagePayload) {
            memcpy ((void*) (_pBuf + pProxyMessage->getMessageHeaderSize()), (void*) pui8MessagePayload, ui16PayloadLen);
            _pui8MessagePayload = &_pBuf[pProxyMessage->getMessageHeaderSize()];
            _ui16PayloadLen = ui16PayloadLen;
        }
        else {
            _pui8MessagePayload = NULL;
            _ui16PayloadLen = 0;
        }
    }

    int ProxyNetworkMessage::setNewPacketPayload (const uint8 * const pNewBuf, uint16 ui16PayloadLen)
    {
        if (!pNewBuf || (ui16PayloadLen == 0)) {
            return -1;
        }

        if (ui16PayloadLen > _ui16PayloadLen) {
            // Need to resize buffer, keeping message header (realloc of the whole buffer is not necessary)
            uint8 *pTempBuf = new uint8[_pProxyMessage->getMessageHeaderSize() + ui16PayloadLen];
            memcpy (pTempBuf, _pBuf, _pProxyMessage->getMessageHeaderSize());
            delete _pBuf;
            _pBuf = pTempBuf;
        }

        memcpy ((void *) (getMessagePayload()), pNewBuf, ui16PayloadLen);
        _ui16PayloadLen = ui16PayloadLen;

        return _ui16PayloadLen;
    }
}
