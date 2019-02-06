/*
 * UDPDatagramPacket.cpp
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

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include <cstring>

#include "UDPDatagramPacket.h"
#include "ProxyMessages.h"
#include "Connection.h"


namespace ACMNetProxy
{
    UDPDatagramPacket::UDPDatagramPacket (Connection * const pConnection, const CompressionSettings & compressionSettings, const Protocol protocol,
                                          const NOMADSUtil::IPHeader * const pIPHeader, const NOMADSUtil::UDPHeader * const pUDPHeader) :
        _ui32RemoteProxyUniqueID{pConnection ? pConnection->getRemoteNetProxyID() : 0}, _pConnection{pConnection}, _compressionSettings{compressionSettings},
        _ui32IPSource{pIPHeader->srcAddr.ui32Addr}, _ui32IPDestination{pIPHeader->destAddr.ui32Addr}, _ui16IPIdentification{pIPHeader->ui16Ident},
        _ui8PMProtocol{static_cast<uint8> (protocol)}, _ui8IPPacketTTL{pIPHeader->ui8TTL}, _i64CreationTime{NOMADSUtil::getTimeInMilliseconds()}
    {
        _bIsFragmented = (pIPHeader->ui16FlagsAndFragOff & IP_MF_FLAG_FILTER) != 0;
        if (!_bIsFragmented) {
            _ui16CurrentPayloadLength = pUDPHeader->ui16Len;
        }
        else {
            _ui16CurrentPayloadLength = pIPHeader->ui16TLen - ((pIPHeader->ui8VerAndHdrLen & 0x0F) * 4);
        }

        _pui8UDPDatagram = new uint8[pUDPHeader->ui16Len];
        memcpy (_pui8UDPDatagram, pUDPHeader, _ui16CurrentPayloadLength);
    }

    const NOMADSUtil::InetAddr * const UDPDatagramPacket::getRemoteProxyAddr (void) const
    {
        return _pConnection ? _pConnection->getRemoteNetProxyInetAddr() : nullptr;
    }

    int UDPDatagramPacket::reassembleFragment (const NOMADSUtil::IPHeader * const pIPHeader, const uint8 * const pui8Fragment)
    {
        if (!isMissingFragment (pIPHeader)) {
            return -1;
        }

        uint16 ui16FragmentLength = pIPHeader->ui16TLen - ((pIPHeader->ui8VerAndHdrLen & 0x0F) * 4);
        if ((_ui16CurrentPayloadLength + ui16FragmentLength) > reinterpret_cast<NOMADSUtil::UDPHeader *> (_pui8UDPDatagram)->ui16Len) {
            return -2;
        }

        uint16 ui16Offset = (pIPHeader->ui16FlagsAndFragOff & IP_OFFSET_FILTER) * 8;
        memcpy (_pui8UDPDatagram + ui16Offset, pui8Fragment, ui16FragmentLength);
        _ui16CurrentPayloadLength += ui16FragmentLength;
        _bIsFragmented = _ui16CurrentPayloadLength < reinterpret_cast<NOMADSUtil::UDPHeader *> (_pui8UDPDatagram)->ui16Len;

        return getPacketLen();
    }
}

