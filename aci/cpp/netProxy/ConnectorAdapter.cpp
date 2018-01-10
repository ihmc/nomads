/*
 * ConnectorAdapter.cpp
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

#include "ConnectorAdapter.h"

#include "CSRAdapter.h"
#include "MocketAdapter.h"
#include "TCPSocketAdapter.h"
#include "UDPSocketAdapter.h"
#include "UDPConnector.h"
#include "Connection.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    ConnectorAdapter::~ConnectorAdapter (void)
    {
        delete[] _pui8MemBuf;
    }

    ConnectorAdapter * const ConnectorAdapter::ConnectorAdapterFactory (ConnectorType connectorType, EncryptionType encryptionType,
                                                                        const InetAddr * const pRemoteProxyInetAddr)
    {
        if (!pRemoteProxyInetAddr || (connectorType == CT_UNDEF)) {
            return nullptr;
        }

        switch (connectorType) {
            case CT_TCPSOCKET:
                return new TCPSocketAdapter (pRemoteProxyInetAddr, encryptionType);
            case CT_UDPSOCKET:
                return UDPConnector::getUDPConnection()->getConnectorAdapter();
            case CT_MOCKETS:
                return new MocketAdapter (pRemoteProxyInetAddr, encryptionType);
            case CT_CSR:
                return new CSRAdapter (pRemoteProxyInetAddr, encryptionType);
            case CT_UNDEF:
                return nullptr;
        }

        return nullptr;
    }

    ConnectorAdapter * const ConnectorAdapter::ConnectorAdapterFactory (Mocket * const pMocket, ConnectorType connectorType)
    {
        switch (connectorType) {
            case CT_MOCKETS:
            {
                return new MocketAdapter (pMocket);
            }
            case CT_CSR:
            {
                return new CSRAdapter (pMocket);
            }
            default:
                break;
        }

        return nullptr;
    }

    ConnectorAdapter * const ConnectorAdapter::ConnectorAdapterFactory (TCPSocket * const pTCPSocket, ConnectorType connectorType)
    {
        (void) connectorType;
        return new TCPSocketAdapter (pTCPSocket);
    }

    ConnectorAdapter * const ConnectorAdapter::ConnectorAdapterFactory (UDPDatagramSocket * const pUDPSocket, ConnectorType connectorType)
    {
        (void) connectorType;
        return new UDPSocketAdapter (pUDPSocket);
    }

    bool ConnectorAdapter::checkCorrectHeader (const void * const pBuf, int32 i32BufSize) const
    {
        if (i32BufSize <= 0) {
            return false;
        }

        ProxyMessage *pMsg = (ProxyMessage*) pBuf;
        switch (pMsg->getMessageType()) {
            case ProxyMessage::PMT_InitializeConnection:
            case ProxyMessage::PMT_ConnectionInitialized:
            case ProxyMessage::PMT_ICMPMessage:
            case ProxyMessage::PMT_UDPUnicastData:
            case ProxyMessage::PMT_MultipleUDPDatagrams:
            case ProxyMessage::PMT_UDPBCastMCastData:
            case ProxyMessage::PMT_TCPOpenConnection:
            case ProxyMessage::PMT_TCPConnectionOpened:
            case ProxyMessage::PMT_TCPData:
            case ProxyMessage::PMT_TCPCloseConnection:
            case ProxyMessage::PMT_TCPResetConnection:
            case ProxyMessage::PMT_TunnelPacket:
                return true;
            default:
                return false;
        }

        return false;
    }
}
