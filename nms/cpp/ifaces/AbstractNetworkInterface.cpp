/*
 * AbstractNetworkInterface.cpp
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

#include "AbstractNetworkInterface.h"

#include "ManycastNetworkMessageReceiver.h"
#include "MessageFactory.h"
#include "NetworkInterface.h"
#include "NetworkMessageV2.h"

#include "Logger.h"
#include "NLFLib.h"

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

LinkCapacityTrait::LinkCapacityTrait (void)
    : _ui32LinkCapacity (0U)
{
}

LinkCapacityTrait::~LinkCapacityTrait (void)
{
}

void LinkCapacityTrait::setLinkCapacity (uint32 ui32Capacity)
{
    _ui32LinkCapacity = ui32Capacity;
}

uint32 LinkCapacityTrait::getLinkCapacity (void)
{
    return _ui32LinkCapacity;
}

//-------------------------------------------------------------------

AsyncTranmissionTrait::AsyncTranmissionTrait (NetworkInterface *pNetIf, bool bAsyncTransmission)
    : _bAsyncTransmission (bAsyncTransmission),
      _bAutoResizeQueue (false),
      _pNetIf (pNetIf),
      _ui32MaxTimeInQueue (3000),
      _ui32TransmissionQueueMaxLength (300),
      _ui32TransmissionQueueMaxLengthCap (300),
      _cvTransmissionQueue (&_mTransmissionQueue)
{
}

namespace NOMADSUtil
{
    template<class T>
    void clearQueue (FIFOQueue &queue)
    {
        for (void *ptmp; (ptmp = queue.dequeue()) != NULL;) {
            delete static_cast<T*>(ptmp);
        }
    }
}

AsyncTranmissionTrait::~AsyncTranmissionTrait (void)
{
    if (isRunning()) {
        requestTerminationAndWait();
    }
    clearQueue<QueuedMessage> (_expeditedTransmissionQueue);
    clearQueue<QueuedMessage> (_transmissionQueue);
}

void AsyncTranmissionTrait::run (void)
{
    const char *pszMethodName = "AsyncTranmissionTrait::run";
    QueuedMessage *pQMsg = NULL;
    started();
    while (!terminationRequested()) {
        _mTransmissionQueue.lock();
        bool bDequeued = false;
        while (!bDequeued) {
            if (NULL != (pQMsg = (QueuedMessage*) _expeditedTransmissionQueue.dequeue())) {
                bDequeued = true;
            }
            else if (NULL != (pQMsg = (QueuedMessage*) _transmissionQueue.dequeue())) {
                bDequeued = true;
            }
            else {
                _cvTransmissionQueue.wait (1000);
            }
        }
        if (_bAutoResizeQueue) {
            autoResizeQueue (pQMsg->pMsg->getLength());
        }
        _cvTransmissionQueue.notifyAll();
        _mTransmissionQueue.unlock ();
        // if the message requires queue length, write the updated value for it
        if (pQMsg->pMsg->getVersion() == 2) {
            uint8 ui8QueueLength = getRescaledTransmissionQueueSize();
            static_cast<NetworkMessageV2*>(pQMsg->pMsg)->setQueueLength (ui8QueueLength);
        }
        int64 i64PauseStartTime = 0;
        while (!_pNetIf->clearToSend()) {
            if (i64PauseStartTime == 0) {
                i64PauseStartTime = getTimeInMilliseconds ();
                checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "about to pause transmission because it is not clear to send\n");
            }
            sleepForMilliseconds (10);
        }
        if (i64PauseStartTime != 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                "paused transmission for %lu ms because it was not clear to send\n",
                static_cast<uint32>(getTimeInMilliseconds() - i64PauseStartTime));
        }

        int rc = _pNetIf->sendMessageNoBuffering (pQMsg->pMsg, pQMsg->ui32DestinationAddr, pQMsg->hints);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "failed with rc = %d\n", rc);
        }
        delete pQMsg->pMsg;
        delete pQMsg;
    }
    terminating();
}

bool AsyncTranmissionTrait::asynchronousTransmission (void)
{
    return _bAsyncTransmission;
}

uint32 AsyncTranmissionTrait::getTransmissionQueueSize (void)
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

uint8 AsyncTranmissionTrait::getRescaledTransmissionQueueSize (void)
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

void AsyncTranmissionTrait::setTransmissionQueueMaxSize (uint32 ui32MaxSize)
{
    _mTransmissionQueue.lock();
    _ui32TransmissionQueueMaxLength = ui32MaxSize;
    _ui32TransmissionQueueMaxLengthCap = ui32MaxSize;
    _cvTransmissionQueue.notifyAll();
    _mTransmissionQueue.unlock();
}

uint32 AsyncTranmissionTrait::getTransmissionQueueMaxSize (void)
{
    return _ui32TransmissionQueueMaxLength;
}

uint32 AsyncTranmissionTrait::getAutoResizeQueue (void)
{
    return _bAutoResizeQueue;
}

void AsyncTranmissionTrait::setAutoResizeQueue (bool bEnable, uint32 ui32MaxTimeInQueue)
{
    _bAutoResizeQueue = bEnable;
    _ui32MaxTimeInQueue = maximum (ui32MaxTimeInQueue, (uint32) 100);
}

void AsyncTranmissionTrait::autoResizeQueue (uint16 ui16MsgSize)
{
    // adjust the size for a maximum delay of _ui32MaxTimeInQueue msec with messages of size ui16MsgSize Bytes
    if (_pNetIf == NULL) {
        return;
    }
    uint32 ui32MaxQueueLength = _ui32MaxTimeInQueue * _pNetIf->getTransmitRateLimit() / (ui16MsgSize * 1000);
    if (ui32MaxQueueLength < _ui32TransmissionQueueMaxLength) {
        //if the queue max size is too big to guarantee a delay lower than _ui32MaxTimeInQueue, decrease the queue max size
        _ui32TransmissionQueueMaxLength = maximum (_ui32TransmissionQueueMaxLength - 1, (uint32) 20);
    }
    else {
        _ui32TransmissionQueueMaxLength++;
        if ((_ui32TransmissionQueueMaxLengthCap > 0) && (_ui32TransmissionQueueMaxLength > _ui32TransmissionQueueMaxLengthCap)) {
            _ui32TransmissionQueueMaxLength = _ui32TransmissionQueueMaxLengthCap;
        }
    }
}

bool AsyncTranmissionTrait::bufferOutgoingMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr,
                                                     bool bExpedited, const char *pszHints,
                                                     const char *pszBindingInterfaceSpec)
{
    const char *pszMethodName = "AsyncTranmissionTrait::bufferOutgoingMessage";
    if (!_bAsyncTransmission) {
        return false;
    }

    QueuedMessage *pQMsg = new QueuedMessage();
    pQMsg->pMsg = MessageFactory::createNetworkMessageFromMessage (*pNetMsg, pNetMsg->getVersion());
    pQMsg->ui32DestinationAddr = ui32IPAddr;
    pQMsg->hints = pszHints;
    _mTransmissionQueue.lock();
    if (bExpedited) {
        _expeditedTransmissionQueue.enqueue (pQMsg);
        checkAndLogMsg (pszMethodName, Logger::L_Info,  "on interface %s expedited queue size is: %lu\n",
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
        checkAndLogMsg (pszMethodName, Logger::L_Info, "on interface %s queue size is: %lu\n",
                        (pszBindingInterfaceSpec != NULL ? pszBindingInterfaceSpec : ""),
                        _transmissionQueue.sizeOfQueue());
    }
    _cvTransmissionQueue.notifyAll();
    _mTransmissionQueue.unlock();
    return true;
}

//-------------------------------------------------------------------

AsyncTranmissionTrait::QueuedMessage::QueuedMessage (void)
{
    pMsg = NULL;
}

AsyncTranmissionTrait::QueuedMessage::~QueuedMessage (void)
{
    pMsg = NULL;
}

//-------------------------------------------------------------------

AbstractNetworkInterface::AbstractNetworkInterface (bool bAsyncTransmission)
    : _bReceiveOnly (false),
      _bSendOnly (false),
      _ui8McastTTL (1),
      _ui16Port (NetworkMessageService::DEFAULT_PORT),
      _pNMSParent (NULL),
      _pReceiver (NULL),
      _asyncTx (this, bAsyncTransmission)
{
}

AbstractNetworkInterface::~AbstractNetworkInterface (void)
{
    if (_pReceiver != NULL) {
        checkAndLogMsg ("NetworkInterface::~NetworkInterface",
                        Logger::L_HighDetailDebug, "terminating receiver thread\n");
        _pReceiver->requestTerminationAndWait ();
    }
    delete _pReceiver;
}

int AbstractNetworkInterface::init (uint16 ui16Port, const char *pszBindingInterfaceSpec,
                                    NetworkInterfaceManagerListener *pNMSParent,
                                    bool bReceiveOnly, bool bSendOnly,
                                    const char *pszPropagationAddr, uint8 ui8McastTTL)
{
    if ((pszBindingInterfaceSpec == NULL) || (pNMSParent == NULL)) {
        return -1;
    }
    _ui16Port = ui16Port;
    _bindingInterfaceSpec = pszBindingInterfaceSpec;
    _pNMSParent = pNMSParent;
    _bReceiveOnly = bReceiveOnly;
    _bSendOnly = bSendOnly;
    _defaultPropagationAddr = pszPropagationAddr;
    _ui8McastTTL = ui8McastTTL;

    if (inet_addr (_bindingInterfaceSpec) == INADDR_ANY) {
        _pReceiver = new ManycastNetworkMessageReceiver (_pNMSParent, this, !_bSendOnly, NetUtils::getNICsInfo (false, false));
    }
    else {
        _pReceiver = new NetworkMessageReceiver (_pNMSParent, this, !_bSendOnly);
    }

    _asyncTx.start();
    return 0;
}

int AbstractNetworkInterface::start (void)
{
    if (!_bSendOnly) {
        _pReceiver->start();
    }
    return 0;
}

int AbstractNetworkInterface::stop (void)
{
    if (_pReceiver->isRunning ()) {
        _pReceiver->requestTerminationAndWait();
    }
    return 0;
}

bool AbstractNetworkInterface::canSend (void)
{
    return !_bReceiveOnly;
}

bool AbstractNetworkInterface::canReceive (void)
{
    return !_bSendOnly;
}

bool AbstractNetworkInterface::boundToWildcardAddr (void)
{
    return ((_bindingInterfaceSpec == IN_ADDR_ANY_STR) == 1);
}

const char * AbstractNetworkInterface::getBindingInterfaceSpec (void) const
{
    return _bindingInterfaceSpec;
}

const char * AbstractNetworkInterface::getBroadcastAddr (void) const
{
    return _broadcastAddr;
}

const char * AbstractNetworkInterface::getNetmask (void) const
{
    return _netmask;
}

uint16 AbstractNetworkInterface::getPort (void)
{
    return _ui16Port;
}

const char * AbstractNetworkInterface::getPropagatonAddr (void)
{
    return _defaultPropagationAddr;
}

NetworkMessageReceiver * AbstractNetworkInterface::getReceiver (void)
{
    return _pReceiver;
}

uint8 AbstractNetworkInterface::getTTL (void)
{
    return _ui8McastTTL;
}

void AbstractNetworkInterface::setLinkCapacity (uint32 ui32Capacity)
{
    _linkCapacity.setLinkCapacity (ui32Capacity);
}

uint32 AbstractNetworkInterface::getLinkCapacity (void)
{
    return _linkCapacity.getLinkCapacity();
}

uint32 AbstractNetworkInterface::getTransmissionQueueSize (void)
{
    return _asyncTx.getTransmissionQueueSize();
}

uint8 AbstractNetworkInterface::getRescaledTransmissionQueueSize (void)
{
    return _asyncTx.getRescaledTransmissionQueueSize();
}

void AbstractNetworkInterface::setTransmissionQueueMaxSize (uint32 ui32MaxSize)
{
    _asyncTx.setTransmissionQueueMaxSize (ui32MaxSize);
}

uint32 AbstractNetworkInterface::getTransmissionQueueMaxSize (void)
{
    return _asyncTx.getTransmissionQueueMaxSize();
}

void AbstractNetworkInterface::setAutoResizeQueue (bool bEnable, uint32 ui32MaxTimeInQueue)
{
    _asyncTx.setAutoResizeQueue (bEnable, ui32MaxTimeInQueue);
}

uint32 AbstractNetworkInterface::getAutoResizeQueue (void)
{
    return _asyncTx.getAutoResizeQueue();
}

void AbstractNetworkInterface::setReceiveRateSampleInterval (uint32 ui32IntervalInMS)
{
    //need to enable the receive rate estimation before being able to use it
    _pReceiver->enableReceiveRateEstimation ();
    //let's assume that if someone enables it, there will be no need to disable it again during runtime
    _pReceiver->setReceiveRateSampleInterval (ui32IntervalInMS);
}

uint32 AbstractNetworkInterface::getReceiveRate(void)
{
    return _pReceiver->getReceiveRate();
}

bool AbstractNetworkInterface::bufferOutgoingMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr,
                                                      bool bExpedited, const char *pszHints,
                                                      const char *pszBindingInterfaceSpec)
{
    return _asyncTx.bufferOutgoingMessage (pNetMsg, ui32IPAddr, bExpedited, pszHints, pszBindingInterfaceSpec);
}

int AbstractNetworkInterface::sendMessage (const NetworkMessage *pNetMsg, bool bExpedited, const char *pszHints)
{
    return sendMessage (pNetMsg, inet_addr (_defaultPropagationAddr), bExpedited, pszHints);
}

int AbstractNetworkInterface::sendMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, bool bExpedited, const char *pszHints)
{
    if (bufferOutgoingMessage (pNetMsg, ui32IPAddr, bExpedited, pszHints, _bindingInterfaceSpec)) {
        return 0;
    }
    return sendMessageNoBuffering (pNetMsg, ui32IPAddr, pszHints);
}

bool AbstractNetworkInterface::operator == (const NetworkInterface &rhsStr) const
{
    // TODO: _networkAddr may not always be set in case of datagram sockets -
    // check this code/ Replace with binding interface?
    return ((_bindingInterfaceSpec == rhsStr.getBindingInterfaceSpec ()) == 1);
}

