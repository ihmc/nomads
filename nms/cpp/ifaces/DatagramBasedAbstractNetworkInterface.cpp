/*
 * DatagramBasedAbstractNetworkInterface.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#include "DatagramBasedAbstractNetworkInterface.h"

#include "Logger.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

AbstractDatagramNetworkInterface::AbstractDatagramNetworkInterface (PROPAGATION_MODE mode, bool bAsyncTransmission)
    : AbstractNetworkInterface (bAsyncTransmission),
     _mode (mode),
     _bIsAvailable (false),
     _pDatagramSocket (NULL),
     _ui8Type (NET_IF),
     _iBufSize (65535)
{
}

AbstractDatagramNetworkInterface::~AbstractDatagramNetworkInterface (void)
{
    if (_pDatagramSocket != NULL) {
        _pDatagramSocket->close ();
    }
    delete _pDatagramSocket;
    _pDatagramSocket = NULL;
}

int AbstractDatagramNetworkInterface::init (uint16 ui16Port, const char *pszBindingInterfaceSpec,
                                            NetworkInterfaceManagerListener *pNMSParent,
                                            bool bReceiveOnly, bool bSendOnly,
                                            const char *pszPropagationAddr, uint8 ui8McastTTL)
{
    _mBind.lock();
    if (AbstractNetworkInterface::init (ui16Port, pszBindingInterfaceSpec, pNMSParent, bReceiveOnly,
                                        bSendOnly, pszPropagationAddr, ui8McastTTL) < 0) {
        _mBind.unlock();
        return -1;
    }
    if (pNMSParent == NULL) {
        _mBind.unlock();
        return -2;
    }

    _bIsAvailable = bind() == 0;
    _mBind.unlock();
    return 0;
}

int AbstractDatagramNetworkInterface::rebind (void)
{
    _mBind.lock();

    // Reconnect the socket if it's disconnected
    int rc = _bIsAvailable ? 0 : bind();

    // If the socket is connected, check whether the receive buffer size is
    // correctly set
    if (rc == 0 && _pDatagramSocket != NULL) {
        const int iBufSize = _pDatagramSocket->getReceiveBufferSize();
        if (iBufSize >= 0) {
            if (iBufSize != _iBufSize) {
                _pDatagramSocket->setSendBufferSize (_iBufSize);
            }
        }
    }
    _mBind.unlock();
    return (rc == 0 ? 0 : -2);
}

uint8 AbstractDatagramNetworkInterface::getMode (void)
{
    return _mode;
}

uint16 AbstractDatagramNetworkInterface::getMTU (void)
{
    if (_pDatagramSocket != NULL) {
        return _pDatagramSocket->getMTU();
    }
    return 0;
}

const char * AbstractDatagramNetworkInterface::getNetworkAddr (void)
{
    const char * const pszMethodName = "AbstractDatagramNetworkInterface::getNetworkAddr";

    // Check to see whether _networkAddr has not been set yet
    // This could happen when using the ProxyDatagramSocket, where the actual
    // address may not be available at initialization time
    if ((_networkAddr.length () <= 0) && (_pDatagramSocket != NULL)) {
        InetAddr addr = _pDatagramSocket->getLocalAddr();
        if (0 != stricmp (addr.getIPAsString(), IN_ADDR_ANY_STR)) {
            _networkAddr = addr.getIPAsString();
            checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "set the local "
                "network address to %s\n", _networkAddr.c_str());
        }
    }

    return _networkAddr;
}

bool AbstractDatagramNetworkInterface::isAvailable (void)
{
    //_mBind.lock();
    bool bRet = _bIsAvailable;
    //_mBind.unlock();
    return bRet;
}

void AbstractDatagramNetworkInterface::setDisconnected (void)
{
    if (_mBind.tryLock() == Mutex::RC_Ok) {
        // If the lock has been acquired by another thread (the thread that sends),
        // that thread will also realize that the socket is no longer connected,
        // and it will take action and attempt to fix the problem and it will
        // set _bIsAvailable to the proper value.
        _bIsAvailable = false;
        _mBind.unlock();
    }
}

int AbstractDatagramNetworkInterface::getReceiveBufferSize (void)
{
    if (_pDatagramSocket == NULL) {
        return 0;
    }
    return _pDatagramSocket->getReceiveBufferSize();
}

int AbstractDatagramNetworkInterface::setReceiveBufferSize (int iBufSize)
{
    if (iBufSize < 0) {
        return -1;
    }
    _iBufSize = iBufSize;
    if (_pDatagramSocket == NULL) {
        return -1;
    }
    return _pDatagramSocket->setReceiveBufferSize (iBufSize);
}

uint32 AbstractDatagramNetworkInterface::getTransmitRateLimit (const char *pszDestinationAddr)
{
    if (_pDatagramSocket == NULL) {
        return 0;
    }
    return _pDatagramSocket->getTransmitRateLimit (pszDestinationAddr);
}

uint32 AbstractDatagramNetworkInterface::getTransmitRateLimit()
{
    if (_pDatagramSocket == NULL) {
        return 0;
    }
    return _pDatagramSocket->getTransmitRateLimit();
}

int AbstractDatagramNetworkInterface::setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit)
{
    if (_pDatagramSocket == NULL) {
        return 0;
    }
    return _pDatagramSocket->setTransmitRateLimit (pszDestinationAddr, ui32RateLimit);
}

int AbstractDatagramNetworkInterface::setTransmitRateLimit (uint32 ui32RateLimit)
{
    if (_pDatagramSocket == NULL) {
        return 0;
    }
    return _pDatagramSocket->setTransmitRateLimit (ui32RateLimit);
}

int AbstractDatagramNetworkInterface::receive (void *pBuf, int iBufSize, InetAddr *pIncomingIfaceByAddr,
                                               InetAddr *pRemoteAddr)
{
    _mBind.lock();
    if ((!_bIsAvailable) || (_pDatagramSocket == NULL)) {
        _bIsAvailable = false;
        _mBind.unlock();
        return -1;
    }
    _mBind.unlock();

    _pDatagramSocket->setTimeout (1000); // it is necessary to set a timeout,
    // otherwise, if the interface goes down,
    // _pDatagramSocket will always wait,
    // until the code tries to rebind.
    // The code to rebind follows the receive,
    // therefore it would wait indefinitely.

    if (pIncomingIfaceByAddr != NULL) {
        *pIncomingIfaceByAddr = _pDatagramSocket->getLocalAddr();
    }

    int rc = 0;
    if (pIncomingIfaceByAddr != NULL && pIncomingIfaceByAddr->getIPAddress() == INADDR_ANY) {
#ifdef UNIX
        if (_pDatagramSocket->pktInfoEnabled ()) {
            int iIncomingIfaceIdx = 0;
            int iRcvdBytes = _pDatagramSocket->receive (pBuf, iBufSize, pRemoteAddr, iIncomingIfaceIdx);
            if (iRcvdBytes > 0) {
                NICInfo **ppNICS = NetUtils::getNICsInfo (false, false);
                if (ppNICS != NULL) {
                    for (unsigned int i = 0; ppNICS[i] != NULL; i++) {
                        if (iIncomingIfaceIdx < 0) {
                            rc = iRcvdBytes;
                            break;
                        }
                        else if (ppNICS[i]->uiIndex == ((unsigned int) iIncomingIfaceIdx)) {
                            *pIncomingIfaceByAddr = ppNICS[i]->getIPAddr ();
                            rc = iRcvdBytes;
                            break;
                        }
                    }
                    NetUtils::freeNICsInfo (ppNICS);
                }
            }
        }
#endif
    }
    else {
        rc = _pDatagramSocket->receive (pBuf, iBufSize, pRemoteAddr);
    }

    if ((rc < 0) && (_mBind.tryLock () == Mutex::RC_Ok)) {
        // if the lock is not acquired it means that either sendInternal()
        // or rebind() have acquired it. Both methods will set _bIsAvailable to
        // the proper value
        _bIsAvailable = false;
        _mBind.unlock();
    }

    return rc;
}

bool AbstractDatagramNetworkInterface::clearToSend (void)
{
    if (_pDatagramSocket != NULL) {
        return _pDatagramSocket->clearToSend();
    }
    return false;
}

uint8 AbstractDatagramNetworkInterface::getType (void)
{
    return _ui8Type;
}

int AbstractDatagramNetworkInterface::sendMessageNoBuffering (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *pszHints)
{
    const char * const pszMethodName = "AbstractDatagramNetworkInterface::sendMessageNoBuffering";

    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
        "sending to socket message of length %u (%u  of metadata)\n",
        pNetMsg->getMsgLen(), pNetMsg->getMetaDataLen());

    if (pNetMsg == NULL) {
        return -1;
    }

    // Make sure the target address is set to the right value
    NetworkMessage *pModifiedNetMsg = (NetworkMessage*) pNetMsg;
    pModifiedNetMsg->setTargetAddress (ui32IPAddr);

    _mBind.lock();

    if ((!_bIsAvailable) || (_pDatagramSocket == NULL)) {
        // The socket is not connected - the message can't be sent
        _mBind.unlock();
        return -2;
    }

    // Send the message
    int rc = _pDatagramSocket->sendTo (ui32IPAddr, _ui16Port, pModifiedNetMsg->getBuf(),
                                       pModifiedNetMsg->getLength(), pszHints);
    if (rc > 0) {
        _pNMSParent->messageSent (pNetMsg, _bindingInterfaceSpec);
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                        "packet sent %lu:%d\n", pNetMsg->getDestinationAddr(),
                        (int) _ui16Port);
    }
    else if (rc < 0) {
        // error - the socket disconnected
        _bIsAvailable = false;
    }

    _mBind.unlock();
    return rc;
}

