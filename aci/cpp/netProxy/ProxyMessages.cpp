/*
 * ProxyMessages.cpp
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

#include "ProxyMessages.h"
#include "CompressionSettings.h"


namespace ACMNetProxy
{
    size_t ProxyMessage::getMessageHeaderSize (const ProxyMessage * const pProxyMessage)
    {
        if (!pProxyMessage) {
            return 0;
        }

        switch (pProxyMessage->getMessageType()) {
        case PacketType::PMT_InitializeConnection:
            {
                auto * const pICPM = reinterpret_cast<const InitializeConnectionProxyMessage *> (pProxyMessage);
                return pICPM->getMessageHeaderSize();
            }
        case PacketType::PMT_ConnectionInitialized:
            {
                auto * const pCIPM = reinterpret_cast<const ConnectionInitializedProxyMessage *> (pProxyMessage);
                return pCIPM->getMessageHeaderSize();
            }
        case PacketType::PMT_ICMPMessage:
            {
                auto * const pICMPPM = reinterpret_cast<const ICMPProxyMessage *> (pProxyMessage);
                return pICMPPM->getMessageHeaderSize();
            }
        case PacketType::PMT_UDPUnicastData:
            {
                auto * const pUDPUDPM = reinterpret_cast<const UDPUnicastDataProxyMessage *> (pProxyMessage);
                return pUDPUDPM->getMessageHeaderSize();
            }
        case PacketType::PMT_MultipleUDPDatagrams:
            {
                auto * const pMUDPDPM = reinterpret_cast<const MultipleUDPDatagramsProxyMessage *> (pProxyMessage);
                return pMUDPDPM->getMessageHeaderSize();
            }
        case PacketType::PMT_UDPBCastMCastData:
            {
                auto * const pUDPBMDPM = reinterpret_cast<const UDPBCastMCastDataProxyMessage *> (pProxyMessage);
                return pUDPBMDPM->getMessageHeaderSize();
            }
        case PacketType::PMT_TCPOpenConnection:
            {
                auto * const pOTCPCPM = reinterpret_cast<const OpenTCPConnectionProxyMessage *> (pProxyMessage);
                return pOTCPCPM->getMessageHeaderSize();
            }
        case PacketType::PMT_TCPConnectionOpened:
            {
                auto * const pTCPCOPM = reinterpret_cast<const TCPConnectionOpenedProxyMessage *> (pProxyMessage);
                return pTCPCOPM->getMessageHeaderSize();
            }
        case PacketType::PMT_TCPData:
            {
                auto * const pTCPDPM = reinterpret_cast<const TCPDataProxyMessage *> (pProxyMessage);
                return pTCPDPM->getMessageHeaderSize();
            }
        case PacketType::PMT_TCPCloseConnection:
            {
                auto * const pCTCPCPM = reinterpret_cast<const CloseTCPConnectionProxyMessage *> (pProxyMessage);
                return pCTCPCPM->getMessageHeaderSize();
            }
        case PacketType::PMT_TCPResetConnection:
            {
                auto * const pRTCPCPM = reinterpret_cast<const ResetTCPConnectionProxyMessage *> (pProxyMessage);
                return pRTCPCPM->getMessageHeaderSize();
            }
        case PacketType::PMT_TunnelPacket:
            {
                auto * const pTPPM = reinterpret_cast<const TunnelPacketProxyMessage *> (pProxyMessage);
                return pTPPM->getMessageHeaderSize();
            }
        case PacketType::PMT_ConnectionError:
            {
                auto * const pCEPM = reinterpret_cast<const ConnectionErrorProxyMessage *> (pProxyMessage);
                return pCEPM->getMessageHeaderSize();
            }
        }

        return 0;
    }

    bool ProxyMessage::belongsToVirtualConnection (const ProxyMessage * const pProxyMessage, uint16 ui16LocalID, uint16 ui16RemoteID)
    {
        switch (static_cast<PacketType> (pProxyMessage->_ui8MsgTypeAndReachability & PACKET_TYPE_FLAGS_MASK)) {
        case PacketType::PMT_TCPOpenConnection:
            {
                const auto pOpenConnectionProxyMessage = static_cast<const OpenTCPConnectionProxyMessage * const> (pProxyMessage);
                return (pOpenConnectionProxyMessage->_ui16LocalID == ui16LocalID);
            }
        case PacketType::PMT_TCPConnectionOpened:
            {
                const auto pConnectionOpenedProxyMessage = static_cast<const TCPConnectionOpenedProxyMessage * const> (pProxyMessage);
                return (pConnectionOpenedProxyMessage->_ui16LocalID == ui16LocalID) && (pConnectionOpenedProxyMessage->_ui16RemoteID == ui16RemoteID);
            }
        case PacketType::PMT_TCPData:
            {
                const auto pTCPDataProxyMessage = static_cast<const TCPDataProxyMessage * const> (pProxyMessage);
                return (pTCPDataProxyMessage->_ui16LocalID == ui16LocalID) && (pTCPDataProxyMessage->_ui16RemoteID == ui16RemoteID);
            }
        case PacketType::PMT_TCPCloseConnection:
            {
                const auto pCloseConnectionProxyMessage = static_cast<const CloseTCPConnectionProxyMessage * const> (pProxyMessage);
                return (pCloseConnectionProxyMessage->_ui16LocalID == ui16LocalID) && (pCloseConnectionProxyMessage->_ui16RemoteID == ui16RemoteID);
            }
        case PacketType::PMT_TCPResetConnection:
            {
                const auto pResetConnectionProxyMessage = static_cast<const ResetTCPConnectionProxyMessage * const> (pProxyMessage);
                return (pResetConnectionProxyMessage->_ui16LocalID == ui16LocalID) && (pResetConnectionProxyMessage->_ui16RemoteID == ui16RemoteID);
            }
        }

        return false;
    }

    InitializeConnectionProxyMessage::InitializeConnectionProxyMessage (uint32 ui32ProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                        uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                                                        uint16 ui16LocalUDPServerPort, uint8 ui8NumberOfInterfaces,
                                                                        bool bReachability) :
        ProxyDataMessage{static_cast<uint8> (static_cast<int> (PacketType::PMT_InitializeConnection) |
                                             static_cast<int> (bReachability ? HostReachability::PMHR_ReachableHost :
                                                                               HostReachability::PMHR_UnreachableHost)),
                         ui8NumberOfInterfaces * sizeof(uint32)},
        _ui32ProxyUniqueID{ui32ProxyUniqueID}, _ui32LocalInterfaceIPv4Address{ui32LocalInterfaceIPv4Address},
        _ui16LocalMocketsServerPort{ui16LocalMocketsServerPort}, _ui16LocalTCPServerPort{ui16LocalTCPServerPort},
        _ui16LocalUDPServerPort{ui16LocalUDPServerPort}, _ui8NumberOfInterfaces{ui8NumberOfInterfaces}
    { }

    ConnectionInitializedProxyMessage::ConnectionInitializedProxyMessage (uint32 ui32ProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                          uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                                                          uint16 ui16LocalUDPServerPort, uint8 ui8NumberOfInterfaces,
                                                                          bool bReachability) :
        ProxyDataMessage{static_cast<uint8> (static_cast<int> (PacketType::PMT_ConnectionInitialized) |
                                         static_cast<int> (bReachability ? HostReachability::PMHR_ReachableHost :
                                                                           HostReachability::PMHR_UnreachableHost)),
                         ui8NumberOfInterfaces * sizeof(uint32)},
        _ui32ProxyUniqueID{ui32ProxyUniqueID}, _ui32LocalInterfaceIPv4Address{ui32LocalInterfaceIPv4Address},
        _ui16LocalMocketsServerPort{ui16LocalMocketsServerPort}, _ui16LocalTCPServerPort{ui16LocalTCPServerPort},
        _ui16LocalUDPServerPort{ui16LocalUDPServerPort}, _ui8NumberOfInterfaces{ui8NumberOfInterfaces}
    { }

    ICMPProxyMessage::ICMPProxyMessage (uint16 ui16PayloadLen, uint8 ui8Type, uint8 ui8Code, uint32 ui32RoH, uint32 ui32LocalIP,
                                        uint32 ui32RemoteIP, uint8 ui8PacketTTL, bool bReachability) :
        ProxyDataMessage{static_cast<uint8> (static_cast<int> (PacketType::PMT_ICMPMessage) |
                                             static_cast<int> (bReachability ? HostReachability::PMHR_ReachableHost :
                                                                               HostReachability::PMHR_UnreachableHost)),
                         ui16PayloadLen}, _ui8Type{ui8Type}, _ui8Code{ui8Code}, _ui32RoH{ui32RoH},
        _ui32LocalIP{ui32LocalIP}, _ui32RemoteIP{ui32RemoteIP}, _ui8PacketTTL{ui8PacketTTL}
    { }

    UDPUnicastDataProxyMessage::UDPUnicastDataProxyMessage (uint16 ui16PayloadLen, uint32 ui32LocalIP, uint32 ui32RemoteIP,
                                                            uint8 ui8PacketTTL, const CompressionSettings & compressionSetting) :
        ProxyDataMessage{static_cast<uint8> (PacketType::PMT_UDPUnicastData), ui16PayloadLen},
        _ui32LocalIP{ui32LocalIP}, _ui32RemoteIP{ui32RemoteIP}, _ui8PacketTTL{ui8PacketTTL},
        _ui8CompressionTypeAndLevel {static_cast<uint8> (compressionSetting.getCompressionLevel() |
                                                         static_cast<int> (compressionSetting.getCompressionType()))}
    { }

    MultipleUDPDatagramsProxyMessage::MultipleUDPDatagramsProxyMessage (uint16 ui16PayloadLen, uint32 ui32LocalIP, uint32 ui32RemoteIP,
                                                                        uint8 ui8WrappedUDPDatagramsNum, const CompressionSettings & compressionSetting) :
        ProxyDataMessage{static_cast<uint8> (PacketType::PMT_MultipleUDPDatagrams), ui16PayloadLen},
        _ui32LocalIP{ui32LocalIP}, _ui32RemoteIP{ui32RemoteIP}, _ui8WrappedUDPDatagramsNum{ui8WrappedUDPDatagramsNum},
        _ui8CompressionTypeAndLevel{static_cast<uint8> (compressionSetting.getCompressionLevel() |
                                                        static_cast<int> (compressionSetting.getCompressionType()))}
    { }

    UDPBCastMCastDataProxyMessage::UDPBCastMCastDataProxyMessage (uint16 ui16PayloadLen, const CompressionSettings & compressionSetting) :
        ProxyDataMessage{static_cast<uint8> (PacketType::PMT_UDPBCastMCastData), ui16PayloadLen},
        _ui8CompressionTypeAndLevel{static_cast<uint8> (compressionSetting.getCompressionLevel() |
                                                        static_cast<int> (compressionSetting.getCompressionType()))}
    { }

    OpenTCPConnectionProxyMessage::OpenTCPConnectionProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16LocalID, uint32 ui32LocalIP,
                                                                  uint16 ui16LocalPort, uint32 ui32RemoteIP, uint16 ui16RemotePort,
                                                                  const CompressionSettings & compressionSetting, bool bReachability) :
        ProxyMessage{static_cast<uint8> (static_cast<int> (PacketType::PMT_TCPOpenConnection) |
                                         static_cast<int> (bReachability ? HostReachability::PMHR_ReachableHost :
                                                                           HostReachability::PMHR_UnreachableHost))},
        _ui32ProxyUniqueID{ui32ProxyUniqueID}, _ui16LocalID{ui16LocalID}, _ui32LocalIP{ui32LocalIP}, _ui16LocalPort{ui16LocalPort},
        _ui32RemoteIP{ui32RemoteIP}, _ui16RemotePort{ui16RemotePort},
        _ui8CompressionTypeAndLevel{static_cast<uint8> (compressionSetting.getCompressionLevel() |
                                                        static_cast<int> (compressionSetting.getCompressionType()))}
    { }

    TCPConnectionOpenedProxyMessage::TCPConnectionOpenedProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16LocalID, uint16 ui16RemoteID,
                                                                      uint32 ui32LocalIP, const CompressionSettings & compressionSetting,
                                                                      bool bReachability) :
        ProxyMessage{static_cast<uint8> (static_cast<int> (PacketType::PMT_TCPConnectionOpened) |
                                         static_cast<int> (bReachability ? HostReachability::PMHR_ReachableHost :
                                                                           HostReachability::PMHR_UnreachableHost))},
        _ui32ProxyUniqueID{ui32ProxyUniqueID}, _ui16LocalID{ui16LocalID}, _ui16RemoteID{ui16RemoteID}, _ui32LocalIP{ui32LocalIP},
        _ui8CompressionTypeAndLevel{static_cast<uint8> (compressionSetting.getCompressionLevel() |
                                                        static_cast<int> (compressionSetting.getCompressionType()))}
    { }

    TCPDataProxyMessage::TCPDataProxyMessage (uint16 ui16PayloadLen, uint16 ui16LocalID, uint16 ui16RemoteID, uint8 ui8TCPFlags) :
        ProxyDataMessage{static_cast<uint8> (PacketType::PMT_TCPData), ui16PayloadLen}, _ui16LocalID{ui16LocalID},
        _ui16RemoteID{ui16RemoteID}, _ui8TCPFlags{ui8TCPFlags}
    { }

    CloseTCPConnectionProxyMessage::CloseTCPConnectionProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID) :
        ProxyMessage{static_cast<uint8> (PacketType::PMT_TCPCloseConnection)}, _ui16LocalID{ui16LocalID},
        _ui16RemoteID{ui16RemoteID}
    { }

    ResetTCPConnectionProxyMessage::ResetTCPConnectionProxyMessage(uint16 ui16LocalID, uint16 ui16RemoteID) :
        ProxyMessage{static_cast<uint8> (PacketType::PMT_TCPResetConnection)}, _ui16LocalID{ui16LocalID},
        _ui16RemoteID{ui16RemoteID}
    { }

    TunnelPacketProxyMessage::TunnelPacketProxyMessage (uint16 ui16PayloadLen) :
        ProxyDataMessage{static_cast<uint8> (PacketType::PMT_TunnelPacket), ui16PayloadLen}
    { }

    ConnectionErrorProxyMessage::ConnectionErrorProxyMessage (uint32 ui32LocalProxyUniqueID, uint32 ui32RemoteProxyUniqueID,
                                                              uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address,
                                                              ConnectorType ct) :
        ProxyMessage{static_cast<uint8> (PacketType::PMT_ConnectionError)}, _ui32LocalProxyUniqueID{ui32LocalProxyUniqueID},
        _ui32RemoteProxyUniqueID{ui32RemoteProxyUniqueID}, _ui32LocalInterfaceIPv4Address{ui32LocalInterfaceIPv4Address},
        _ui32RemoteInterfaceIPv4Address{ui32RemoteInterfaceIPv4Address}, _ui8ConnectorType{static_cast<uint8> (ct)}
    { }
}
