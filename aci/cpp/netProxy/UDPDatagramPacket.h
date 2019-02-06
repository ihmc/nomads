#ifndef INCL_UDP_DATAGRAM_PACKET_H
#define INCL_UDP_DATAGRAM_PACKET_H

/*
 * UDPDatagramPacket.h
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
 *
 * Class that stores UDP Datagrams and provides useful
 * methods to reassembly fragmented datagrams.
 */

#include "FTypes.h"
#include "net/NetworkHeaders.h"
#include "InetAddr.h"

#include "CompressionSettings.h"


namespace ACMNetProxy
{
    class Connection;


    class UDPDatagramPacket
    {
    public:
        UDPDatagramPacket (Connection * const pConnection, const CompressionSettings & compressionSettings, const Protocol protocol,
                           const NOMADSUtil::IPHeader * const pIPHeader, const NOMADSUtil::UDPHeader * const pUDPHeader);
        ~UDPDatagramPacket (void);

        bool operator == (const UDPDatagramPacket & rhs) const;

        const NOMADSUtil::InetAddr * const getRemoteProxyAddr (void) const;
        uint32 getRemoteProxyUniqueID (void) const;
        Connection * const getConnection (void) const;
        const CompressionSettings & getCompressionSetting (void) const;
        uint8 getPMProtocol (void) const;
        uint32 getSourceIPAddr (void) const;
        uint16 getSourcePortNum (void) const;
        uint32 getDestinationIPAddr (void) const;
        uint16 getDestinationPortNum (void) const;
        uint16 getIPIdentification (void) const;
        uint8 getIPPacketTTL (void) const;
        uint16 getPacketLen (void) const;
        uint16 getCurrentPacketLen (void) const;
        int64 getCreationTime (void) const;
        bool isDatagramComplete (void) const;
        const uint8 * const getUDPPacket (void) const;
        const uint8 * const getUDPPayload (void) const;

        int reassembleFragment (const NOMADSUtil::IPHeader * const pIPHeader, const uint8 * const pui8Fragment);
        bool matchesIPPacket (const NOMADSUtil::IPHeader * const pIPHeader) const;
        bool isMissingFragment (const NOMADSUtil::IPHeader * const pIPHeader) const;
        bool canBeWrappedTogether (const UDPDatagramPacket * const pUDPDatagramPacket) const;
        bool belongToTheSameStream (const UDPDatagramPacket * const pUDPDatagramPacket) const;


    private:
        const uint32 _ui32RemoteProxyUniqueID;
        Connection * const _pConnection;
        const CompressionSettings _compressionSettings;
        const uint32 _ui32IPSource;
        const uint32 _ui32IPDestination;
        const uint16 _ui16IPIdentification;
        const uint8 _ui8PMProtocol;
        const uint8 _ui8IPPacketTTL;
        uint8 * _pui8UDPDatagram;

        bool _bIsFragmented;
        uint16 _ui16CurrentPayloadLength;
        const int64 _i64CreationTime;
    };


    inline UDPDatagramPacket::~UDPDatagramPacket (void)
    {
        delete[] _pui8UDPDatagram;
    }

    inline bool UDPDatagramPacket::operator == (const UDPDatagramPacket & rhs) const
    {
        return (this == &rhs);
    }

    inline uint32 UDPDatagramPacket::getRemoteProxyUniqueID (void) const
    {
        return _ui32RemoteProxyUniqueID;
    }

    inline Connection * const UDPDatagramPacket::getConnection (void) const
    {
        return _pConnection;
    }

    inline const CompressionSettings & UDPDatagramPacket::getCompressionSetting (void) const
    {
        return _compressionSettings;
    }

    inline uint8 UDPDatagramPacket::getPMProtocol (void) const
    {
        return _ui8PMProtocol;
    }

    inline uint32 UDPDatagramPacket::getSourceIPAddr (void) const
    {
        return _ui32IPSource;
    }

    inline uint16 UDPDatagramPacket::getSourcePortNum (void) const
    {
        return reinterpret_cast<NOMADSUtil::UDPHeader *> (_pui8UDPDatagram)->ui16SPort;
    }

    inline uint16 UDPDatagramPacket::getDestinationPortNum (void) const
    {
        return reinterpret_cast<NOMADSUtil::UDPHeader *> (_pui8UDPDatagram)->ui16DPort;
    }

    inline uint32 UDPDatagramPacket::getDestinationIPAddr (void) const
    {
        return _ui32IPDestination;
    }

    inline uint16 UDPDatagramPacket::getIPIdentification (void) const
    {
        return _ui16IPIdentification;
    }

    inline uint8 UDPDatagramPacket::getIPPacketTTL(void) const
    {
        return _ui8IPPacketTTL;
    }

    inline uint16 UDPDatagramPacket::getPacketLen (void) const
    {
        return _bIsFragmented ?
            0 : reinterpret_cast<NOMADSUtil::UDPHeader *> (_pui8UDPDatagram)->ui16Len;
    }

    inline uint16 UDPDatagramPacket::getCurrentPacketLen (void) const
    {
        return _ui16CurrentPayloadLength;
    }

    inline int64 UDPDatagramPacket::getCreationTime (void) const
    {
        return _i64CreationTime;
    }

    inline bool UDPDatagramPacket::isDatagramComplete (void) const
    {
        return !_bIsFragmented;
    }

    inline const uint8 * const UDPDatagramPacket::getUDPPacket (void) const
    {
        if (_bIsFragmented) {
            // Data not completely available yet
            return nullptr;
        }

        return _pui8UDPDatagram;
    }

    inline const uint8 * const UDPDatagramPacket::getUDPPayload (void) const
    {
        if (_bIsFragmented) {
            // Data not completely available yet
            return nullptr;
        }

        return _pui8UDPDatagram + sizeof(NOMADSUtil::UDPHeader);
    }

    inline bool UDPDatagramPacket::matchesIPPacket (const NOMADSUtil::IPHeader * const pIPHeader) const
    {
        return (_ui32IPSource == pIPHeader->srcAddr.ui32Addr) && (_ui32IPDestination == pIPHeader->destAddr.ui32Addr) &&
                (_ui16IPIdentification == pIPHeader->ui16Ident);
    }

    inline bool UDPDatagramPacket::isMissingFragment (const NOMADSUtil::IPHeader * const pIPHeader) const
    {
        return _bIsFragmented && matchesIPPacket (pIPHeader);
    }

    inline bool UDPDatagramPacket::canBeWrappedTogether (const UDPDatagramPacket * const pUDPDatagramPacket) const
    {
        return (getRemoteProxyAddr() == pUDPDatagramPacket->getRemoteProxyAddr()) &&
            (_compressionSettings == pUDPDatagramPacket->_compressionSettings) &&
            (_ui8PMProtocol == pUDPDatagramPacket->_ui8PMProtocol) &&
            (_ui32IPSource == pUDPDatagramPacket->_ui32IPSource) &&
            (_ui32IPDestination == pUDPDatagramPacket->_ui32IPDestination);
    }

    inline bool UDPDatagramPacket::belongToTheSameStream (const UDPDatagramPacket * const pUDPDatagramPacket) const
    {
        return (_ui32IPSource == pUDPDatagramPacket->_ui32IPSource) && (_ui32IPDestination == pUDPDatagramPacket->_ui32IPDestination) &&
                (getSourcePortNum() == pUDPDatagramPacket->getSourcePortNum()) && (getDestinationPortNum() == pUDPDatagramPacket->getDestinationPortNum());
    }
}

#endif      // INCL_UDP_DATAGRAM_PACKET_H
