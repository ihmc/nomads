/*
 * UDPSocketAdapter.cpp
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

#include "Logger.h"

#include "UDPSocketAdapter.h"
#include "ConnectorWriter.h"
#include "ConnectionManager.h"
#include "GUIUpdateMessage.h"
#include "ConfigurationManager.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    void UDPSocketAdapter::UDPConnectionThread::run (void)
    {
        started();
        int rc;
        uint16 ui16MessageSize = 0;
        const ProxyNetworkMessage *pProxyNetworkMessage = nullptr;

        _mUDPConnectionThread.lock();
        while (!terminationRequested()) {
            _mPtrQueue.lock();
            while ((pProxyNetworkMessage = _ProxyNetworkMessagesPtrQueue.dequeue())) {
                _mPtrQueue.unlock();
                ui16MessageSize = pProxyNetworkMessage->getMessageTotalLength();
                if (0 != (rc = sendProxyNetworkMessageToRemoteProxy (pProxyNetworkMessage))) {
                    checkAndLogMsg ("UDPConnector::UDPConnection::run", Logger::L_MildError,
                                    "sendProxyNetworkMessageToRemoteProxy() to remote host %s failed with rc = %d; ProxyMessage total size was %hu; "
                                    "deleting current ProxyMessage and waiting before attempting new transmissions\n",
                                    pProxyNetworkMessage->getDestIPAddrAsString(), rc, pProxyNetworkMessage->getMessageTotalLength());
                    delete pProxyNetworkMessage;
                    _mPtrQueue.lock();
                    _ui32TotalBufferedBytes -= ui16MessageSize;
                    break;
                }
                // Transmission attempt successful --> deleting head of the queue;
                delete pProxyNetworkMessage;
                _mPtrQueue.lock();
                _ui32TotalBufferedBytes -= ui16MessageSize;
            }
            _mPtrQueue.unlock();
            if (terminationRequested()) {
                break;
            }
            _cvUDPConnectionThread.wait (UC_TIME_BETWEEN_ITERATIONS);
        }
        _mUDPConnectionThread.unlock();

        terminating();
    }

    int UDPSocketAdapter::UDPConnectionThread::addProxyNetworkMessage (const InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage, uint32 ui32DestVirtualIPAddr,
                                                                       const uint8 * const pui8MessagePayload, const uint16 ui16MessagePayloadLen)
    {
        if (!pProxyAddr) {
            return -1;
        }
        if (!pProxyMessage) {
            return -2;
        }

        int rc = 0;
        if (NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS != 0) {
            // Enqueuing Proxy Message
            _mPtrQueue.lock();
            const ProxyNetworkMessage * const pProxyNetworkMessage = new ProxyNetworkMessage (pProxyAddr, pProxyMessage, ui32DestVirtualIPAddr, pui8MessagePayload, ui16MessagePayloadLen);
            _ProxyNetworkMessagesPtrQueue.enqueue (pProxyNetworkMessage);
            _ui32TotalBufferedBytes += pProxyNetworkMessage->getMessageTotalLength();
            _mPtrQueue.unlock();
            if (Mutex::RC_Ok == _mUDPConnectionThread.tryLock()) {
                _cvUDPConnectionThread.notifyAll();
                _mUDPConnectionThread.unlock();
            }
        }
        else {
            // Immediate send to remote side via UDP Socket
            if (0 >= (rc = sendProxyNetworkMessageToRemoteProxy (pProxyAddr, pProxyMessage, ui32DestVirtualIPAddr, pui8MessagePayload, ui16MessagePayloadLen))) {
                checkAndLogMsg ("UDPConnector::UDPConnection::addProxyNetworkMessage", Logger::L_MildError,
                                "sendProxyNetworkMessageToRemoteProxy() to remote host %s failed with rc = %d; ProxyMessage total size was %hu\n",
                                pProxyAddr->getIPAsString(), rc, pProxyMessage->getMessageHeaderSize() + ui16MessagePayloadLen);
                return -3;
            }
        }

        return rc;
    }

    int UDPSocketAdapter::UDPConnectionThread::sendProxyNetworkMessageToRemoteProxy (const InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage,
                                                                                     uint32 ui32DestVirtualIPAddr, const uint8 *pui8MessagePayload, uint16 ui16MessagePayloadLen)
    {
        static ConnectionManager * const pConnectionManager = ConnectionManager::getConnectionManagerInstance();
        static GUIStatsManager * const pGUIStatsManager = GUIStatsManager::getGUIStatsManager();

        // Check if buffer is large enough to buffer packet
        uint32 ui32TotalMessageSize = pProxyMessage->getMessageHeaderSize() + ui16MessagePayloadLen;
        if (ui32TotalMessageSize > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("UDPConnector::UDPConnection::sendProxyNetworkMessageToRemoteProxy", Logger::L_Warning,
                            "impossible to send packet via UDP as it does not fit the buffer; total message size is %u, max allowed buffer size is %u\n",
                            ui32TotalMessageSize, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
            return -1;
        }

        // Copy packet to buffer and perform send to remote proxy
        memcpy (_pucOutBuf, pProxyMessage, pProxyMessage->getMessageHeaderSize());
        memcpy (_pucOutBuf + pProxyMessage->getMessageHeaderSize(), pui8MessagePayload, ui16MessagePayloadLen);
        if (ui32TotalMessageSize != _pSocket->sendTo (pProxyAddr->getIPAddress(), pProxyAddr->getPort(), _pucOutBuf, ui32TotalMessageSize)) {
            return -2;
        }

        QueryResult query (pConnectionManager->queryConnectionToRemoteHostForConnectorType (ui32DestVirtualIPAddr, 0, CT_UDPSOCKET, _encryptionType));
        pGUIStatsManager->increaseTrafficOut (CT_UDPSOCKET, ui32DestVirtualIPAddr, query.getRemoteProxyUniqueID(), pProxyAddr->getIPAddress(), pProxyAddr->getPort(),
                                              pProxyMessage->getProtocolType(), pProxyMessage->getMessageHeaderSize() + ui16MessagePayloadLen);

        return ui32TotalMessageSize;
    }

    unsigned int UDPSocketAdapter::UDPConnectionThread::removeTCPTypePacketsFromTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID)
    {
        if (NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS != 0) {
            unsigned int uiPacketsRemoved = 0;
            const ProxyNetworkMessage *pProxyNetworkMessage = nullptr;
            _mPtrQueue.lock();
            _ProxyNetworkMessagesPtrQueue.resetGet();
            while ((pProxyNetworkMessage = _ProxyNetworkMessagesPtrQueue.getNext())) {
                if (ProxyMessage::belongsToVirtualConnection (pProxyNetworkMessage->getProxyMessage(),uiLocalID, uiRemoteID) &&
                    pProxyNetworkMessage->getMessageType() != ProxyMessage::PMT_TCPResetConnection) {
                    // TCP Reset packets are skipped, so that is still possible to notify remote applications about any received RST
                    _ui32TotalBufferedBytes -= pProxyNetworkMessage->getMessageTotalLength();
                    delete _ProxyNetworkMessagesPtrQueue.remove (pProxyNetworkMessage);
                    uiPacketsRemoved++;
                }
            }
            _mPtrQueue.unlock();

            return uiPacketsRemoved;
        }

        return 0;
    }

    int UDPSocketAdapter::receiveMessage (void * const pBuf, uint32 ui32BufSize)
    {
        int32 i32BytesInBuffer;
        while (0 == (i32BytesInBuffer = receive (pBuf, ui32BufSize, &_remoteProxyInetAddr))) {
            continue;
        }
        if (i32BytesInBuffer < 0) {
            return i32BytesInBuffer;
        }

        return verifyMessageSizeMatching (pBuf, i32BytesInBuffer);
    }

    int UDPSocketAdapter::receiveMessage (void * const pBuf, uint32 ui32BufSize, InetAddr * const pInetAddr)
    {
        int32 i32BytesInBuffer;
        while (0 == (i32BytesInBuffer = receive (pBuf, ui32BufSize, pInetAddr))) {
            continue;
        }
        if (i32BytesInBuffer < 0) {
            return i32BytesInBuffer;
        }

        return verifyMessageSizeMatching (pBuf, i32BytesInBuffer);
    }

    int UDPSocketAdapter::gsend (const InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable, bool bSequenced,
                                 const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2)
    {
        (void) bReliable;
        (void) bSequenced;

        if (!pInetAddr) {
            return -1;
        }

        if (!pBuf1) {
            if (ui32BufSize1 > 0) {
                return -2;
            }
            return 0;
        }

        uint32 ui32TotalPayloadBytes = 0;
        while (va_arg (valist1, const void *) != nullptr) {
            ui32TotalPayloadBytes += va_arg (valist1, uint32);
        }
        if (ui32TotalPayloadBytes > _ui16MemBufSize) {
            return -3;
        }

        const void *pBuf = va_arg (valist2, const void *);
        uint32 ui32BufSize = va_arg (valist2, uint32), ui32CopiedBytes = 0;
        _m.lock();
        while (pBuf) {
            memcpy (_pui8MemBuf + ui32CopiedBytes, pBuf, ui32BufSize);
            ui32CopiedBytes += ui32BufSize;
            pBuf = va_arg (valist2, const void *);
            ui32BufSize = va_arg (valist2, uint32);
        }

        if (0 > _udpConnectionThread.addProxyNetworkMessage (pInetAddr, (ProxyMessage * const) pBuf1, ui32DestVirtualIPAddr, _pui8MemBuf, ui32TotalPayloadBytes)) {
            return -4;
        }
        _m.unlock();

        return 0;
    }

    int UDPSocketAdapter::verifyMessageSizeMatching (void * const pBuf, uint32 ui32BufSize)
    {
        ProxyMessage *pMsg = (ProxyMessage*) pBuf;
        uint8 ui8ProxyPcktType = pMsg->getMessageType();

        if (!checkCorrectHeader (pBuf, ui32BufSize)) {
            checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                            "received a proxy message of unknown type %hhu and %d bytes - ignoring\n",
                            ui8ProxyPcktType, ui32BufSize);
            return 0;
        }

        switch (ui8ProxyPcktType) {
            case ProxyMessage::PMT_InitializeConnection:
            {
                if (ui32BufSize != sizeof(InitializeConnectionProxyMessage)) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "PMT_InitializeConnection - %d bytes have been received, but message header is %u bytes\n",
                                    ui32BufSize, sizeof(InitializeConnectionProxyMessage));
                    return 0;
                }
                break;
            }

            case ProxyMessage::PMT_ConnectionInitialized:
            {
                if (ui32BufSize != sizeof(ConnectionInitializedProxyMessage)) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "PMT_ConnectionInitialized - %d bytes have been received, but message header is %u bytes\n",
                                    ui32BufSize, sizeof(ConnectionInitializedProxyMessage));
                    return 0;
                }
                break;
            }

            case ProxyMessage::PMT_ICMPMessage:
            {
                const ICMPProxyMessage * const pICMPPM = (ICMPProxyMessage*) pBuf;
                if (ui32BufSize != (sizeof(ICMPProxyMessage) + pICMPPM->getPayloadLen())) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "PMT_ICMPMessage - %d bytes have been received, but message header is %u bytes and payload is %hu bytes long\n",
                                    ui32BufSize, sizeof(ICMPProxyMessage), pICMPPM->getPayloadLen());
                    return 0;
                }
                break;
            }

            case ProxyMessage::PMT_UDPUnicastData:
            {
                const UDPUnicastDataProxyMessage * const pUDPUDPM = (UDPUnicastDataProxyMessage*) pBuf;
                if (ui32BufSize != (sizeof(UDPUnicastDataProxyMessage) + pUDPUDPM->getPayloadLen())) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "PMT_UDPUnicastData - %d bytes have been received, but message header is %u bytes and payload is %hu bytes long\n",
                                    ui32BufSize, sizeof(UDPUnicastDataProxyMessage), pUDPUDPM->getPayloadLen());
                    return 0;
                }
                break;
            }

            case ProxyMessage::PMT_MultipleUDPDatagrams:
            {
                const MultipleUDPDatagramsProxyMessage * const pMUDPDPM = (MultipleUDPDatagramsProxyMessage*) pBuf;
                if (ui32BufSize != (sizeof(MultipleUDPDatagramsProxyMessage) + pMUDPDPM->getPayloadLen())) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "PMT_MultipleUDPDatagrams - %d bytes have been received, but message header is %u bytes and payload is %hu bytes long\n",
                                    ui32BufSize, sizeof(MultipleUDPDatagramsProxyMessage), pMUDPDPM->getPayloadLen());
                    return 0;
                }
                break;
            }

            case ProxyMessage::PMT_UDPBCastMCastData:
            {
                const UDPBCastMCastDataProxyMessage * const udpBMDPM = (UDPBCastMCastDataProxyMessage*) pBuf;
                if (ui32BufSize != (sizeof(UDPBCastMCastDataProxyMessage) + udpBMDPM->getPayloadLen())) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "PMT_UDPBCastMCastData - %d bytes have been received, but message header is %u bytes and payload is %hu bytes long\n",
                                    ui32BufSize, sizeof(UDPBCastMCastDataProxyMessage), udpBMDPM->getPayloadLen());
                    return 0;
                }
                break;
            }

            case ProxyMessage::PMT_TCPOpenConnection:
            {
                if (ui32BufSize != sizeof(OpenConnectionProxyMessage)) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "PMT_TCPOpenConnection - %d bytes have been received, but message header is %u bytes long\n",
                                    ui32BufSize, sizeof(OpenConnectionProxyMessage));
                    return 0;
                }
                break;
            }

            case ProxyMessage::PMT_TCPConnectionOpened:
            {
                if (ui32BufSize != sizeof(ConnectionOpenedProxyMessage)) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "PMT_TCPConnectionOpened - %d bytes have been received, but message header is %u bytes long\n",
                                    ui32BufSize, sizeof(ConnectionOpenedProxyMessage));
                    return 0;
                }
                break;
            }

            case ProxyMessage::PMT_TCPData:
            {
                const TCPDataProxyMessage * const pTCPDataMsg = (TCPDataProxyMessage*) pBuf;
                if (ui32BufSize != (sizeof(TCPDataProxyMessage) + pTCPDataMsg->_ui16PayloadLen)) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "PMT_TCPData - %d bytes have been received, but message header is %u bytes and payload is %hu bytes long\n",
                                    ui32BufSize, sizeof(TCPDataProxyMessage), pTCPDataMsg->getPayloadLen());
                    return 0;
                }
                break;
            }

            case ProxyMessage::PMT_TCPCloseConnection:
            {
                if (ui32BufSize != sizeof(CloseConnectionProxyMessage)) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "PMT_TCPCloseConnection - %d bytes have been received, but message header is %u bytes long\n",
                                    ui32BufSize, sizeof(CloseConnectionProxyMessage));
                    return 0;
                }
                break;
            }

            case ProxyMessage::PMT_TCPResetConnection:
            {
                if (ui32BufSize != sizeof(ResetConnectionProxyMessage)) {
                    checkAndLogMsg ("UDPSocketAdapter::receiveMessage", Logger::L_Warning,
                                    "ResetConnectionProxyMessage - %d bytes have been received, but message header is %u bytes long\n",
                                    ui32BufSize, sizeof(ResetConnectionProxyMessage));
                    return 0;
                }
                break;
            }
        }

        return ui32BufSize;
    }
}
