/*
 * NetworkInterface.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#include "NetworkInterface.h"

#include "NetworkMessageV2.h"
#include "MessageFactory.h"
#include "ManycastNetworkMessageReceiver.h"
#include "MulticastUDPDatagramSocket.h"
#include "NetUtils.h"
#include "NetworkMessageService.h"
#include "NLFLib.h"
#include "StringTokenizer.h"
#include "UInt32Hashtable.h"

#if !defined (ANDROID)
    #include "ProxyDatagramSocket.h"
#include "DatagramSocket.h"
#endif

#include <stdlib.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

const char * NetworkInterface::IN_ADDR_ANY_STR = "0.0.0.0";

uint8 NetworkInterface::NET_IF = 0x00;

NetworkInterface::NetworkInterface (bool bAsyncTransmission)
    : _ui8Type (NET_IF), _bAsyncTransmission (bAsyncTransmission),
      _cvTransmissionQueue (&_mTransmissionQueue)
{    
    construct();
}

NetworkInterface::NetworkInterface (uint8 ui8Type, bool bAsyncTransmission)
    : _ui8Type (ui8Type), _bAsyncTransmission (bAsyncTransmission),
      _cvTransmissionQueue (&_mTransmissionQueue)
{
    construct();
}

void NetworkInterface::construct (void)
{
    _ui16Port = NetworkMessageService::DEFAULT_PORT;
    _pDatagramSocket = NULL;
    _ui32LinkCapacity = 0;
    _ui32TransmissionQueueMaxLength = 300;
    _ui32TransmissionQueueMaxLengthCap = 300;
    _bAutoResizeQueue = false;
    _ui32MaxTimeInQueue = 3000;
    _ui8McastTTL = 1;
    _pNMSParent = NULL;
    _pReceiver = NULL;
    _bIsConnected =  false;
    _ui16BufSize = 65535;
}

NetworkInterface::~NetworkInterface (void)
{
    if (_pReceiver != NULL) {
        checkAndLogMsg ("NetworkInterface::~NetworkInterface",
                        Logger::L_HighDetailDebug, "terminating receiver thread\n");
        _pReceiver->requestTerminationAndWait(); 
    }
    if (_pDatagramSocket != NULL) {
        _pDatagramSocket->close();
    }
    _pReceiver->requestTerminationAndWait();
    delete _pReceiver;
    _pReceiver = NULL;
    delete _pDatagramSocket;
    _pDatagramSocket = NULL;
}

int NetworkInterface::init (uint16 ui16Port, const char *pszBindingInterfaceSpec,
                            NetworkMessageService *pNMSParent,
                            bool bReceiveOnly , bool bSendOnly,
                            const char *pszPropagationAddr, uint8 ui8McastTTL)
{
    if ((pszBindingInterfaceSpec == NULL) || (pNMSParent == NULL)) {
        return -1;
    }

    _mBind.lock();

    _ui16Port = ui16Port;
    _bReceiveOnly = bReceiveOnly;
    _bSendOnly = bSendOnly;
    _bindingInterfaceSpec = pszBindingInterfaceSpec;
    _pNMSParent = pNMSParent;
    _ui8Mode = pNMSParent->getPropagationMode();
    _ui8McastTTL = ui8McastTTL;
    _defaultPropagationAddr = pszPropagationAddr;

    _bIsConnected = bind() == 0;

    if (inet_addr (_bindingInterfaceSpec) == INADDR_ANY) {
        _pReceiver = new ManycastNetworkMessageReceiver (_pNMSParent, this, !_bSendOnly, NetUtils::getNICsInfo (false, false));
    }
    else {
        _pReceiver = new NetworkMessageReceiver (_pNMSParent, this, !_bSendOnly);
    }

    if (_bAsyncTransmission) {
        checkAndLogMsg ("NetworkInterface::init", Logger::L_Info,
                        "enabling asynchronous transmission\n");
        _ostTransmissionThread.start (transmissionThread, this);
    }

    _mBind.unlock();
    return 0;
}

int NetworkInterface::bind()
{
    const char * const pszMethodName = "NetworkInterface::bind";

    if (_bindingInterfaceSpec == IN_ADDR_ANY_STR) {   // "0.0.0.0"
        _networkAddr = IN_ADDR_ANY_STR;
        _broadcastAddr = IN_ADDR_ANY_STR;
        _netmask = "255.255.255.255";
    }
    else if (!_bindingInterfaceSpec.startsWith ("pds://")) {
        NICInfo *pNICInfo = NetUtils::getNICInfo (_bindingInterfaceSpec.c_str());
        if (pNICInfo != NULL) {
            _networkAddr = pNICInfo->getIPAddrAsString();
            _broadcastAddr = pNICInfo->getBroadcastAddrAsString();
            _netmask = pNICInfo->getNetmaskAsString();
            delete pNICInfo;
        }
        else {
            return -1;
        }
    }

    if (_bindingInterfaceSpec.startsWith ("pds://")) {
        // This is an address for the ProxyDatagramSocket
        #if defined (ANDROID)
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "ProxyDatagramSocket not supported on Android\n");
            return -1;
        #else
            int rc;
            StringTokenizer st (((const char*)_bindingInterfaceSpec) + 6, ':');
            const char *pszProxyAddr = st.getNextToken();
            const char *pszProxyPort = st.getNextToken();
            if ((pszProxyAddr == NULL) || (pszProxyPort == NULL)) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                "proxy address and/or proxy port not specified for ProxyDatagramSocket address\n");
                return -2;
            }
            if (_pDatagramSocket == NULL) {
                ProxyDatagramSocket *pDatagramSocket = new ProxyDatagramSocket();
                if ((rc = pDatagramSocket->init (pszProxyAddr, (uint16) atoui32 (pszProxyPort), _ui16Port) < 0)) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                    "init() for ProxyDatagramSocket failed with rc = %d\n", rc);
                    return -3;
                }
                _pDatagramSocket = pDatagramSocket;
            }
        #endif
    }
    else {
        // This is a regular local socket
        uint32 ui32ListenAddr = inet_addr (_bindingInterfaceSpec.c_str());
        MulticastUDPDatagramSocket *pDatagramSocket = (_pDatagramSocket == NULL ?
                                                      #if defined UNIX
                                                          new MulticastUDPDatagramSocket (ui32ListenAddr == INADDR_ANY) :
                                                      #else
                                                          new MulticastUDPDatagramSocket() :
                                                      #endif
                                                      (MulticastUDPDatagramSocket *)_pDatagramSocket);
        if (pDatagramSocket->init (_ui16Port, ui32ListenAddr) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "UDPDatagramSocket init failed\n");
            return -4;
        }

        if (_pNMSParent->getPropagationMode() == NetworkMessageService::MULTICAST) {
            if (!_bSendOnly) {
                checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                                "interface %s joining group %s\n",
                                _bindingInterfaceSpec.c_str(), (const char*) _defaultPropagationAddr);
                pDatagramSocket->joinGroup (inet_addr (_defaultPropagationAddr),
                                            inet_addr (_bindingInterfaceSpec.c_str()));
            }

            checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                            "interface %s setting TTL to %d\n",
                            _bindingInterfaceSpec.c_str(), (int) _ui8McastTTL);
            pDatagramSocket->setTTL (_ui8McastTTL);
        }
        _pDatagramSocket = pDatagramSocket;
    }

    _bIsConnected = true;
    return 0;
}

int NetworkInterface::rebind (void)
{
    _mBind.lock();

    // Reconnect the socket if it's disconnected
    int rc = _bIsConnected ? 0 : bind();

    // If the socket is connected, check whether the receive buffer size is
    // correctly set
    if (rc == 0 && _pDatagramSocket != NULL) {
        int iBufSize = _pDatagramSocket->getReceiveBufferSize();
        if (iBufSize >= 0) {
            if (((uint16)iBufSize) != _ui16BufSize) {
                _pDatagramSocket->setSendBufferSize (_ui16BufSize);
            }
        }
    }
    _mBind.unlock();
    return (rc == 0 ? 0 : -2);
}

int NetworkInterface::start (void)
{
    if (!_bSendOnly) {
        _pReceiver->start();
    }
    return 0;
}

int NetworkInterface::stop (void)
{
    if (_pReceiver->isRunning()) {
        _pReceiver->requestTerminationAndWait();
    }
    return 0;
}

uint16 NetworkInterface::getMTU (void)
{
    if (_pDatagramSocket != NULL) {
        return _pDatagramSocket->getMTU();
    }
    return 0;
}

const char * NetworkInterface::getBindingInterfaceSpec (void) const
{
    return _bindingInterfaceSpec;
}

const char * NetworkInterface::getNetworkAddr (void)
{
    // Check to see whether _networkAddr has not been set yet
    // This could happen when using the ProxyDatagramSocket, where the actual
    // address may not be available at initialization time
    if ((_networkAddr.length() <= 0) && (_pDatagramSocket != NULL)) {
        InetAddr addr = _pDatagramSocket->getLocalAddr();
        if (0 != stricmp (addr.getIPAsString(), IN_ADDR_ANY_STR)) {
            _networkAddr = addr.getIPAsString();
            checkAndLogMsg ("NetworkInterface::getNetworkAddr", Logger::L_LowDetailDebug,
                            "set the local network address to %s\n", (const char*) _networkAddr);
        }
    }

    return _networkAddr;
}

const char * NetworkInterface::getBroadcastAddr (void) const
{
    return _broadcastAddr;
}

const char * NetworkInterface::getNetmask (void) const
{
    return _netmask;
}

uint16 NetworkInterface::getPort (void)
{
    return _ui16Port;
}

const char * NetworkInterface::getPropagatonAddr (void)
{
    return _defaultPropagationAddr;
}

NetworkMessageReceiver * NetworkInterface::getReceiver (void)
{
    return _pReceiver;
}

uint8 NetworkInterface::getTTL (void)
{
    return _ui8McastTTL;
}

bool NetworkInterface::isConnected()
{
    //_mBind.lock();
    bool bRet = _bIsConnected;
    //_mBind.unlock();
    return bRet;
}

bool NetworkInterface::boundToWildcardAddr (void)
{
    return ((_bindingInterfaceSpec == IN_ADDR_ANY_STR) == 1);
}

void NetworkInterface::setDisconnected (void)
{
    if (_mBind.tryLock() == Mutex::RC_Ok) {
        // If the lock has been acquired by another thread (the thread that sends),
        // that thread will also realize that the socket is no longer connected,
        // and it will take action and attempt to fix the problem and it will
        // set _bIsConnected to the proper value.
        _bIsConnected = false;
        _mBind.unlock();
    }
}

int NetworkInterface::getReceiveBufferSize (void)
{
    if (_pDatagramSocket == NULL) {
        return 0;
    }
    return _pDatagramSocket->getReceiveBufferSize();
}

int NetworkInterface::setReceiveBufferSize (uint16 ui16BufSize)
{
    _ui16BufSize = ui16BufSize;
    if (_pDatagramSocket == NULL) {
        return -1;
    }
    return _pDatagramSocket->setReceiveBufferSize ((int) ui16BufSize);
}

uint32 NetworkInterface::getTransmitRateLimit (const char *pszDestinationAddr)
{
    if (_pDatagramSocket == NULL) {
        return 0;
    }
    return _pDatagramSocket->getTransmitRateLimit (pszDestinationAddr);
}

uint32 NetworkInterface::getTransmitRateLimit()
{
    if (_pDatagramSocket == NULL) {
        return 0;
    }
    return _pDatagramSocket->getTransmitRateLimit();
}

int NetworkInterface::setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit)
{
    if (_pDatagramSocket == NULL) {
        return 0;
    }
    return _pDatagramSocket->setTransmitRateLimit (pszDestinationAddr, ui32RateLimit);
}

int NetworkInterface::setTransmitRateLimit (uint32 ui32RateLimit)
{
    if (_pDatagramSocket == NULL) {
        return 0;
    }
    return _pDatagramSocket->setTransmitRateLimit (ui32RateLimit);
}

void NetworkInterface::setReceiveRateSampleInterval (uint32 ui32IntervalInMS)
{
    //need to enable the receive rate estimation before being able to use it
    _pReceiver->enableReceiveRateEstimation();
    //let's assume that if someone enables it, there will be no need to disable it again during runtime
    _pReceiver->setReceiveRateSampleInterval(ui32IntervalInMS);
}

uint32 NetworkInterface::getReceiveRate (void)
{
    return _pReceiver->getReceiveRate();
}

uint32 NetworkInterface::getTransmissionQueueSize (void)
{
    uint32 ui32QueueSize = 0;
    if (_bAsyncTransmission) {
        _mTransmissionQueue.lock();
        ui32QueueSize = _transmissionQueue.sizeOfQueue();
        _cvTransmissionQueue.notifyAll();
        _mTransmissionQueue.unlock();
    }
    return ui32QueueSize;
}

uint8 NetworkInterface::getRescaledTransmissionQueueSize (void)
{
    uint8 ui8RescaledQueueSize = 0;
    if (_bAsyncTransmission) {
        _mTransmissionQueue.lock();
        uint32 ui32QueueSize = _transmissionQueue.sizeOfQueue();
        uint32 ui32MaxSize = _ui32TransmissionQueueMaxLength;
        _cvTransmissionQueue.notifyAll();
        _mTransmissionQueue.unlock();
        if (ui32MaxSize > 0) {
            ui8RescaledQueueSize = ui32QueueSize * 255 / ui32MaxSize;
        }
        else {
            ui8RescaledQueueSize = minimum (ui32QueueSize, (uint32) 255);
        }
    }
    return ui8RescaledQueueSize;
}

void NetworkInterface::setTransmissionQueueMaxSize (uint32 ui32MaxSize)
{
    _mTransmissionQueue.lock();
    _ui32TransmissionQueueMaxLength = ui32MaxSize;
    _ui32TransmissionQueueMaxLengthCap = ui32MaxSize;
    _cvTransmissionQueue.notifyAll();
    _mTransmissionQueue.unlock();
}

uint32 NetworkInterface::getTransmissionQueueMaxSize (void)
{
    return _ui32TransmissionQueueMaxLength;
}

uint32 NetworkInterface::getLinkCapacity()
{
    return _ui32LinkCapacity;
}

void NetworkInterface::setLinkCapacity (uint32 ui32Capacity)
{
    _ui32LinkCapacity = ui32Capacity;
}

uint32 NetworkInterface::getAutoResizeQueue (void)
{
    return _bAutoResizeQueue;
}

void NetworkInterface::setAutoResizeQueue (bool bEnable, uint32 ui32MaxTimeInQueue)
{
    _bAutoResizeQueue = bEnable;
    _ui32MaxTimeInQueue = maximum (ui32MaxTimeInQueue, (uint32) 100);
}

int NetworkInterface::receive (void *pBuf, int iBufSize, InetAddr *pIncomingIfaceByAddr,
                               InetAddr *pRemoteAddr)
{
    _mBind.lock();
    if (!_bIsConnected || _pDatagramSocket == NULL) {
        _bIsConnected = false;
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
            if (_pDatagramSocket->pktInfoEnabled()) {
                int iIncomingIfaceIdx = 0;
                int iRcvdBytes = _pDatagramSocket->receive (pBuf, iBufSize, pRemoteAddr, iIncomingIfaceIdx);
                if (iRcvdBytes > 0) {
                    NICInfo **ppNICS = NetUtils::getNICsInfo (false, false);
                    if (ppNICS !=  NULL) {
                        for (unsigned int i = 0; ppNICS[i] != NULL; i++) {
                            if (ppNICS[i]->uiIndex == iIncomingIfaceIdx) {
                                *pIncomingIfaceByAddr = ppNICS[i]->getIPAddr();
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

    if (rc < 0 && _mBind.tryLock() == Mutex::RC_Ok) {
        // if the lock is not acquired it means that either sendInternal()
        // or rebind() have acquired it. Both methods will set _bIsConnected to
        // the proper value
        _bIsConnected = false;
        _mBind.unlock();
    }

    return rc;
}

int NetworkInterface::sendMessage (const NetworkMessage *pNetMsg, bool bExpedited, const char *pszHints)
{
    return sendMessage (pNetMsg, inet_addr (_defaultPropagationAddr), bExpedited, pszHints);
}

int NetworkInterface::sendMessage (const NetworkMessage *pNetMsg, const char *pszIPAddr, bool bExpedited, const char *pszHints)
{
    return sendMessage (pNetMsg, inet_addr (pszIPAddr), bExpedited, pszHints);
}

int NetworkInterface::sendMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, bool bExpedited, const char *pszHints)
{
    if (_bAsyncTransmission) {
        QueuedMessage *pQMsg = new QueuedMessage();
        pQMsg->pMsg = MessageFactory::createNetworkMessageFromMessage (*pNetMsg, pNetMsg->getVersion());
        pQMsg->ui32DestinationAddr = ui32IPAddr;
        pQMsg->hints = pszHints;
        _mTransmissionQueue.lock();
        if (bExpedited) {
            _expeditedTransmissionQueue.enqueue (pQMsg);
            const char *pszBindingInterfaceSpec = _bindingInterfaceSpec;
            checkAndLogMsg ("NetworkInterface::sendMessage", Logger::L_Info,
                            "on interface %s expedited queue size is: %lu\n",
                            (pszBindingInterfaceSpec != NULL ? pszBindingInterfaceSpec : ""),
                            _expeditedTransmissionQueue.sizeOfQueue());
        }
        else {
            bool bEnqueued = false;
            while (!bEnqueued) {
                // Check if the message can be enqueued
                if ((_ui32TransmissionQueueMaxLength == 0) || (_transmissionQueue.sizeOfQueue() < _ui32TransmissionQueueMaxLength)) {
                    _transmissionQueue.enqueue (pQMsg);
                    bEnqueued = true;
                }
                else {
                    _cvTransmissionQueue.wait (1000);
                }
            }

            const char *pszBindingInterfaceSpec = _bindingInterfaceSpec;
            checkAndLogMsg ("NetworkInterface::sendMessage", Logger::L_Info,
                            "on interface %s queue size is: %lu\n",
                            (pszBindingInterfaceSpec != NULL ? pszBindingInterfaceSpec : ""),
                            _transmissionQueue.sizeOfQueue());
        }
        _cvTransmissionQueue.notifyAll();
        _mTransmissionQueue.unlock();
        return 0;
    }
    else {
        return sendMessageInternal (pNetMsg, ui32IPAddr, pszHints);
    }
}

bool NetworkInterface::clearToSend (void)
{
    if (_pDatagramSocket != NULL) {
        return _pDatagramSocket->clearToSend();
    }
    return false;
}

void NetworkInterface::autoResizeQueue (uint16 ui16MsgSize)
{
    /*
     * adjust the size for a maximum delay of _ui32MaxTimeInQueue msec with messages of size ui16MsgSize Bytes
     */
    if (_pDatagramSocket == NULL) {
        return;
    }
    uint32 ui32MaxQueueLength = _ui32MaxTimeInQueue * _pDatagramSocket->getTransmitRateLimit() / (ui16MsgSize * 1000);
    if (ui32MaxQueueLength < _ui32TransmissionQueueMaxLength) {
        //if the queue max size is too big to guarantee a delay lower than _ui32MaxTimeInQueue, decrease the queue max size
        _ui32TransmissionQueueMaxLength = maximum (_ui32TransmissionQueueMaxLength - 1, (uint32) 20);
    }
    else {
        _ui32TransmissionQueueMaxLength++;
        if ((_ui32TransmissionQueueMaxLengthCap > 0) && (_ui32TransmissionQueueMaxLength > _ui32TransmissionQueueMaxLengthCap)) {
            _ui32TransmissionQueueMaxLength =  _ui32TransmissionQueueMaxLengthCap;
        }
    }
}

int NetworkInterface::sendMessageInternal (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *pszHints)
{
    const char * const pszMethodName = "NetworkInterface::sendMessageInternal";

    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                    "sending to socket\n");

    if (pNetMsg == NULL) {
        return -1;
    }

    // Make sure the target address is set to the right value
    NetworkMessage *pModifiedNetMsg = (NetworkMessage*) pNetMsg;
    pModifiedNetMsg->setTargetAddress (ui32IPAddr);

    _mBind.lock();

    if (!_bIsConnected || _pDatagramSocket == NULL) {
        // The socket is not connected - the message can't be sent
        _mBind.unlock();
        return -2;
    }

    // Send the message
    int rc = _pDatagramSocket->sendTo (ui32IPAddr, _ui16Port, pModifiedNetMsg->getBuf(),
                                       pModifiedNetMsg->getLength(), pszHints);
    if (rc > 0) {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                        "packet sent %lu:%d\n", pNetMsg->getTargetAddress(),
                        (int) _ui16Port);
    }
    else if (rc < 0) {
        // error - the socket disconnected
        _bIsConnected = false;
    }

    _mBind.unlock();
    return rc;
}

void NetworkInterface::transmissionThread (void *pArg)
{
    NetworkInterface *pThis = (NetworkInterface *) pArg;
    QueuedMessage *pQMsg;
    while (true) {      /*!!*/ // Should add some termination condition here, like for a managed thread
        pThis->_mTransmissionQueue.lock();
        bool bDequeued = false;
        while (!bDequeued) {
            if (NULL != (pQMsg = (QueuedMessage*) pThis->_expeditedTransmissionQueue.dequeue())) {
                bDequeued = true;
            }
            else if (NULL != (pQMsg = (QueuedMessage*) pThis->_transmissionQueue.dequeue())) {
                bDequeued = true;
            }
            else {
                pThis->_cvTransmissionQueue.wait (1000);
            }
        }
        if (pThis->_bAutoResizeQueue) {
            pThis->autoResizeQueue (pQMsg->pMsg->getLength());
        }
        pThis->_cvTransmissionQueue.notifyAll();
        pThis->_mTransmissionQueue.unlock();
        //if the message requires queue length, write the updated value for it
        if (pQMsg->pMsg->getVersion() == 2) {
            uint8 ui8QueueLength = pThis->getRescaledTransmissionQueueSize();
            ((NetworkMessageV2*)pQMsg->pMsg)->setQueueLength (ui8QueueLength);
        }
        int64 i64PauseStartTime = 0;
        while (!pThis->clearToSend()) {
            if (i64PauseStartTime == 0) {
                i64PauseStartTime = getTimeInMilliseconds();
                checkAndLogMsg ("NetworkInterface::transmissionThread", Logger::L_Info,
                                "about to pause transmission because it is not clear to send\n");
            }
            sleepForMilliseconds (10);
        }
        if (i64PauseStartTime != 0) {
            checkAndLogMsg ("NetworkInterface::transmissionThread", Logger::L_Info,
                            "paused transmission for %lu ms because it was not clear to send\n",
                            (uint32) (getTimeInMilliseconds() - i64PauseStartTime));
        }

        int rc = pThis->sendMessageInternal (pQMsg->pMsg, pQMsg->ui32DestinationAddr, pQMsg->hints);
        if (rc < 0) {
            checkAndLogMsg ("NetworkInterface::transmissionThread", Logger::L_LowDetailDebug,
                            "sendMessageInternal() failed with rc = %d\n", rc);
        }
        delete pQMsg->pMsg;
        delete pQMsg;
    };
}
