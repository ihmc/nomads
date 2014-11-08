/*
 * NetworkMessageService.cpp
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

#include "NetworkMessageService.h"

#include "BufferReader.h"
#include "Exceptions.h"
#include "Logger.h"
#include "MessageFactory.h"
#include "NetUtils.h"
#include "NetworkMessageServiceListener.h"
#include "NLFLib.h"
#include "NICInfo.h"
#include "Reassembler.h"
#include "SequentialArithmetic.h"
#include "NetworkMessageReceiver.h"
#include "ManycastForwardingNetworkInterface.h"
#include "ManycastNetworkMessageReceiver.h"
#include "NetworkMessage.h"
#include "NetworkMessageV2.h"
#include "StringHashset.h"

#include <stdio.h>
#include <stdlib.h>

#if defined (UNIX)
    #include <netinet/in.h>
    #define UINT32_ADDRESS s_addr
#else
    #define UINT32_ADDRESS S_un.S_addr
#endif

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

const char ** strArrayDup (const char **ppszString, uint8 ui8NStrings);

NetworkMessageService::NetworkMessageService (PROPAGATION_MODE mode, bool bAsyncDelivery, bool bAsyncTransmission,
                                              uint8 ui8MessageVersion, bool bReplyViaUnicast)
	: _lastMsgs (true, true), _cvDeliveryQueue (&_mDeliveryQueue),
          _interfaces (false, true, true, false), _bReplyViaUnicast (bReplyViaUnicast)
{
    _mode = mode;
    _bAsyncDelivery = bAsyncDelivery;
    _bAsyncTransmission = bAsyncTransmission;

    if (!pLogger) {
        pLogger = new Logger();
        pLogger->enableScreenOutput();
        pLogger->initLogFile("networkmessage.log", false);
        pLogger->enableFileOutput();
        pLogger->setDebugLevel(Logger::L_MildError);
    }

    _ui16BroadcastedMsgCounter = 0;
    _ui32RetransmissionTimeout = DEFAULT_RETRANSMISSION_TIME;
    _ui8DefMaxNumOfRetransmissions = DEFAULT_MAX_NUMBER_OF_RETRANSMISSIONS;
    _pMessageFactory = new MessageFactory (ui8MessageVersion);
    _ui32MaxMsecsInOutgoingQueue = 5000;
    _pReassembler = new Reassembler (_ui32RetransmissionTimeout);
    _ui32PrimaryInterface = 0;
    _bPrimaryInterfaceIdSet = false;
    _ui16MTU = DEFAULT_MTU;
}

NetworkMessageService::~NetworkMessageService()
{
    if (_bAsyncDelivery) {
        if (_ostDeliveryThread.wasStarted()) {
            _ostDeliveryThread.waitForThreadToTerminate (0);
        }
    }
    for (NOMADSUtil::StringHashtable<ByInterface>::Iterator iInterface = _tQueueLengthByInterface.getAllElements(); !iInterface.end(); iInterface.nextElement()) {
        ByInterface* pInterface = iInterface.getValue();
        for (NOMADSUtil::UInt32Hashtable<ByNeighbor>::Iterator iNeighbor = pInterface->_tByNeighbor.getAllElements(); !iNeighbor.end(); iNeighbor.nextElement()) {
            ByNeighbor* pNeighbor = iNeighbor.getValue();
            delete pNeighbor;
        }
        pInterface->_tByNeighbor.removeAll();
        delete pInterface;
    }
    _tQueueLengthByInterface.removeAll();

    delete _pReassembler;
    delete _pMessageFactory;
    _pReassembler = NULL;
    _pMessageFactory = NULL;

    _interfaces.removeAll();
    _lastMsgs.removeAll();
}

int NetworkMessageService::init (uint16 ui16Port)
{
    switch (_mode) {
        case BROADCAST: {
            return init (ui16Port, NMS_BROADCAST_ADDRESS);
        }
        case MULTICAST: {
            return init (ui16Port, NMS_MULTICAST_ADDRESS);
        }
        default: {
            return init (ui16Port, NMS_BROADCAST_ADDRESS);
        }
    }
}

int NetworkMessageService::init (uint16 ui16Port, const char *pszDestAddr, uint8 ui8McastTTL)
{
    return init (ui16Port, NULL, NULL, pszDestAddr, ui8McastTTL);
}

int NetworkMessageService::init (uint16 ui16Port, const char **ppszBindingInterfaces,
                                 const char **ppszIgnoredInterfaces)
{
    switch (_mode) {
        case BROADCAST:
            return init (ui16Port, ppszBindingInterfaces, ppszIgnoredInterfaces, NMS_BROADCAST_ADDRESS);

        case MULTICAST:
            return init (ui16Port, ppszBindingInterfaces, ppszIgnoredInterfaces, NMS_MULTICAST_ADDRESS);

        default:
            return init (ui16Port, ppszBindingInterfaces, ppszIgnoredInterfaces, NMS_BROADCAST_ADDRESS);
    }
}

int NetworkMessageService::init (uint16 ui16Port, const char **ppszBindingInterfaces,
                                 const char **ppszIgnoredInterfaces, const char *pszDestAddr,
                                 uint8 ui8McastTTL)
{
    return init (ui16Port, ppszBindingInterfaces, ppszIgnoredInterfaces, NULL, pszDestAddr, ui8McastTTL);
}

int NetworkMessageService::init (uint16 ui16Port, const char **ppszBindingInterfaces,
                                 const char **ppszIgnoredInterfaces, const char **ppszAddedInterfaces,
                                 const char *pszDestAddr, uint8 ui8McastTTL)
{
    if (pszDestAddr == NULL) {
        return 0;
    }

    if (ppszBindingInterfaces != NULL) {
        if (ppszIgnoredInterfaces != NULL) {
            checkAndLogMsg ("NetworkMessageService::init", Logger::L_Warning,
                            "binding interfaces specified - ignoring ignore interface list\n");
        }
        if (ppszAddedInterfaces != NULL) {
            checkAndLogMsg ("NetworkMessageService::init", Logger::L_Warning,
                            "binding interfaces specified - ignoring additional interface list\n");
        }
        return initInternal (ui16Port, ppszBindingInterfaces, pszDestAddr, ui8McastTTL);
    }

    // If no binding interfaces were specified, use all the available interfaces, except those
    // specified in ppszIgnoredInterfaces, and adding any specified in ppszAddedInterfaces
    NICInfo **ppNICInterfaces = NetUtils::getNICsInfo (false, false);
    if (ppNICInterfaces == NULL) {
        if ((ppszAddedInterfaces == NULL) || (ppszAddedInterfaces[0] == NULL)) {
            checkAndLogMsg ("NetworkMessageService::init", Logger::L_MildError,
                            "no binding interfaces specified, no interfaces were discovered, and no additional interfaces were specified - cannot initialize\n");
            return -1;
        }
    }

    unsigned int uiCount = 0;
    if (ppNICInterfaces != NULL) {
        for ( ; ppNICInterfaces[uiCount] != NULL; uiCount++);
    }
    if (ppszAddedInterfaces != NULL) {
        for (unsigned int i = 0; ppszAddedInterfaces[i] != NULL; i++) {
            uiCount++;
        }
    }
    char **ppszAllInterfaces = (char **) calloc (uiCount + 1, sizeof (char *));
    unsigned int uiIndex = 0;
    for (unsigned int i = 0; ppNICInterfaces[i] != NULL; i++) {
        char *psziface = ppNICInterfaces[i]->getIPAddrAsString().r_str();
        if (psziface != NULL) {
            bool bIgnore = false;
            checkAndLogMsg ("NetworkMessageService::init", Logger::L_Info,
                            "checking interface <%s> to see if it is in the ignore list\n", psziface);
            if (ppszIgnoredInterfaces != NULL) {
                for (unsigned int k = 0; ppszIgnoredInterfaces[k] != NULL; k++) {
                    if (wildcardStringCompare (psziface, ppszIgnoredInterfaces[k])) {
                        checkAndLogMsg ("NetworkMessageService::init", Logger::L_Info,
                                        "ignoring interface <%s> because it matches ignore interface spec <%s>\n",
                                        psziface, ppszIgnoredInterfaces[k]);
                        bIgnore = true;
                    }
                }
            }
            if (!bIgnore) {
                ppszAllInterfaces[uiIndex++] = psziface;
            }
        }
    }
    if (ppszAddedInterfaces != NULL) {
        for (unsigned int i = 0; ppszAddedInterfaces[i] != NULL; i++) {
            ppszAllInterfaces[uiIndex++] = strDup (ppszAddedInterfaces[i]);
            checkAndLogMsg ("NetworkMessageService::init", Logger::L_Info,
                            "adding additional interface <%s>\n", ppszAddedInterfaces[i]);
        }
    }
    NetUtils::freeNICsInfo (ppNICInterfaces);

    int rc = initInternal (ui16Port, (const char **)ppszAllInterfaces, pszDestAddr, ui8McastTTL);

    for (unsigned int i = 0; ppszAllInterfaces[i] != NULL; i++) {
        free (ppszAllInterfaces[i]);
        ppszAllInterfaces[i] = NULL;
    }
    free (ppszAllInterfaces);

    return rc;
}

int NetworkMessageService::initInternal (uint16 ui16Port, const char **ppszBindingInterfaces,
                                         const char *pszDestAddr, uint8 ui8McastTTL)
{
    const char * const pszMethodName = "NetworkMessageService::initInternal";
    if (ppszBindingInterfaces == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "destination address not set\n");
        return -1;
    }
    if (pszDestAddr == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "destination address not set\n");
        return -2;
    }

    switch (_mode) {
        case BROADCAST:
            checkAndLogMsg (pszMethodName, Logger::L_Info, "using broadcast\n");
            break;

        case MULTICAST:
            checkAndLogMsg (pszMethodName, Logger::L_Info, "using multicast\n");
            break;
    }

    _interfaces.removeAll();

    // Bind all the network interfaces specified by ppszBindingInterfaces
    for (uint8 ui8Count = 0; ppszBindingInterfaces[ui8Count]; ui8Count++) {

        NetworkInterface *pNetInt = _bReplyViaUnicast ? new ManycastForwardingNetworkInterface (_bAsyncTransmission) :
                                                        new NetworkInterface (_bAsyncTransmission);
        if (pNetInt == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "memory exhausted. Could not allocate"
                            " network interface for %s\n", ppszBindingInterfaces[ui8Count]);
            return -3;
        }

        if (NULL != strstr (ppszBindingInterfaces[ui8Count], "pds://")) {
            // This is a ProxyDatagramSocket interface
            int rc = pNetInt->init (ui16Port, ppszBindingInterfaces[ui8Count], this, false, false, pszDestAddr, ui8McastTTL);
            if (rc == 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "initialized %s for network address %s;\n",
                                (pNetInt->getType() == ManycastForwardingNetworkInterface::MCAST_FWD_IF ? "ManycastForwardingNetworkInterface" : "NetworkInterface"),
                                ppszBindingInterfaces[ui8Count], rc);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not initialize NetworkInterface for network address %s; rc = %d\n",
                                ppszBindingInterfaces[ui8Count], rc);
                delete pNetInt;
                return -4;
            }

            // Store the interface to be resolved later
            _proxyInterfacesToResolve.append (pNetInt);
        }
        else {
            int rc = pNetInt->init (ui16Port, ppszBindingInterfaces[ui8Count], this, false, false, pszDestAddr, ui8McastTTL);
            if (rc == 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "initialized %s for network "
                                "address %s/%s/%s. Current receive buffer size is %d\n",
                                (pNetInt->getType() == ManycastForwardingNetworkInterface::MCAST_FWD_IF ? "ManycastForwardingNetworkInterface" : "NetworkInterface"),
                                pNetInt->getBindingInterfaceSpec(), pNetInt->getNetmask(),
                                pNetInt->getBroadcastAddr(), pNetInt->getReceiveBufferSize());
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not initialize NetworkInterface for network address %s; rc = %d\n",
                                ppszBindingInterfaces[ui8Count], rc);
            }

            // Set receive Buffer size
            pNetInt->setReceiveBufferSize (65535);
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "set receive receive buffer size for %s to 65535; current UDP receive buffer size is %d\n",
                            pNetInt->getBindingInterfaceSpec(), pNetInt->getReceiveBufferSize());

            // Store the interface
            _interfaces.put (ppszBindingInterfaces[ui8Count], pNetInt);

            // Initialize the primary interface, if it has not been done yet
            if (!_bPrimaryInterfaceIdSet) {
                const char *pszAddr = pNetInt->getBindingInterfaceSpec();
                if (pszAddr != NULL) {
                    InetAddr primary (pszAddr);
                    _ui32PrimaryInterface = primary.getIPAddress();
                    _bPrimaryInterfaceIdSet = true;
                }
            }
        }
    }

    #ifndef WIN32
        // NOTE: when binding to a specific address, the UDP socket is not receiving
        // broadcast packets any more. This is the defacto behavior for UDP sockets
        // ever. Win32 takes a stance that diverges from the standard, allowing
        // the reception of broadcasts anyways.
        //
        // To receive broadcasts in Linux (and UNIX) systems we have to bind an
        // additional UDP socket to the wildcard address.
        NetworkInterface *pNetInt = pNetInt = new NetworkInterface (false);
        int rc = pNetInt->init (ui16Port, NetworkInterface::IN_ADDR_ANY_STR, this, true, false, pszDestAddr, ui8McastTTL);
        if (rc == 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "initialized NetworkInterface for network "
                            "address %s/%s/%s. Current receive buffer size is %d\n",
                            pNetInt->getNetworkAddr(), pNetInt->getNetmask(),
                            pNetInt->getBroadcastAddr(), pNetInt->getReceiveBufferSize());
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "could not initialize NetworkInterface for network address %s; rc = %d\n",
                            pNetInt->getBindingInterfaceSpec(), rc);
        }

        pNetInt->setReceiveBufferSize (65535);
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "set receive receive buffer size for %s to 65535; current UDP receive buffer size is %d\n",
                            pNetInt->getNetworkAddr(), pNetInt->getReceiveBufferSize());

        // Store the interface
        _interfaces.put (NetworkInterface::IN_ADDR_ANY_STR, pNetInt);
    #endif

    // Set the sample interval on all interfaces. this will also start the rate estimation
    for (NOMADSUtil::StringHashtable<NetworkInterface>::Iterator iInterface = _interfaces.getAllElements();
         !iInterface.end(); iInterface.nextElement()) {
        iInterface.getValue()->setReceiveRateSampleInterval (500);
        if ((_ui32MaxMsecsInOutgoingQueue > 0) && (_bAsyncTransmission)) {
            iInterface.getValue()->setAutoResizeQueue (true, _ui32MaxMsecsInOutgoingQueue);
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "transmission queue auto-resizing enabled for interface %s\n", iInterface.getKey());
        }
    }

    _ui16MTU = DEFAULT_MTU;
    if (_bAsyncDelivery) {
        _ostDeliveryThread.start (deliveryThread, this);
    }

    return 0; //to handle!!
}

int NetworkMessageService::setPrimaryInterface (const char *pszInterfaceAddr)
{
    if (pszInterfaceAddr == NULL) {
        return -1;
    }
    if (_bPrimaryInterfaceIdSet) {
        // Once set, the primary interface can't be changed
        return -2;
    }
    InetAddr primary (pszInterfaceAddr);
    _ui32PrimaryInterface = primary.getIPAddress();
    _bPrimaryInterfaceIdSet = true;
    return 0;
}

bool NetworkMessageService::isLocalAddress (uint32 ui32Addr)
{
    return NetUtils::isLocalAddress (ui32Addr, NULL);
}

int NetworkMessageService::start (void)
{
    for (StringHashtable<NetworkInterface>::Iterator i = _interfaces.getAllElements(); !i.end(); i.nextElement()) {
        NetworkInterface *pInterface = i.getValue();
        pInterface->start();
    }
    return ManageableThread::start();
}

int NetworkMessageService::stop (void)
{
    requestTerminationAndWait();
    for (StringHashtable<NetworkInterface>::Iterator i = _interfaces.getAllElements(); !i.end(); i.nextElement()) {
        NetworkInterface *pInterface = i.getValue();
        pInterface->stop();
    }
    return 0;
}

int NetworkMessageService::registerHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener)
{
    const char * const pszMethodName = "NetworkMessageService::registerHandlerCallback";
    NMSListerList *ptrLListeners = (NMSListerList*) _listeners.get (ui8MsgType);
    if (ptrLListeners == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                        "initializing listener list for msgtype=%d\n", ui8MsgType);

        ptrLListeners = new NMSListerList();
        _listeners.put (ui8MsgType, ptrLListeners);
    }
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                    "adding a listener for msgtype=%d\n", ui8MsgType);

    ptrLListeners->append (pListener);
    ptrLListeners = NULL;

    return 0; //to handle!!
}

int NetworkMessageService::broadcastMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32BroadcastAddress,
                                             uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                             const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen, bool bExpedited,
                                             const char *pszHints)
{
    resolveProxyDatagramSocketAddresses();

    if (!_bPrimaryInterfaceIdSet) {
        return -1;
    }

    _m.lock();
    const char * const pszMethodName = "NetworkMessageService::broadcastMessage";

    checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug,
                    "outgoing packet src: %u\n",
                    NetUtils::getLocalIPAddress().s_addr); //can I consider this the "main" ip address?

    NetworkMessage *pNetMsg = _pMessageFactory->getDataMessage (ui8MsgType,
                                                                _ui32PrimaryInterface,
                                                                ui32BroadcastAddress,
                                                                _ui16BroadcastedMsgCounter++,
                                                                ui8HopCount, ui8TTL,
                                                                NetworkMessage::CT_DataMsgComplete,
                                                                pMsgMetaData, ui16MsgMetaDataLen,
                                                                pMsg, ui16MsgLen);

    if (sendNetworkMessage (pNetMsg, ppszOutgoingInterfaces, ui32BroadcastAddress, ui16DelayTolerance, false, bExpedited, true, pszHints) != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "no interface available to broadcast\n");
        _m.unlock();
        return -2;
    }

    _m.unlock();
    return 0;
}

int NetworkMessageService::transmitMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                            uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                            const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen,
                                            const char *pszHints)
{
    _m.lock();
    
    // !!!
    // !!! NOTE: unreliable transmission is still not supported, set bReliable on true for now!
    // !!!
    int rc = fragmentAndTransmitMessage (ui8MsgType, ppszOutgoingInterfaces, ui32TargetAddress,
                                         ui16MsgId, ui8HopCount, ui8TTL, ui16DelayTolerance,
                                         pMsgMetaData, ui16MsgMetaDataLen, pMsg, ui16MsgLen,
                                         true, false, pszHints);
    _m.unlock();
    return rc;
}

int NetworkMessageService::transmitReliableMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                                    uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                                    const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen,
                                                    const char *pszHints)
{
    _m.lock();
    checkAndLogMsg ("NetworkMessageService::transmitReliableMessage", Logger::L_MediumDetailDebug,
                    "transmitting a reliable message of size %d\n",
                    (int) ui16MsgLen);
    int rc = fragmentAndTransmitMessage (ui8MsgType, ppszOutgoingInterfaces, ui32TargetAddress,
                                         ui16MsgId, ui8HopCount, ui8TTL, ui16DelayTolerance,
                                         pMsgMetaData, ui16MsgMetaDataLen, pMsg, ui16MsgLen,
                                         true, true, pszHints);
    _m.unlock();
    return rc;
}

int NetworkMessageService::fragmentAndTransmitMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                                       uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                                       const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen,
                                                       bool bReliable,  bool bExpedited, const char *pszHints)
{
    const char * const pszMethodName = "NetworkMessageService::fragmentAndTransmitMessage";

    resolveProxyDatagramSocketAddresses();

    if (!_bPrimaryInterfaceIdSet) {
        return -1;
    }

    uint32 ui32BindingInterface = (_bPrimaryInterfaceIdSet ? _ui32PrimaryInterface : NetUtils::getLocalIPAddress().s_addr);
    checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug,
                    "outgoing packet src addr: %u\n",
                    ui32BindingInterface);

    if (getMinMTU() < NetworkMessage::FIXED_HEADER_LENGTH) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "minimum MTU is %d, which is smaller that even the header length of %d - cannot transmit message\n",
                        (int) getMinMTU(), (int) NetworkMessage::FIXED_HEADER_LENGTH);
        return -2;
    }
    uint16 ui16PayLoadLen = getMinMTU() - NetworkMessage::FIXED_HEADER_LENGTH;
    uint32 ui32TotDataLen = ui16MsgMetaDataLen + ui16MsgLen;
    NetworkMessage *pNetMsg = NULL;
    checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug,
                    "MTU = %d; payload length = %d; total data length = %d\n",
                    (int) getMinMTU(), (int) ui16PayLoadLen, (int) ui32TotDataLen);

    if (ui32TotDataLen <=  ui16PayLoadLen) {
        //===================================
        // NO NEED TO FRAMGENT
        //===================================
        pNetMsg = bReliable ? _pMessageFactory->getReliableDataMessage (ui8MsgType, _ui32PrimaryInterface,
                                                                        ui32TargetAddress, ui8HopCount, ui8TTL,
                                                                        NetworkMessage::CT_DataMsgComplete,
                                                                        pMsgMetaData, ui16MsgMetaDataLen,
                                                                        pMsg, ui16MsgLen) :
                              _pMessageFactory->getDataMessage (ui8MsgType, _ui32PrimaryInterface,
                                                                ui32TargetAddress, _ui16BroadcastedMsgCounter++,
                                                                ui8HopCount, ui8TTL,
                                                                NetworkMessage::CT_DataMsgComplete,
                                                                pMsgMetaData, ui16MsgMetaDataLen,
                                                                pMsg, ui16MsgLen);

        return sendNetworkMessage (pNetMsg, ppszOutgoingInterfaces, ui32TargetAddress,
                                   ui16DelayTolerance, bReliable, bExpedited, true, pszHints);
    }

    //===================================
    // THE MESSAGE MUST BE FRAGMENTED
    //===================================
    bool isFirst = true;
    uint16 ui16Offset = 0;
    //-----------------------------------
    // Fragment the MetaData
    // NOTE: I assume a Message always contains some Data
    //-----------------------------------
    NetworkMessage::ChunkType chunkType;
    for (; (ui16Offset + ui16PayLoadLen) <= ui16MsgMetaDataLen; ui16Offset += ui16PayLoadLen) {
        chunkType = NetworkMessage::CT_DataMsgInter;
        if (isFirst) {
            chunkType = NetworkMessage::CT_DataMsgStart;
            isFirst = false;
        }
        pNetMsg = bReliable ? _pMessageFactory->getReliableDataMessage (ui8MsgType, _ui32PrimaryInterface,
                                                                        ui32TargetAddress, ui8HopCount, ui8TTL, chunkType,
                                                                        ((const char *) pMsgMetaData) + ui16Offset, ui16PayLoadLen,
                                                                        NULL, 0) :
                              _pMessageFactory->getDataMessage (ui8MsgType, _ui32PrimaryInterface, ui32TargetAddress,
                                                                _ui16BroadcastedMsgCounter++, ui8HopCount, ui8TTL, chunkType,
                                                                ((const char *) pMsgMetaData) + ui16Offset, ui16PayLoadLen,
                                                                NULL, 0);

        //pNetMsg->display();
        sendNetworkMessage (pNetMsg, ppszOutgoingInterfaces, ui32TargetAddress,
                            ui16DelayTolerance, bReliable, false, true, pszHints);
    }
    // Remind the last part of the MetaData...
    const void *pMsgMetaDataReminder = NULL;
    uint16 ui16MsgMetaDataReminderLen = ui16MsgMetaDataLen - ui16Offset;
    if (ui16MsgMetaDataReminderLen > 0) {
        pMsgMetaDataReminder = ((const char *) pMsgMetaData) + ui16Offset;
    }

    //-----------------------------------
    // Fragment the Message
    //-----------------------------------
    ui16Offset = 0;
    uint16 ui16MsgLenTmp = ui16PayLoadLen - ui16MsgMetaDataReminderLen;
    if (ui16MsgLen <=  ui16MsgLenTmp) {
        // Write the first and last message fragment
        pNetMsg = bReliable ? _pMessageFactory->getReliableDataMessage (ui8MsgType, _ui32PrimaryInterface,
                                                                        ui32TargetAddress, ui8HopCount, ui8TTL,
                                                                        NetworkMessage::CT_DataMsgEnd,
                                                                        pMsgMetaDataReminder, ui16MsgMetaDataReminderLen,
                                                                        pMsg, ui16MsgLen) :
                              _pMessageFactory->getDataMessage (ui8MsgType, _ui32PrimaryInterface,
                                                                ui32TargetAddress, _ui16BroadcastedMsgCounter++,
                                                                ui8HopCount, ui8TTL, NetworkMessage::CT_DataMsgEnd,
                                                                pMsgMetaDataReminder, ui16MsgMetaDataReminderLen,
                                                                pMsg, ui16MsgLen);

        //pNetMsg->display();
        sendNetworkMessage (pNetMsg, ppszOutgoingInterfaces, ui32TargetAddress,
                            ui16DelayTolerance, bReliable, false, true, pszHints);
    }
    else {
        // Write the FIRST message fragment of the Message
        if (isFirst) {
            chunkType = NetworkMessage::CT_DataMsgStart;
            isFirst = false;
        }
        else {
            chunkType = NetworkMessage::CT_DataMsgInter;
        }
        pNetMsg = bReliable ? _pMessageFactory->getReliableDataMessage (ui8MsgType, _ui32PrimaryInterface,
                                                                        ui32TargetAddress, ui8HopCount, ui8TTL, chunkType,
                                                                        pMsgMetaDataReminder, ui16MsgMetaDataReminderLen,
                                                                        pMsg, ui16MsgLenTmp) :
                              _pMessageFactory->getDataMessage (ui8MsgType, _ui32PrimaryInterface,
                                                                ui32TargetAddress, _ui16BroadcastedMsgCounter++,
                                                                ui8HopCount, ui8TTL, chunkType,
                                                                pMsgMetaDataReminder, ui16MsgMetaDataReminderLen,
                                                                pMsg, ui16MsgLenTmp);

        //pNetMsg->display();
        sendNetworkMessage (pNetMsg, ppszOutgoingInterfaces, ui32TargetAddress,
                            ui16DelayTolerance, bReliable, false, true, pszHints);

        ui16Offset += ui16MsgLenTmp;

        // Write the INTERNAL fragments of the Message (if any)
        const void *pMsgTmp = NULL;
        while ((ui16Offset + ui16PayLoadLen) < ui16MsgLen) {
            pMsgTmp = ((const char *) pMsg) + ui16Offset;
            pNetMsg = bReliable ? _pMessageFactory->getReliableDataMessage (ui8MsgType, _ui32PrimaryInterface,
                                                                            ui32TargetAddress, ui8HopCount, ui8TTL,
                                                                            NetworkMessage::CT_DataMsgInter,
                                                                            NULL, 0, pMsgTmp, ui16PayLoadLen) :
                                  _pMessageFactory->getDataMessage (ui8MsgType, _ui32PrimaryInterface, ui32TargetAddress,
                                                                    _ui16BroadcastedMsgCounter++, ui8HopCount, ui8TTL,
                                                                    NetworkMessage::CT_DataMsgInter,
                                                                    NULL, 0, pMsgTmp, ui16PayLoadLen);

            //pNetMsg->display();
            sendNetworkMessage (pNetMsg, ppszOutgoingInterfaces, ui32TargetAddress,
                                ui16DelayTolerance, bReliable, false, true, pszHints);
            ui16Offset += ui16PayLoadLen;
        }

        // Write the LAST fragment of the Message
        pMsgTmp = ((const char *) pMsg) + ui16Offset;
        ui16MsgLenTmp = ui16MsgLen - ui16Offset;
        pNetMsg = bReliable ? _pMessageFactory->getReliableDataMessage (ui8MsgType, _ui32PrimaryInterface,
                                                                        ui32TargetAddress, ui8HopCount, ui8TTL,
                                                                        NetworkMessage::CT_DataMsgEnd,
                                                                        NULL, 0, pMsgTmp, ui16MsgLenTmp) :
                              _pMessageFactory->getDataMessage (ui8MsgType, _ui32PrimaryInterface, ui32TargetAddress,
                                                                _ui16BroadcastedMsgCounter++, ui8HopCount, ui8TTL,
                                                                NetworkMessage::CT_DataMsgEnd,
                                                                NULL, 0, pMsgTmp, ui16MsgLenTmp);

        //pNetMsg->display();
        sendNetworkMessage (pNetMsg, ppszOutgoingInterfaces, ui32TargetAddress,
                            ui16DelayTolerance, bReliable, false, true, pszHints);
    }

    return 0;
}

void NetworkMessageService::run()
{
    started();
    uint8 ui8Count = 0;
    while(!terminationRequested()) {
        sendSAckMessagesNetworkMessage();
        if (ui8Count == K) {
            resendUnacknowledgedMessages();
            ui8Count = 0;
        }
        sleepForMilliseconds (_ui32RetransmissionTimeout / K);
        ui8Count++;
        //periodically reset queue length of dead/silent neighbors to 0
        cleanOldNeighborQueueLengths();
    }
    terminating();
}

uint16 NetworkMessageService::getMinMTU()
{
    StringHashtable<NetworkInterface>::Iterator i = _interfaces.getAllElements();
    uint16 ui16MinMTU = _ui16MTU;    // Set it to the default MTU
    while (!i.end()) {
        NetworkInterface *pNIC = i.getValue();
        if ((pNIC != NULL) && (pNIC->canSend()) && (pNIC->isConnected())) {
            if (ui16MinMTU == 0) {
                ui16MinMTU = pNIC->getMTU();
            }
            else if (pNIC->getMTU() < ui16MinMTU) {
                ui16MinMTU = pNIC->getMTU();
            }
        }
        i.nextElement();
    }
    if (ui16MinMTU < NetworkMessageV2::FIXED_HEADER_LENGTH) {
        return 0;
    }
    else {
        return ui16MinMTU - NetworkMessageV2::FIXED_HEADER_LENGTH;
    }
}

char ** NetworkMessageService::getActiveNICsInfoAsString (void)
{
	resolveProxyDatagramSocketAddresses();

    _m.lock();
    unsigned int ulLen = _interfaces.getCount() + 1;
    if (ulLen == 1) {
        _m.unlock();
        return NULL;
    }
    char **ppszInterfaces = (char **) calloc (ulLen, sizeof (char *));
    if (ppszInterfaces != NULL) {
        StringHashtable<NetworkInterface>::Iterator iter = _interfaces.getAllElements();
        for (unsigned int i = 0; !iter.end() && i < ulLen; iter.nextElement()) {
            NetworkInterface *pNetInt = iter.getValue();
            if ((pNetInt != NULL) && (pNetInt->getNetworkAddr() != NULL) && (pNetInt->isConnected()) && (!pNetInt->boundToWildcardAddr())) {
                ppszInterfaces[i] = strDup (pNetInt->getNetworkAddr());
                if (ppszInterfaces[i] != NULL) {
                    i++;
                }
            }
        }
    }
    _m.unlock();
    return ppszInterfaces;
}

char ** NetworkMessageService::getActiveNICsInfoAsStringForDestinationAddr (const char *pszDestination)
{
    String sAddr (pszDestination);
    if (sAddr.startsWith ("pds://")) {
        // TODO: implement this case
    }
    else if (InetAddr::isIPv4Addr (pszDestination)) {
        InetAddr addr (pszDestination);
        return getActiveNICsInfoAsStringForDestinationAddr (addr.getIPAddress());
    }
    return NULL;
}

char ** NetworkMessageService::getActiveNICsInfoAsStringForDestinationAddr (uint32 ulSenderRemoteIPv4Addr)
{
    _m.lock();
    unsigned int ulLen = _interfaces.getCount() + 1;
    if (ulLen == 1) {
        _m.unlock();
        return NULL;
    }
    char **ppszInterfaces = (char **) calloc (ulLen, sizeof (char *));
    if (ppszInterfaces != NULL) {
        StringHashtable<NetworkInterface>::Iterator iter = _interfaces.getAllElements();
        for (unsigned int i = 0; !iter.end() && i < ulLen; iter.nextElement()) {
            NetworkInterface *pNetInt = iter.getValue();
            if ((pNetInt != NULL) && (pNetInt->getNetworkAddr() != NULL) && (pNetInt->isConnected()) && (!pNetInt->boundToWildcardAddr())) {
                InetAddr senderRemoteAddr (ulSenderRemoteIPv4Addr);
                if (NetUtils::areInSameNetwork (pNetInt->getNetworkAddr(), pNetInt->getNetmask(),
                                                senderRemoteAddr.getIPAsString(), pNetInt->getNetmask())) {
                    ppszInterfaces[i] = strDup (pNetInt->getNetworkAddr());
                    if (ppszInterfaces[i] != NULL) {
                        i++;
                    }
                }
            }
        }
    }
    _m.unlock();
    if ((ppszInterfaces != NULL) && (ppszInterfaces[0] == NULL)) {
        free (ppszInterfaces);
        ppszInterfaces = NULL;
    }
    return ppszInterfaces;
}

NetworkMessageService::PROPAGATION_MODE NetworkMessageService::getPropagationMode()
{
    return _mode;
}

uint32 NetworkMessageService::getDeliveryQueueSize (void)
{
    _mDeliveryQueue.lock();
    uint32 ui32QueueSize = _deliveryQueue.sizeOfQueue();
    _mDeliveryQueue.unlock();
    return ui32QueueSize;
}

int64 NetworkMessageService::getReceiveRate (const char *pszAddr)
{
    if (pszAddr == NULL) {
        return -1;
    }
    NetworkInterface *pNetIF = _interfaces.get (pszAddr);
    if (pNetIF == NULL) {
        return -2;
    }
    if (!InetAddr::isIPv4Addr (pszAddr)) {
        return -3;
    }
    
    uint32 ui32Bytes = pNetIF->getReceiveRate();

    // In UNIX the INADDR_ANY address must be bound in order to receive broadcast
    // messages (refer at the comment in init() for more information); thus it
    // is necessary to check whether traffic for the interface identified by
    // ui32Addr was received by the socket listening on 0.0.0.0.
    pNetIF = _interfaces.get (NetworkInterface::IN_ADDR_ANY_STR);
    if (pNetIF != NULL) {
        NetworkMessageReceiver *pRec = pNetIF->getReceiver();
        if (pRec != NULL && pRec->getType() == NetworkMessageReceiver::MCAST_NET_RCV) {
            ui32Bytes += ((ManycastNetworkMessageReceiver*) pRec)->getRateByBcastAddr (pNetIF->getBroadcastAddr());
        }
    }

    return ui32Bytes;
}

uint32 NetworkMessageService::getTransmissionQueueSize (const char *pszOutgoingInterface)
{
    if (pszOutgoingInterface == NULL) {
        return 0;
    }
    NetworkInterface *pNetIf = _interfaces.get (pszOutgoingInterface);
    if (pNetIf == NULL) {
        return 0;
    }
    return pNetIf->getTransmissionQueueSize();
}

uint8 NetworkMessageService::getRescaledTransmissionQueueSize (const char *pszOutgoingInterface)
{
    if (pszOutgoingInterface == NULL) {
        return 0;
    }
    NetworkInterface *pNetIf = _interfaces.get (pszOutgoingInterface);
    if (pNetIf == NULL) {
        return 0;
    }
    return pNetIf->getRescaledTransmissionQueueSize();
}

uint32 NetworkMessageService::getTransmissionQueueMaxSize (const char *pszOutgoingInterface)
{
    if (pszOutgoingInterface == NULL) {
        return 0;
    }
    NetworkInterface *pNetIf = _interfaces.get (pszOutgoingInterface);
    if (pNetIf == NULL) {
        return 0;
    }
    return pNetIf->getTransmissionQueueMaxSize();
}

uint32 NetworkMessageService::getTransmitRateLimit (const char *pszInterface)
{
    if (pszInterface == NULL) {
        return 0;
    }
    NetworkInterface *pNetIf = _interfaces.get (pszInterface);
    if (pNetIf == NULL) {
        return 0;
    }
    return pNetIf->getTransmitRateLimit();
}

int NetworkMessageService::setTransmissionQueueMaxSize (const char *pszOutgoingInterface,
                                                        uint32 ui32MaxSize)
{
    if (pszOutgoingInterface == NULL) {
        return -1;
    }
    NetworkInterface *pNetIf = _interfaces.get (pszOutgoingInterface);
    if (pNetIf == NULL) {
        return -2;
    }
    pNetIf->setTransmissionQueueMaxSize (ui32MaxSize);
    return 0;
}

int NetworkMessageService::setTransmitRateLimit (const char *pszInterface, const char *pszDestinationAddress,
                                                 uint32 ui32RateLimit)
{
    if (pszInterface == NULL) {
        checkAndLogMsg ("NetworkMessageService::setTransmitRateLimit1", Logger::L_Warning,
                        "pszInterface is NULL\n");
        return -1;
    }

    NetworkInterface *pNetIf = _interfaces.get (pszInterface);
    if (pNetIf == NULL) {
        return -2;
    }

    if (pszDestinationAddress == NULL) {
        return pNetIf->setTransmitRateLimit (ui32RateLimit);
    }
    else {
        return pNetIf->setTransmitRateLimit (pszDestinationAddress, ui32RateLimit);
    }
}

int NetworkMessageService::setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit)
{
    if (pszDestinationAddress == NULL) {
        checkAndLogMsg ("NetworkMessageService::setTransmitRateLimit3", Logger::L_Warning, "pszDestinationAddress is NULL\n");
        return -1;
    }

    StringHashtable<NetworkInterface>::Iterator i = _interfaces.getAllElements();
    while (!i.end()) {
        i.getValue()->setTransmitRateLimit (pszDestinationAddress, ui32RateLimit);
        i.nextElement();
    }
    return 0;
}

int NetworkMessageService::setTransmitRateLimit (uint32 ui32RateLimit)
{
    StringHashtable<NetworkInterface>::Iterator i = _interfaces.getAllElements();
    while (!i.end()) {
        i.getValue()->setTransmitRateLimit (ui32RateLimit);
        i.nextElement();
    }
    return 0;
}

uint32 NetworkMessageService::getLinkCapacity (const char *pszInterface)
{
    if (pszInterface == NULL) {
        checkAndLogMsg ("NetworkMessageService::getLinkCapacity", Logger::L_Warning, "pszInterface is NULL\n");
        return -1;
    }
    else {
        NetworkInterface *pNetIf = _interfaces.get (pszInterface);
        if (pNetIf == NULL) {
            checkAndLogMsg ("NetworkMessageService::getLinkCapacity", Logger::L_Warning,
                            "interface %s not found\n", pszInterface);
            return -2;
        }
        else {
            return pNetIf->getLinkCapacity();
        }
    }
}

void NetworkMessageService::setLinkCapacity (const char *pszInterface, uint32 ui32Capacity)
{
    if (pszInterface == NULL) {
        checkAndLogMsg ("NetworkMessageService::setLinkCapacity", Logger::L_Warning, "pszInterface is NULL\n");
    }
    else {
        NetworkInterface *pNetIf = _interfaces.get (pszInterface);
        if (pNetIf == NULL) {
            checkAndLogMsg ("NetworkMessageService::setLinkCapacity", Logger::L_Warning,
                            "interface %s not found\n", pszInterface);
        }
        else {
            pNetIf->setLinkCapacity (ui32Capacity);
        }
    }
}

uint8 NetworkMessageService::getNeighborQueueLength (const char *pchIncomingInterface,
                                                     unsigned long int ulSenderRemoteAddr)
{
    _mQueueLengthsTable.lock();
    ByInterface* pInterface = _tQueueLengthByInterface.get (pchIncomingInterface);
    if (pInterface == NULL) {
        _mQueueLengthsTable.unlock();
        return 0;
    }
    ByNeighbor* pNeighbor = pInterface->_tByNeighbor.get (ulSenderRemoteAddr);
    if (pNeighbor == NULL) {
        _mQueueLengthsTable.unlock();
        return 0;
    }
    _mQueueLengthsTable.unlock();
    return pNeighbor->_ui8QueueLength;
}

bool NetworkMessageService::clearToSend (const char *pszInterface)
{
    NetworkInterface *pNetIf = _interfaces.get (pszInterface);
    if (pNetIf != NULL) {
        return pNetIf->clearToSend();
    }
    return false;
}

bool NetworkMessageService::clearToSendOnAllInterfaces (void)
{
    for (StringHashtable<NetworkInterface>::Iterator iter = _interfaces.getAllElements(); !iter.end(); iter.nextElement()) {
        NetworkInterface *pNetIf = iter.getValue();
        if ((pNetIf->canSend()) && (!pNetIf->clearToSend())) {
            return false;
        }
    }
    return true;
}

//////////////////////////// Private Methods ///////////////////////////////////

bool NetworkMessageService::checkOldMessages (uint32 ui32SourceAddress, uint16 ui16SessionId, uint16 ui16MsgId)

{
    PeerState *pPS = _lastMsgs.get (ui32SourceAddress);

    if (pPS == NULL) {
        // New host
        return true;
    }
    else {
        if (ui16SessionId != pPS->ui16SessionId) {
            return true;
        }
        return !pPS->checkIfReceived (ui16MsgId);
    }
}

void NetworkMessageService::updateOldMessagesList (uint32 ui32SourceAddress, uint16 ui16SessionId, uint16 ui16MsgId)
{
    PeerState *pPS = _lastMsgs.get (ui32SourceAddress);
    if (pPS == NULL) {
        pPS = new PeerState;
        pPS->ui16SessionId = ui16SessionId;
        _lastMsgs.put (ui32SourceAddress, pPS);
    }
    else if (pPS->ui16SessionId != ui16SessionId) {
        pPS->ui16SessionId = ui16SessionId;
        pPS->resetMessageHistory();
    }
    pPS->setAsReceived (ui16MsgId);
}

int NetworkMessageService::messageArrived (NetworkMessage *pNetMsg, const char *pszIncomingInterface,
                                           unsigned long ulSenderRemoteAddress)
{
    if (pszIncomingInterface == NULL) {
        StringHashtable<NetworkInterface>::Iterator iter = _interfaces.getAllElements();
        while (!iter.end()) {
            NetworkInterface *pNetInt = iter.getValue();
            if (pNetInt != NULL && pNetInt->isConnected()) {
                const char *pszInterfaceAddr = pNetInt->getNetworkAddr();
                if (pszInterfaceAddr != NULL && (strcmp (pszInterfaceAddr, "0.0.0.0") != 0)) {
                    if (pszIncomingInterface == NULL) {
                        pszIncomingInterface = strDup (pszInterfaceAddr);
                    }
                    else {
                        // There are more than one connected network interface, I can't guess
                        free ((char *)pszIncomingInterface);
                        pszIncomingInterface = NULL;
                        break;
                    }
                }
            }
            iter.nextElement();
        }
    }

    if ((pszIncomingInterface != NULL) && (strcmp (pszIncomingInterface, "255.255.255.255") != 0)) {
        NetworkInterface *pNetIf = _interfaces.get (pszIncomingInterface);
        if (pNetIf != NULL && isUnicast (pNetMsg->getSourceAddress(), pszIncomingInterface)) {
            if (pNetIf->getType() == ManycastForwardingNetworkInterface::MCAST_FWD_IF) {
                ManycastForwardingNetworkInterface *pMcastFwdIf = (ManycastForwardingNetworkInterface*) pNetIf;
                pMcastFwdIf->addForwardingIpAddress (pNetMsg->getSourceAddress());
            }
        }
    }

    _mxMessageArrived.lock();
    uint32 ui32SourceAddress = pNetMsg->getSourceAddress();
    _pReassembler->refresh (ui32SourceAddress);

    //if the message contains a queue length, update the value for the corresponding neighbor
    uint8 ui8QueueLength;
    if (pNetMsg->getVersion() == 2) {
        ui8QueueLength = ((NetworkMessageV2*) pNetMsg)->getQueueLength();
    }
    else {
        ui8QueueLength = 0;
    }
    setNeighborQueueLength (ulSenderRemoteAddress, ui8QueueLength);

    const char * const pszMethodName = "NetworkMessageService::messageArrived";

    if (pNetMsg->getChunkType() == NetworkMessage::CT_SAck) {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "SAck message arrived\n");
        ackArrived (pNetMsg, pszIncomingInterface);
    }
    else {
        uint16 ui16MsgId = pNetMsg->getMsgId();
        if (isUnicast (pNetMsg->getTargetAddress(), pszIncomingInterface)) {
            checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                            "isUnicast() returned true for address %u\n",
                            pNetMsg->getTargetAddress());
            if (!_pReassembler->hasTSN (ui32SourceAddress, ui16MsgId) ||
                _pReassembler->isNewSessionId (ui32SourceAddress, pNetMsg->getSessionId())) {
                // It MAY be a new message
                int ret = _pReassembler->push (ui32SourceAddress, pNetMsg);
                if (ret == 0) {
                    // It is a new message and it has been added to the reassembler
                    for (NetworkMessage *pInnerNetMsg = _pReassembler->pop (ui32SourceAddress);
                         pInnerNetMsg != NULL; pInnerNetMsg = _pReassembler->pop (ui32SourceAddress)) {
                        notifyListeners (pInnerNetMsg, pszIncomingInterface, ulSenderRemoteAddress);
                        delete pInnerNetMsg;
                        pInnerNetMsg = NULL;
                    }
                }
                else if (ret > 0) {
                    // There is no need to deleted it, it is deleted by the
                    // Reassembler
                    checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug,
                                    "duplicated message - discarding\n");
                }
                else {
                    delete pNetMsg;
                    pNetMsg = NULL;
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                    "the message could not be stored in the reassembler.\n");
                }
            }
            // Send an SAck to the node that just sent us a unicast message
            int rc;
            uint32 ui32MsgLen;
            void *pSAck = _pReassembler->getSacks (ui32SourceAddress, _ui16MTU, ui32MsgLen);
            if (pSAck == NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                "getSacks() on Reassembler returned NULL for address %lu\n", ui32SourceAddress);
            }
            else if (_bPrimaryInterfaceIdSet) {
                resolveProxyDatagramSocketAddresses();
                NetworkMessage *pSAckMsg = _pMessageFactory->getSAckMessage (NMS_CTRL_MSG, _ui32PrimaryInterface,
                                                                             ui32SourceAddress, 0, 1,
                                                                             NULL, 0,
                                                                             pSAck, ui32MsgLen);
                if (0 != (rc = sendNetworkMessage (pSAckMsg, NULL, ui32SourceAddress, 0, false, true))) {
                    checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                    "sendNetworkMessage() failed with rc = %d\n", rc);
                }
                else {
                    uint16 ui16CumSak = _pReassembler->getCumulativeTSN (ui32SourceAddress);
                    checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                                    "sendNetworkMessage() succeeded: the cumulative "
                                    "TSN is %d\n", ui16CumSak);
                }
                free (pSAck);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not "
                                "acknowledge message because primary interface is not set\n");
                free (pSAck);
            }
        }
        else {
            bool bNewMessage = checkOldMessages (pNetMsg->getSourceAddress(), pNetMsg->getSessionId(), ui16MsgId);
            updateOldMessagesList (ui32SourceAddress, pNetMsg->getSessionId(), ui16MsgId);
            if (bNewMessage) {
                // Retransmit and notify (need to be mutex-ed since other
                // interfaces may be re-broadcasting the same message)
                //_mxRebroad.lock();
                rebroadcastMessage (pNetMsg, pszIncomingInterface);
                notifyListeners (pNetMsg, pszIncomingInterface, ulSenderRemoteAddress);
                delete pNetMsg;
                pNetMsg = NULL;
                //_mxRebroad.unlock();
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                                "duplicated message: sessionId: %u, msgId: %u\n",
                                pNetMsg->getSessionId(), ui16MsgId);
                delete pNetMsg;
                pNetMsg = NULL;
            }
        }
    }
    // TODO: delete arrived message ???
    _mxMessageArrived.unlock();
    return 0;
}

int NetworkMessageService::resendUnacknowledgedMessages()
{
    const char * const pszMethodName = "NetworkMessageService::resendUnacknowledgedMessages";
    // For each target
    _mxUnackedSentMessagesByDestination.lock();
    UnackedSentMessagesByDestination::Iterator i = _unackedSentMessagesByDestination.getAllElements();
    while (!i.end()) {
        uint32 ui32TargetAddress = i.getKey();
        UnackedSentMessagesByMsgId *pByMsgId = i.getValue();
        // resend the unacknowledged messages which timed out
        int64 i64RetransmissionStartingTime = getTimeInMilliseconds();
        UnackedMessageWrapper *pMsgWrap = pByMsgId->getFirst();
        UnackedMessageWrapper *pNextMsgWrap;
        while (pMsgWrap) {
            if (pMsgWrap->_i64SendingTime >= i64RetransmissionStartingTime) {
                // every time a message is retransmitted, its removed from its
                // current location in the UnackedSentMessagesByMsgId queue and
                // re-entered at the end of it.
                // Thus this check in order not to loop indefinitely
                break;
            }
            int64 ui64Now = getTimeInMilliseconds();
            // pMsgWrap may be deleted by the sendNetworkMessage() thus it is
            // necessary to retrieve the reference to the next element here.
            pNextMsgWrap = pByMsgId->getNext();
            uint32 ui32TimeOut = pMsgWrap->_ui32TimeOut * (1 + (pMsgWrap->_ui32RetransmitCount <= 5 ? pMsgWrap->_ui32RetransmitCount : 5));
            checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug,
                            "computed timeout for message %d to be %lu; last transmit time was %lu ms ago\n",
                            (int) pMsgWrap->_pNetMsg->getMsgId(), ui32TimeOut, (uint32) (ui64Now - pMsgWrap->_i64SendingTime));
            if ((pMsgWrap->_i64SendingTime + ui32TimeOut) <= ui64Now) {
                checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug,
                                "resending message %u\n", pMsgWrap->_pNetMsg->getMsgId());
                sendNetworkMessage (pMsgWrap->_pNetMsg, pMsgWrap->_ppszOutgoingInterfaces,
                                    ui32TargetAddress, pMsgWrap->_ui16DelayTolerance, true, true); // Send this as an expedited message, in order to get around the 300 message limit on the outgoing queue in NetworkInterface
            }
            else {
                // The UnackedMessageWrappers are ordered by time-out time, therefore,
                // if the current UnackedMessageWrapper timeout has not been triggered
                // yet, the further one's will not either.
                checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                                "NOT resending message %u\n", pMsgWrap->_pNetMsg->getMsgId());
                break;
            }
            pMsgWrap = pNextMsgWrap;
        }
        i.nextElement();
    }
    _mxUnackedSentMessagesByDestination.unlock();
    return 0;
}

int NetworkMessageService::resolveProxyDatagramSocketAddresses (void)
{
    _mProxyInterfacesToResolve.lock();
    if (_proxyInterfacesToResolve.isEmpty()) {
        _mProxyInterfacesToResolve.unlock();
        return 0;
    }
    NetworkInterface *pNetInt;
    bool bResolved = false;
    _proxyInterfacesToResolve.resetGet();
    while (NULL != (pNetInt = _proxyInterfacesToResolve.getNext())) {
        if (pNetInt->getNetworkAddr() != NULL) {
            InetAddr actualAddr = InetAddr (pNetInt->getNetworkAddr());
            _interfaces.put (actualAddr.getIPAsString(), pNetInt);
            checkAndLogMsg ("NetworkMessageService::resolveProxyDatagramSocketAddresses", Logger::L_Info,
                            "resolved the address for %s to be %s\n", pNetInt->getBindingInterfaceSpec(), pNetInt->getNetworkAddr());
            if (!_bPrimaryInterfaceIdSet) {
                InetAddr actualAddr = InetAddr (pNetInt->getNetworkAddr());
                _ui32PrimaryInterface = actualAddr.getIPAddress();
                _bPrimaryInterfaceIdSet = true;
                checkAndLogMsg ("NetworkMessageService::resolveProxyDatagramSocketAddresses", Logger::L_Info,
                                "set the primary interface address to be %s\n", pNetInt->getNetworkAddr());
            }
			pNetInt->start();
            _proxyInterfacesToResolve.remove (pNetInt);
            _proxyInterfacesToResolve.resetGet();      // Must restart the iteration when deleting elements from the list
        }
        else {
            checkAndLogMsg ("NetworkMessageService::resolveProxyDatagramSocketAddresses", Logger::L_LowDetailDebug,
                            "not yet resolved address for %s\n", pNetInt->getBindingInterfaceSpec());
        }
    }
    _mProxyInterfacesToResolve.unlock();
    return 0;
}

int NetworkMessageService::ackArrived (NetworkMessage *pNetMsg, const char *)
{
    const char * const pszMethodName = "NetworkMessageService::ackArrived";
    BufferReader bw (pNetMsg->getMsg(), pNetMsg->getLength());
    SAckTSNRangeHandler tsnhandler;
    tsnhandler.read (&bw, pNetMsg->getLength());

    _mxUnackedSentMessagesByDestination.lock();
    UnackedSentMessagesByMsgId *pByMsgId = _unackedSentMessagesByDestination.get (pNetMsg->getSourceAddress());
    if (pByMsgId != NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "pByMsgId is not NULL\n");
        uint16 ui16Start, ui16End;
        UI16Wrapper *pWrap = new UI16Wrapper;
        pWrap->ui16 = tsnhandler.getCumulativeTSN();
        _cumulativeTSNByDestination.put (pNetMsg->getSourceAddress(), pWrap);
        // Every message
        // - with msgId less or equal to the cumulative TSN          OR
        // - with msgId included in a TSN range                      OR
        // - that have reached the threshold of retransmissions
        // must be deleted from the queue of the un-acked messages

        // Remove every message with msgId less or equal than the cumulative TSN
        UnackedMessageWrapper * pMsgWrap = pByMsgId->getFirst();
        UnackedMessageWrapper *pNextMsgWrap;
        uint16 ui16MsgId;
        while (pMsgWrap) {
            pNextMsgWrap = pByMsgId->getNext();
            ui16MsgId = pMsgWrap->_pNetMsg->getMsgId();
            if (SequentialArithmetic::lessThanOrEqual (ui16MsgId, tsnhandler.getCumulativeTSN())) {
                checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                                "Removing message %u from UnackedSentMessagesByMsgId, for node %u\n",
                                ui16MsgId, pNetMsg->getSourceAddress());
                pByMsgId->remove (pMsgWrap);
                delete pMsgWrap;
            }
            pMsgWrap = pNextMsgWrap;
        }
        // Remove every message left that is included in a TSN range or that
        // has reached the threshold of retransmissions
        tsnhandler.resetGet();
        pByMsgId->resetGet();
        for (int i = tsnhandler.getFirst (ui16Start, ui16End); i == 0; i = tsnhandler.getNext (ui16Start, ui16End)) {
            pMsgWrap = pByMsgId->getFirst();
            while (pMsgWrap) {
                pNextMsgWrap = pByMsgId->getNext();
                ui16MsgId = pMsgWrap->_pNetMsg->getMsgId();
                if (((SequentialArithmetic::greaterThanOrEqual (ui16MsgId, ui16Start)) &&
                     (SequentialArithmetic::lessThanOrEqual (ui16MsgId, ui16End))) ||
                    ((_ui8DefMaxNumOfRetransmissions != 0) &&
                     (pMsgWrap->_ui32RetransmitCount >= _ui8DefMaxNumOfRetransmissions))) {
                    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                                    "Removing message %u from UnackedSentMessagesByMsgId, for node %u\n",
                                    ui16MsgId, pNetMsg->getSourceAddress());
                    pByMsgId->remove (pMsgWrap);
                    delete pMsgWrap;
                }
                else {
                   /* checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                                      "NON Removing message %u from UnackedSentMessagesByMsgId, for node %u. the range is %u <= X <= %u\n",
                                      pMsgWrap->_pNetMsg->getMsgId(), pNetMsg->getSourceAddress(), ui16Start, ui16End);*/
                }
                pMsgWrap = pNextMsgWrap;
            }
        }
    }
    _mxUnackedSentMessagesByDestination.unlock();
    delete pNetMsg;
    pNetMsg = NULL;
    this->yield();
    return 0;
 }

int NetworkMessageService::rebroadcastMessage (NetworkMessage *pNetMsg, const char *pchIncomingInterface)
{
    const char *const pszMethodName = "NetworkMessageService::rebroadcastMessage";
    int returnValue = 0;

    //----------------------------------------------------------------------
    // AGGREGATION
    //----------------------------------------------------------------------
    // if not aggregation -> serialize and broadcast** ("NetworkMessageServiceSender"? - aggregation and serialization)

    //----------------------------------------------------------------------
    // REBROADCAST
    //----------------------------------------------------------------------
    if (pNetMsg->getHopCount() < pNetMsg->getTTL()) {
        // TTL check (hopcount has already been incremented by receiver (caller of this method)
        checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug,
                        "Rebroadcasting...\n");

        return sendNetworkMessage (pNetMsg, NULL, EMPTY_RECIPIENT, 0, false);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                        "Hop limit\n");
    }

    return returnValue;
}

int NetworkMessageService::notifyListeners (NetworkMessage *pNetMsg, const char *pszIncomingInterface, unsigned long ulSenderRemoteAddr)
{
    if (!_bAsyncDelivery) {
        return callListeners (pNetMsg, pszIncomingInterface, ulSenderRemoteAddr);
    }
    else {
        QueuedMessage *pQMsg = new QueuedMessage();
        pQMsg->pMsg = MessageFactory::createNetworkMessageFromMessage (*pNetMsg, pNetMsg->getVersion());
        pQMsg->incomingInterface = pszIncomingInterface;
        pQMsg->ulSenderRemoteAddress = ulSenderRemoteAddr;
        _mDeliveryQueue.lock();
        _deliveryQueue.enqueue (pQMsg);
        checkAndLogMsg ("NetworkMessageService::notifyListeners", Logger::L_Info,
                        "queue size is: %lu\n", _deliveryQueue.sizeOfQueue());
        _cvDeliveryQueue.notifyAll();
        _mDeliveryQueue.unlock();
        return 0;
    }
}

int NetworkMessageService::callListeners (NetworkMessage *pNetMsg, const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr)
{
    const char * const pszMethodName = "NetworkMessageService::callListeners";
    // Get the listeners
    checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug, "Calling message listeners\n");
    NMSListerList *ptrLListeners = (NMSListerList*) _listeners.get (pNetMsg->getMsgType());
    if (ptrLListeners) {
        for (NetworkMessageServiceListener *pPtrLListener = ptrLListeners->getFirst();
             pPtrLListener; pPtrLListener = ptrLListeners->getNext()) {
            pPtrLListener->messageArrived (pchIncomingInterface, ulSenderRemoteAddr,
                                           pNetMsg->getMsgType(), pNetMsg->getMsgId(),
                                           pNetMsg->getHopCount(), pNetMsg->getTTL(),
                                           pNetMsg->getMetaData(), pNetMsg->getMetaDataLen(),
                                           pNetMsg->getMsg(), pNetMsg->getMsgLen());
        }
    }
    else {        
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                        "No listeners for msgtype\n");
    }
    return 0;
}

int NetworkMessageService::sendNetworkMessage (NetworkMessage *pNetMsg, const char **ppszOutgoingInterfaces,
                                               uint32 ui32Address, uint16 ui16DelayTolerance,
                                               bool bReliable, bool bExpedited, bool bDeallocatedNetMsg, const char *pszHints)
{
    const char * const pszMethodName = "NetworkMessageService::sendNetworkMessage";
    uint8 ui8Counter = 0;
    bool bAtLeastOneIF = false;
    if (ppszOutgoingInterfaces == NULL || ppszOutgoingInterfaces[ui8Counter] == NULL) {
        // No interface selected: send out on all interfaces (eventually pick one)
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                        "sending from all interfaces\n");
        StringHashtable<NetworkInterface>::Iterator itInterfaces = _interfaces.getAllElements();
        while (!itInterfaces.end()) {
            if (0 != stricmp (itInterfaces.getKey(), NetworkInterface::IN_ADDR_ANY_STR)) {
                checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                                "    interface: %s\n", itInterfaces.getKey());
                NetworkInterface *pNetInt = itInterfaces.getValue();
                if (pNetInt && pNetInt->canSend()) {
                    if (ui32Address == EMPTY_RECIPIENT) {
                        // Broadcast (or multicast) to the whole network (or group)
                        pNetInt->sendMessage (pNetMsg, bExpedited, pszHints);
                    }
                    else {
                        // Unicast the specified address
                        pNetInt->sendMessage (pNetMsg, ui32Address, bExpedited, pszHints);
                    }
                    bAtLeastOneIF = true;
                }
            }
            itInterfaces.nextElement();
        }
    }
    else {
        while (ppszOutgoingInterfaces[ui8Counter] != NULL) {
            uint32 ui32Outgoing = inet_addr (ppszOutgoingInterfaces[ui8Counter]);
            checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                            "sending from interface %s\n", ppszOutgoingInterfaces[ui8Counter]);
            NetworkInterface *pNetInt = _interfaces.get (ppszOutgoingInterfaces[ui8Counter]);

            if (pNetInt && pNetInt->canSend()) {
                if (ui32Address == EMPTY_RECIPIENT) {
                    // Broadcast (or multicast) to the whole network (or group)
                    pNetInt->sendMessage (pNetMsg, bExpedited, pszHints);
                }
                else {
                    // Unicast the specified address
                    pNetInt->sendMessage (pNetMsg, ui32Address, bExpedited, pszHints);
                }
                bAtLeastOneIF = true;
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Warning,
                                "specified interface (%u) not %s\n",
                                ui32Outgoing, (pNetInt ? "allowed to send" : "present"));
            }
            ui8Counter++;
        }
    }

    if (bReliable && bAtLeastOneIF) {
        // buffer it
        _mxUnackedSentMessagesByDestination.lock();
        UnackedSentMessagesByMsgId *pByMsgId = _unackedSentMessagesByDestination.get (ui32Address);
        if (pByMsgId == NULL) {
            pByMsgId = new UnackedSentMessagesByMsgId();
            _unackedSentMessagesByDestination.put (ui32Address, pByMsgId);
        }
        UI16Wrapper *pWrap = _cumulativeTSNByDestination.get (ui32Address);
        if ((pWrap == NULL) || SequentialArithmetic::greaterThan(pNetMsg->getMsgId(), pWrap->ui16)) {
            // Add the message to the queue of the un-acked messages only if the
            // message has message id greater than the cumulativeTSN.
            UnackedMessageWrapper *pMsgWrapper = new UnackedMessageWrapper (pNetMsg);
            pMsgWrapper->_i64SendingTime = getTimeInMilliseconds();
            pMsgWrapper->_ui32TimeOut = DEFAULT_TIME_OUT;
            pMsgWrapper->_ppszOutgoingInterfaces = strArrayDup (ppszOutgoingInterfaces, ui8Counter);
            pMsgWrapper->_ui16DelayTolerance = ui16DelayTolerance;
            pMsgWrapper->_ui32RetransmitCount = 0;
            UnackedMessageWrapper *pOldMsgWrapper = pByMsgId->remove (pMsgWrapper);
            if (pOldMsgWrapper != NULL) {
                pOldMsgWrapper->_bDeleteNetMsg = false;
                pMsgWrapper->_ui32RetransmitCount = pOldMsgWrapper->_ui32RetransmitCount + 1;
                delete pOldMsgWrapper;
                pOldMsgWrapper = NULL;
            }
            pByMsgId->append (pMsgWrapper);
            checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                            "added msg %u; pByMsgId has %d elements\n",
                            pMsgWrapper->_pNetMsg->getMsgId(), pByMsgId->getCount());
        }
        _mxUnackedSentMessagesByDestination.unlock();
    }
    else if (bDeallocatedNetMsg) {
        // delete the message
        delete pNetMsg;
        pNetMsg = NULL;
    }

    if (!bAtLeastOneIF) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "no interface available\n");
        return -1;
    }

    return 0;
}

int NetworkMessageService::sendSAckMessagesNetworkMessage (void)
{
    resolveProxyDatagramSocketAddresses();
    if (!_bPrimaryInterfaceIdSet) {
        checkAndLogMsg ("NetworkMessageService::sendSAckMessagesNetworkMessage",
                        Logger::L_Warning, "could not acknowledge message because "
                        "primary interface is not set\n");
        return -1;
    }
    int rc;
    uint32 ui32NumOfNeighbors, ui32MsgLen;
    checkAndLogMsg ("NetworkMessageService::sendSAckMessagesNetworkMessage", Logger::L_Info,
                    "entered\n");
    uint32 *pNeighbors = _pReassembler->getNeighborsToBeAcknowledged (ui32NumOfNeighbors);
    checkAndLogMsg ("NetworkMessageService::sendSAckMessagesNetworkMessage", Logger::L_Info,
                    "number of neighbors that should be acknowledged is %lu\n", ui32NumOfNeighbors);
    for (uint32 i = 0; i < ui32NumOfNeighbors; i++) {
        void *pSAck = _pReassembler->getSacks (pNeighbors[i], _ui16MTU, ui32MsgLen);
        NetworkMessage *pSAckMsg = _pMessageFactory->getSAckMessage (NMS_CTRL_MSG, _ui32PrimaryInterface,
                                                                     pNeighbors[i], 0, 1,
                                                                     NULL, 0,
                                                                     pSAck, ui32MsgLen);
        if (0 != (rc = sendNetworkMessage (pSAckMsg, NULL, pNeighbors[i], 0, false, true))) {
            checkAndLogMsg ("NetworkMessageService::sendSAckMessagesNetworkMessage", Logger::L_MildError,
                            "sendNetworkMessage() failed with rc = %d\n", rc);
            delete[] pNeighbors;
            return -2;
        }
        else {
            uint16 ui16CumSak = _pReassembler->getCumulativeTSN (pNeighbors[i]);
            checkAndLogMsg ("NetworkMessageService::sendSAckMessagesNetworkMessage",
                            Logger::L_LowDetailDebug, "sendNetworkMessage() succeded: "
                            "the cumulative TSN is %d\n", ui16CumSak);
        }
    }
    delete[] pNeighbors;
    return 0;
}

bool NetworkMessageService::isUnicast (uint32 ui32Address, const char *pszIncomingInterface)
{
    if (ui32Address == INADDR_BROADCAST || pszIncomingInterface == NULL) {
        return false;
    }

    NetworkInterface *pNetIf = _interfaces.get (pszIncomingInterface);
    if (pNetIf != NULL && !pNetIf->boundToWildcardAddr()) {          /*!!*/ // Check how this will work for the CSR
        const char *pszNetmask = pNetIf->getNetmask();
        if (pszNetmask != NULL) {
            // TODO: this assumes an IPv4 address - fix it
            InetAddr mask (pszNetmask);
            if (NetUtils::isBroadcastAddress (ui32Address, mask.getIPAddress())) {
                return false;
            }
        }
    }

    if (NetUtils::isMulticastAddress (inet_ntoa (*(struct in_addr *)&ui32Address))) {
        return false;
    }

    return true;
}

void NetworkMessageService::setNeighborQueueLength (unsigned long int ulSenderRemoteAddr, uint8 ui8QueueLength)
{
    char *pszInterface = NULL;
    for (NOMADSUtil::StringHashtable<NetworkInterface>::Iterator iInterface = _interfaces.getAllElements(); !iInterface.end(); iInterface.nextElement()) {
        NetworkInterface *pNetIf = iInterface.getValue();
        if ((pNetIf != NULL) && (pNetIf->getNetworkAddr() != NULL) && (pNetIf->isConnected())) {
            InetAddr senderRemoteAddr (ulSenderRemoteAddr);
            if (NetUtils::areInSameNetwork (pNetIf->getNetworkAddr(), pNetIf->getNetmask(),
                                            senderRemoteAddr.getIPAsString(), pNetIf->getNetmask())) {
                pszInterface = strDup (pNetIf->getNetworkAddr());
            }
        }
    }

    _mQueueLengthsTable.lock();
    ByInterface *pInterface = _tQueueLengthByInterface.get (pszInterface);
    if (pInterface == NULL) {
        pInterface = new ByInterface;
        _tQueueLengthByInterface.put (pszInterface, pInterface);
    }
    if (pszInterface != NULL) {
        free (pszInterface); // _tQueueLengthByInterface copies the key
    }
    ByNeighbor *pNeighbor = pInterface->_tByNeighbor.get (ulSenderRemoteAddr);
    if (pNeighbor == NULL) {
        pNeighbor = new ByNeighbor;
        pInterface->_tByNeighbor.put (ulSenderRemoteAddr, pNeighbor);
    }
    pNeighbor->_ui8QueueLength = ui8QueueLength;
    pNeighbor->_bUpdatedSinceLastCheck = true;
    _mQueueLengthsTable.unlock();
}

void NetworkMessageService::cleanOldNeighborQueueLengths (void)
{
    _mQueueLengthsTable.lock();
    for (StringHashtable<ByInterface>::Iterator iInterface = _tQueueLengthByInterface.getAllElements();
         !iInterface.end(); iInterface.nextElement()) {
        for (UInt32Hashtable<ByNeighbor>::Iterator iNeighbor = iInterface.getValue()->_tByNeighbor.getAllElements();
             !iNeighbor.end(); iNeighbor.nextElement()) {
            ByNeighbor* pNeighbor = iNeighbor.getValue();
            //if there has been no update, it means no message from the node has
            // been received. set its queue length to 0
            if (!pNeighbor->_bUpdatedSinceLastCheck) {
                pNeighbor->_ui8QueueLength = 0;
            }
            pNeighbor->_bUpdatedSinceLastCheck = false;
        }
    }
    _mQueueLengthsTable.unlock();
}

void NetworkMessageService::deliveryThread (void *pArg)
{
    NetworkMessageService *pThis = (NetworkMessageService *) pArg;
    QueuedMessage *pQMsg;
    while (!pThis->terminationRequested()) {
        pThis->_mDeliveryQueue.lock();
        while (NULL == (pQMsg = (QueuedMessage*) pThis->_deliveryQueue.dequeue())) {
            if (pThis->terminationRequested()) {
                pThis->_mDeliveryQueue.unlock();
                return;
            }
            pThis->_cvDeliveryQueue.wait (1000);
        }
        pThis->_mDeliveryQueue.unlock();
        pThis->callListeners (pQMsg->pMsg, (const char*) pQMsg->incomingInterface,
                              pQMsg->ulSenderRemoteAddress);
        delete pQMsg->pMsg;
        delete pQMsg;
    }
}

const char ** strArrayDup (const char **ppszString, uint8 ui8NStrings)
{
    if (ppszString == NULL || ui8NStrings == 0) {
        return NULL;
    }
    char **ppszStringCpys = (char **) calloc (sizeof (char*), ui8NStrings+1);
    for (uint8 ui8 = 0; ppszString[ui8] != NULL; ui8++) {
        ppszStringCpys[ui8] = strDup (ppszString[ui8]);
    }
    return (const char**) ppszStringCpys;
}

