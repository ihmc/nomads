/*
 * NetworkMessageService.cpp
 *
 * This file is part of the IHMC Network Message Service Library
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

#include "NetworkMessageService.h"

#include "ConfigManager.h"
#include "InetAddr.h"
#include "ManageableDatagramSocket.h"

#include "NMSCommandProcessor.h"
#include "NetworkInterfaceManager.h"
#include "NetworkMessageServiceImpl.h"
#include "NetworkMessageServiceProxyServer.h"
#include "NMSProperties.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;
using namespace CryptoUtils;

NetworkMessageService::NetworkMessageService (PROPAGATION_MODE mode, bool bAsyncDelivery,
                                              bool bAsyncTransmission, uint8 ui8MessageVersion,
                                              bool bReplyViaUnicast, const char *pszSessionKey, const char * pszGroupKeyFilename)

    : _pNetIntMgr (new NetworkInterfaceManager (mode, bReplyViaUnicast, bAsyncTransmission)),
      _Impl (new NetworkMessageServiceImpl (mode, bAsyncDelivery, ui8MessageVersion, _pNetIntMgr, pszSessionKey, pszGroupKeyFilename)),
      _pCmdProc (NULL)
{
}

NetworkMessageService::~NetworkMessageService (void)
{
    if (_pCmdProc != NULL) {
        delete _pCmdProc;
    }
}

NetworkMessageService * NetworkMessageService::getInstance (ConfigManager *pCfgMgr, const char * pszSessionKey)
{
    const char *pszMethodName = "NetworkMessageService::getInstance";
    if (pCfgMgr == NULL) {
        return NULL;
    }

    PROPAGATION_MODE mode = MULTICAST;
    uint32 ui32PropagationMode = pCfgMgr->getValueAsUInt32 (NMSProperties::NMS_TRANSMISSION_MODE, MULTICAST);

    switch (ui32PropagationMode) {
        case BROADCAST:
            mode = BROADCAST;
            break;
        case MULTICAST:
            mode = MULTICAST;
            break;
        case NORM:
            mode = NORM;
            break;

        default:
            return NULL;
    }

    const bool bAsyncDelivery = pCfgMgr->getValueAsBool (NMSProperties::NMS_DELIVERY_ASYNC, true);
    const bool bAsyncTransmission = pCfgMgr->getValueAsBool (NMSProperties::NMS_TRANSMISSION_ASYNC, false);
    const bool bReplyViaUnicast = pCfgMgr->getValueAsBool (NMSProperties::NMS_TRANSMISSION_UNICAST_REPLY, false);
    const uint32 ui32MsgVersion = pCfgMgr->getValueAsUInt32 (NMSProperties::NMS_MSG_VERSION, 2);
    const bool bUseManagedIfaces = pCfgMgr->getValueAsBool (NMSProperties::NMS_USE_MANAGED_INTERFACES, false);
	const char *pszGroupKeyFilename = pCfgMgr->getValue(NMSProperties::NMS_GROUP_KEY_FILE, NULL);

    switch (ui32MsgVersion) {
        case 1:
        case 2:
            break;

        default:
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "message version number not supported: %u", ui32MsgVersion);
            return NULL;
    }

    NetworkMessageService *pNMS = new NetworkMessageService (mode, bAsyncDelivery, bAsyncTransmission,
                                                             static_cast<uint8>(ui32MsgVersion), bReplyViaUnicast, pszSessionKey, pszGroupKeyFilename);

    if ((pNMS != NULL) && bUseManagedIfaces) {
        pNMS->_pMgblSockMgr = new ManageableDatagramSocketManager();
    }
    return pNMS;
}

NetworkMessageServiceProxyServer * NetworkMessageService::getProxySvrInstance (ConfigManager *pCfgMgr)
{
    const char *pszMethodName = "NetworkMessageService::getProxySvrInstance";
    ConfigManager dummyCfgMgr;

    if (pCfgMgr == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "null config manager.\n");
        pCfgMgr = &dummyCfgMgr;
    }

    NetworkMessageService *pNMS = NetworkMessageService::getInstance (pCfgMgr);
    if (pNMS == NULL) {
        return NULL;
    }

    int rc = pNMS->init (pCfgMgr);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "network message service init returned an error: %d.\n", rc);
        delete pNMS;
        return NULL;
    }

    NetworkMessageServiceProxyServer *pNMSProxySrv = new NetworkMessageServiceProxyServer (pNMS, true);
    if (pNMSProxySrv == NULL) {
        delete pNMS;
        return NULL;
    }

    const uint32 ui32NMSProxyPort = pCfgMgr->getValueAsUInt32 (NMSProperties::NMS_PROXY_PORT, NMSProperties::DEFAULT_NMS_PROXY_PORT);
    if (ui32NMSProxyPort > 0xFFFF) {
        delete pNMSProxySrv; // no need to deallocated pNMS, pNMSProxySrv will
        return NULL;
    }
    rc = pNMSProxySrv->init ((uint16) ui32NMSProxyPort);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "init() returned an error: %d.\n", rc);
        delete pNMSProxySrv; // no need to deallocated pNMS, pNMSProxySrv will
        return NULL;
    }

    if (pCfgMgr->getValueAsBool (NMSProperties::NMS_CMD_PROCESSOR, true)) {
        if (pNMS->_pCmdProc != NULL) {
            pNMS->_pCmdProc->enableNetworkAccess (pCfgMgr->getValueAsUInt32 (NMSProperties::NMS_CMD_PROCESSOR_PORT,
                                                                             NMSProperties::DEFAULT_CMD_PROCESSOR_PORT));
        }
    }

    return pNMSProxySrv;
}

NMSCommandProcessor * NetworkMessageService::getCmdProcessor (void)
{
    return _pCmdProc;
}

int NetworkMessageService::init (ConfigManager *pCfgMgr)
{
    MutexUnlocker unlocker (&_m);
    _pNetIntMgr->registerListener (_Impl);
    if (_pNetIntMgr->init (pCfgMgr) < 0) {
        return -1;
    }
    if (_Impl->init (pCfgMgr) < 0) {
        return -2;
    }
    _pCmdProc = new NMSCommandProcessor (this);
    _pCmdProc->setPrompt ("NMS");

    _pNetIntMgr->start();

    return 0;
}

int NetworkMessageService::init (uint16 ui16Port, const char **ppszBindingInterfaces,
                                 const char **ppszIgnoredInterfaces, const char **ppszAddedInterfaces,
                                 const char *pszDestAddr, uint8 ui8McastTTL)

{
    MutexUnlocker unlocker (&_m);
    _pNetIntMgr->registerListener (_Impl);
    if (_pNetIntMgr->init (ppszBindingInterfaces, ppszIgnoredInterfaces, ppszAddedInterfaces,
                           pszDestAddr, ui16Port, ui8McastTTL) < 0) {
        return -1;
    }
    ConfigManager cfgMgr;
    cfgMgr.init();
    if (_Impl->init (&cfgMgr) < 0) {
        return -2;
    }
    return 0;
}

String NetworkMessageService::getEncryptionKeyHash (void)
{
    return _Impl->getEncryptionKeyHash();
}

int NetworkMessageService::changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len)
{
    return _Impl->changeEncryptionKey (pchKey, ui32Len);
}

int NetworkMessageService::setRetransmissionTimeout (uint32 ui32Timeout)
{
    MutexUnlocker unlocker (&_m);
    return _Impl->setRetransmissionTimeout (ui32Timeout);
}

int NetworkMessageService::setPrimaryInterface (const char *pszInterfaceAddr)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->setPrimaryInterface (pszInterfaceAddr);
}

int NetworkMessageService::start (void)
{
    MutexUnlocker unlocker (&_m);
    _pNetIntMgr->start();
    return _Impl->start();
}

int NetworkMessageService::stop (void)
{
    MutexUnlocker unlocker (&_m);
    _Impl->requestTerminationAndWait();
    _pNetIntMgr->stop();
    return 0;
}

int NetworkMessageService::registerHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener)
{
    MutexUnlocker unlocker (&_m);
    return _Impl->registerHandlerCallback (ui8MsgType, pListener);
}

int NetworkMessageService::deregisterHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener)
{
    MutexUnlocker unlocker (&_m);
    return _Impl->deregisterHandlerCallback (ui8MsgType, pListener);
}

int NetworkMessageService::broadcastMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces,
                                             uint32 ui32BroadcastAddress, uint16 ui16MsgId,
                                             uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                             const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                             const void *pMsg, uint16 ui16MsgLen, bool bExpedited,
                                             const char *pszHints)
{
    MutexUnlocker unlocker (&_m);
    TransmissionInfo ti;
    ti.bReliable = false;
    ti.bExpedited = bExpedited;
    ti.ui8HopCount = ui8HopCount;
    ti.ui8TTL = ui8TTL;
    ti.ui16DelayTolerance = ui16DelayTolerance;
    ti.ui8MsgType = ui8MsgType;
    ti.ui32DestinationAddress = ui32BroadcastAddress;
    ti.ppszOutgoingInterfaces = ppszOutgoingInterfaces;
    ti.pszHints = pszHints;
    MessageInfo mi;
    mi.ui16MsgMetaDataLen = ui16MsgMetaDataLen;
    mi.pMsgMetaData = pMsgMetaData;
    // If bSecure the encryption of the payload will be performed
    mi.pMsg = pMsg;
    mi.ui16MsgLen = ui16MsgLen;
    return _Impl->broadcastMessage (ti, mi);
}

int NetworkMessageService::transmitMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                            uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                            const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg,
                                            uint16 ui16MsgLen, const char *pszHints)

{
    MutexUnlocker unlocker (&_m);
    TransmissionInfo ti;
    // !!!
    // !!! NOTE: unreliable transmission is still not supported, set bReliable on true for now!
    // !!!
    ti.bReliable = true;
    ti.bExpedited = false;
    ti.ui8HopCount = ui8HopCount;
    ti.ui8TTL = ui8TTL;
    ti.ui16DelayTolerance = ui16DelayTolerance;
    ti.ui8MsgType = ui8MsgType;
    ti.ui32DestinationAddress = ui32TargetAddress;
    ti.ppszOutgoingInterfaces = ppszOutgoingInterfaces;
    ti.pszHints = pszHints;
    MessageInfo mi;
    mi.ui16MsgMetaDataLen = ui16MsgMetaDataLen;
    mi.pMsgMetaData = pMsgMetaData;
    mi.pMsg = pMsg;
    mi.ui16MsgLen = ui16MsgLen;
    return _Impl->transmitMessage (ti, mi);
}

int NetworkMessageService::transmitReliableMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                                    uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                                    const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen,
                                                    const char *pszHints)
{
    MutexUnlocker unlocker (&_m);
    TransmissionInfo ti;
    ti.bReliable = true;
    ti.bExpedited = true;   // expedite reliable messages to minimize the risk of unnecessary re-transmissions
    ti.ui8HopCount = ui8HopCount;
    ti.ui8TTL = ui8TTL;
    ti.ui16DelayTolerance = ui16DelayTolerance;
    ti.ui8MsgType = ui8MsgType;
    ti.ui32DestinationAddress = ui32TargetAddress;
    ti.ppszOutgoingInterfaces = ppszOutgoingInterfaces;
    ti.pszHints = pszHints;
    MessageInfo mi;
    mi.ui16MsgMetaDataLen = ui16MsgMetaDataLen;
    mi.pMsgMetaData = pMsgMetaData;
    mi.pMsg = pMsg;
    mi.ui16MsgLen = ui16MsgLen;
    return _Impl->transmitMessage (ti, mi);
}

uint16 NetworkMessageService::getMinMTU()
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->getMinMTU();
}

char ** NetworkMessageService::getActiveNICsInfoAsString (void)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->getActiveNICsInfoAsString();
}

char ** NetworkMessageService::getActiveNICsInfoAsStringForDestinationAddr (const char *pszDestination)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->getActiveNICsInfoAsStringForDestinationAddr (pszDestination);
}

char ** NetworkMessageService::getActiveNICsInfoAsStringForDestinationAddr (uint32 ulSenderRemoteIPv4Addr)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->getActiveNICsInfoAsStringForDestinationAddr (ulSenderRemoteIPv4Addr);
}

PROPAGATION_MODE NetworkMessageService::getPropagationMode (void)
{
    MutexUnlocker unlocker (&_m);
    return _Impl->getPropagationMode();
}

uint32 NetworkMessageService::getDeliveryQueueSize (void)
{
    MutexUnlocker unlocker (&_m);
    return _Impl->getDeliveryQueueSize();
}

int64 NetworkMessageService::getReceiveRate (const char *pszAddr)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->getReceiveRate (pszAddr);
}

uint32 NetworkMessageService::getTransmissionQueueSize (const char *pszOutgoingInterface)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->getTransmissionQueueSize (pszOutgoingInterface);
}

uint8 NetworkMessageService::getRescaledTransmissionQueueSize (const char *pszOutgoingInterface)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->getRescaledTransmissionQueueSize (pszOutgoingInterface);
}

uint32 NetworkMessageService::getTransmissionQueueMaxSize (const char *pszOutgoingInterface)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->getTransmissionQueueMaxSize (pszOutgoingInterface);
}

uint32 NetworkMessageService::getTransmitRateLimit (const char *pszInterface)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->getTransmitRateLimit (pszInterface);
}

int NetworkMessageService::setTransmissionQueueMaxSize (const char *pszOutgoingInterface, uint32 ui32MaxSize)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->setTransmissionQueueMaxSize (pszOutgoingInterface, ui32MaxSize);
}

int NetworkMessageService::setTransmitRateLimit (const char *pszInterface, const char *pszDestinationAddress, uint32 ui32RateLimit)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->setTransmitRateLimit (pszInterface, pszDestinationAddress, ui32RateLimit);
}

int  NetworkMessageService::setTransmitRateLimit (const char *pszInterface, uint32 ui32DestinationAddress, uint32 ui32RateLimit)
{
    MutexUnlocker unlocker (&_m);
    InetAddr inet (ui32DestinationAddress);
    return _pNetIntMgr->setTransmitRateLimit (pszInterface, inet.getIPAsString(), ui32RateLimit);
}

int NetworkMessageService::setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->setTransmitRateLimit (pszDestinationAddress, ui32RateLimit);
}

int NetworkMessageService::setTransmitRateLimit (uint32 ui32RateLimit)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->setTransmitRateLimit (ui32RateLimit);
}

uint32 NetworkMessageService::getLinkCapacity (const char *pszInterface)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->getLinkCapacity (pszInterface);
}

void NetworkMessageService::setLinkCapacity (const char *pszInterface, uint32 ui32Capacity)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->setLinkCapacity (pszInterface, ui32Capacity);
}

uint8 NetworkMessageService::getNeighborQueueLength (const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr)
{
    MutexUnlocker unlocker (&_m);
    return _Impl->getNeighborQueueLength (pchIncomingInterface, ulSenderRemoteAddr);
}

bool NetworkMessageService::clearToSend (const char *pszInterface)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->clearToSend (pszInterface);
}

bool NetworkMessageService::clearToSendOnAllInterfaces (void)
{
    MutexUnlocker unlocker (&_m);
    return _pNetIntMgr->clearToSendOnAllInterfaces();
}

