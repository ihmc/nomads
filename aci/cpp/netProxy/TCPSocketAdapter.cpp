/*
 * TCPSocketAdapter.cpp
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

#include "TCPSocketAdapter.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    int TCPSocketAdapter::receiveMessage (void * const pBuf, uint32 ui32BufSize)
    {
        (void) ui32BufSize;
        int rc=0, headerLen=0, payLoadLen=0;
        if ((rc = receive (pBuf, 1)) < 0) {
            checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                            "receive() of 1 byte from socket failed with rc = %d\n", rc);
            return rc;
        }

        ProxyMessage *pMsg = reinterpret_cast<ProxyMessage*> (pBuf);
        uint8 * const ui8Buf = (uint8 * const) pBuf;
        uint8 ui8ProxyPcktType = pMsg->getMessageType();
        headerLen = rc;
        switch (ui8ProxyPcktType) {
            case ProxyMessage::PMT_InitializeConnection:
            {
                InitializeConnectionProxyMessage *pICPM = (InitializeConnectionProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (pICPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_InitializeConnection - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }

            case ProxyMessage::PMT_ConnectionInitialized:
            {
                ConnectionInitializedProxyMessage *pCIPM = (ConnectionInitializedProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (pCIPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_ConnectionInitialized - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }

            case ProxyMessage::PMT_ICMPMessage:
            {
                ICMPProxyMessage *pICMPPM = (ICMPProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (pICMPPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_ICMPMessage - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pICMPPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pICMPPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                        "PMT_ICMPMessage - receive() of %u/%u pcktPayload bytes on socket failed with rc = %d\n",
                                        payLoadLen, pICMPPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

            case ProxyMessage::PMT_UDPUnicastData:
            {
                UDPUnicastDataProxyMessage *pUDPUDPM = (UDPUnicastDataProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (pUDPUDPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_UDPUnicastData - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pUDPUDPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pUDPUDPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                        "PMT_UDPUnicastData - receive() of %u/%u pcktPayload bytes on socket failed with rc = %d\n",
                                        payLoadLen, pUDPUDPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

            case ProxyMessage::PMT_MultipleUDPDatagrams:
            {
                MultipleUDPDatagramsProxyMessage *pMUDPDPM = (MultipleUDPDatagramsProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (pMUDPDPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_MultipleUDPDatagrams - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pMUDPDPM->getPayloadLen() - payLoadLen)) + payLoadLen) < pMUDPDPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                        "PMT_MultipleUDPDatagrams - receive() of %u/%u pcktPayload bytes on socket failed with rc = %d\n",
                                        payLoadLen, pMUDPDPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;

                break;
            }

            case ProxyMessage::PMT_UDPBCastMCastData:
            {
                UDPBCastMCastDataProxyMessage *udpBMDPM = (UDPBCastMCastDataProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (udpBMDPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_UDPBCastMCastData - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, udpBMDPM->getPayloadLen() - payLoadLen)) + payLoadLen) < udpBMDPM->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                        "PMT_UDPBCastMCastData - receive() of %u/%u pcktPayload bytes on socket failed with rc = %d\n",
                                        payLoadLen, udpBMDPM->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

            case ProxyMessage::PMT_TCPOpenConnection:
            {
                OpenConnectionProxyMessage *pOCPM = (OpenConnectionProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (pOCPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_TCPOpenConnection - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }

            case ProxyMessage::PMT_TCPConnectionOpened:
            {
                ConnectionOpenedProxyMessage *pCOPM = (ConnectionOpenedProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (pCOPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_TCPConnectionOpened - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }

            case ProxyMessage::PMT_TCPData:
            {
                TCPDataProxyMessage *pTCPDataMsg = (TCPDataProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (pTCPDataMsg, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_TCPData - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }

                while (((rc = receive (ui8Buf + headerLen + payLoadLen, pTCPDataMsg->getPayloadLen() - payLoadLen)) + payLoadLen) < pTCPDataMsg->getPayloadLen()) {
                    if (rc < 0) {
                        checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                        "L%hu-R%hu: PMT_TCPData - receive() of %u/%u pcktPayload bytes on socket failed with rc = %d\n",
                                        pTCPDataMsg->getPayloadLen(), pTCPDataMsg->getPayloadLen(), payLoadLen, pTCPDataMsg->getPayloadLen(), rc);
                        return rc;
                    }
                    payLoadLen += rc;
                }
                payLoadLen += rc;
                break;
            }

            case ProxyMessage::PMT_TCPCloseConnection:
            {
                CloseConnectionProxyMessage *pCCPM = (CloseConnectionProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (pCCPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_TCPCloseConnection - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }

            case ProxyMessage::PMT_TCPResetConnection:
            {
                ResetConnectionProxyMessage *pRCPM = (ResetConnectionProxyMessage*) pMsg;
                if (0 != (rc = receiveProxyHeader (pRCPM, headerLen))) {
                    checkAndLogMsg ("TCPSocketAdapter::receiveMessage", Logger::L_MildError,
                                    "PMT_TCPResetConnection - rcvProxyHeader() on socket failed with rc = %d\n", rc);
                    return rc;
                }
                break;
            }
        }

        return headerLen + payLoadLen;
    }

    int TCPSocketAdapter::gsend (const InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable, bool bSequenced,
                                    const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2)
    {
        (void) pInetAddr;
        (void) ui32DestVirtualIPAddr;
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
        if (ui32TotalBytes > _ui16MemBufSize) {
            return -2;
        }

        const void *pBuf = pBuf1;
        uint32 ui32BufSize = ui32BufSize1, ui32CopiedBytes = 0;
        _m.lock();
        while (pBuf) {
            memcpy (_pui8MemBuf + ui32CopiedBytes, pBuf, ui32BufSize);
            ui32CopiedBytes += ui32BufSize;
            pBuf = va_arg (valist2, const void*);
            ui32BufSize = va_arg (valist2, uint32);
        }
        int rc = _pTCPSocket->send (_pui8MemBuf, ui32TotalBytes);
        _m.unlock();

        return (rc == ui32TotalBytes) ? 0 : rc;
    }

    // retrieves the header of an incoming packet sent by a remote NetProxy
    template <class T> int TCPSocketAdapter::receiveProxyHeader (T * const pMess, int &iReceived)
    {
        int rc=0;
        uint8 * const ui8Buf = (uint8 * const) pMess;

        while ((rc = _pTCPSocket->receive (ui8Buf + iReceived, sizeof(T) - iReceived)) + iReceived < sizeof(T)) {
            if (rc < 0) {
                return rc;
            }
            iReceived += rc;
        }
        iReceived += rc;

        return 0;
    }
}
