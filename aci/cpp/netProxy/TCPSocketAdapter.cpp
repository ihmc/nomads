/*
 * TCPSocketAdapter.cpp
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

#include "TCPSocketAdapter.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    int TCPSocketAdapter::receiveMessage (void * const pBuf, uint32 ui32BufSize)
    {
        (void) ui32BufSize;
        int rc=0, headerLen=0, payLoadLen=0;
        if ((rc = receive (pBuf, 1)) < 0) {
            checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                            "receive() of 1 byte from socket failed with rc = %d\n", rc);
            return rc;
        }

        headerLen = rc;
        ProxyMessage * pMsg = reinterpret_cast<ProxyMessage*> (pBuf);
        uint8 * const ui8Buf = reinterpret_cast<uint8 * const> (pBuf);
        switch (pMsg->getMessageType()) {
        case PacketType::PMT_InitializeConnection:
            {
                const auto * const pICPM = reinterpret_cast<const InitializeConnectionProxyMessage *> (pMsg);
                if (0 != (rc = receiveProxyHeader (pICPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_InitializeConnection - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pICPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pICPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                        "PMT_InitializeConnection - receive() of %u/%u bytes from socket failed with rc = %d\n",
                                        payLoadLen, pICPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

        case PacketType::PMT_ConnectionInitialized:
            {
                const auto * const pCIPM = reinterpret_cast<const ConnectionInitializedProxyMessage *> (pMsg);
                if (0 != (rc = receiveProxyHeader (pCIPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_ConnectionInitialized - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pCIPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pCIPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                        "PMT_ConnectionInitialized - receive() of %u/%u bytes from socket failed with rc = %d\n",
                                        payLoadLen, pCIPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

        case PacketType::PMT_ICMPMessage:
            {
                const auto * const pICMPPM = reinterpret_cast<const ICMPProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pICMPPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_ICMPMessage - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pICMPPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pICMPPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                        "PMT_ICMPMessage - receive() of %u/%u bytes from socket failed with rc = %d\n",
                                        payLoadLen, pICMPPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

        case PacketType::PMT_UDPUnicastData:
            {
                const auto * const pUDPUDPM = reinterpret_cast<const UDPUnicastDataProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pUDPUDPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_UDPUnicastData - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pUDPUDPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pUDPUDPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                        "PMT_UDPUnicastData - receive() of %u/%u bytes from socket failed with rc = %d\n",
                                        payLoadLen, pUDPUDPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

        case PacketType::PMT_MultipleUDPDatagrams:
            {
                const auto * const pMUDPDPM = reinterpret_cast<const MultipleUDPDatagramsProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pMUDPDPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_MultipleUDPDatagrams - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pMUDPDPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pMUDPDPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                        "PMT_MultipleUDPDatagrams - receive() of %u/%u bytes from socket failed with rc = %d\n",
                                        payLoadLen, pMUDPDPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;

                break;
            }

        case PacketType::PMT_UDPBCastMCastData:
            {
                const auto * const pUDPBMDPM = reinterpret_cast<const UDPBCastMCastDataProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pUDPBMDPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_UDPBCastMCastData - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pUDPBMDPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pUDPBMDPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                        "PMT_UDPBCastMCastData - receive() of %u/%u bytes from socket failed with rc = %d\n",
                                        payLoadLen, pUDPBMDPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

        case PacketType::PMT_TCPOpenConnection:
            {
                const auto * const pOTCPCPM = reinterpret_cast<const OpenTCPConnectionProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pOTCPCPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_TCPOpenConnection - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }

        case PacketType::PMT_TCPConnectionOpened:
            {
                const auto * const pTCPCOPM = reinterpret_cast<const TCPConnectionOpenedProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pTCPCOPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_TCPConnectionOpened - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }

        case PacketType::PMT_TCPData:
            {
                const auto * const pTCPDPM = reinterpret_cast<const TCPDataProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pTCPDPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_TCPData - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pTCPDPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pTCPDPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                        "L%hu-R%hu: PMT_TCPData - receive() of %u/%u bytes from socket failed with rc = %d\n",
                                        pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, payLoadLen, pTCPDPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

        case PacketType::PMT_TCPCloseConnection:
            {
                const auto * const pCTCPCPM = reinterpret_cast<const CloseTCPConnectionProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pCTCPCPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_TCPCloseConnection - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }

        case PacketType::PMT_TCPResetConnection:
            {
                const auto * const pRTCPCPM = reinterpret_cast<const ResetTCPConnectionProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pRTCPCPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_TCPResetConnection - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }

        case PacketType::PMT_TunnelPacket:
            {
                const auto * const pTPPM = reinterpret_cast<const TunnelPacketProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pTPPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_TunnelPacket - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }


                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pTPPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pTPPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                        "PMT_TunnelPacket - receive() of %u/%u bytes from socket failed with rc = %d\n",
                                        payLoadLen, pTPPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

        case PacketType::PMT_ConnectionError:
            {
                const auto * const pCEPM = reinterpret_cast<const ConnectionErrorProxyMessage *> (pBuf);
                if (0 != (rc = receiveProxyHeader (pCEPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", NOMADSUtil::Logger::L_MildError,
                                    "PMT_ConnectionError - rcvProxyHeader() from socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }
        }

        return headerLen + payLoadLen;
    }

    int TCPSocketAdapter::gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr, bool bReliable,
                                 bool bSequenced, const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2)
    {
        (void) pInetAddr;
        (void) ui32SourceIPAddr;
        (void) ui32DestinationIPAddr;
        (void) bReliable;
        (void) bSequenced;

        if (!pBuf1) {
            if (ui32BufSize1 > 0) {
                return -1;
            }
            return 0;
        }

        uint32 ui32TotalBytes = ui32BufSize1;
        while (va_arg (valist1, const void*) != nullptr) {
            ui32TotalBytes += va_arg (valist1, uint32);
        }
        if (ui32TotalBytes > _ui16BufferSize) {
            return -2;
        }

        const void *pBuf = pBuf1;
        uint32 ui32BufSize = ui32BufSize1, ui32CopiedBytes = 0;

        std::lock_guard<std::mutex> lg{_mtx};
        while (pBuf) {
            memcpy (_upui8Buffer.get() + ui32CopiedBytes, pBuf, ui32BufSize);
            ui32CopiedBytes += ui32BufSize;
            pBuf = va_arg (valist2, const void*);
            ui32BufSize = va_arg (valist2, uint32);
        }
        int rc = _upTCPSocket->send (_upui8Buffer.get(), ui32TotalBytes);

        return (rc == ui32TotalBytes) ? 0 : rc;
    }

    // retrieves the header of an incoming packet sent by a remote NetProxy
    template <class T> int TCPSocketAdapter::receiveProxyHeader (T * const pMess, int & iReceived)
    {
        int rc=0;
        uint8 * const ui8Buf = (uint8 * const) pMess;

        while ((rc = _upTCPSocket->receive (ui8Buf + iReceived, sizeof(T) - iReceived)) + iReceived < sizeof(T)) {
            if (rc < 0) {
                return rc;
            }
            iReceived += rc;
        }
        iReceived += rc;

        return 0;
    }
}
