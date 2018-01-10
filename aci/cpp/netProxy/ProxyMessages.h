#ifndef INCL_PROXY_MESSAGES_H
#define INCL_PROXY_MESSAGES_H

/*
 * ProxyMessages.h
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
 * Structure of the header of messages exchanged by two instances of NetProxy.
 */

#include "FTypes.h"

#include "Utilities.h"

#define PACKET_TYPE_FLAGS_MASK 0x0FU
#define COMPRESSION_LEVEL_FLAGS_MASK 0x0FU
#define COMPRESSION_TYPE_FLAGS_MASK 0x70U
#define HOST_REACHABILITY_FLAG_MASK 0x80U


namespace ACMNetProxy
{
    class CompressionSetting;

    #pragma pack (push, 1)
    // The basic struct of message exchanged between two instances of the NetProxy
    struct ProxyMessage
    {
        enum PacketType
        {
            PMT_InitializeConnection,
            PMT_ConnectionInitialized,
            PMT_ICMPMessage,
            PMT_UDPUnicastData,
            PMT_MultipleUDPDatagrams,
            PMT_UDPBCastMCastData,
            PMT_TCPOpenConnection,
            PMT_TCPConnectionOpened,
            PMT_TCPData,
            PMT_TCPCloseConnection,
            PMT_TCPResetConnection,
            PMT_TunnelPacket
        };

        enum HostReachability
        {
            PMHR_UnreachableHost = 0x00,            //0000 0000
            PMHR_ReachableHost = 0x80               //1000 0000
        };

        enum CompressionType
        {
            PMC_UncompressedData = 0x00,            //0000 0000
            PMC_ZLibCompressedData = 0x10,          //0001 0000
            PMC_LZMACompressedData = 0x20           //0010 0000
        };

        enum Protocol
        {
            PMP_TCP = 0xF0,
            PMP_UDP = 0xF1,
            PMP_MocketsRS = 0xF2,
            PMP_MocketsUU = 0xF3,
            PMP_MocketsRU = 0xF4,
            PMP_MocketsUS = 0xF5,
            PMP_UNDEF_MOCKETS = 0xF6,
            PMP_CSRRS = 0xF7,
            PMP_CSRUU = 0xF8,
            PMP_CSRRU = 0xF9,
            PMP_CSRUS = 0xFA,
            PMP_UNDEF_CSR = 0xFB,
            PMP_UNDEF_PROTOCOL = 0xFF
        };


        ProxyMessage (uint8 ui8MsgTypeAndReachability);

        PacketType getMessageType (void) const;
        static PacketType getMessageType (const ProxyMessage * const pProxyMessage);
        uint32 getMessageHeaderSize (void) const;
        static uint32 getMessageHeaderSize (const ProxyMessage * const pProxyMessage);
        bool getRemoteProxyReachability (void) const;
        static bool getRemoteProxyReachability (const ProxyMessage * const pProxyMessage);

        ProtocolType getProtocolType (void) const;
        static ProtocolType getProtocolType (const ProxyMessage * const pProxyMessage);
        bool isICMPTypePacket (void) const;
        bool isUDPTypePacket (void) const;
        bool isTCPTypePacket (void) const;
        static bool isICMPTypePacket (const ProxyMessage * const pProxyMessage);
        static bool isUDPTypePacket (const ProxyMessage * const pProxyMessage);
        static bool isTCPTypePacket (const ProxyMessage * const pProxyMessage);

        static bool belongsToVirtualConnection (const ProxyMessage * const pProxyMessage, uint16 ui16LocalID, uint16 ui16RemoteID);

        uint8 _ui8MsgTypeAndReachability;
    };


    struct ProxyDataMessage : public ProxyMessage
    {
        ProxyDataMessage (uint8 ui8MsgTypeAndReachability, uint16 ui16PayloadLen);
        uint16 getPayloadLen (void) const;

        uint16 _ui16PayloadLen;
    };

    struct InitializeConnectionProxyMessage : public ProxyMessage
    {
        InitializeConnectionProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                          uint16 ui16LocalUDPServerPort, bool bReachability);

        uint32 _ui32ProxyUniqueID;
        uint16 _ui16LocalMocketsServerPort;
        uint16 _ui16LocalTCPServerPort;
        uint16 _ui16LocalUDPServerPort;
    };


    struct ConnectionInitializedProxyMessage : public ProxyMessage
    {
        ConnectionInitializedProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                           uint16 ui16LocalUDPServerPort, bool bReachability);

        uint32 _ui32ProxyUniqueID;
        uint16 _ui16LocalMocketsServerPort;
        uint16 _ui16LocalTCPServerPort;
        uint16 _ui16LocalUDPServerPort;
    };


    struct ICMPProxyMessage : public ProxyDataMessage
    {
        ICMPProxyMessage (uint16 ui16PayloadLen, uint8 ui8Type, uint8 ui8Code, uint32 ui32RoH, uint32 ui32LocalIP, uint32 ui32RemoteIP, uint32 ui32ProxyUniqueID,
                          uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort, uint16 ui16LocalUDPServerPort, uint8 ui8PacketTTL, bool bReachability);

        uint8 _ui8Type;
        uint8 _ui8Code;
        uint32 _ui32RoH;
        uint32 _ui32LocalIP;
        uint32 _ui32RemoteIP;
        uint32 _ui32ProxyUniqueID;
        uint16 _ui16LocalMocketsServerPort;
        uint16 _ui16LocalTCPServerPort;
        uint16 _ui16LocalUDPServerPort;
        uint8 _ui8PacketTTL;

        #pragma warning (disable:4200)
            uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct UDPUnicastDataProxyMessage : public ProxyDataMessage
    {
        UDPUnicastDataProxyMessage (uint16 ui16PayloadLen, uint32 ui32LocalIP, uint32 ui32RemoteIP, uint32 ui32ProxyUniqueID,
                                    uint8 ui8PacketTTL, const CompressionSetting * const pCompressionSetting);

        uint32 _ui32LocalIP;
        uint32 _ui32RemoteIP;
        uint32 _ui32ProxyUniqueID;
        uint8 _ui8PacketTTL;
        uint8 _ui8CompressionTypeAndLevel;

        #pragma warning (disable:4200)
            uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct MultipleUDPDatagramsProxyMessage : public ProxyDataMessage
    {
        MultipleUDPDatagramsProxyMessage (uint16 ui16PayloadLen, uint32 ui32LocalIP, uint32 ui32RemoteIP, uint32 ui32ProxyUniqueID,
                                          uint8 ui8WrappedUDPDatagramsNum, const CompressionSetting * const pCompressionSetting);

        uint32 _ui32LocalIP;
        uint32 _ui32RemoteIP;
        uint32 _ui32ProxyUniqueID;
        uint8 _ui8WrappedUDPDatagramsNum;
        uint8 _ui8CompressionTypeAndLevel;

        #pragma warning (disable:4200)
            uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct UDPBCastMCastDataProxyMessage : public ProxyDataMessage
    {
        UDPBCastMCastDataProxyMessage (uint16 ui16PayloadLen, uint32 ui32ProxyUniqueID, const CompressionSetting * const pCompressionSetting);

        uint32 _ui32ProxyUniqueID;
        uint8 _ui8CompressionTypeAndLevel;

        #pragma warning (disable:4200)
        uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct OpenConnectionProxyMessage : public ProxyMessage
    {
        OpenConnectionProxyMessage (uint16 ui16LocalID, uint32 ui32LocalIP, uint16 ui16LocalPort, uint32 ui32RemoteIP, uint16 ui16RemotePort, uint32 ui32ProxyUniqueID,
                                    uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort, uint16 ui16LocalUDPServerPort,
                                    const CompressionSetting * const pCompressionSetting, bool bReachability);

        uint16 _ui16LocalID;
        uint32 _ui32LocalIP;
        uint16 _ui16LocalPort;
        uint32 _ui32RemoteIP;
        uint16 _ui16RemotePort;
        uint32 _ui32ProxyUniqueID;
        uint16 _ui16LocalMocketsServerPort;
        uint16 _ui16LocalTCPServerPort;
        uint16 _ui16LocalUDPServerPort;
        uint8 _ui8CompressionTypeAndLevel;
    };


    struct ConnectionOpenedProxyMessage : public ProxyMessage
    {
        ConnectionOpenedProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID, uint32 ui32LocalIP, uint32 ui32ProxyUniqueID, uint16 ui16LocalMocketsServerPort,
                                      uint16 ui16LocalTCPServerPort, uint16 ui16LocalUDPServerPort, const CompressionSetting * const pCompressionSetting, bool bReachability);

        uint16 _ui16LocalID;
        uint16 _ui16RemoteID;
        uint32 _ui32LocalIP;
        uint32 _ui32ProxyUniqueID;
        uint16 _ui16LocalMocketsServerPort;
        uint16 _ui16LocalTCPServerPort;
        uint16 _ui16LocalUDPServerPort;
        uint8 _ui8CompressionTypeAndLevel;
    };


    struct TCPDataProxyMessage : public ProxyDataMessage
    {
        TCPDataProxyMessage (uint16 ui16PayloadLen, uint16 ui16LocalID, uint16 ui16RemoteID, uint8 ui8TCPFlags = 0);

        uint16 _ui16LocalID;
        uint16 _ui16RemoteID;
        uint8 _ui8TCPFlags;

        #pragma warning (disable:4200)
            uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct CloseConnectionProxyMessage : public ProxyMessage
    {
        CloseConnectionProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID);

        uint16 _ui16LocalID;
        uint16 _ui16RemoteID;
    };


    struct ResetConnectionProxyMessage : public ProxyMessage
    {
        ResetConnectionProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID);

        uint16 _ui16LocalID;
        uint16 _ui16RemoteID;
    };


    struct TunnelPacketProxyMessage : public ProxyDataMessage
    {
        TunnelPacketProxyMessage (uint16 ui16PayloadLen);

        #pragma warning (disable:4200)
        uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    #pragma pack (pop)

    inline ProxyMessage::ProxyMessage (uint8 ui8MsgTypeAndReachability) :
        _ui8MsgTypeAndReachability (ui8MsgTypeAndReachability) {}

    inline uint32 ProxyMessage::getMessageHeaderSize (void) const
    {
        return ProxyMessage::getMessageHeaderSize (this);
    }

    inline ProxyMessage::PacketType ProxyMessage::getMessageType (void) const
    {
        return ProxyMessage::getMessageType (this);
    }

    inline ProxyMessage::PacketType ProxyMessage::getMessageType (const ProxyMessage * const pProxyMessage)
    {
        return ProxyMessage::PacketType (pProxyMessage->_ui8MsgTypeAndReachability & PACKET_TYPE_FLAGS_MASK);
    }

    inline bool ProxyMessage::getRemoteProxyReachability (void) const
    {
        return ProxyMessage::getRemoteProxyReachability (this);
    }

    inline bool ProxyMessage::getRemoteProxyReachability (const ProxyMessage * const pProxyMessage)
    {
        return (pProxyMessage->_ui8MsgTypeAndReachability & HOST_REACHABILITY_FLAG_MASK) == ProxyMessage::PMHR_ReachableHost;
    }

    inline ProtocolType ProxyMessage::getProtocolType (void) const
    {
        return ProxyMessage::getProtocolType (this);
    }

    inline ProtocolType ProxyMessage::getProtocolType (const ProxyMessage * const pProxyMessage)
    {
        if (ProxyMessage::isTCPTypePacket (pProxyMessage)) {
            return PT_TCP;
        }
        if (ProxyMessage::isUDPTypePacket (pProxyMessage)) {
            return PT_UDP;
        }
        if (ProxyMessage::isICMPTypePacket (pProxyMessage)) {
            return PT_ICMP;
        }
        return PT_UNDEF;
    }

    inline bool ProxyMessage::isICMPTypePacket (void) const
    {
        return ProxyMessage::isICMPTypePacket (this);
    }

    inline bool ProxyMessage::isUDPTypePacket (void) const
    {
        return ProxyMessage::isUDPTypePacket (this);
    }

    inline bool ProxyMessage::isTCPTypePacket (void) const
    {
        return ProxyMessage::isTCPTypePacket (this);
    }

    inline bool ProxyMessage::isICMPTypePacket (const ProxyMessage * const pProxyMessage)
    {
        switch (pProxyMessage->getMessageType()) {
            case (ProxyMessage::PMT_ICMPMessage):
                return true;
            default:
                break;
        }

        return false;
    }

    inline bool ProxyMessage::isUDPTypePacket (const ProxyMessage * const pProxyMessage)
    {
        switch (pProxyMessage->getMessageType()) {
            case (ProxyMessage::PMT_UDPUnicastData):
            case (ProxyMessage::PMT_MultipleUDPDatagrams):
            case (ProxyMessage::PMT_UDPBCastMCastData):
                return true;
            default:
                break;
        }

        return false;
    }

    inline bool ProxyMessage::isTCPTypePacket (const ProxyMessage * const pProxyMessage)
    {
        switch (pProxyMessage->getMessageType()) {
            case (ProxyMessage::PMT_TCPOpenConnection):
            case (ProxyMessage::PMT_TCPConnectionOpened):
            case (ProxyMessage::PMT_TCPData):
            case (ProxyMessage::PMT_TCPCloseConnection):
            case (ProxyMessage::PMT_TCPResetConnection):
                return true;
            default:
                break;
        }

        return false;
    }

    inline ProxyDataMessage::ProxyDataMessage (uint8 ui8MsgTypeAndReachability, uint16 ui16PayloadLen) :
        ProxyMessage (ui8MsgTypeAndReachability), _ui16PayloadLen (ui16PayloadLen) {}

    inline uint16 ProxyDataMessage::getPayloadLen (void) const
    {
        return _ui16PayloadLen;
    }

}

#endif   // #ifndef INCL_PROXY_MESSAGES_H
