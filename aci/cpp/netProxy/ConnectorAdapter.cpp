/*
 * ConnectorAdapter.cpp
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

#include "ConnectorAdapter.h"

#include "CSRAdapter.h"
#include "MocketAdapter.h"
#include "TCPSocketAdapter.h"
#include "UDPSocketAdapter.h"
#include "UDPConnector.h"
#include "ConnectionManager.h"


namespace ACMNetProxy
{
    ConnectorAdapter * const ConnectorAdapter::ConnectorAdapterFactory (ConnectionManager & rConnectionManager, ConnectorType connectorType,
                                                                        EncryptionType encryptionType, const NOMADSUtil::InetAddr & iaLocalProxyAddr,
                                                                        const NOMADSUtil::InetAddr & iaRemoteProxyAddr)
    {
        switch (connectorType) {
        case CT_TCPSOCKET:
            return new TCPSocketAdapter{iaLocalProxyAddr, iaRemoteProxyAddr, encryptionType};
        case CT_UDPSOCKET:
        {
            auto * const pConnector = rConnectionManager.getConnectorBoundToAddressForType (iaLocalProxyAddr.getIPAddress(), CT_UDPSOCKET);
            if (pConnector == nullptr) {
                return nullptr;
            }

            auto * const pUDPConnector = dynamic_cast<UDPConnector *> (pConnector);
            return pUDPConnector ? pUDPConnector->getUDPSocketAdapter() : nullptr;
        }
        case CT_MOCKETS:
            return new MocketAdapter{iaLocalProxyAddr, iaRemoteProxyAddr, encryptionType};
        case CT_CSR:
            return new CSRAdapter{iaLocalProxyAddr, iaRemoteProxyAddr, encryptionType};
        case CT_UNDEF:
            return nullptr;
        }

        return nullptr;
    }

    ConnectorAdapter * const ConnectorAdapter::ConnectorAdapterFactory (Mocket * const pMocket, const NOMADSUtil::InetAddr & iaLocalProxyAddr,
                                                                        ConnectorType connectorType)
    {
        switch (connectorType) {
        case CT_MOCKETS:
            return new MocketAdapter{pMocket, iaLocalProxyAddr};
        case CT_CSR:
            return new CSRAdapter{pMocket, iaLocalProxyAddr};
        default:
            break;
        }

        return nullptr;
    }

    ConnectorAdapter * const ConnectorAdapter::ConnectorAdapterFactory (NOMADSUtil::TCPSocket * const pTCPSocket, const NOMADSUtil::InetAddr & iaLocalProxyAddr,
                                                                        ConnectorType connectorType)
    {
        (void) connectorType;
        return new TCPSocketAdapter{pTCPSocket, iaLocalProxyAddr};
    }

    ConnectorAdapter * const ConnectorAdapter::ConnectorAdapterFactory (NOMADSUtil::UDPDatagramSocket * const pUDPSocket,
                                                                        const NOMADSUtil::InetAddr & iaLocalProxyAddr,
                                                                        ConnectorType connectorType)
    {
        (void) connectorType;
        return new UDPSocketAdapter{std::unique_ptr<NOMADSUtil::UDPDatagramSocket> {pUDPSocket}, iaLocalProxyAddr};
    }

    bool ConnectorAdapter::checkCorrectHeader (const void * const pBuf, int32 i32BufSize) const
    {
        if (i32BufSize <= 0) {
            return false;
        }

        ProxyMessage *pMsg = (ProxyMessage*) pBuf;
        switch (pMsg->getMessageType()) {
        case PacketType::PMT_InitializeConnection:
        case PacketType::PMT_ConnectionInitialized:
        case PacketType::PMT_ICMPMessage:
        case PacketType::PMT_UDPUnicastData:
        case PacketType::PMT_MultipleUDPDatagrams:
        case PacketType::PMT_UDPBCastMCastData:
        case PacketType::PMT_TCPOpenConnection:
        case PacketType::PMT_TCPConnectionOpened:
        case PacketType::PMT_TCPData:
        case PacketType::PMT_TCPCloseConnection:
        case PacketType::PMT_TCPResetConnection:
        case PacketType::PMT_TunnelPacket:
        case PacketType::PMT_ConnectionError:
            return true;
        default:
            return false;
        }

        return false;
    }

}
