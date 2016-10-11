/*
 * UDPDatagramPacket.cpp
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

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include "UDPDatagramPacket.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    UDPDatagramPacket::UDPDatagramPacket (const InetAddr * const pRemoteProxyAddr, Connection * const pConnection, Connector * const pConnector,
                                          const CompressionSetting * const pCompressionSetting, const ProxyMessage::Protocol pmProtocol,
                                          const IPHeader * const pIPHeader, const UDPHeader * const pUDPHeader) :
        _pRemoteProxyAddr (pRemoteProxyAddr), _pConnection (pConnection), _pConnector (pConnector), _pCompressionSetting (pCompressionSetting),
        _ui32IPSource (pIPHeader->srcAddr.ui32Addr), _ui32IPDestination (pIPHeader->destAddr.ui32Addr), _ui16IPIdentification(pIPHeader->ui16Ident),
        _ui8PMProtocol(pmProtocol), _ui8IPPacketTTL (pIPHeader->ui8TTL), _i64CreationTime (getTimeInMilliseconds())
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

    int UDPDatagramPacket::reassembleFragment (const IPHeader * const pIPHeader, const uint8 * const pui8Fragment)
    {
        if (!isMissingFragment (pIPHeader)) {
            return -1;
        }

        uint16 ui16FragmentLength = pIPHeader->ui16TLen - ((pIPHeader->ui8VerAndHdrLen & 0x0F) * 4);
        if ((_ui16CurrentPayloadLength + ui16FragmentLength) > ((UDPHeader*) _pui8UDPDatagram)->ui16Len) {
            return -2;
        }

        uint16 ui16Offset = (pIPHeader->ui16FlagsAndFragOff & IP_OFFSET_FILTER) * 8;
        memcpy (_pui8UDPDatagram + ui16Offset, pui8Fragment, ui16FragmentLength);
        _ui16CurrentPayloadLength += ui16FragmentLength;
        _bIsFragmented = (_ui16CurrentPayloadLength < ((UDPHeader*) _pui8UDPDatagram)->ui16Len);

        return getPacketLen();
    }
}

