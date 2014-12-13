/*
 * ProxyMessages.cpp
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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
        switch (pProxyMessage->ui8MsgTypeAndReachability) {
            case (ProxyMessage::PMT_TCPOpenConnection):
            {
                const OpenConnectionProxyMessage * const pOpenConnectionProxyMessage = (const OpenConnectionProxyMessage * const) (pProxyMessage);
                return (pOpenConnectionProxyMessage->ui16LocalID == ui16LocalID);
            }
            case (ProxyMessage::PMT_TCPConnectionOpened):
            {
                const ConnectionOpenedProxyMessage * const pConnectionOpenedProxyMessage = (const ConnectionOpenedProxyMessage * const) (pProxyMessage);
                return (pConnectionOpenedProxyMessage->ui16LocalID == ui16LocalID) && (pConnectionOpenedProxyMessage->ui16RemoteID == ui16RemoteID);
            }
            case (ProxyMessage::PMT_TCPData):
            {
                const TCPDataProxyMessage * const pTCPDataProxyMessage = (const TCPDataProxyMessage * const) (pProxyMessage);
                return (pTCPDataProxyMessage->ui16LocalID == ui16LocalID) && (pTCPDataProxyMessage->ui16RemoteID == ui16RemoteID);
            }
            case (ProxyMessage::PMT_TCPCloseConnection):
            {
                const CloseConnectionProxyMessage * const pCloseConnectionProxyMessage = (const CloseConnectionProxyMessage * const) (pProxyMessage);
                return (pCloseConnectionProxyMessage->ui16LocalID == ui16LocalID) && (pCloseConnectionProxyMessage->ui16RemoteID == ui16RemoteID);
            }
            case (ProxyMessage::PMT_TCPResetConnection):
            {
                const ResetConnectionProxyMessage * const pResetConnectionProxyMessage = (const ResetConnectionProxyMessage * const) (pProxyMessage);
                return (pResetConnectionProxyMessage->ui16LocalID == ui16LocalID) && (pResetConnectionProxyMessage->ui16RemoteID == ui16RemoteID);
            }
        }

        return false;
    }

    InitializeConnectionProxyMessage::InitializeConnectionProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                                                        uint16 ui16LocalUDPServerPort, bool bReachability)
    {
        this->ui8MsgTypeAndReachability = PMT_InitializeConnection | (bReachability ? PMHR_ReachableHost : PMHR_UnreachableHost);
        this->ui32ProxyUniqueID = ui32ProxyUniqueID;
        this->ui16LocalMocketsServerPort = ui16LocalMocketsServerPort;
        this->ui16LocalTCPServerPort = ui16LocalTCPServerPort;
        this->ui16LocalUDPServerPort = ui16LocalUDPServerPort;
    }

    ConnectionInitializedProxyMessage::ConnectionInitializedProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                                                          uint16 ui16LocalUDPServerPort, bool bReachability)
    {
        this->ui8MsgTypeAndReachability = PMT_ConnectionInitialized | (bReachability ? PMHR_ReachableHost : PMHR_UnreachableHost);
        this->ui32ProxyUniqueID = ui32ProxyUniqueID;
        this->ui16LocalMocketsServerPort = ui16LocalMocketsServerPort;
        this->ui16LocalTCPServerPort = ui16LocalTCPServerPort;
        this->ui16LocalUDPServerPort = ui16LocalUDPServerPort;
    }

    ICMPProxyMessage::ICMPProxyMessage (uint8 iType, uint8 ui8Code, uint32 ui32RoH, uint32 ui32LocalIP, uint32 ui32RemoteIP, uint32 ui32ProxyUniqueID,
                                        uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort, uint16 ui16LocalUDPServerPort,
                                        uint16 ui16PayloadLen, bool bReachability)
    {
        this->ui8MsgTypeAndReachability = PMT_ICMPMessage | (bReachability ? PMHR_ReachableHost : PMHR_UnreachableHost);
        this->ui8Type = iType;
        this->ui8Code = ui8Code;
        this->ui32RoH = ui32RoH;
        this->ui32LocalIP = ui32LocalIP;
        this->ui32RemoteIP = ui32RemoteIP;
        this->ui32ProxyUniqueID = ui32ProxyUniqueID;
        this->ui16LocalMocketsServerPort = ui16LocalMocketsServerPort;
        this->ui16LocalTCPServerPort = ui16LocalTCPServerPort;
        this->ui16LocalUDPServerPort = ui16LocalUDPServerPort;
        this->ui16PayloadLen = ui16PayloadLen;
    }

    UDPUnicastDataProxyMessage::UDPUnicastDataProxyMessage (uint32 ui32LocalIP, uint32 ui32RemoteIP, uint32 ui32ProxyUniqueID, uint16 ui16PayloadLen,
                                                            const CompressionSetting * const pCompressionSetting)
    {
        this->ui8MsgTypeAndReachability = PMT_UDPUnicastData;
        this->ui32LocalIP = ui32LocalIP;
        this->ui32RemoteIP = ui32RemoteIP;
        this->ui32ProxyUniqueID = ui32ProxyUniqueID;
        this->ui16PayloadLen = ui16PayloadLen;
        this->ui8CompressionTypeAndLevel = pCompressionSetting->getCompressionLevel() | pCompressionSetting->getCompressionType();
    }

    MultipleUDPDatagramsProxyMessage::MultipleUDPDatagramsProxyMessage (uint32 ui32LocalIP, uint32 ui32RemoteIP, uint32 ui32ProxyUniqueID, int iWrappedUDPDatagramsNum,
                                                                        uint16 ui16PayloadLen, const CompressionSetting * const pCompressionSetting)
    {
        this->ui8MsgTypeAndReachability = PMT_MultipleUDPDatagrams;
        this->ui32LocalIP = ui32LocalIP;
        this->ui32RemoteIP = ui32RemoteIP;
        this->ui32ProxyUniqueID = ui32ProxyUniqueID;
        this->ui8WrappedUDPDatagramsNum = (uint8) iWrappedUDPDatagramsNum;
        this->ui16PayloadLen = ui16PayloadLen;
        this->ui8CompressionTypeAndLevel = pCompressionSetting->getCompressionLevel() | pCompressionSetting->getCompressionType();
    }

    UDPBCastMCastDataProxyMessage::UDPBCastMCastDataProxyMessage (uint32 ui32ProxyUniqueID, uint16 ui16PayloadLen, const CompressionSetting * const pCompressionSetting)
    {
        this->ui8MsgTypeAndReachability = PMT_UDPBCastMCastData;
        this->ui32ProxyUniqueID = ui32ProxyUniqueID;
        this->ui16PayloadLen = ui16PayloadLen;
        this->ui8CompressionTypeAndLevel = pCompressionSetting->getCompressionLevel() | pCompressionSetting->getCompressionType();
    }

    OpenConnectionProxyMessage::OpenConnectionProxyMessage (uint16 ui16LocalID, uint32 ui32LocalIP, uint16 ui16LocalPort, uint32 ui32RemoteIP, uint16 ui16RemotePort,
                                                            uint32 ui32ProxyUniqueID, uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort,
                                                            uint16 ui16LocalUDPServerPort, const CompressionSetting * const pCompressionSetting, bool bReachability)
    {
        this->ui8MsgTypeAndReachability  = PMT_TCPOpenConnection | (bReachability ? PMHR_ReachableHost : PMHR_UnreachableHost);
        this->ui16LocalID = ui16LocalID;
        this->ui32LocalIP = ui32LocalIP;
        this->ui16LocalPort = ui16LocalPort;
        this->ui32RemoteIP = ui32RemoteIP;
        this->ui16RemotePort = ui16RemotePort;
        this->ui32ProxyUniqueID = ui32ProxyUniqueID;
        this->ui16LocalMocketsServerPort = ui16LocalMocketsServerPort;
        this->ui16LocalTCPServerPort = ui16LocalTCPServerPort;
        this->ui16LocalUDPServerPort = ui16LocalUDPServerPort;
        this->ui8CompressionTypeAndLevel = pCompressionSetting->getCompressionLevel() | pCompressionSetting->getCompressionType();
    }

    ConnectionOpenedProxyMessage::ConnectionOpenedProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID, uint32 ui32LocalIP, uint32 ui32ProxyUniqueID,
                                                                uint16 ui16LocalMocketsServerPort, uint16 ui16LocalTCPServerPort, uint16 ui16LocalUDPServerPort,
                                                                const CompressionSetting * const pCompressionSetting, bool bReachability)
    {
        this->ui8MsgTypeAndReachability = PMT_TCPConnectionOpened | (bReachability ? PMHR_ReachableHost : PMHR_UnreachableHost);
        this->ui16LocalID = ui16LocalID;
        this->ui16RemoteID = ui16RemoteID;
        this->ui32LocalIP = ui32LocalIP;
        this->ui32ProxyUniqueID = ui32ProxyUniqueID;
        this->ui16LocalMocketsServerPort = ui16LocalMocketsServerPort;
        this->ui16LocalTCPServerPort = ui16LocalTCPServerPort;
        this->ui16LocalUDPServerPort = ui16LocalUDPServerPort;
        this->ui8CompressionTypeAndLevel = pCompressionSetting->getCompressionLevel() | pCompressionSetting->getCompressionType();
    }

    TCPDataProxyMessage::TCPDataProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID, uint16 ui16PayloadLen, uint8 ui8TCPFlags)
    {
        this->ui8MsgTypeAndReachability = PMT_TCPData;
        this->ui16LocalID = ui16LocalID;
        this->ui16RemoteID = ui16RemoteID;
        this->ui16PayloadLen = ui16PayloadLen;
        this->ui8TCPFlags = ui8TCPFlags;
    }

    CloseConnectionProxyMessage::CloseConnectionProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID)
    {
        this->ui8MsgTypeAndReachability = PMT_TCPCloseConnection;
        this->ui16LocalID = ui16LocalID;
        this->ui16RemoteID = ui16RemoteID;
    }

    ResetConnectionProxyMessage::ResetConnectionProxyMessage (uint16 ui16LocalID, uint16 ui16RemoteID)
    {
        this->ui8MsgTypeAndReachability = PMT_TCPResetConnection;
        this->ui16LocalID = ui16LocalID;
        this->ui16RemoteID = ui16RemoteID;
    }
}
