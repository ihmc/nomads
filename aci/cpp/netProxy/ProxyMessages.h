#ifndef INCL_PROXY_MESSAGES_H
#define INCL_PROXY_MESSAGES_H

/*
 * ProxyMessages.h
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
 * Structure of the header of messages exchanged by two instances of NetProxy.
 */

#include "FTypes.h"

#include "Utilities.h"


namespace ACMNetProxy
{
    class CompressionSettings;


    // Up to 15 different PacketTypes are supported with the current header format
    enum class PacketType : uint8
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
        PMT_TunnelPacket,
        PMT_ConnectionError = 0x0F
    };


    enum class HostReachability : uint8
    {
        PMHR_UnreachableHost = 0x00,            //0000 0000
        PMHR_ReachableHost = 0x80               //1000 0000
    };


    enum class CompressionType : uint8
    {
        PMC_UncompressedData = 0x00,            //0000 0000
        PMC_ZLibCompressedData = 0x10,          //0001 0000
        PMC_LZMACompressedData = 0x20           //0010 0000
    };


    enum class Protocol : uint8
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


    #pragma pack (push, 1)
    // The basic struct of message exchanged between two instances of the NetProxy
    struct ProxyMessage
    {
        ProxyMessage (uint8 ui8MsgTypeAndReachability);

        PacketType getMessageType (void) const;
        size_t getMessageHeaderSize (void) const;
        bool getRemoteProxyReachability (void) const;

        ProtocolType getProtocolType (void) const;
        bool isICMPTypePacket (void) const;
        bool isUDPTypePacket (void) const;
        bool isTCPTypePacket (void) const;


        static PacketType getMessageType (const ProxyMessage * const pProxyMessage);
        static size_t getMessageHeaderSize (const ProxyMessage * const pProxyMessage);
        static bool getRemoteProxyReachability (const ProxyMessage * const pProxyMessage);
        static ProtocolType getProtocolType (const ProxyMessage * const pProxyMessage);
        static Protocol getProxyMessageProtocolFromConnectorType (const ConnectorType ct);
        static bool isICMPTypePacket (const ProxyMessage * const pProxyMessage);
        static bool isUDPTypePacket (const ProxyMessage * const pProxyMessage);
        static bool isTCPTypePacket (const ProxyMessage * const pProxyMessage);

        static bool belongsToVirtualConnection (const ProxyMessage * const pProxyMessage, uint16 ui16LocalID, uint16 ui16RemoteID);


        static constexpr uint8 PACKET_TYPE_FLAGS_MASK = 0x0FU;
        static constexpr uint8 COMPRESSION_LEVEL_FLAGS_MASK = 0x0FU;
        static constexpr uint8 COMPRESSION_TYPE_FLAGS_MASK = 0x70U;
        static constexpr uint8 HOST_REACHABILITY_FLAG_MASK = 0x80U;

        uint8 _ui8MsgTypeAndReachability;
    };


    struct ProxyDataMessage : public ProxyMessage
    {
        ProxyDataMessage (uint8 ui8MsgTypeAndReachability, uint16 ui16PayloadLen);
        uint16 getPayloadLen (void) const;

        uint16 _ui16PayloadLen;
    };


    struct InitializeConnectionProxyMessage : public ProxyDataMessage
    {
        InitializeConnectionProxyMessage (uint32 ui32ProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address, uint16 ui16LocalMocketsServerPort,
                                          uint16 ui16LocalTCPServerPort, uint16 ui16LocalUDPServerPort, uint8 ui8NumberOfInterfaces, bool bReachability);

        size_t getMessageHeaderSize (void) const;

        const uint32 _ui32ProxyUniqueID;
        const uint32 _ui32LocalInterfaceIPv4Address;
        const uint16 _ui16LocalMocketsServerPort;
        const uint16 _ui16LocalTCPServerPort;
        const uint16 _ui16LocalUDPServerPort;
        const uint8 _ui8NumberOfInterfaces;

        #pragma warning (disable:4200)
        uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct ConnectionInitializedProxyMessage : public ProxyDataMessage
    {
        ConnectionInitializedProxyMessage (uint32 ui32ProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address, uint16 ui16LocalMocketsServerPort,
                                           uint16 ui16LocalTCPServerPort, uint16 ui16LocalUDPServerPort, uint8 ui8NumberOfInterfaces, bool bReachability);

        size_t getMessageHeaderSize (void) const;

        const uint32 _ui32ProxyUniqueID;
        const uint32 _ui32LocalInterfaceIPv4Address;
        const uint16 _ui16LocalMocketsServerPort;
        const uint16 _ui16LocalTCPServerPort;
        const uint16 _ui16LocalUDPServerPort;
        const uint8 _ui8NumberOfInterfaces;

        #pragma warning (disable:4200)
        uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct ICMPProxyMessage : public ProxyDataMessage
    {
        ICMPProxyMessage (uint16 ui16PayloadLen, uint8 ui8Type, uint8 ui8Code, uint32 ui32RoH,
                          uint32 ui32LocalIP, uint32 ui32RemoteIP, uint8 ui8PacketTTL, bool bReachability);

        size_t getMessageHeaderSize (void) const;

        const uint8 _ui8Type;
        const uint8 _ui8Code;
        const uint32 _ui32RoH;
        const uint32 _ui32LocalIP;
        const uint32 _ui32RemoteIP;
        const uint8 _ui8PacketTTL;

        #pragma warning (disable:4200)
        uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct UDPUnicastDataProxyMessage : public ProxyDataMessage
    {
        UDPUnicastDataProxyMessage (uint16 ui16PayloadLen, uint32 ui32LocalIP, uint32 ui32RemoteIP,
                                    uint8 ui8PacketTTL, const CompressionSettings & compressionSetting);

        size_t getMessageHeaderSize (void) const;

        const uint32 _ui32LocalIP;
        const uint32 _ui32RemoteIP;
        const uint8 _ui8PacketTTL;
        uint8 _ui8CompressionTypeAndLevel;

        #pragma warning (disable:4200)
        uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct MultipleUDPDatagramsProxyMessage : public ProxyDataMessage
    {
        MultipleUDPDatagramsProxyMessage (uint16 ui16PayloadLen, uint32 ui32LocalIP, uint32 ui32RemoteIP,
                                          uint8 ui8WrappedUDPDatagramsNum, const CompressionSettings & compressionSetting);

        size_t getMessageHeaderSize (void) const;

        const uint32 _ui32LocalIP;
        const uint32 _ui32RemoteIP;
        const uint8 _ui8WrappedUDPDatagramsNum;
        uint8 _ui8CompressionTypeAndLevel;

        #pragma warning (disable:4200)
        uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct UDPBCastMCastDataProxyMessage : public ProxyDataMessage
    {
        UDPBCastMCastDataProxyMessage (uint16 ui16PayloadLen, const CompressionSettings & compressionSetting);

        size_t getMessageHeaderSize (void) const;

        uint8 _ui8CompressionTypeAndLevel;

        #pragma warning (disable:4200)
        uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct OpenTCPConnectionProxyMessage : public ProxyMessage
    {
        OpenTCPConnectionProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16LocalID, uint32 ui32LocalIP, uint16 ui16LocalPort, uint32 ui32RemoteIP,
                                       uint16 ui16RemotePort, const CompressionSettings & compressionSetting, bool bReachability);

        size_t getMessageHeaderSize (void) const;

        const uint32 _ui32ProxyUniqueID;
        const uint16 _ui16LocalID;
        const uint32 _ui32LocalIP;
        const uint16 _ui16LocalPort;
        const uint32 _ui32RemoteIP;
        const uint16 _ui16RemotePort;
        uint8 _ui8CompressionTypeAndLevel;
    };


    struct TCPConnectionOpenedProxyMessage : public ProxyMessage
    {
        TCPConnectionOpenedProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16LocalID, uint16 ui16RemoteID, uint32 ui32LocalIP,
                                         const CompressionSettings & compressionSetting, bool bReachability);

        size_t getMessageHeaderSize (void) const;

        const uint32 _ui32ProxyUniqueID;
        const uint16 _ui16LocalID;
        const uint16 _ui16RemoteID;
        const uint32 _ui32LocalIP;
        uint8 _ui8CompressionTypeAndLevel;
    };


    struct TCPDataProxyMessage : public ProxyDataMessage
    {
        TCPDataProxyMessage (uint16 ui16PayloadLen, uint16 ui16LocalID, uint16 ui16RemoteID, uint8 ui8TCPFlags = 0);

        size_t getMessageHeaderSize (void) const;

        const uint16 _ui16LocalID;
        const uint16 _ui16RemoteID;
        const uint8 _ui8TCPFlags;

        #pragma warning (disable:4200)
            uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct CloseTCPConnectionProxyMessage : public ProxyMessage
    {
        CloseTCPConnectionProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID);

        size_t getMessageHeaderSize (void) const;

        const uint16 _ui16LocalID;
        const uint16 _ui16RemoteID;
    };


    struct ResetTCPConnectionProxyMessage : public ProxyMessage
    {
        ResetTCPConnectionProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID);

        size_t getMessageHeaderSize (void) const;

        const uint16 _ui16LocalID;
        const uint16 _ui16RemoteID;
    };


    struct TunnelPacketProxyMessage : public ProxyDataMessage
    {
        TunnelPacketProxyMessage (uint16 ui16PayloadLen);

        size_t getMessageHeaderSize (void) const;

        #pragma warning (disable:4200)
        uint8 _aui8Data[];
        #pragma warning (default:4200)
    };


    struct ConnectionErrorProxyMessage : public ProxyMessage
    {
        ConnectionErrorProxyMessage (uint32 ui32LocalProxyUniqueID, uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                     uint32 ui32RemoteInterfaceIPv4Address, ConnectorType ct);

        size_t getMessageHeaderSize (void) const;

        const uint32 _ui32LocalProxyUniqueID;
        const uint32 _ui32RemoteProxyUniqueID;
        const uint32 _ui32LocalInterfaceIPv4Address;
        const uint32 _ui32RemoteInterfaceIPv4Address;
        const uint8 _ui8ConnectorType;
    };


    #pragma pack (pop)

    inline ProxyMessage::ProxyMessage (uint8 ui8MsgTypeAndReachability) :
        _ui8MsgTypeAndReachability{ui8MsgTypeAndReachability}
    { }

    inline PacketType ProxyMessage::getMessageType (void) const
    {
        return ProxyMessage::getMessageType (this);
    }

    inline size_t ProxyMessage::getMessageHeaderSize (void) const
    {
        return ProxyMessage::getMessageHeaderSize (this);
    }

    inline bool ProxyMessage::getRemoteProxyReachability (void) const
    {
        return ProxyMessage::getRemoteProxyReachability (this);
    }

    inline ProtocolType ProxyMessage::getProtocolType (void) const
    {
        return ProxyMessage::getProtocolType (this);
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

    inline PacketType ProxyMessage::getMessageType (const ProxyMessage * const pProxyMessage)
    {
        return static_cast<PacketType> (pProxyMessage->_ui8MsgTypeAndReachability & PACKET_TYPE_FLAGS_MASK);
    }

    inline bool ProxyMessage::getRemoteProxyReachability (const ProxyMessage * const pProxyMessage)
    {
        return (pProxyMessage->_ui8MsgTypeAndReachability & HOST_REACHABILITY_FLAG_MASK) ==
            static_cast<uint8> (HostReachability::PMHR_ReachableHost);
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

    inline Protocol ProxyMessage::getProxyMessageProtocolFromConnectorType (const ConnectorType ct)
    {
        switch (ct) {
        case ConnectorType::CT_TCPSOCKET:
            return Protocol::PMP_TCP;
        case ConnectorType::CT_UDPSOCKET:
            return Protocol::PMP_UDP;
        case ConnectorType::CT_MOCKETS:
            return Protocol::PMP_UNDEF_MOCKETS;
        case ConnectorType::CT_CSR:
            return Protocol::PMP_UNDEF_CSR;
        }

        return Protocol::PMP_UNDEF_PROTOCOL;
    }

    inline bool ProxyMessage::isICMPTypePacket (const ProxyMessage * const pProxyMessage)
    {
        switch (pProxyMessage->getMessageType()) {
        case (PacketType::PMT_ICMPMessage):
            return true;
        default:
            break;
        }

        return false;
    }

    inline bool ProxyMessage::isUDPTypePacket (const ProxyMessage * const pProxyMessage)
    {
        switch (pProxyMessage->getMessageType()) {
        case PacketType::PMT_UDPUnicastData:
        case PacketType::PMT_MultipleUDPDatagrams:
        case PacketType::PMT_UDPBCastMCastData:
            return true;
        default:
            break;
        }

        return false;
    }

    inline bool ProxyMessage::isTCPTypePacket (const ProxyMessage * const pProxyMessage)
    {
        switch (pProxyMessage->getMessageType()) {
        case PacketType::PMT_TCPOpenConnection:
        case PacketType::PMT_TCPConnectionOpened:
        case PacketType::PMT_TCPData:
        case PacketType::PMT_TCPCloseConnection:
        case PacketType::PMT_TCPResetConnection:
            return true;
        default:
            break;
        }

        return false;
    }

    inline ProxyDataMessage::ProxyDataMessage (uint8 ui8MsgTypeAndReachability, uint16 ui16PayloadLen) :
        ProxyMessage{ui8MsgTypeAndReachability}, _ui16PayloadLen{ui16PayloadLen}
    { }

    inline uint16 ProxyDataMessage::getPayloadLen (void) const
    {
        return _ui16PayloadLen;
    }

    inline size_t InitializeConnectionProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(InitializeConnectionProxyMessage);
    }

    inline size_t ConnectionInitializedProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(ConnectionInitializedProxyMessage);
    }

    inline size_t ICMPProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(ICMPProxyMessage);
    }

    inline size_t UDPUnicastDataProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(UDPUnicastDataProxyMessage);
    }

    inline size_t MultipleUDPDatagramsProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(MultipleUDPDatagramsProxyMessage);
    }

    inline size_t UDPBCastMCastDataProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(UDPBCastMCastDataProxyMessage);
    }

    inline size_t OpenTCPConnectionProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(OpenTCPConnectionProxyMessage);
    }

    inline size_t TCPConnectionOpenedProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(TCPConnectionOpenedProxyMessage);
    }

    inline size_t TCPDataProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(TCPDataProxyMessage);
    }

    inline size_t CloseTCPConnectionProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(CloseTCPConnectionProxyMessage);
    }

    inline size_t ResetTCPConnectionProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(ResetTCPConnectionProxyMessage);
    }

    inline size_t TunnelPacketProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(TunnelPacketProxyMessage);
    }

    inline size_t ConnectionErrorProxyMessage::getMessageHeaderSize (void) const
    {
        return sizeof(ConnectionErrorProxyMessage);
    }

    inline bool isProtocolReliable (Protocol protocol)
    {
        return (protocol == Protocol::PMP_MocketsRS) || (protocol == Protocol::PMP_MocketsRU) ||
               (protocol == Protocol::PMP_UNDEF_MOCKETS) || (protocol == Protocol::PMP_CSRRS) ||
               (protocol == Protocol::PMP_CSRRU) || (protocol == Protocol::PMP_UNDEF_CSR) ||
               (protocol == Protocol::PMP_TCP);
    }

    inline bool isProtocolSequenced (Protocol protocol)
    {
        return (protocol == Protocol::PMP_MocketsRS) || (protocol == Protocol::PMP_MocketsUS) ||
               (protocol == Protocol::PMP_UNDEF_MOCKETS) || (protocol == Protocol::PMP_CSRRS) ||
               (protocol == Protocol::PMP_CSRUS) || (protocol == Protocol::PMP_UNDEF_CSR) ||
               (protocol == Protocol::PMP_TCP);
    }

}

#endif   // #ifndef INCL_PROXY_MESSAGES_H
