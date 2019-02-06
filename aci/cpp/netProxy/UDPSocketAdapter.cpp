/*
 * UDPSocketAdapter.cpp
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
*/

#include "Logger.h"

#include "UDPSocketAdapter.h"
#include "ConnectorWriter.h"
#include "ConnectionManager.h"
#include "StatisticsManager.h"
#include "ConfigurationManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    void UDPSocketAdapter::UDPConnectionThread::run (void)
    {
        started();
        int rc;
        uint16 ui16MessageSize = 0;

        std::unique_lock<std::mutex> ulUDPConnectionThread{_mUDPConnectionThread};
        while (!terminationRequested()) {
            const auto nextCycleTimeInMS = std::chrono::system_clock::now() + UC_TIME_BETWEEN_ITERATIONS;
            _cvUDPConnectionThread.wait_until (ulUDPConnectionThread, nextCycleTimeInMS, [this, nextCycleTimeInMS]
                { return (std::chrono::system_clock::now() >= nextCycleTimeInMS) || terminationRequested(); });
            if (terminationRequested()) {
                break;
            }

            bool bRemovedPackets = false;
            std::unique_lock<std::mutex> ulProxyMessagesDeq{_mtxProxyMessagesDeq};
            while (_deqProxyNetworkMessages.size() > 0) {
                const auto & rProxyNetworkMessage = _deqProxyNetworkMessages.front();
                ulProxyMessagesDeq.unlock();
                bRemovedPackets = true;
                ui16MessageSize = rProxyNetworkMessage.getMessageTotalSize();
                ulUDPConnectionThread.unlock();
                if (0 != (rc = sendProxyNetworkMessageToRemoteProxy (rProxyNetworkMessage))) {
                    checkAndLogMsg ("UDPSocketAdapter::UDPConnectionThread::run", NOMADSUtil::Logger::L_MildError,
                                    "sendProxyNetworkMessageToRemoteProxy() to remote host %s failed with rc = %d; ProxyMessage total "
                                    "size was %hu; deleting current ProxyMessage and waiting before attempting new transmissions\n",
                                    rProxyNetworkMessage.getDestIPAddrAsString(), rc, rProxyNetworkMessage.getMessageTotalSize());
                    ulUDPConnectionThread.lock();
                    ulProxyMessagesDeq.lock();
                    _deqProxyNetworkMessages.pop_front();
                    _ui32TotalBufferedBytes -= ui16MessageSize;
                    break;
                }

                // Transmission attempt successful --> deleting head of the queue;
                ulUDPConnectionThread.lock();
                ulProxyMessagesDeq.lock();
                _deqProxyNetworkMessages.pop_front();
                _ui32TotalBufferedBytes -= ui16MessageSize;
            }

            if (bRemovedPackets) {
                _deqProxyNetworkMessages.shrink_to_fit();
            }
        }

        checkAndLogMsg ("UDPSocketAdapter::UDPConnectionThread::run", NOMADSUtil::Logger::L_Info,
                        "termination request received; thread will terminate execution\n");

        terminating();
    }

    int UDPSocketAdapter::UDPConnectionThread::addProxyNetworkMessage (const NOMADSUtil::InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage, uint32 ui32SourceIPAddr,
                                                                       uint32 ui32DestinationIPAddr, const uint8 * const pui8MessagePayload, const uint16 ui16MessagePayloadLen)
    {
        if (!pProxyAddr) {
            return -1;
        }
        if (!pProxyMessage) {
            return -2;
        }

        int rc = 0;
        if (NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS != 0) {
            {
                // Enqueuing ProxyMessage
                std::lock_guard<std::mutex> lg{_mtxProxyMessagesDeq};
                _deqProxyNetworkMessages.emplace_front (pProxyAddr, pProxyMessage, ui32SourceIPAddr, ui32DestinationIPAddr, pui8MessagePayload, ui16MessagePayloadLen);
                _ui32TotalBufferedBytes += _deqProxyNetworkMessages.front().getMessageTotalSize();
            }

            // Notify the Connection thread since a new message is available for transmission
            _cvUDPConnectionThread.notify_all();
        }
        else {
            // Immediate send to the remote NetProxy
            if (0 >= (rc = sendProxyNetworkMessageToRemoteProxy (pProxyAddr, pProxyMessage, ui32SourceIPAddr, ui32DestinationIPAddr,
                                                                 pui8MessagePayload, ui16MessagePayloadLen))) {
                checkAndLogMsg ("UDPSocketAdapter::UDPConnectionThread::addProxyNetworkMessage", NOMADSUtil::Logger::L_MildError,
                                "sendProxyNetworkMessageToRemoteProxy() to remote host %s failed with rc = %d; ProxyMessage total size was %hu\n",
                                pProxyAddr->getIPAsString(), rc, pProxyMessage->getMessageHeaderSize() + ui16MessagePayloadLen);
                return -3;
            }
        }

        return rc;
    }

    int UDPSocketAdapter::UDPConnectionThread::sendProxyNetworkMessageToRemoteProxy (const NOMADSUtil::InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage,
                                                                                     uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr, const uint8 *pui8MessagePayload,
                                                                                     uint16 ui16MessagePayloadLen)
    {
        // Check if buffer is large enough to buffer packet
        uint32 ui32TotalMessageSize = pProxyMessage->getMessageHeaderSize() + ui16MessagePayloadLen;
        if (ui32TotalMessageSize > NetworkConfigurationSettings::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("UDPSocketAdapter::UDPConnectionThread::sendProxyNetworkMessageToRemoteProxy", NOMADSUtil::Logger::L_Warning,
                            "impossible to send packet via UDP as it does not fit the buffer; total message size is %u, max message size is %u\n",
                            ui32TotalMessageSize, NetworkConfigurationSettings::PROXY_MESSAGE_MTU);
            return -1;
        }

        // Copy packet to buffer and perform send to remote proxy
        memcpy (_pucOutBuf, pProxyMessage, pProxyMessage->getMessageHeaderSize());
        memcpy (_pucOutBuf + pProxyMessage->getMessageHeaderSize(), pui8MessagePayload, ui16MessagePayloadLen);
        if (ui32TotalMessageSize != _pUDPSocket->sendTo (pProxyAddr->getIPAddress(), pProxyAddr->getPort(), _pucOutBuf, ui32TotalMessageSize)) {
            return -2;
        }

        return ui32TotalMessageSize;
    }

    bool UDPSocketAdapter::UDPConnectionThread::areThereTCPTypePacketsInUDPTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID) const
    {
        if (NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS == 0) {
            return 0;
        }

        std::lock_guard<std::mutex> lg{_mtxProxyMessagesDeq};
        for (auto itProxyNetworkMessages = _deqProxyNetworkMessages.cbegin();
             itProxyNetworkMessages != _deqProxyNetworkMessages.cend(); ++itProxyNetworkMessages) {
            if ((itProxyNetworkMessages->getMessageType() != PacketType::PMT_TCPResetConnection) &&
                ProxyMessage::belongsToVirtualConnection (itProxyNetworkMessages->getProxyMessage(), uiLocalID, uiRemoteID)) {
                // TCP Reset packets are skipped, so that is still possible to notify remote applications about any received RST
                return true;
            }
        }

        return false;
    }

    unsigned int UDPSocketAdapter::UDPConnectionThread::removeTCPTypePacketsFromTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID)
    {
        if (NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS == 0) {
            return 0;
        }

        unsigned int uiPacketsRemoved = 0;
        std::lock_guard<std::mutex> lg{_mtxProxyMessagesDeq};
        for (auto itProxyNetworkMessages = _deqProxyNetworkMessages.cbegin(); itProxyNetworkMessages != _deqProxyNetworkMessages.cend(); ) {
            if (ProxyMessage::belongsToVirtualConnection (itProxyNetworkMessages->getProxyMessage(), uiLocalID, uiRemoteID) &&
                (itProxyNetworkMessages->getMessageType() != PacketType::PMT_TCPResetConnection)) {
                // TCP Reset packets are skipped, so that is still possible to notify remote applications about any received RST
                _ui32TotalBufferedBytes -= itProxyNetworkMessages->getMessageTotalSize();
                _deqProxyNetworkMessages.erase (itProxyNetworkMessages++);
                uiPacketsRemoved++;
            }
            else {
                ++itProxyNetworkMessages;
            }
        }

        if (uiPacketsRemoved) {
            _deqProxyNetworkMessages.shrink_to_fit();
        }

        return uiPacketsRemoved;
    }

    int UDPSocketAdapter::receiveMessage (void * const pBuf, uint32 ui32BufSize)
    {
        int iBytesInBuffer;
        while (0 == (iBytesInBuffer = receive (pBuf, ui32BufSize, &_iaRemoteProxyAddr)));
        if (iBytesInBuffer < 0) {
            return iBytesInBuffer;
        }

        return verifyMessageSizeMatching (pBuf, iBytesInBuffer);
    }

    int UDPSocketAdapter::receiveMessage (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr & iaRemoteAddress)
    {
        int32 i32BytesInBuffer;
        while (0 == (i32BytesInBuffer = receive (pBuf, ui32BufSize, &iaRemoteAddress))) {
            continue;
        }
        if (i32BytesInBuffer < 0) {
            return i32BytesInBuffer;
        }

        return verifyMessageSizeMatching (pBuf, i32BytesInBuffer);
    }

    int UDPSocketAdapter::receiveMessage (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr & iaLocalAddress,
                                          NOMADSUtil::InetAddr & iaRemoteAddress)
    {
        int32 i32BytesInBuffer;
        while (0 == (i32BytesInBuffer = receive (pBuf, ui32BufSize, &iaLocalAddress, &iaRemoteAddress))) {
            continue;
        }
        if (i32BytesInBuffer < 0) {
            return i32BytesInBuffer;
        }

        return verifyMessageSizeMatching (pBuf, i32BytesInBuffer);
    }

    int UDPSocketAdapter::gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr, bool bReliable,
                                 bool bSequenced, const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2)
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
        if (ui32TotalPayloadBytes > _ui16BufferSize) {
            return -3;
        }

        const void *pBuf = va_arg (valist2, const void *);
        uint32 ui32BufSize = va_arg (valist2, uint32), ui32CopiedBytes = 0;

        std::lock_guard<std::mutex> lg{_mtx};
        while (pBuf) {
            memcpy (_upui8Buffer.get() + ui32CopiedBytes, pBuf, ui32BufSize);
            ui32CopiedBytes += ui32BufSize;
            pBuf = va_arg (valist2, const void *);
            ui32BufSize = va_arg (valist2, uint32);
        }

        if (0 > _udpConnectionThread.addProxyNetworkMessage (pInetAddr, (ProxyMessage * const) pBuf1, ui32SourceIPAddr,
                                                             ui32DestinationIPAddr, _upui8Buffer.get(), ui32TotalPayloadBytes)) {
            return -4;
        }

        return 0;
    }

    int UDPSocketAdapter::verifyMessageSizeMatching (void * const pBuf, uint32 ui32BufSize)
    {
        ProxyMessage * const pMsg = reinterpret_cast<ProxyMessage *> (pBuf);
        auto ui8ProxyPcktType = static_cast<uint8> (pMsg->getMessageType());

        if (!checkCorrectHeader (pBuf, ui32BufSize)) {
            checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                            "received a ProxyMessage of unknown type %hhu and %d bytes - ignoring\n",
                            ui8ProxyPcktType, ui32BufSize);
            return 0;
        }

        switch (pMsg->getMessageType()) {
        case PacketType::PMT_InitializeConnection:
            {
                const auto * const pICPM = reinterpret_cast<const InitializeConnectionProxyMessage *> (pMsg);
                if (ui32BufSize != pICPM->getMessageHeaderSize() + pICPM->getPayloadLen()) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_InitializeConnection - %u bytes have been received, but message header is %zu bytes and the "
                                    "payload is %hu bytes\n", ui32BufSize, pICPM->getMessageHeaderSize(), pICPM->getPayloadLen());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_ConnectionInitialized:
            {
                const auto * const pCIPM = reinterpret_cast<const ConnectionInitializedProxyMessage *> (pMsg);
                if (ui32BufSize != pCIPM->getMessageHeaderSize() + pCIPM->getPayloadLen()) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_ConnectionInitialized - %u bytes have been received, but the message header is %zu bytes and "
                                    "the payload is %hu bytes\n", ui32BufSize, pCIPM->getMessageHeaderSize(), pCIPM->getPayloadLen());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_ICMPMessage:
            {
                const auto * const pICMPPM = reinterpret_cast<const ICMPProxyMessage *> (pBuf);
                if (ui32BufSize != (pICMPPM->getMessageHeaderSize() + pICMPPM->getPayloadLen())) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_ICMPMessage - %u bytes have been received, but the message header is %zu bytes and the payload "
                                    "is %hu bytes\n", ui32BufSize, pICMPPM->getMessageHeaderSize(), pICMPPM->getPayloadLen());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_UDPUnicastData:
            {
                const auto * const pUDPUDPM = reinterpret_cast<const UDPUnicastDataProxyMessage *> (pBuf);
                if (ui32BufSize != (pUDPUDPM->getMessageHeaderSize() + pUDPUDPM->getPayloadLen())) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_UDPUnicastData - %u bytes have been received, but the message header is %zu bytes and the payload "
                                    "is %hu bytes\n", ui32BufSize, pUDPUDPM->getMessageHeaderSize(), pUDPUDPM->getPayloadLen());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_MultipleUDPDatagrams:
            {
                const auto * const pMUDPDPM = reinterpret_cast<const MultipleUDPDatagramsProxyMessage *> (pBuf);
                if (ui32BufSize != (pMUDPDPM->getMessageHeaderSize() + pMUDPDPM->getPayloadLen())) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_MultipleUDPDatagrams - %u bytes have been received, but the message header is %zu bytes and the "
                                    "payload is %hu bytes\n", ui32BufSize, pMUDPDPM->getMessageHeaderSize(), pMUDPDPM->getPayloadLen());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_UDPBCastMCastData:
            {
                const auto * const pUDPBMDPM = reinterpret_cast<const UDPBCastMCastDataProxyMessage *> (pBuf);
                if (ui32BufSize != (pUDPBMDPM->getMessageHeaderSize() + pUDPBMDPM->getPayloadLen())) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_UDPBCastMCastData - %u bytes have been received, but the message header is %zu bytes and the "
                                    "payload is %hu bytes\n", ui32BufSize, pUDPBMDPM->getMessageHeaderSize(), pUDPBMDPM->getPayloadLen());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_TCPOpenConnection:
            {
                const auto * const pOTCPCPM = reinterpret_cast<const OpenTCPConnectionProxyMessage *> (pBuf);
                if (ui32BufSize != pOTCPCPM->getMessageHeaderSize()) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_TCPOpenConnection - %u bytes have been received, but the message header is %zu bytes\n",
                                    ui32BufSize, pOTCPCPM->getMessageHeaderSize());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_TCPConnectionOpened:
            {
                const auto * const pTCPCOPM = reinterpret_cast<const TCPConnectionOpenedProxyMessage *> (pBuf);
                if (ui32BufSize != pTCPCOPM->getMessageHeaderSize()) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_TCPConnectionOpened - %u bytes have been received, but the message header is %zu bytes\n",
                                    ui32BufSize, pTCPCOPM->getMessageHeaderSize());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_TCPData:
            {
                const auto * const pTCPDPM = reinterpret_cast<const TCPDataProxyMessage *> (pBuf);
                if (ui32BufSize != (pTCPDPM->getMessageHeaderSize() + pTCPDPM->getPayloadLen())) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_TCPData - %u bytes have been received, but the message header is %zu bytes and the payload "
                                    "is %hu bytes\n", ui32BufSize, pTCPDPM->getMessageHeaderSize(), pTCPDPM->getPayloadLen());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_TCPCloseConnection:
            {
                const auto * const pCTCPCPM = reinterpret_cast<const CloseTCPConnectionProxyMessage *> (pBuf);
                if (ui32BufSize != pCTCPCPM->getMessageHeaderSize()) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_TCPCloseConnection - %u bytes have been received, but the message header is %zu bytes long\n",
                                    ui32BufSize, pCTCPCPM->getMessageHeaderSize());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_TCPResetConnection:
            {
                const auto * const pRTCPCPM = reinterpret_cast<const ResetTCPConnectionProxyMessage *> (pBuf);
                if (ui32BufSize != pRTCPCPM->getMessageHeaderSize()) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_TCPResetConnection - %u bytes have been received, but the message header is %zu bytes\n",
                                    ui32BufSize, pRTCPCPM->getMessageHeaderSize());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_TunnelPacket:
            {
                const auto * const pTPPM = reinterpret_cast<const TunnelPacketProxyMessage *> (pBuf);
                if (ui32BufSize != (pTPPM->getMessageHeaderSize() + pTPPM->getPayloadLen())) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_TunnelPacket - %u bytes have been received, but the message header is %zu bytes and the "
                                    "payload is %hu bytes\n", ui32BufSize, pTPPM->getMessageHeaderSize(), pTPPM->getPayloadLen());
                    return 0;
                }
                break;
            }

        case PacketType::PMT_ConnectionError:
            {
                const auto * const pCEPM = reinterpret_cast<const ConnectionErrorProxyMessage *> (pBuf);
                if (ui32BufSize != pCEPM->getMessageHeaderSize()) {
                    checkAndLogMsg ("UDPSocketAdapter::verifyMessageSizeMatching", NOMADSUtil::Logger::L_Warning,
                                    "PMT_ConnectionError - %u bytes have been received, but the message header is %zu bytes\n",
                                    ui32BufSize, pCEPM->getMessageHeaderSize());
                    return 0;
                }
                break;
            }
        }

        return ui32BufSize;
    }

    constexpr const std::chrono::milliseconds UDPSocketAdapter::UDPConnectionThread::UC_TIME_BETWEEN_ITERATIONS;
}