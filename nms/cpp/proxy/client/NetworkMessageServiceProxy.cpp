/* 
 * NetworkMessageServiceProxy.cpp
 *
 * This file is part of the IHMC Network Message Service Library
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 27, 2015, 5:00 PM
 */

#include "NetworkMessageServiceProxy.h"

#include "NetworkMessageServiceUnmarshaller.h"
#include "SerializationUtil.h"

#include "InetAddr.h"
#include "Logger.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
#define synchronized MutexUnlocker unlocker

using namespace NOMADSUtil;

namespace NOMADSUtil
{
    SimpleCommHelper2::Error sendLineAndLog (const char *pszMethodName, SimpleCommHelper2 *pCommHelper)
    {
        if (pCommHelper == NULL) {
            return SimpleCommHelper2::CommError;
        }
        SimpleCommHelper2::Error error = SimpleCommHelper2::None;
        checkAndLogMsg ("NetworkMessageServiceProxy::sendLineAndLog", Logger::L_HighDetailDebug, "Invoking <%s>.\n", pszMethodName);
        pCommHelper->sendLine (error, pszMethodName);
        checkAndLogMsg ("NetworkMessageServiceProxy::sendLineAndLog", Logger::L_HighDetailDebug, "<%s> terminated.\n", pszMethodName);
        return error;
    }

    int castMessage (const char *pszMethodName, SimpleCommHelper2 *pCommHelper, uint8 ui8MsgType, const char **ppszOutgoingInterfaces,
                     uint32 ui32BroadcastAddress, uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                     const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen, bool *pbExpedited,
                     const char *pszHints)
    {
        if (pCommHelper == NULL) {
            return -1;
        }
        SimpleCommHelper2::Error error = sendLineAndLog (pszMethodName, pCommHelper);
        if (error != SimpleCommHelper2::None) {
            return -2;
        }

        Writer *pWriter = pCommHelper->getWriterRef();
        if (pWriter == NULL) {
            return -3;
        }

        if (pWriter->writeUI8 (&ui8MsgType) < 0) {
            error = SimpleCommHelper2::CommError;
            return -4;
        }

        if ((writeStringArray (pWriter, ppszOutgoingInterfaces, error) < 0) || (error != SimpleCommHelper2::None)) {
            return -5;
        }

        if (pWriter->writeUI32 (&ui32BroadcastAddress) < 0) {
            error = SimpleCommHelper2::CommError;
            return -7;
        }

        if (pWriter->writeUI16 (&ui16MsgId) < 0) {
            error = SimpleCommHelper2::CommError;
            return -8;
        }

        if (pWriter->writeUI8 (&ui8HopCount) < 0) {
            error = SimpleCommHelper2::CommError;
            return -9;
        }

        if (pWriter->writeUI8 (&ui8TTL) < 0) {
            error = SimpleCommHelper2::CommError;
            return -10;
        }

        if (pWriter->writeUI16 (&ui16DelayTolerance) < 0) {
            error = SimpleCommHelper2::CommError;
            return -11;
        }

        if (pWriter->writeUI16 (&ui16MsgMetaDataLen) < 0) {
            error = SimpleCommHelper2::CommError;
            return -12;
        }
        if ((ui16MsgMetaDataLen > 0) && (pWriter->writeBytes (pMsgMetaData, ui16MsgMetaDataLen) < 0)) {
            error = SimpleCommHelper2::CommError;
            return -13;
        }

        if (pWriter->writeUI16 (&ui16MsgLen) < 0) {
            error = SimpleCommHelper2::CommError;
            return -14;
        }
        if ((ui16MsgLen > 0) && (pWriter->writeBytes (pMsg, ui16MsgLen) < 0)) {
            error = SimpleCommHelper2::CommError;
            return -15;
        }

        if ((pbExpedited != NULL) && (pWriter->writeBool (pbExpedited) < 0)) {
            error = SimpleCommHelper2::CommError;
            return -16;
        }

        if (pWriter->writeString (pszHints) < 0) {
            error = SimpleCommHelper2::CommError;
            return -17;
        }

        Reader *pReader = pCommHelper->getReaderRef();
        int32 rc = 0;
        if (pReader->read32 (&rc) < 0) {
            error = SimpleCommHelper2::CommError;
            return -18;
        }

        pCommHelper->receiveMatch (error, "OK");
        return (error == SimpleCommHelper2::None ? (int) rc : -19);
    }
}
NetworkMessageServiceProxy::NetworkMessageServiceProxy (uint16 ui16ADesiredApplicationId, bool bUseBackgroundReconnect)
    : Stub (ui16ADesiredApplicationId, &NetworkMessageServiceUnmarshaller::methodArrived,
            NetworkMessageServiceUnmarshaller::SERVICE, NetworkMessageServiceUnmarshaller::VERSION,
            bUseBackgroundReconnect),
      _pListener (NULL)
{
}

NetworkMessageServiceProxy::~NetworkMessageServiceProxy (void)
{
}

int NetworkMessageServiceProxy::registerHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener)
{
    synchronized (&_stubMutex);
    if (_pListener != NULL) {
        return -1;
    }
    _pListener = pListener;
    if (_pCommHelper == NULL) {
        return -2;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::REGISTER_LISTENER_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -3;
    }

    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->writeUI8 (&ui8MsgType) < 0) {
        return -4;
    }

    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }

    return 0;
}

int NetworkMessageServiceProxy::deregisterHandlerCallback(uint8 ui8MsgType, NetworkMessageServiceListener *pListener)
{
    synchronized (&_stubMutex);

    if (_pListener != NULL) {
        return -1;
    }
    _pListener = NULL;
    return 0;
}

int NetworkMessageServiceProxy::broadcastMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces,
                                                  uint32 ui32BroadcastAddress, uint16 ui16MsgId,
                                                  uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                                  const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                  const void *pMsg, uint16 ui16MsgLen, bool bExpedited,
                                                  const char *pszHints)
{
    synchronized (&_stubMutex);
    return castMessage (NetworkMessageServiceUnmarshaller::BROADCAST_METHOD, _pCommHelper, ui8MsgType, ppszOutgoingInterfaces,
                        ui32BroadcastAddress, ui16MsgId, ui8HopCount, ui8TTL, ui16DelayTolerance,
                        pMsgMetaData, ui16MsgMetaDataLen, pMsg, ui16MsgLen, &bExpedited, pszHints);
}

int NetworkMessageServiceProxy::transmitMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                                 uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                                 const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg,
                                                 uint16 ui16MsgLen, const char *pszHints)
{
    synchronized (&_stubMutex);
    return castMessage (NetworkMessageServiceUnmarshaller::TRANSMIT_METHOD, _pCommHelper, ui8MsgType, ppszOutgoingInterfaces,
                        ui32TargetAddress, ui16MsgId, ui8HopCount, ui8TTL, ui16DelayTolerance,
                        pMsgMetaData, ui16MsgMetaDataLen, pMsg, ui16MsgLen, NULL, pszHints);
}

int NetworkMessageServiceProxy::transmitReliableMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                                         uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                                         const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen,
                                                         const char *pszHints)
{
    synchronized (&_stubMutex);
    return castMessage (NetworkMessageServiceUnmarshaller::TRANSMIT_RELIABLE_METHOD, _pCommHelper, ui8MsgType,
                        ppszOutgoingInterfaces, ui32TargetAddress, ui16MsgId, ui8HopCount, ui8TTL,
                        ui16DelayTolerance, pMsgMetaData, ui16MsgMetaDataLen, pMsg, ui16MsgLen, NULL, pszHints);
}

int NetworkMessageServiceProxy::setRetransmissionTimeout (uint32 ui32Timeout)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::SET_TRANSMISSION_TIMEOUT_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->write32 (&ui32Timeout)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    int32 rc = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->read32 (&rc)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    return 0;
}

int NetworkMessageServiceProxy::setPrimaryInterface (const char *pszInterfaceAddr)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == NULL) || (pszInterfaceAddr == NULL)) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::SET_PRIMARY_INTERFACE_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->writeString (pszInterfaceAddr)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    int32 rc = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->read32 (&rc)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    return 0;
}

int NetworkMessageServiceProxy::startSvc (void)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return -1;
    }
    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::START_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    int32 rc = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->read32 (&rc)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    return 0;
}

int NetworkMessageServiceProxy::stopSvc (void)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return -1;
    }
    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::STOP_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    int32 rc = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->read32 (&rc)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    return 0;
}

uint16 NetworkMessageServiceProxy::getMinMTU (void)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return 0;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_MIN_MTU_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    uint16 ui16MinMTU = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->read16 (&ui16MinMTU)) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    return ui16MinMTU;
}

char ** NetworkMessageServiceProxy::getActiveNICsInfoAsString (void)
{
    const char *pszDestination = NULL;
    return getActiveNICsInfoAsStringForDestinationAddr (pszDestination);
}

char ** NetworkMessageServiceProxy::getActiveNICsInfoAsStringForDestinationAddr (const char *pszDestination)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return NULL;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_ACTIVE_NIC_AS_STRING_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return NULL;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pszDestination == NULL) {
        // Set to empty string
        pszDestination = "";
    }
    if (pWriter->writeString (pszDestination)) {
        error = SimpleCommHelper2::CommError;
        return NULL;
    }

    Reader *pReader = _pCommHelper->getReaderRef();
    char **ppszDstAddresses = readStringArray (pReader, error);

    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return NULL;
    }
    return ppszDstAddresses;
}

char ** NetworkMessageServiceProxy::getActiveNICsInfoAsStringForDestinationAddr (uint32 ulSenderRemoteIPv4Addr)
{
    const InetAddr addr (ulSenderRemoteIPv4Addr);
    return getActiveNICsInfoAsStringForDestinationAddr (addr.getIPAsString());
}

PROPAGATION_MODE NetworkMessageServiceProxy::getPropagationMode (void)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return BROADCAST;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_PROPAGATION_MODE_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return BROADCAST;
    }
    uint8 ui8PropMode = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->read8 (&ui8PropMode)) {
        error = SimpleCommHelper2::CommError;
        return BROADCAST;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return BROADCAST;
    }
    switch (ui8PropMode) {
        case BROADCAST:
            return BROADCAST;

        case MULTICAST:
            return MULTICAST;

        default:
            error = SimpleCommHelper2::CommError;
            return BROADCAST;
    }
}

uint32 NetworkMessageServiceProxy::getDeliveryQueueSize (void)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return 0;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_DELIVERY_QUEUE_SIZE_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    uint32 ui32QueueSize = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readUI32 (&ui32QueueSize)) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    return ui32QueueSize;
}

int64 NetworkMessageServiceProxy::getReceiveRate (const char *pszAddr)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return 0;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_RECEIVE_RATE_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if ((pWriter->writeString (pszAddr) < 0)) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    int64 i64RcvdRate = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->read64 (&i64RcvdRate)) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    return i64RcvdRate;
}

uint32 NetworkMessageServiceProxy::getTransmissionQueueSize (const char *pszOutgoingInterface)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == NULL) || (pszOutgoingInterface == NULL)) {
        return 0;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_TRANSMISSION_QUEUE_SIZE_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if ((pWriter->writeString (pszOutgoingInterface) < 0)) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }

    uint32 ui32 = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readUI32 (&ui32) < 0) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    return ui32;
}

uint8 NetworkMessageServiceProxy::getRescaledTransmissionQueueSize (const char *pszOutgoingInterface)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == NULL) || (pszOutgoingInterface == NULL)) {
        return 0;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_RESCALED_TRANSMISSION_QUEUE_MAX_SIZE_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if ((pWriter->writeString (pszOutgoingInterface) < 0)) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }

    uint8 ui8RescaledTrQueueSize = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readUI8 (&ui8RescaledTrQueueSize) < 0) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    return ui8RescaledTrQueueSize;
}

uint32 NetworkMessageServiceProxy::getTransmissionQueueMaxSize (const char *pszOutgoingInterface)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == NULL) || (pszOutgoingInterface == NULL)) {
        return 0;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if ((pWriter->writeString (pszOutgoingInterface) < 0)) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }

    uint32 ui32TrQueueMaxSize = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readUI32 (&ui32TrQueueMaxSize) < 0) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    return ui32TrQueueMaxSize;
}

uint32 NetworkMessageServiceProxy::getTransmitRateLimit (const char *pszInterface)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == NULL) || (pszInterface == NULL)) {
        return 0;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_TRANSMISSION_RATE_LIMIT_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if ((pWriter->writeString (pszInterface) < 0)) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }

    uint32 ui32TrRateLimit = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readUI32 (&ui32TrRateLimit) < 0) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    return ui32TrRateLimit;
}

int NetworkMessageServiceProxy::setTransmissionQueueMaxSize (const char *pszOutgoingInterface, uint32 ui32MaxSize)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == NULL) || (pszOutgoingInterface == NULL)) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::SET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if ((pWriter->writeString (pszOutgoingInterface) < 0) ||
        (pWriter->write32 (&ui32MaxSize) < 0)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }

    int32 rc = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->read32 (&rc) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -5;
    }
    if (rc < 0) {
        return -6;
    }
    return 0;
}

int NetworkMessageServiceProxy::setTransmitRateLimit (const char *pszInterface, const char *pszDestinationAddress, uint32 ui32RateLimit)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::SET_TRANSMIT_RATE_LIMIT_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    if (pszInterface == NULL) {
        // Set to empty string
        pszInterface = "";
    }
    if (pszDestinationAddress == NULL) {
        // Set to empty string
        pszDestinationAddress = "";
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if ((pWriter->writeString (pszInterface) < 0) ||
        (pWriter->writeString (pszDestinationAddress) < 0) ||
        (pWriter->write32 (&ui32RateLimit) < 0)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }

    int32 rc = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->read32 (&rc) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -5;
    }
    if (rc < 0) {
        return -6;
    }
    return 0;
}

int NetworkMessageServiceProxy::setTransmitRateLimit (const char *pszInterface, uint32 ui32DestinationAddress, uint32 ui32RateLimit)
{
    InetAddr addr (ui32DestinationAddress);
    return setTransmitRateLimit (pszInterface, addr.getIPAsString(), ui32RateLimit);
}

int NetworkMessageServiceProxy::setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit)
{
    return setTransmitRateLimit (NULL, pszDestinationAddress, ui32RateLimit);
}

int NetworkMessageServiceProxy::setTransmitRateLimit (uint32 ui32RateLimit)
{
    return setTransmitRateLimit (NULL, ui32RateLimit);
}

uint32 NetworkMessageServiceProxy::getLinkCapacity (const char *pszInterface)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return 0;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_LINK_CAPACITY_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return 0;
    }

    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->writeString (pszInterface) < 0) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    uint32 ui32Capacity = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readUI32 (&ui32Capacity) < 0) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    return ui32Capacity;
}

void NetworkMessageServiceProxy::setLinkCapacity (const char *pszInterface, uint32 ui32Capacity)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::SET_LINK_CAPACITY_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pszInterface == NULL) {
        // Set to empty string
        pszInterface = "";
    }
    if ((pWriter->writeString (pszInterface) < 0) || (pWriter->write32 (&ui32Capacity) < 0)) {
        error = SimpleCommHelper2::CommError;
        return;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return;
    }
}

uint8 NetworkMessageServiceProxy::getNeighborQueueLength (const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return 0;
    }
    
    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::GET_NEIGHBOR_QUEUE_LENGTH_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pchIncomingInterface == NULL) {
        // Set to empty string
        pchIncomingInterface = "";
    }
    if ((pWriter->writeString (pchIncomingInterface) < 0) || (pWriter->write32 (&ulSenderRemoteAddr) < 0)) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }

    uint8 ui8Len = 0;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readUI8 (&ui8Len) < 0) {
        error = SimpleCommHelper2::CommError;
        return 0;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return 0;
    }
    return ui8Len;
}

bool NetworkMessageServiceProxy::clearToSend (const char *pszInterface)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return false;
    }
    
    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::CLEAR_TO_SEND_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pszInterface == NULL) {
        // Set to empty string
        pszInterface = "";
    }
    if (pWriter->writeString (pszInterface) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    bool bClearToSend = false;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readBool (&bClearToSend) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    return bClearToSend;
}

bool NetworkMessageServiceProxy::clearToSendOnAllInterfaces (void)
{
    return clearToSend (NULL);
}

int NetworkMessageServiceProxy::ping (void)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == NULL) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (NetworkMessageServiceUnmarshaller::PING_METHOD, _pCommHelper);
    if (error == SimpleCommHelper2::None) {
        char buf[128];
        _pCommHelper->receiveLine (buf, sizeof (buf), error);
        printf ("Received %s\n", buf);
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    return ((error == SimpleCommHelper2::None) ? 0 : -2);
}

// --- Callbacks ---------------------------------------------------------------

int NetworkMessageServiceProxy::messageArrived (const char *pszIncomingInterface,
                                                uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                                uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                                const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                const void *pMsg, uint16 ui16MsgLen, int64 i64Timestamp)
{
    if (_pListener == NULL) {
        return -1;
    }
    _pListener->messageArrived (pszIncomingInterface, ui32SourceIPAddress, ui8MsgType, ui16MsgId,
                                ui8HopCount, ui8TTL, pMsgMetaData, ui16MsgMetaDataLen, pMsg, ui16MsgLen, i64Timestamp);
    return 0;
}

int NetworkMessageServiceProxy::pong (void)
{
    printf ("pong\n");
    return 0;
}

