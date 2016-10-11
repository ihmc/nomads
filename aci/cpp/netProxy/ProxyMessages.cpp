/*
 * ProxyMessages.cpp
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

#include "ProxyMessages.h"
#include "CompressionSetting.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    uint32 ProxyMessage::getMessageHeaderSize (const ProxyMessage * const pProxyMessage)
    {
        if (!pProxyMessage) {
            return 0;
        }

        switch (pProxyMessage->getMessageType()) {
            case ProxyMessage::PMT_InitializeConnection:
                return sizeof (InitializeConnectionProxyMessage);
            case ProxyMessage::PMT_ConnectionInitialized:
                return sizeof (ConnectionInitializedProxyMessage);
            case ProxyMessage::PMT_ICMPMessage:
                return sizeof (ICMPProxyMessage);
            case ProxyMessage::PMT_UDPUnicastData:
                return sizeof (UDPUnicastDataProxyMessage);
            case ProxyMessage::PMT_MultipleUDPDatagrams:
                return sizeof (MultipleUDPDatagramsProxyMessage);
            case ProxyMessage::PMT_UDPBCastMCastData:
                return sizeof (UDPBCastMCastDataProxyMessage);
            case ProxyMessage::PMT_TCPOpenConnection:
                return sizeof (OpenConnectionProxyMessage);
            case ProxyMessage::PMT_TCPConnectionOpened:
                return sizeof (ConnectionOpenedProxyMessage);
            case ProxyMessage::PMT_TCPData:
                return sizeof (TCPDataProxyMessage);
            case ProxyMessage::PMT_TCPCloseConnection:
                return sizeof (CloseConnectionProxyMessage);
            case ProxyMessage::PMT_TCPResetConnection:
                return sizeof (ResetConnectionProxyMessage);
        }

        return 0;
    }

    bool ProxyMessage::belongsToVirtualConnection (const ProxyMessage * const pProxyMessage, uint16 ui16LocalID, uint16 ui16RemoteID)
    {
        switch (pProxyMessage->_ui8MsgTypeAndReachability) {
            case (ProxyMessage::PMT_TCPOpenConnection):
            {
                const OpenConnectionProxyMessage * const pOpenConnectionProxyMessage = (const OpenConnectionProxyMessage * const) (pProxyMessage);
                return (pOpenConnectionProxyMessage->_ui16LocalID == ui16LocalID);
            }
            case (ProxyMessage::PMT_TCPConnectionOpened):
            {
                const ConnectionOpenedProxyMessage * const pConnectionOpenedProxyMessage = (const ConnectionOpenedProxyMessage * const) (pProxyMessage);
                return (pConnectionOpenedProxyMessage->_ui16LocalID == ui16LocalID) && (pConnectionOpenedProxyMessage->_ui16RemoteID == ui16RemoteID);
            }
            case (ProxyMessage::PMT_TCPData):
            {
                const TCPDataProxyMessage * const pTCPDataProxyMessage = (const TCPDataProxyMessage * const) (pProxyMessage);
                return (pTCPDataProxyMessage->_ui16LocalID == ui16LocalID) && (pTCPDataProxyMessage->_ui16RemoteID == ui16RemoteID);
            }
            case (ProxyMessage::PMT_TCPCloseConnection):
            {
                const CloseConnectionProxyMessage * const pCloseConnectionProxyMessage = (const CloseConnectionProxyMessage * const) (pProxyMessage);
                return (pCloseConnectionProxyMessage->_ui16LocalID == ui16LocalID) && (pCloseConnectionProxyMessage->_ui16RemoteID == ui16RemoteID);
            }
            case (ProxyMessage::PMT_TCPResetConnection):
            {
                const ResetConnectionProxyMessage * const pResetConnectionProxyMessage = (const ResetConnectionProxyMessage * const) (pProxyMessage);
                return (pResetConnectionProxyMessage->_ui16LocalID == ui16LocalID) && (pResetConnectionProxyMessage->_ui16RemoteID == ui16RemoteID);
            }
        }

        return false;
    }

    InitializeConnectionProxyMessage::InitializeConnectionProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                                                        uint16 ui16LocalUDPServerPort, bool bReachability) :
        ProxyMessage (PMT_InitializeConnection | (bReachability ? PMHR_ReachableHost : PMHR_UnreachableHost)), _ui32ProxyUniqueID (ui32ProxyUniqueID),
        _ui16LocalMocketsServerPort (ui16LocalMocketsServerPort), _ui16LocalTCPServerPort (ui16LocalTCPServerPort), _ui16LocalUDPServerPort (ui16LocalUDPServerPort) {}

    ConnectionInitializedProxyMessage::ConnectionInitializedProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                                                          uint16 ui16LocalUDPServerPort, bool bReachability) :
        ProxyMessage (PMT_ConnectionInitialized | (bReachability ? PMHR_ReachableHost : PMHR_UnreachableHost)), _ui32ProxyUniqueID (ui32ProxyUniqueID),
        _ui16LocalMocketsServerPort (ui16LocalMocketsServerPort), _ui16LocalTCPServerPort (ui16LocalTCPServerPort), _ui16LocalUDPServerPort (ui16LocalUDPServerPort) {}

    ICMPProxyMessage::ICMPProxyMessage (uint16 ui16PayloadLen, uint8 ui8Type, uint8 ui8Code, uint32 ui32RoH, uint32 ui32LocalIP, uint32 ui32RemoteIP,
                                        uint32 ui32ProxyUniqueID, uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                        uint16 ui16LocalUDPServerPort, uint8 ui8PacketTTL, bool bReachability) :
        ProxyDataMessage (PMT_ICMPMessage | (bReachability ? PMHR_ReachableHost : PMHR_UnreachableHost), ui16PayloadLen), _ui8Type (ui8Type), _ui8Code (ui8Code), _ui32RoH (ui32RoH),
        _ui32LocalIP (ui32LocalIP), _ui32RemoteIP (ui32RemoteIP), _ui32ProxyUniqueID (ui32ProxyUniqueID), _ui16LocalMocketsServerPort (ui16LocalMocketsServerPort),
        _ui16LocalTCPServerPort (ui16LocalTCPServerPort), _ui16LocalUDPServerPort (ui16LocalUDPServerPort), _ui8PacketTTL(ui8PacketTTL) {}

    UDPUnicastDataProxyMessage::UDPUnicastDataProxyMessage (uint16 ui16PayloadLen, uint32 ui32LocalIP, uint32 ui32RemoteIP, uint32 ui32ProxyUniqueID, uint8 ui8PacketTTL,
                                                            const CompressionSetting * const pCompressionSetting) :
        ProxyDataMessage (PMT_UDPUnicastData, ui16PayloadLen), _ui32LocalIP (ui32LocalIP), _ui32RemoteIP (ui32RemoteIP), _ui32ProxyUniqueID (ui32ProxyUniqueID),
        _ui8PacketTTL(ui8PacketTTL), _ui8CompressionTypeAndLevel (pCompressionSetting->getCompressionLevel() | pCompressionSetting->getCompressionType()) {}

    MultipleUDPDatagramsProxyMessage::MultipleUDPDatagramsProxyMessage (uint16 ui16PayloadLen, uint32 ui32LocalIP, uint32 ui32RemoteIP, uint32 ui32ProxyUniqueID,
                                                                        uint8 ui8WrappedUDPDatagramsNum, const CompressionSetting * const pCompressionSetting) :
        ProxyDataMessage (PMT_MultipleUDPDatagrams, ui16PayloadLen), _ui32LocalIP (ui32LocalIP), _ui32RemoteIP (ui32RemoteIP), _ui32ProxyUniqueID (ui32ProxyUniqueID),
        _ui8WrappedUDPDatagramsNum (ui8WrappedUDPDatagramsNum), _ui8CompressionTypeAndLevel (pCompressionSetting->getCompressionLevel() | pCompressionSetting->getCompressionType()) {}

    UDPBCastMCastDataProxyMessage::UDPBCastMCastDataProxyMessage (uint16 ui16PayloadLen, uint32 ui32ProxyUniqueID, const CompressionSetting * const pCompressionSetting) :
        ProxyDataMessage (PMT_UDPBCastMCastData, ui16PayloadLen), _ui32ProxyUniqueID(ui32ProxyUniqueID),
        _ui8CompressionTypeAndLevel (pCompressionSetting->getCompressionLevel() | pCompressionSetting->getCompressionType()) {}

    OpenConnectionProxyMessage::OpenConnectionProxyMessage (uint16 ui16LocalID, uint32 ui32LocalIP, uint16 ui16LocalPort, uint32 ui32RemoteIP, uint16 ui16RemotePort,
                                                            uint32 ui32ProxyUniqueID, uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                                            uint16 ui16LocalUDPServerPort, const CompressionSetting * const pCompressionSetting, bool bReachability) :
        ProxyMessage (PMT_TCPOpenConnection | (bReachability ? PMHR_ReachableHost : PMHR_UnreachableHost)), _ui16LocalID (ui16LocalID), _ui32LocalIP (ui32LocalIP),
        _ui16LocalPort (ui16LocalPort), _ui32RemoteIP (ui32RemoteIP), _ui16RemotePort (ui16RemotePort), _ui32ProxyUniqueID (ui32ProxyUniqueID),
        _ui16LocalMocketsServerPort (ui16LocalMocketsServerPort), _ui16LocalTCPServerPort (ui16LocalTCPServerPort), _ui16LocalUDPServerPort (ui16LocalUDPServerPort),
        _ui8CompressionTypeAndLevel(pCompressionSetting->getCompressionLevel() | pCompressionSetting->getCompressionType()) {}

    ConnectionOpenedProxyMessage::ConnectionOpenedProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID, uint32 ui32LocalIP, uint32 ui32ProxyUniqueID,
                                                                uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort, uint16 ui16LocalUDPServerPort,
                                                                const CompressionSetting * const pCompressionSetting, bool bReachability) :
        ProxyMessage (PMT_TCPConnectionOpened | (bReachability ? PMHR_ReachableHost : PMHR_UnreachableHost)), _ui16LocalID (ui16LocalID), _ui16RemoteID (ui16RemoteID),
        _ui32LocalIP (ui32LocalIP), _ui32ProxyUniqueID (ui32ProxyUniqueID), _ui16LocalMocketsServerPort (ui16LocalMocketsServerPort), _ui16LocalTCPServerPort (ui16LocalTCPServerPort),
        _ui16LocalUDPServerPort (ui16LocalUDPServerPort), _ui8CompressionTypeAndLevel (pCompressionSetting->getCompressionLevel() | pCompressionSetting->getCompressionType()) {}

    TCPDataProxyMessage::TCPDataProxyMessage (uint16 ui16PayloadLen, uint16 ui16LocalID, uint16 ui16RemoteID, uint8 ui8TCPFlags) :
        ProxyDataMessage (PMT_TCPData, ui16PayloadLen), _ui16LocalID (ui16LocalID), _ui16RemoteID (ui16RemoteID), _ui8TCPFlags (ui8TCPFlags) {}

    CloseConnectionProxyMessage::CloseConnectionProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID) :
        ProxyMessage(PMT_TCPCloseConnection), _ui16LocalID (ui16LocalID), _ui16RemoteID (ui16RemoteID) {}

    ResetConnectionProxyMessage::ResetConnectionProxyMessage(uint16 ui16LocalID, uint16 ui16RemoteID) :
    ProxyMessage (PMT_TCPResetConnection), _ui16LocalID (ui16LocalID), _ui16RemoteID (ui16RemoteID) {}
}
