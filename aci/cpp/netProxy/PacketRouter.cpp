/*
 * PacketRouter.cpp
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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

#if defined (LINUX)
    #include <algorithm>
#endif

#include "SequentialArithmetic.h"
#include "net/NetUtils.h"
#include "StringTokenizer.h"
#include "InetAddr.h"
#include "Logger.h"

#include "PacketRouter.h"
#include "ARPCache.h"
#include "AutoConnectionEntry.h"
#include "ConnectorReader.h"
#include "CSRConnector.h"
#include "MocketConnector.h"
#include "NetProxyConfigManager.h"
#include "SocketConnector.h"
#include "TCPConnTable.h"
#include "TCPManager.h"
#include "UDPConnector.h"
#include "UDPDatagramPacket.h"
#include "Constants.h"


#if defined (USE_DISSERVICE)
    using namespace IHMC_ACI;
#endif
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    int PacketRouter::init (NetworkInterface * const pInternalInterface, NetworkInterface * const pExternalInterface)
    {
        int rc;
        #if defined (USE_DISSERVICE)
            // Initialize DisService
            //_pDisService = new DisseminationService (_pConfigurationManager);  /*!!*/ // Need to fix
            if (0 != (rc = _pDisService->init())) {
                checkAndLogMsg ("PacketRouter::init", Logger::L_MildError,
                                "failed to initialize DisseminationService; rc = %d\n", rc);
                return -1;
            }
            _pDisService->registerDisseminationServiceListener (0, this);
            _pDisService->subscribe (0, "netproxy.reliable", 0, true, true, true);
            _pDisService->subscribe (0, "netproxy.unreliable", 0, false, true, true);
        #endif

        _pInternalInterface = pInternalInterface;
        _pExternalInterface = pExternalInterface;
        _daInternalHosts[0] = NetProxyApplicationParameters::NETPROXY_INTERNAL_INTERFACE_MAC_ADDR;
        _daExternalHosts[0] = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;

        UInt32Hashset * const hsEnabledConnectors = _pConfigurationManager->getEnabledConnectorsSet();
        for (UInt32Hashset::Iterator iter (hsEnabledConnectors); !iter.end(); iter.nextElement()) {
            ConnectorType ct = static_cast<ConnectorType> (iter.getKey());
            const uint16 ui16AcceptServerPort = Connector::getAcceptServerPortForConnector (ct);
            Connector * const pConnector = Connector::connectorFactoryMethod (ct);
            if (0 != (rc = pConnector->init (ui16AcceptServerPort))) {
                checkAndLogMsg ("PacketRouter::init", Logger::L_MildError,
                                "failed to initialize %sConnector on port %hu; rc = %d\n",
                                pConnector->getConnectorTypeAsString(), ui16AcceptServerPort, rc);
                return -2;
            }
            else {
                checkAndLogMsg ("PacketRouter::init", Logger::L_Info,
                                "successfully created and initialized a %sConnector listening on port %hu\n",
                                pConnector->getConnectorTypeAsString(), ui16AcceptServerPort);
            }

            // The following instruction also updates all entries in the AutoConnection Table that make use of the specified connector
            _pConnectionManager->registerConnector (pConnector);
        }

        return 0;
    }

    int PacketRouter::startThreads (void)
    {
        Connector *pConnector = NULL;
        UInt32Hashset * const hsEnabledConnectors = _pConfigurationManager->getEnabledConnectorsSet();
        for (UInt32Hashset::Iterator iter (hsEnabledConnectors); !iter.end(); iter.nextElement()) {
            ConnectorType ct = static_cast<ConnectorType> (iter.getKey());
            pConnector = _pConnectionManager->getConnectorForType (ct);
            if (!pConnector) {
                checkAndLogMsg ("PacketRouter::startThreads", Logger::L_Warning,
                                "found null pointer in the ConnectorSet for type %s; ignoring connector\n",
                                connectorTypeToString (ct));
                continue;
            }

            ManageableThread *pMT = dynamic_cast<ManageableThread*> (pConnector);
            if (pMT != NULL) {
                pMT->start();
                checkAndLogMsg ("PacketRouter::startThreads", Logger::L_Info,
                                "%sConnector started\n", pConnector->getConnectorTypeAsString());
            }
            else {
                checkAndLogMsg ("PacketRouter::startThreads", Logger::L_SevereError,
                                "unable to start connectorThread for connector of type %s %s\n",
                                pConnector->getConnectorTypeAsString());
                return -1;
            }
        }

        if (0 != _internalReceiverThread.start (false)) {
            return -2;
        }
        if (NetProxyApplicationParameters::GATEWAY_MODE && (0 != _externalReceiverThread.start (false))) {
            return -3;
        }
        if (0 != _localUDPDatagramsManagerThread.start (false)) {
            return -4;
        }
        if (0 != _localTCPTransmitterThread.start (false)) {
            return -5;
        }
        if (0 != _remoteTCPTransmitterThread.start (false)) {
            return -6;
        }
        if (_pConnectionManager->getAutoConnectionTable()->size() > 0) {
            if (0 != _autoConnectionManagerThread.start (false)) {
                return -7;
            }
        }
        if (0 != _cleanerThread.start (false)) {
            return -8;
        }
        if (NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED) {
            _updateGUIThread.init (_pConfigurationManager->getValue ("StatusNotificationAddresses"));
            if (0 != _updateGUIThread.start (false)) {
                return -9;
            }
        }

        while (!_bLocalUDPDatagramsManagerThreadRunning) {
            // Wait for UDPDatagramsManager thread to actually start execution!
            sleepForMilliseconds (10);
        }

        return 0;
    }

    int PacketRouter::joinThreads (void)
    {
        if (0 != PacketRouter::_localUDPDatagramsManagerThread.join()) {
            return -1;
        }
        if (0 != PacketRouter::_internalReceiverThread.join()) {
            return -2;
        }
        if (NetProxyApplicationParameters::GATEWAY_MODE && (0 != PacketRouter::_externalReceiverThread.join())) {
            return -3;
        }
        if (0 != PacketRouter::_localTCPTransmitterThread.join()) {
            return -4;
        }
        if (0 != PacketRouter::_remoteTCPTransmitterThread.join()) {
            return -5;
        }
        if (0 != PacketRouter::_cleanerThread.join()) {
            return -6;
        }
        PacketRouter::_autoConnectionManagerThread.join();
        PacketRouter::_updateGUIThread.join();

        return 0;
    }

    int PacketRouter::sendARPRequestForGatewayMACAddress (void) {
        int rc = 0;
        if (!NetProxyApplicationParameters::NETPROXY_IP_ADDR) {
            checkAndLogMsg ("PacketRouter::sendARPRequestForGatewayMACAddress", Logger::L_MildError,
                            "unable to generate an ARP request packet because the local node IP address is unknown\n");
            return -1;
        }
        if (!NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR) {
            checkAndLogMsg ("PacketRouter::sendARPRequestForGatewayMACAddress", Logger::L_MildError,
                            "unable to generate an ARP request packet because the gateway node IP address is unknown\n");
            return -2;
        }
        if (0 != (rc = sendARPRequest (_pExternalInterface, NetProxyApplicationParameters::NETPROXY_IP_ADDR,
                                       NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR))) {
            checkAndLogMsg ("PacketRouter::sendARPRequestForGatewayMACAddress", Logger::L_MildError,
                            "could not send an ARP request for IP address %s; sendARPRequest() failed with rc = %d\n",
                            InetAddr(NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR).getIPAsString(), rc);
            return -3;
        }
        checkAndLogMsg ("PacketRouter::sendARPRequestForGatewayMACAddress", Logger::L_HighDetailDebug,
                        "successfully sent an ARP request to the gateway with IP address %s\n",
                        InetAddr(NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR).getIPAsString());

        return 0;
    }

    void PacketRouter::wakeUpAutoConnectionAndRemoteTransmitterThreads (void)
    {
        if (_pConnectionManager->getAutoConnectionTable()->size() > 0) {
            _mAutoConnectionManager.lock();
            _cvAutoConnectionManager.notifyAll();
            _mAutoConnectionManager.unlock();
            checkAndLogMsg ("PacketRouter::wakeUpAutoConnectionAndRemoteTransmitterThreads", Logger::L_HighDetailDebug,
                            "AutoConnectionManager thread notified\n");
        }

        if (_pTCPConnTable->getActiveLocalConnectionsCount() > 0) {
            _mRemoteTCPTransmitter.lock();
            _cvRemoteTCPTransmitter.notifyAll();
            _mRemoteTCPTransmitter.unlock();
            checkAndLogMsg ("PacketRouter::wakeUpAutoConnectionAndRemoteTransmitterThreads", Logger::L_HighDetailDebug,
                            "RemoteTCPTransmitter thread notified\n");
        }
    }

    void PacketRouter::requestTermination (void)
    {
        if (PacketRouter::isTerminationRequested()) {
            return;
        }
        _bTerminationRequested = true;

        if (_pInternalInterface) {
            _pInternalInterface->requestTermination();     // This will end the execution of the internal receiver thread
        }
        if (_pExternalInterface) {
            _pExternalInterface->requestTermination();     // This will end the execution of the external receiver thread
        }

        // Lock AutoConnectionManager thread, clean AutoConnection Table, and notify the thread so that it can terminate its execution
        _mAutoConnectionManager.lock();
        _pConnectionManager->clearAutoConnectionTable();
        _cvAutoConnectionManager.notify();
        _mAutoConnectionManager.unlock();

        // Clear all entries in the TCP Connections Table
        _pTCPConnTable->clearTable();

        UInt32Hashset * const hsEnabledConnectors = _pConfigurationManager->getEnabledConnectorsSet();
        for (UInt32Hashset::Iterator iter (hsEnabledConnectors); !iter.end(); iter.nextElement()) {
            Connector * const pConnector = _pConnectionManager->getConnectorForType (static_cast<ConnectorType> (iter.getKey()));
            if (pConnector) {
                pConnector->terminateExecution();
            }
        }
        _bConnectorsDeleted = true;

        // Clear all connection cache, maps, and tables from memory
        _pConnectionManager->clearAllConnectionMappings();

        if (_mLocalUDPDatagramsManager.tryLock() == Mutex::RC_Ok) {
            _cvLocalUDPDatagramsManager.notify();
            _mLocalUDPDatagramsManager.unlock();
        }
        if (_mLocalTCPTransmitter.tryLock() == Mutex::RC_Ok) {
            _cvLocalTCPTransmitter.notify();
            _mLocalTCPTransmitter.unlock();
        }
        if (_mRemoteTCPTransmitter.tryLock() == Mutex::RC_Ok) {
            _cvRemoteTCPTransmitter.notify();
            _mRemoteTCPTransmitter.unlock();
        }
        if (_mCleaner.tryLock() == Mutex::RC_Ok) {
            _cvCleaner.notify();
            _mCleaner.unlock();
        }
        if (_mGUIUpdater.tryLock() == Mutex::RC_Ok) {
            _cvGUIUpdater.notify();
            _mGUIUpdater.unlock();
        }
    }

    void PacketRouter::InternalReceiverThread::run (void)
    {
        int rc, received = 0;
        uint8 ui8Buf[NetProxyApplicationParameters::ETHERNET_MAXIMUM_MFS];

        _bInternalReceiverThreadRunning = true;
        while (!PacketRouter::isTerminationRequested()) {
            received = PacketRouter::_pInternalInterface->readPacket (ui8Buf, sizeof (ui8Buf));
            if (received > 0) {
                if ((rc = handlePacketFromInternalInterface (ui8Buf, (uint16) received)) < 0) {
                    checkAndLogMsg ("PacketRouter::InternalReceiverThread::run", Logger::L_Warning,
                                    "handlePacketFromInternalInterface() failed when processing a %d "
                                    "bytes long packet; rc = %d\n", received, rc);
                }
            }
        }
        _bInternalReceiverThreadRunning = false;
    }

    void PacketRouter::ExternalReceiverThread::run (void)
    {
        int rc, received = 0;
        uint8 ui8Buf[NetProxyApplicationParameters::ETHERNET_MAXIMUM_MFS];

        _bExternalReceiverThreadRunning = true;
        while (!PacketRouter::isTerminationRequested()) {
            received = PacketRouter::_pExternalInterface->readPacket (ui8Buf, sizeof (ui8Buf));
            if (received > 0) {
                if ((rc = handlePacketFromExternalInterface (ui8Buf, (uint16) received)) < 0) {
                    checkAndLogMsg ("PacketRouter::ExternalReceiverThread::run", Logger::L_Warning,
                                    "handlePacketFromExternalInterface() failed when processing a %d "
                                    "bytes long packet; rc = %d\n", received, rc);
                }
            }
        }
        _bExternalReceiverThreadRunning = false;
    }

    void PacketRouter::LocalUDPDatagramsManagerThread::run (void)
    {
        _bLocalUDPDatagramsManagerThreadRunning = true;
        int rc;
        MutexUDPQueue readyUDPDatagramPacketQueue;
        register MutexUDPQueue *pUDPDatagramsQueue = NULL;
        register UDPDatagramPacket *pUDPDatagramPacket = NULL, *pReferenceUDPDatagramPacket = NULL;
        int64 i64CurrentCycleTime, i64NextCycleTime, i64TimeToWait;

        _mLocalUDPDatagramsManager.lock();
        if (NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT == 0) {
            pUDPDatagramsQueue = _pUDPReassembledDatagramsQueue;
            while (!PacketRouter::isTerminationRequested()) {
                i64CurrentCycleTime = getTimeInMilliseconds();
                pUDPDatagramsQueue->lock();
                pUDPDatagramsQueue->resetGet();

                while (pUDPDatagramPacket = pUDPDatagramsQueue->getNext()) {
                    if (!pUDPDatagramPacket->isDatagramComplete()) {
                        if ((i64CurrentCycleTime - pUDPDatagramPacket->getCreationTime()) >= LDMT_TIME_BETWEEN_ITERATIONS) {
                            // Timeout expired --> UDP Datagram could not be reassembled --> deleting incomplete fragment
                            delete pUDPDatagramsQueue->remove (pUDPDatagramPacket);
                            checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::run", Logger::L_MediumDetailDebug,
                                            "packet still incomplete after reassembly timeout expired; dropping fragment addressed to <%s:%hu>\n",
                                            InetAddr(pUDPDatagramPacket->getDestinationIPAddr()).getIPAsString(), pUDPDatagramPacket->getDestinationPortNum());
                            pUDPDatagramPacket = NULL;
                        }
                        continue;
                    }

                    if (pUDPDatagramPacket->getPacketLen() >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                        // Send packet by itself (since data was sent using UDP, it's not necessary to maintain the order)
                        if (pUDPDatagramPacket->getConnector()->isEnqueueingAllowed()) {
                            pUDPDatagramPacket->getConnection()->sendUDPUnicastPacketToRemoteHost (pUDPDatagramPacket->getRemoteProxyAddr(), pUDPDatagramPacket->getSourceIPAddr(),
                                                                                                   pUDPDatagramPacket->getDestinationIPAddr(), pUDPDatagramPacket->getUDPPacket(),
                                                                                                   pUDPDatagramPacket->getPacketLen(), pUDPDatagramPacket->getCompressionSetting(),
                                                                                                   ProxyMessage::Protocol (pUDPDatagramPacket->getPMProtocol()));
                        }
                        else {
                            checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::run", Logger::L_LowDetailDebug,
                                            "connector buffer is full; dropping an UDP Datagram of %u bytes addressed to address <%s:%hu>\n",
                                            pUDPDatagramPacket->getPacketLen(), InetAddr(pUDPDatagramPacket->getDestinationIPAddr()).getIPAsString(),
                                            pUDPDatagramPacket->getDestinationPortNum());
                        }
                        delete pUDPDatagramsQueue->remove (pUDPDatagramPacket);
                        pUDPDatagramPacket = NULL;
                    }
                    else if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() == 0) {
                        // In this case, no packets had previously been enqueued and packet size is less than the configured threshold
                        pReferenceUDPDatagramPacket = pUDPDatagramPacket;
                        readyUDPDatagramPacketQueue.enqueue (pUDPDatagramsQueue->remove (pUDPDatagramPacket));
                    }
                    else {
                        // In this case, some packets have already been enqueued and packet size is less than the configured threshold --> check if wrapping together is possible
                        if (pReferenceUDPDatagramPacket->canBeWrappedTogether (pUDPDatagramPacket)) {
                            // It's possible to wrap packets together
                            readyUDPDatagramPacketQueue.enqueue (pUDPDatagramsQueue->remove (pUDPDatagramPacket));
                        }
                    }

                    if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                        // Threshold has been reached --> sending new multiple UDP datagrams packet
                        if (0 > (rc = sendEnqueuedDatagramsToRemoteProxy (&readyUDPDatagramPacketQueue))) {
                            checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::run", Logger::L_MildError,
                                            "sendEnqueuedDatagramsToRemoteProxy() failed with rc = %d\n", rc);
                        }
                        readyUDPDatagramPacketQueue.removeAll (true);
                        pReferenceUDPDatagramPacket = NULL;
                        pUDPDatagramsQueue->resetGet();     // Need to reconsider any packet which was skipped because it could not be wrapped together with the others
                    }
                }

                if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() > 0) {
                    // Transmitting any enqueued UDP Datagrams
                    if (0 > (rc = sendEnqueuedDatagramsToRemoteProxy (&readyUDPDatagramPacketQueue))) {
                        checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::run", Logger::L_MildError,
                                        "sendEnqueuedDatagramsToRemoteProxy() failed with rc = %d\n", rc);
                    }
                    readyUDPDatagramPacketQueue.removeAll (true);
                    pReferenceUDPDatagramPacket = NULL;
                }
                pUDPDatagramsQueue->unlock();

                _cvLocalUDPDatagramsManager.wait (LDMT_TIME_BETWEEN_ITERATIONS);
            }
        }
        else {
            while (!PacketRouter::isTerminationRequested()) {
                i64CurrentCycleTime = getTimeInMilliseconds();
                i64NextCycleTime = 0;
                _mUDPDatagramsQueueHashTable.lock();
                UInt32Hashtable<MutexUDPQueue>::Iterator udpDatagramsQueueIterator = _ui32UDPDatagramsQueueHashTable.getAllElements();
                _mUDPDatagramsQueueHashTable.unlock();
                while (pUDPDatagramsQueue = udpDatagramsQueueIterator.getValue()) {
                    if (!pUDPDatagramsQueue->isEmpty()) {
                        pUDPDatagramsQueue->lock();
                        pUDPDatagramsQueue->resetGet();
                        while (pUDPDatagramPacket = pUDPDatagramsQueue->getNext()) {
                            if (!pUDPDatagramPacket->isDatagramComplete()) {
                                // Fragment incomplete
                                if ((i64CurrentCycleTime - pUDPDatagramPacket->getCreationTime()) >= NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT) {
                                    // Timeout expired --> UDP Datagram could not be reassembled --> deleting incomplete fragment
                                    checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::run", Logger::L_MediumDetailDebug,
                                                    "packet still incomplete after reassembly timeout expired; dropping fragment addressed to <%s:%hu>\n",
                                                    InetAddr(pUDPDatagramPacket->getDestinationIPAddr()).getIPAsString(), pUDPDatagramPacket->getDestinationPortNum());
                                    delete pUDPDatagramsQueue->remove (pUDPDatagramPacket);
                                    pUDPDatagramPacket = NULL;
                                }
                                continue;
                            }
                            if (pUDPDatagramPacket->getPacketLen() >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                                // Send packet by itself; since the used protocol is UDP, it's not necessary to maintain any order
                                if (pUDPDatagramPacket->getConnector()->isEnqueueingAllowed()) {
                                    pUDPDatagramPacket->getConnection()->sendUDPUnicastPacketToRemoteHost (pUDPDatagramPacket->getRemoteProxyAddr(), pUDPDatagramPacket->getSourceIPAddr(),
                                                                                                           pUDPDatagramPacket->getDestinationIPAddr(), pUDPDatagramPacket->getUDPPacket(),
                                                                                                           pUDPDatagramPacket->getPacketLen(), pUDPDatagramPacket->getCompressionSetting(),
                                                                                                           ProxyMessage::Protocol (pUDPDatagramPacket->getPMProtocol()));
                                }
                                else {
                                    checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::run", Logger::L_LowDetailDebug,
                                                    "connector buffer is full; dropping a UDP Datagram of %u bytes addressed to address <%s:%hu>\n",
                                                    pUDPDatagramPacket->getPacketLen(), InetAddr(pUDPDatagramPacket->getDestinationIPAddr()).getIPAsString(),
                                                    pUDPDatagramPacket->getDestinationPortNum());
                                }
                                delete pUDPDatagramsQueue->remove (pUDPDatagramPacket);
                                pUDPDatagramPacket = NULL;
                            }
                            else if ((readyUDPDatagramPacketQueue.getEnqueuedBytesCount() == 0) &&
                                ((i64CurrentCycleTime - pUDPDatagramPacket->getCreationTime()) < NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT)) {
                                // Check if the necessary size to send out a packet can be reached
                                pReferenceUDPDatagramPacket = pUDPDatagramPacket;
                                readyUDPDatagramPacketQueue.enqueue (pUDPDatagramPacket);
                                while ((readyUDPDatagramPacketQueue.getEnqueuedBytesCount() < NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) &&
                                    (pUDPDatagramPacket = pUDPDatagramsQueue->getNext())) {
                                    // Check if packet is complete, or continue to the next one
                                    if  (pUDPDatagramPacket->isDatagramComplete()) {
                                        // Check whether packet has to be sent by itself or it can be wrapped together with other packets
                                        if (pUDPDatagramPacket->getPacketLen() >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                                            // Send packet by itself; since the used protocol is UDP, it's not necessary to maintain any order
                                            if (pUDPDatagramPacket->getConnector()->isEnqueueingAllowed()) {
                                                pUDPDatagramPacket->getConnection()->sendUDPUnicastPacketToRemoteHost (pUDPDatagramPacket->getRemoteProxyAddr(),
                                                                                                                       pUDPDatagramPacket->getSourceIPAddr(),
                                                                                                                       pUDPDatagramPacket->getDestinationIPAddr(),
                                                                                                                       pUDPDatagramPacket->getUDPPacket(),
                                                                                                                       pUDPDatagramPacket->getPacketLen(),
                                                                                                                       pUDPDatagramPacket->getCompressionSetting(),
                                                                                                                       ProxyMessage::Protocol (pUDPDatagramPacket->getPMProtocol()));
                                            }
                                            else {
                                                checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::run", Logger::L_LowDetailDebug,
                                                                "connector buffer is full; dropping a UDP Datagram of %u bytes addressed to address <%s:%hu>\n",
                                                                pUDPDatagramPacket->getPacketLen(), InetAddr(pUDPDatagramPacket->getDestinationIPAddr()).getIPAsString(),
                                                                pUDPDatagramPacket->getDestinationPortNum());
                                            }
                                            delete pUDPDatagramsQueue->remove (pUDPDatagramPacket);
                                            pUDPDatagramPacket = NULL;
                                        }
                                        else if (pReferenceUDPDatagramPacket->canBeWrappedTogether (pUDPDatagramPacket)) {
                                            // Enqueue packet for multiple UDP datagrams later sending
                                            readyUDPDatagramPacketQueue.enqueue (pUDPDatagramPacket);
                                        }
                                    }
                                }

                                if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                                    // Required size reached --> creating and sending out MultipleUDPPacket and deleting buffered UDP Datagrams
                                    readyUDPDatagramPacketQueue.resetGet();
                                    while (pUDPDatagramPacket = readyUDPDatagramPacketQueue.getNext()) {
                                        pUDPDatagramsQueue->remove (pUDPDatagramPacket);
                                    }
                                    if (0 > (rc = sendEnqueuedDatagramsToRemoteProxy (&readyUDPDatagramPacketQueue))) {
                                        checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::run", Logger::L_MildError,
                                                        "sendEnqueuedDatagramsToRemoteProxy() failed with rc = %d\n", rc);
                                    }
                                    pUDPDatagramsQueue->resetGet();                 // Need to reconsider any skipped packet
                                    readyUDPDatagramPacketQueue.removeAll (true);
                                }
                                else {
                                    // Required size NOT reached --> wait until timeout expires or more packets come in
                                    readyUDPDatagramPacketQueue.removeAll (false);
                                }
                                pReferenceUDPDatagramPacket = NULL;
                            }
                            else if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() == 0) {
                                // In this case, configured timeout is expired, no packets had previously been enqueued and packet size is less than the configured threshold
                                pReferenceUDPDatagramPacket = pUDPDatagramPacket;
                                readyUDPDatagramPacketQueue.enqueue (pUDPDatagramsQueue->remove (pUDPDatagramPacket));
                            }
                            else {
                                // In this case, some packets have already been enqueued and packet size is less than the configured threshold --> check if wrapping together is possible
                                if (pReferenceUDPDatagramPacket->canBeWrappedTogether (pUDPDatagramPacket)) {
                                    readyUDPDatagramPacketQueue.enqueue (pUDPDatagramsQueue->remove (pUDPDatagramPacket));
                                }
                            }

                            // If the threshold has been reached, send multiple UDP datagrams packet to remote proxy and reset queue to continue processing
                            if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                                if (0 > (rc = sendEnqueuedDatagramsToRemoteProxy (&readyUDPDatagramPacketQueue))) {
                                    checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::run", Logger::L_MildError,
                                                    "sendEnqueuedDatagramsToRemoteProxy() failed with rc = %d\n", rc);
                                }
                                readyUDPDatagramPacketQueue.removeAll (true);
                                pReferenceUDPDatagramPacket = NULL;
                                pUDPDatagramsQueue->resetGet();     // Need to reconsider any packet which was skipped because it could not be wrapped together with the others
                            }
                        }

                        if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() > 0) {
                            // Transmitting any expired UDP Datagrams
                            if (0 > (rc = sendEnqueuedDatagramsToRemoteProxy (&readyUDPDatagramPacketQueue))) {
                                checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::run", Logger::L_MildError,
                                                "sendEnqueuedDatagramsToRemoteProxy() failed with rc = %d\n", rc);
                            }
                            readyUDPDatagramPacketQueue.removeAll (true);
                            pReferenceUDPDatagramPacket = NULL;
                        }

                        // Calculate next time when a timeout for a certain UDP datagram will expire
                        if (pUDPDatagramPacket = pUDPDatagramsQueue->peek()) {
                            i64NextCycleTime = (i64NextCycleTime == 0) ? (pUDPDatagramPacket->getCreationTime() + NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT) :
                                                                         std::min (i64NextCycleTime, (pUDPDatagramPacket->getCreationTime() +
                                                                                                      NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT));
                        }
                        pUDPDatagramsQueue->unlock();
                    }
                    udpDatagramsQueueIterator.nextElement();
                }

                if (PacketRouter::isTerminationRequested()) {
                    break;
                }
                i64TimeToWait = (i64NextCycleTime == 0) ? LDMT_TIME_BETWEEN_ITERATIONS : (i64NextCycleTime - i64CurrentCycleTime);
                _cvLocalUDPDatagramsManager.wait (i64TimeToWait);
            }
        }
        _mLocalUDPDatagramsManager.unlock();

        _bLocalUDPDatagramsManagerThreadRunning = false;
    }

    int PacketRouter::LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue (const InetAddr * const pRemoteProxyAddr, Connection * const pConnection, Connector * const pConnector,
                                                                                  const CompressionSetting * const pCompressionSetting, const ProxyMessage::Protocol pmProtocol,
                                                                                  const IPHeader * const pIPHeader, const UDPHeader * const pUDPHeader)
    {
        if (!pRemoteProxyAddr || !pConnection || !pConnector ||
            !pCompressionSetting || !pIPHeader || !pUDPHeader) {
            // All information is required
            return -1;
        }

        int rc;
        bool bCompleteFragment = false;
        MutexUDPQueue *pUDPDatagramsQueue = NULL;
        if (NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT == 0) {
            pUDPDatagramsQueue = _pUDPReassembledDatagramsQueue;
        }
        else {
            _mUDPDatagramsQueueHashTable.lock();
            pUDPDatagramsQueue = _ui32UDPDatagramsQueueHashTable.get (pIPHeader->destAddr.ui32Addr);
            if (!pUDPDatagramsQueue) {
                // New destination IP Address --> adding Queue to the hash table
                pUDPDatagramsQueue = new MutexUDPQueue();
                _ui32UDPDatagramsQueueHashTable.put (pIPHeader->destAddr.ui32Addr, pUDPDatagramsQueue);
            }
            _mUDPDatagramsQueueHashTable.unlock();
        }

        pUDPDatagramsQueue->lock();
        if ((pIPHeader->ui16FlagsAndFragOff & IP_OFFSET_FILTER) != 0) {
            // Fragment received --> looking for relative incomplete UDP datatgram
            rc = pUDPDatagramsQueue->reassembleUDPDatagram (pIPHeader, pUDPHeader);
            if ((rc == MutexUDPQueue::REASSEMBLING_NULL) || (rc == MutexUDPQueue::REASSEMBLING_IMPOSSIBLE) || (rc == MutexUDPQueue::REASSEMBLING_ERROR)) {
                pUDPDatagramsQueue->unlock();
                return -2;
            }
            else if (rc == MutexUDPQueue::REASSEMBLING_COMPLETE) {
                bCompleteFragment = true;
                const UDPDatagramPacket * const pUDPDatagramPacket = pUDPDatagramsQueue->findPacketFromIPHeader (pIPHeader);
                checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", Logger::L_MediumDetailDebug,
                                "correctly reassembled UDP datagram packet with %hu bytes of data and addressed to address %s:%hu\n",
                                (pUDPDatagramPacket->getPacketLen() - sizeof (UDPHeader)),
                                InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(), pUDPDatagramPacket->getDestinationPortNum());
            }
        }
        else {
            // Nagle's algorithm entails buffering received packets, adding them to a different queue, based on the destination IP
            UDPDatagramPacket *pUDPDatagramPacket = new UDPDatagramPacket (pRemoteProxyAddr, pConnection, pConnector, pCompressionSetting, pmProtocol, pIPHeader, pUDPHeader);
            if (!pUDPDatagramPacket) {
                checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", Logger::L_MildError,
                                "impossible to allocate memory for received packet of %hu bytes in total and addressed to IP address %s;\n",
                                pIPHeader->ui16TLen, InetAddr(pIPHeader->destAddr.ui32Addr).getIPAddress());
                pUDPDatagramsQueue->unlock();
                return -3;
            }
            bCompleteFragment = (pIPHeader->ui16FlagsAndFragOff & IP_MF_FLAG_FILTER) == 0;          // If flags and offset are all 0s, a complete packet has just been enqueued

            rc = pUDPDatagramsQueue->enqueue (pUDPDatagramPacket);
            if ((rc == MutexUDPQueue::ENQUEUING_NULL) || (rc == MutexUDPQueue::ENQUEUING_ERROR)) {
                // ENQUEUING_NULL should never be returned because of the previous checking
                if (rc == MutexUDPQueue::ENQUEUING_ERROR) {
                    checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", Logger::L_Warning,
                                    "impossible to enqueue received UDP packet with %hu bytes of data; rc = %d\n",
                                    pUDPDatagramPacket->getCurrentPacketLen(), rc);
                }
                pUDPDatagramsQueue->unlock();
                return -4;
            }
            else if (rc == MutexUDPQueue::ENQUEUING_BUFFER_FULL) {
                checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", Logger::L_LowDetailDebug,
                                "impossible to enqueue received UDP packet with %hu bytes of data; "
                                "%u bytes are already in the queue (%u bytes left); discarding packet\n",
                                pUDPDatagramPacket->getPacketLen(), pUDPDatagramsQueue->getEnqueuedBytesCount(),
                                pUDPDatagramsQueue->getSpaceLeftInBuffer());
            }
            else if (bCompleteFragment) {
                checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", Logger::L_HighDetailDebug,
                                "correctly enqueued received UDP packet with %hu bytes of data; %u bytes and %d packets are currently in the queue\n",
                                pUDPDatagramPacket->getPacketLen(), pUDPDatagramsQueue->getEnqueuedBytesCount(), pUDPDatagramsQueue->size());
            }
            else {
                checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", Logger::L_HighDetailDebug,
                                "correctly enqueued an incomplete UDP fragment; %u bytes and %d packets are currently in the queue\n",
                                pUDPDatagramsQueue->getEnqueuedBytesCount(), pUDPDatagramsQueue->size());
            }
        }
        uint32 ui32TotalEnqueuedBytes = pUDPDatagramsQueue->getEnqueuedBytesCount();
        pUDPDatagramsQueue->unlock();

        if ((ui32TotalEnqueuedBytes >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) ||
            ((NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT == 0) && bCompleteFragment)) {
            // Waking up LocalUDPDatagramsManager Thread, if sleeping, since packets might be ready to be sent to remote proxies
            if (Mutex::RC_Ok == _mLocalUDPDatagramsManager.tryLock()) {
                _cvLocalUDPDatagramsManager.notify();
                _mLocalUDPDatagramsManager.unlock();
            }
            if (ui32TotalEnqueuedBytes >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                // Force yielding to LocalUDPDatagramsManagerThread so that transmission can be performed
                yield();
            }
        }

        return 0;
    }

    int PacketRouter::LocalUDPDatagramsManagerThread::sendEnqueuedDatagramsToRemoteProxy (MutexUDPQueue * const pUDPDatagramsQueue) const
    {
        static const UDPDatagramPacket *pReferenceUDPDatagramPacket = NULL;

        if (!pUDPDatagramsQueue) {
            return -1;
        }
        if (pUDPDatagramsQueue->isEmpty()) {
            return 0;
        }

        pReferenceUDPDatagramPacket = pUDPDatagramsQueue->peek();
        Connection * const pConnection = pReferenceUDPDatagramPacket->getConnection();
        if (pReferenceUDPDatagramPacket->getConnector()->isEnqueueingAllowed()) {
            int rc;
            if (pUDPDatagramsQueue->size() > 1) {
                if (0 != (rc = pConnection->sendMultipleUDPDatagramsToRemoteHost (pReferenceUDPDatagramPacket->getRemoteProxyAddr(), pReferenceUDPDatagramPacket->getSourceIPAddr(),
                                                                                  pReferenceUDPDatagramPacket->getDestinationIPAddr(), pUDPDatagramsQueue,
                                                                                  pReferenceUDPDatagramPacket->getCompressionSetting(),
                                                                                  ProxyMessage::Protocol (pReferenceUDPDatagramPacket->getPMProtocol())))) {
                    checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::sendEnqueuedDatagramsToRemoteProxy", Logger::L_MildError,
                                    "sendMultipleUDPDatagramsToRemoteHost() failed with rc = %d\n", rc);
                    return -2;
                }
            }
            else {
                if (0 != (rc = pConnection->sendUDPUnicastPacketToRemoteHost (pReferenceUDPDatagramPacket->getRemoteProxyAddr(), pReferenceUDPDatagramPacket->getSourceIPAddr(),
                                                                              pReferenceUDPDatagramPacket->getDestinationIPAddr(), pReferenceUDPDatagramPacket->getUDPPacket(),
                                                                              pReferenceUDPDatagramPacket->getPacketLen(), pReferenceUDPDatagramPacket->getCompressionSetting(),
                                                                              ProxyMessage::Protocol (pReferenceUDPDatagramPacket->getPMProtocol())))) {
                    checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::sendEnqueuedDatagramsToRemoteProxy", Logger::L_MildError,
                                    "sendUDPUnicastPacketToRemoteHost() failed with rc = %d\n", rc);
                    return -3;
                }
            }
        }
        else {
            checkAndLogMsg ("PacketRouter::LocalUDPDatagramsManagerThread::sendEnqueuedDatagramsToRemoteProxy", Logger::L_LowDetailDebug,
                            "connector buffer is full; dropping a Proxy Packet of %u bytes (containing %d UDP datagrams)\n",
                            pUDPDatagramsQueue->getEnqueuedBytesCount(), pUDPDatagramsQueue->size());
            return 0;
        }

        return pUDPDatagramsQueue->getEnqueuedBytesCount();
    }

    void PacketRouter::LocalTCPTransmitterThread::run (void)
    {
        int rc;
        int64 i64CurrTime = 0;
        register Entry *pEntry = NULL;
        ReceivedData *pData = NULL;
        const TCPSegment *pSegment = NULL;

        _bLocalTCPTransmitterThreadRunning = true;
        _mLocalTCPTransmitter.lock();
        while (!PacketRouter::isTerminationRequested()) {
            // Check the outgoing buffers in the TCPConnTable to see if there is data to be transmitted
            _pTCPConnTable->lock();
            _pTCPConnTable->resetGet();
            while (NULL != (pEntry = _pTCPConnTable->getNextActiveLocalEntry())) {
                if (pEntry->tryLock() == Mutex::RC_Ok) {
                    i64CurrTime = getTimeInMilliseconds();
                    pData = pEntry->outBuf.getFirst();

                    // Check if remote connection was lost (exclude SYN_SENT status because local application has not replied with a SYN+ACK yet)
                    if (!pEntry->getConnection() && (pEntry->localStatus != TCTLS_SYN_SENT) &&
                        (pEntry->remoteStatus != TCTRS_Unknown) && (pEntry->remoteStatus != TCTRS_Disconnected)) {
                        checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_Warning,
                                        "L%hu-R%hu: Connection object to remote proxy was lost; sending an RST to local host\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_Warning,
                                            "L%hu-R%hu: sendTCPPacketToHost() failed sending an RST to local host; with rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        pEntry->reset();
                        pEntry->unlock();
                        continue;
                    }

                    if (pData != NULL) {
                        if ((pData->_i64LastTransmitTime > 0) && ((pData->_i64LastTransmitTime - pEntry->i64LastAckTime) > TCPConnTable::STANDARD_MSL)) {
                            // It's not the first transmission attempt and it has already been tried to retransmit many times --> LOCAL CONNECTION IS DOWN: CLOSE IT.
                            if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_MildError,
                                                "L%hu-R%hu: failed to send RST packet to host; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_Warning,
                                            "L%hu-R%hu: local host seems not being answering to packets; sent a reset request to both remote and local applications\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            pEntry->unlock();
                            pData = NULL;
                            continue;
                        }

                        if ((pEntry->localStatus == TCTLS_SYN_SENT) && ((pData->getTCPFlags() & TCPHeader::TCPF_SYN) != 0) &&
                            ((i64CurrTime - pEntry->i64LastAckTime) > NetProxyApplicationParameters::SYN_SENT_RETRANSMISSION_TIMEOUTS[pEntry->ui8RetransmissionAttempts])) {
                            // A retransmission for the SYN packet is required
                            if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, pData->getTCPFlags(), pData->getSequenceNumber(),
                                                                              pData->getData(), pData->getItemLength()))) {
                                checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_MildError,
                                                "L%hu-R%hu: failed to send packet with flag %hu to host; rc = %d\n", pData->getTCPFlags(), rc);
                                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                            }
                            else {
                                pEntry->i64LocalActionTime = i64CurrTime;
                                pData->_i64LastTransmitTime = i64CurrTime;
                                ++(pEntry->ui8RetransmissionAttempts);
                                checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                "L%hu-R%hu: RETRANSMITTED a SYN packet to local host; new RTO is %hu\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui16RTO);
                            }
                            // In both cases (success or failure), unlock the lock on the entry and continue with the next entry in the TCPConnTable
                            pEntry->unlock();
                            pData = NULL;
                            continue;
                        }

                        // Connection to local host is active: check if there are any packets with data segments to send to the local application
                        if ((pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_FIN_WAIT_1) ||
                            (pEntry->localStatus == TCTLS_CLOSE_WAIT)) {
                            while ((pData != NULL) && (pData->getItemLength() > 0)) {
                                // Check if in the receiver buffer there is enough room to receive the packet
                                if (SequentialArithmetic::lessThanOrEqual (SequentialArithmetic::delta (pData->getFollowingSequenceNumber(), pEntry->ui32LastAckSeqNum),
                                                                            (uint32) pEntry->ui16ReceiverWindowSize)) {
                                    // Check if enough time has elapsed to perform a (re)transmission
                                    if ((i64CurrTime - pData->_i64LastTransmitTime) > pEntry->ui16RTO) {
                                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, pData->getTCPFlags(), pData->getSequenceNumber(),
                                                                                          pData->getData(), pData->getItemLength()))) {
                                            checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_Warning,
                                                            "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                            pEntry->reset();
                                            pData = NULL;
                                            break;
                                        }
                                        if (pData->_i64LastTransmitTime == 0) {
                                            // First transmission --> incrementing outgoing SEQ number
                                            pEntry->ui32OutSeqNum = pData->getFollowingSequenceNumber();
                                            checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_MediumDetailDebug,
                                                            "L%hu-R%hu: transmitted to local host %hu bytes of data with FLAGs %hhu starting at SEQ number %u "
                                                            "(relative %u); next packet will have SEQ number %u\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                            pData->getItemLength(), pData->getTCPFlags(), pData->getSequenceNumber(),
                                                            SequentialArithmetic::delta (pData->getSequenceNumber(), pEntry->ui32StartingOutSeqNum), pEntry->ui32OutSeqNum);
                                        }
                                        else {
                                            if (SequentialArithmetic::lessThan (pEntry->ui32OutSeqNum, pData->getFollowingSequenceNumber())) {
                                                // Check if outgoing SEQ number needs to be updated (this happens when the first transmission of the packet was an octet)
                                                pEntry->ui32OutSeqNum = pData->getFollowingSequenceNumber();
                                            }
                                            while (pEntry->ui16SRTT <= pEntry->ui16RTO) {
                                                // Doubling RTO and SRTT because of the retransmission
                                                if (pEntry->ui16SRTT == TCPConnTable::UB_RTO) {
                                                    break;
                                                }
                                                if (pEntry->ui16SRTT == 0) {
                                                    pEntry->ui16SRTT = TCPConnTable::LB_RTO;
                                                    continue;
                                                }
                                                else if (pEntry->ui16SRTT > (TCPConnTable::UB_RTO / 2)) {
                                                    pEntry->ui16SRTT = TCPConnTable::UB_RTO / 2;
                                                }
                                                pEntry->ui16SRTT *= 2;
                                            }
                                            pEntry->ui16RTO = pEntry->ui16SRTT;
                                            checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_MediumDetailDebug,
                                                            "L%hu-R%hu: RETRANSMITTED %hu bytes of data with FLAGs %hhu starting at sequence number %u (relative %u); "
                                                            "new RTO is %hu and next packet will have SEQ number %u\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                            pData->getItemLength(), pData->getTCPFlags(), pData->getSequenceNumber(),
                                                            SequentialArithmetic::delta (pData->getSequenceNumber(), pEntry->ui32StartingOutSeqNum),
                                                            pEntry->ui16RTO, pEntry->ui32OutSeqNum);
                                        }

                                        pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                                        pEntry->i64LocalActionTime = i64CurrTime;
                                        pData->_i64LastTransmitTime = i64CurrTime;
                                    }
                                    // Subsequent packets might satisfy requests in term of timeouts to perform a (re)transmission
                                    pData = pEntry->outBuf.getNext();
                                }
                                else {
                                    // Packet should be (re)transmitted, but there is not enough space on the receiver buffer
                                    checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: impossible to transmit data to local host; last ACK received is %u, next packet to transmit "
                                                    "has SEQ number %u (relative %u) and is %hu bytes long; receiver window size is %hu and there are "
                                                    "%d packets in the outgoing queue; waiting before transmitting\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32LastAckSeqNum, pData->getSequenceNumber(),
                                                    SequentialArithmetic::delta (pData->getSequenceNumber(), pEntry->ui32StartingOutSeqNum),
                                                    pData->getItemLength(), pEntry->ui16ReceiverWindowSize, pEntry->outBuf.size());

                                    // Check if we can send an octet to force update
                                    if (((i64CurrTime - pData->_i64LastTransmitTime) > pEntry->ui16RTO) && ((i64CurrTime - pEntry->i64LastAckTime) > pEntry->ui16RTO)) {
                                        while (pEntry->ui16SRTT <= pEntry->ui16RTO) {
                                            // Doubling RTO and SRTT to avoid forcing updates too often
                                            if (pEntry->ui16SRTT == TCPConnTable::UB_RTO) {
                                                break;
                                            }
                                            if (pEntry->ui16SRTT == 0) {
                                                pEntry->ui16SRTT = TCPConnTable::LB_RTO;
                                                continue;
                                            }
                                            else if (pEntry->ui16SRTT > (TCPConnTable::UB_RTO / 2)) {
                                                pEntry->ui16SRTT = TCPConnTable::UB_RTO / 2;
                                            }
                                            pEntry->ui16SRTT *= 2;
                                        }
                                        pEntry->ui16RTO = pEntry->ui16SRTT;

                                        const int iPeekedOctet = pData->peekOctet (pEntry->ui32OutSeqNum);
                                        if ((iPeekedOctet < 0) || (iPeekedOctet > 255)) {
                                            checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_Warning,
                                                            "L%hu-R%hu: failed to extract an octed with SEQ number %u from a packet with SEQ number %u and %hu bytes long; rc = %d\n",
                                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum,
                                                            pData->getSequenceNumber(), pData->getItemLength(), iPeekedOctet);
                                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                            pEntry->reset();
                                            pData = NULL;
                                        }
                                        else {
                                            // Octet correctly peeked
                                            const uint8 ui8PeekedOctet = iPeekedOctet;
                                            if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_PSH | TCPHeader::TCPF_ACK,
                                                                                              pEntry->ui32OutSeqNum, &ui8PeekedOctet, 1))) {
                                                checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_MildError,
                                                                "L%hu-R%hu: failed to send PSH+ACK packet to host to force sending a window update; rc = %d\n",
                                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                                pEntry->reset();
                                                pData = NULL;
                                            }
                                            else {
                                                pData->_i64LastTransmitTime = i64CurrTime;
                                                pEntry->i64LocalActionTime = i64CurrTime;
                                                pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                                                if (SequentialArithmetic::lessThan (SequentialArithmetic::delta (pEntry->ui32OutSeqNum, pEntry->ui32LastAckSeqNum),
                                                                                    (uint32) pEntry->ui16ReceiverWindowSize)) {
                                                    pEntry->ui32OutSeqNum++;
                                                    checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                                    "L%hu-R%hu: successfully sent one octet with SEQ number %lu to force local host to send a window update; "
                                                                    "next packet will be sent with SEQ number %lu\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                                    pEntry->ui32OutSeqNum - 1, pEntry->ui32OutSeqNum);
                                                }
                                                else {
                                                    checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                                    "L%hu-R%hu: successfully sent one octet with SEQ number %u to force local host to send a window update; "
                                                                    "next packet will still be sent with the same SEQ number, as receiver window is full\n",
                                                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum, pEntry->ui32OutSeqNum);
                                                }
                                            }
                                        }
                                    }

                                    // Making sure that we transmit an octet, if it was possible to do, only once
                                    pData = NULL;
                                    break;
                                }
                            }
                        }

                        // Check if there is an enqueued FIN packet
                        if (pData && ((pData->getTCPFlags() & TCPHeader::TCPF_FIN) != 0) && ((pEntry->localStatus == TCTLS_ESTABLISHED) ||
                            (pEntry->localStatus == TCTLS_CLOSE_WAIT) || ((i64CurrTime - pData->_i64LastTransmitTime) >= pEntry->ui16RTO))) {
                            /*
                             * Placeholder for a FIN packet. Note that, technically, before transmitting the FIN, we should
                             * check that the remote window size has at least one free byte to enqueue the FIN.
                             */
                            if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, pData->getTCPFlags(), pData->getSequenceNumber(),
                                                                              pData->getData(), pData->getItemLength()))) {
                                checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_MildError,
                                                "L%hu-R%hu: failed to send packet with flag %hu to host; rc = %d\n", pData->getTCPFlags(), rc);
                                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                pEntry->unlock();
                                pData = NULL;
                                continue;
                            }
                            else {
                                if (pEntry->localStatus == TCTLS_ESTABLISHED) {
                                    pEntry->localStatus = TCTLS_FIN_WAIT_1;
                                    pEntry->ui32OutSeqNum++;
                                    checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: sent FIN, moving to FIN_WAIT_1; SEQ number is: %u (relative %u)\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, pData->getSequenceNumber(),
                                                    SequentialArithmetic::delta (pData->getSequenceNumber(), pEntry->ui32StartingOutSeqNum));
                                }
                                else if (pEntry->localStatus == TCTLS_CLOSE_WAIT) {
                                    pEntry->localStatus = TCTLS_LAST_ACK;
                                    pEntry->ui32OutSeqNum++;
                                    checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: sent FIN while in CLOSE_WAIT, moving to LAST_ACK; SEQ number is: %u (relative %u)\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, pData->getSequenceNumber(),
                                                    SequentialArithmetic::delta (pData->getSequenceNumber(), pEntry->ui32StartingOutSeqNum));
                                }
                                else if ((pEntry->localStatus == TCTLS_FIN_WAIT_1) || (pEntry->localStatus == TCTLS_LAST_ACK)) {
                                    while (pEntry->ui16SRTT <= pEntry->ui16RTO) {
                                        // Doubling RTO and SRTT because of the retransmission
                                        if (pEntry->ui16SRTT == TCPConnTable::UB_RTO) {
                                            break;
                                        }
                                        if (pEntry->ui16SRTT == 0) {
                                            pEntry->ui16SRTT = TCPConnTable::LB_RTO;
                                            continue;
                                        }
                                        else if (pEntry->ui16SRTT > (TCPConnTable::UB_RTO / 2)) {
                                            pEntry->ui16SRTT = TCPConnTable::UB_RTO / 2;
                                        }
                                        pEntry->ui16SRTT *= 2;
                                    }
                                    pEntry->ui16RTO = pEntry->ui16SRTT;
                                    checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: retransmitted FIN with SEQ number %u (relative %u) and ACK number %u "
                                                    "due to an expired timeout while in status %d; new RTO is %hu\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, pData->getSequenceNumber(),
                                                    SequentialArithmetic::delta (pData->getSequenceNumber(), pEntry->ui32StartingOutSeqNum),
                                                    pEntry->ui32NextExpectedInSeqNum, pEntry->localStatus, pEntry->ui16RTO);
                                }
                                pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                                pEntry->i64LocalActionTime = i64CurrTime;
                                pData->_i64LastTransmitTime = i64CurrTime;
                            }
                        }
                    }
                    else if (((i64CurrTime - pEntry->i64LocalActionTime) > NetProxyApplicationParameters::IDLE_TCP_FAST_RETRANSMIT_TRIGGER_TIMEOUT) &&
                        pEntry->areThereHolesInOutgoingDataBuffer() && !pEntry->isOutgoingDataReady()) {
                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                            "L%hu-R%hu: failed to send ACK packet to local host to trigger fast retransmit; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            pEntry->unlock();
                            continue;
                        }
                        pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                        pEntry->i64LocalActionTime = i64CurrTime;
                        checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: sent ACK to local host to trigger fast retransmit; first missing packet has SEQ number %u (relative %u)\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                                        SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum));
                    }
                    else if (((i64CurrTime - pEntry->i64LocalActionTime) > NetProxyApplicationParameters::IDLE_TCP_CONNECTION_NOTIFICATION_TIME) &&
                        ((i64CurrTime - pEntry->i64RemoteActionTime) > NetProxyApplicationParameters::IDLE_TCP_CONNECTION_NOTIFICATION_TIME) &&
                        ((i64CurrTime - pEntry->i64IdleTime) > NetProxyApplicationParameters::IDLE_TCP_CONNECTION_NOTIFICATION_TIME)) {
                        // Check performed only if there is no data to transmit
                        if (!(pEntry->getConnector()->isConnectedToRemoteAddr (&pEntry->remoteProxyAddr) ||
                            pEntry->getConnector()->isConnectingToRemoteAddr (&pEntry->remoteProxyAddr))) {
                            if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_MildError,
                                                "L%hu-R%hu: failed to send RST packet to host; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_Warning,
                                            "L%hu-R%hu: connection to remote proxy has been lost; sent a reset request to local applications\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                            pEntry->reset();
                            pEntry->unlock();
                            // pData = NULL; /*!!*/ // This was not commented-out in the IHMC branch
                            continue;
                        }
                        else {
                            pEntry->i64IdleTime = i64CurrTime;
                            checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_LowDetailDebug,
                                            "L%hu-R%hu: no activity detected (local status = %d; remote status = %d) for the last %llu milliseconds;\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus, pEntry->remoteStatus,
                                            i64CurrTime - pEntry->i64LocalActionTime);
                        }
                        /*
                        if (pEntry->getOutgoingBufferRemainingSpacePercentage() >= 30.0) {
                            // If window is not full, we send an heartbeat
                            if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                                "L%hu-R%hu: failed to send ACK packet to signal present TCP Window Size to host; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                pEntry->unlock();
                                continue;
                            }
                        }
                        */
                    }

                    // Check if it's necessary to send an ACK to local host to update LastACKedSeqNum
                    if (SequentialArithmetic::lessThan (pEntry->ui32LastACKedSeqNum, pEntry->ui32NextExpectedInSeqNum) && ((pEntry->localStatus == TCTLS_ESTABLISHED) ||
                        (pEntry->localStatus == TCTLS_FIN_WAIT_1) || (pEntry->localStatus == TCTLS_FIN_WAIT_2))) {
                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_MildError,
                                            "L%hu-R%hu: failed to send ACK packet with number %u (relative %u) and SEQ number %u (relative %u) to local host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                                            SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                            pEntry->ui32OutSeqNum, SequentialArithmetic::delta (pEntry->ui32OutSeqNum, pEntry->ui32StartingOutSeqNum), rc);
                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            pEntry->unlock();
                            pData = NULL;
                            continue;
                        }
                        else {
                            // ACK sent --> last ACKed SEQ number updated
                            pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                            pEntry->i64LocalActionTime = i64CurrTime;
                            checkAndLogMsg ("PacketRouter::localTransmitterThreadRun", Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: sent an ACK to local host to update ACK status\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                    }

                    pEntry->unlock();
                }
            }
            _pTCPConnTable->unlock();
            if (PacketRouter::isTerminationRequested()) {
                break;
            }
            _cvLocalTCPTransmitter.wait (LTT_TIME_BETWEEN_ITERATIONS);
        }

        _mLocalTCPTransmitter.unlock();
        _bLocalTCPTransmitterThreadRunning = false;
    }

    void PacketRouter::RemoteTCPTransmitterThread::run (void)
    {
        int rc;
        uint64 i64CurrTime = 0;
        double beginningTCPWindowPercentage;
        AutoConnectionEntry *pAutoConnectionEntry = NULL;
        register Entry *pEntry = NULL;
        Connector *pConnector = NULL;
        Connection *pConnection = NULL;
        bool bReachable = true;

        _bRemoteTCPTransmitterThreadRunning = true;
        _mRemoteTCPTransmitter.lock();
        while (!PacketRouter::isTerminationRequested()) {
            // Check the incoming buffers in the TCPConnTable to see if there is data to be transmitted
            _pTCPConnTable->lock();
            _pTCPConnTable->resetGet();
            while (NULL != (pEntry = _pTCPConnTable->getNextActiveRemoteEntry())) {
                if (pEntry->tryLock() == Mutex::RC_Ok) {
                    if (!(pConnector = pEntry->getConnector())) {
                        checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                        "L%hu-R%hu: getConnector() returned a NULL pointer; sending an RST packet to local host "
                                        "and clearing connection\n", pEntry->ui16ID, pEntry->ui16RemoteID);
                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                            "L%hu-R%hu: failed to send RST packet to host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        pEntry->reset();
                        pEntry->unlock();
                        continue;
                    }
                    if (!(pConnection = pEntry->getConnection())) {
                        checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                        "L%hu-R%hu: getConnection() returned a NULL pointer; sending an RST packet to local host "
                                        "and clearing connection\n", pEntry->ui16ID, pEntry->ui16RemoteID);
                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                            "L%hu-R%hu: failed to send RST packet to host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        pEntry->reset();
                        pEntry->unlock();
                        continue;
                    }

                    if ((pEntry->remoteStatus == TCTRS_WaitingConnEstablishment) && pConnector->isEnqueueingAllowed() && pConnection->isConnected()) {
                        // It is still necessary to send an OpenTCPConnection Request to the remote proxy
                        bReachable = _pConnectionManager->getReachabilityFromRemoteProxyWithID (pConnection->getRemoteProxyID());
                        if (0 != (rc = pConnection->sendOpenTCPConnectionRequest (pEntry, bReachable))) {
                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                            "L%hu-R0: sendOpenTCPConnectionRequest() failed with rc = %d\n",
                                            pEntry->ui16ID, rc);
                            pEntry->ui32StartingOutSeqNum = 0;
                            if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST | TCPHeader::TCPF_ACK, 0))) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                                "L%hu-R0: failed to send RST+ACK packet; rc = %d\n",
                                                pEntry->ui16ID, rc);
                            }
                            else {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                "L%hu-R0: impossible to send an OpenTCPConnection request to remote host;"
                                                " sent an RST back to the local application\n",
                                                pEntry->ui16ID);
                            }
                            pEntry->reset();
                        }
                        else {
                            pEntry->remoteStatus = TCTRS_ConnRequested;
                            pEntry->i64RemoteActionTime = getTimeInMilliseconds();
                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_HighDetailDebug,
                                            "L%hu-R0: successfully sent an OpenTCPConnection request via %s"
                                            " to the remote proxy at address <%s:%hu>\n",
                                            pEntry->ui16ID, pEntry->getConnector()->getConnectorTypeAsString(),
                                            pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort());
                        }
                        pEntry->unlock();
                        continue;
                    }
                    else if ((pEntry->remoteStatus == TCTRS_ConnRequested) && (pEntry->localStatus == TCTLS_ESTABLISHED) && pConnector->isEnqueueingAllowed()) {
                        // It is still necessary to send an OpenTCPConnection Request to the remote proxy
                        bReachable = _pConnectionManager->getReachabilityFromRemoteProxyWithID (pConnection->getRemoteProxyID());
                        if (pConnection->isConnected()) {
                            if (0 != (rc = pConnection->sendTCPConnectionOpenedResponse (pEntry, bReachable))) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                                "L%hu-R%hu: sendOpenTCPConnectionRequest() failed with rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                                    "L%hu-R%hu: failed to send RST+ACK packet; rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }
                                else {
                                    checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                                    "L%hu-R%hu: impossible to send an OpenTCPConnection request to remote NetProxy at address %s:%hu; "
                                                    "sent an RST back to the local host and resetting entry\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                    pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort());
                                }
                                pEntry->reset();
                                pEntry->unlock();
                                continue;
                            }
                            else {
                                pEntry->remoteStatus = TCTRS_ConnEstablished;
                                pEntry->i64RemoteActionTime = getTimeInMilliseconds();
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_HighDetailDebug,
                                                "L%hu-R%hu: successfully sent a TCPConnectionOpened response via %s to the remote "
                                                "NetProxy at address %s:%hu\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                pConnector->getConnectorTypeAsString(), pEntry->remoteProxyAddr.getIPAsString(),
                                                pEntry->remoteProxyAddr.getPort());
                                // Continue after having moved to TCTRS_ConnEstablished
                            }
                        }
                        else if (pConnection->isConnecting()) {
                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MediumDetailDebug,
                                            "L%hu-R%hu: connection with the remote NetProxy at address %s:%hu is still in status %d; skipping entry\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString(),
                                            pEntry->remoteProxyAddr.getPort(), pConnection->getStatus());
                            pEntry->unlock();
                            continue;
                        }
                        else {
                            if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                                "L%hu-R%hu: failed to send RST+ACK packet; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                            "L%hu-R%hu: could not establish a connection with the remote NetProxy at address %s:%hu; sent back "
                                            "an RST to the local host and resetting entry\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                            pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort());
                            pEntry->reset();
                            pEntry->unlock();
                            continue;
                        }
                    }

                    beginningTCPWindowPercentage = pEntry->getOutgoingBufferRemainingSpacePercentage();
                    // While there is data ready to be transmitted to remote proxy --> transmit
                    while ((pConnector->isEnqueueingAllowed()) && (pEntry->isOutgoingDataReady()) && (pConnection->getConnectorAdapter()->getOutgoingBufferSize() > 2048)) {
                        checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_HighDetailDebug,
                                        "L%hu-R%hu: there are %u bytes of data in the buffer, of which %u are ready to be transmitted\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getOutgoingTotalBytesCount(),
                                        pEntry->getOutgoingReadyBytesCount());
                        // Transmission to remote proxy is allowed only if the remote connection is either in status ConnEstablished or in status DisconnRequestReceived
                        if ((pEntry->remoteStatus == TCTRS_ConnEstablished) || (pEntry->remoteStatus == TCTRS_DisconnRequestReceived)) {
                            const TCPSegment * const pTCPSegment = pEntry->dequeueLocallyReceivedData (NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
                            if (pTCPSegment == NULL) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                                "L%hu-R%hu: dequeue() failed; sending RST packet to local host and clearing connection\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID);
                                if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                                    "L%hu-R%hu: sendTCPPacketToHost() failed sending an RST to local host; with rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }
                                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                delete pTCPSegment;
                                break;
                            }
                            if (pTCPSegment->getTCPFlags() & TCPHeader::TCPF_FIN) {
                                // If we have not ACKed the FIN yet, we should send an ACK to the local application
                                if ((pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_FIN_WAIT_1) ||
                                    (pEntry->localStatus == TCTLS_FIN_WAIT_2)) {
                                    // FIN packet has been received --> it has to be ACKed --> ui32NextExpectedInSeqNum has to be incremented by 1 and localstatus needs to be changed
                                    pEntry->ui32NextExpectedInSeqNum++;
                                    pEntry->i64LocalActionTime = pEntry->i64LastAckTime;
                                    if ((pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_SYN_RCVD)) {
                                        // Moving to CLOSE_WAIT
                                        checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                        "L%hu-R%hu: received all missing packets, local status moved from %d to CLOSE_WAIT;\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus);
                                        pEntry->localStatus = TCTLS_CLOSE_WAIT;
                                    }
                                    else {
                                        // Local status is TCTLS_FIN_WAIT_1 or TCTLS_FIN_WAIT_2 and a previously received FIN is being processed --> sending back an ACK
                                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                                            checkAndLogMsg ("TCPManager::remoteTransmitterThreadRun", Logger::L_MildError,
                                                            "L%hu-R%hu: failed to send FIN+ACK packet; rc = %d\n",
                                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                            pEntry->reset();
                                            pEntry->unlock();
                                            break;
                                        }
                                        if (pEntry->localStatus == TCTLS_FIN_WAIT_1) {
                                            // Our FIN still has to be ACKed and a FIN from local application has been received --> moving to CLOSING
                                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                            "L%hu-R%hu: received all missing packets while local status was FIN_WAIT_1 - "
                                                            "sent ACK to local application and moved to CLOSING (Simultaneous Close Sequence)\n",
                                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                                            pEntry->localStatus = TCTLS_CLOSING;
                                        }
                                        else if (pEntry->localStatus == TCTLS_FIN_WAIT_2) {
                                            // Moving to TIME_WAIT
                                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Info,
                                                            "L%hu-R%hu: received all missing packets while local status was FIN_WAIT_2 - "
                                                            "sent ACK to local application and moved to TIME_WAIT\n",
                                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                                            pEntry->localStatus = TCTLS_TIME_WAIT;
                                        }
                                    }
                                }

                                if (pTCPSegment->getItemLength() > 0) {
                                    if (0 != (rc = pConnection->sendTCPDataToRemoteHost (pEntry, pTCPSegment->getData(), pTCPSegment->getItemLength(),
                                                                                         pTCPSegment->getTCPFlags() & TCP_DATA_FLAGS_MASK))) {
                                        checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                                        "L%hu-R%hu: sendTCPDataToRemoteHost() failed with rc = %d; sending RST packet to local host and clearing connection\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                                            "L%hu-R%hu: sendTCPPacketToHost() failed sending an RST to local host; with rc = %d\n",
                                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                        }
                                        pEntry->reset();
                                        delete pTCPSegment;
                                        break;
                                    }
                                    else {
                                        pEntry->i64RemoteActionTime = i64CurrTime;
                                        checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MediumDetailDebug,
                                                        "L%hu-R%hu: transmitted %hu bytes of TCP data with FLAGs %hhu via %s to remote proxy\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID, pTCPSegment->getItemLength(),
                                                        pTCPSegment->getTCPFlags(), pEntry->getConnector()->getConnectorTypeAsString());
                                    }
                                }

                                // Flush any data left in compressor buffer
                                if (0 != (rc = flushAndSendCloseConnectionRequest (pEntry))) {
                                    checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                                    "L%hu-R%hu: failed to flush data and to send a CloseTCPConnection request to remote proxy; rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                    if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                        checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                                        "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                    }
                                    pEntry->reset();
                                }
                                else {
                                    if (pEntry->remoteStatus == TCTRS_ConnEstablished) {
                                        pEntry->remoteStatus = TCTRS_DisconnRequestSent;
                                        checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                        "L%hu-R%hu: sent a CloseTCPConnection request to remote proxy; remoteStatus set to DisconnRequestedSent\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                                    }
                                    else {
                                        pEntry->remoteStatus = TCTRS_Disconnected;
                                        checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                        "L%hu-R%hu: sent a CloseTCPConnection response to remote proxy; remoteStatus set to Disconnected\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                                        pEntry->resetConnectors();
                                    }
                                    pEntry->i64RemoteActionTime = i64CurrTime;
                                }

                                // CloseTCPConnection request has been sent to remote proxy --> nothing else to do for this entry
                                delete pTCPSegment;
                                break;
                            }
                            else if (pTCPSegment->getItemLength() == 0) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_HighDetailDebug,
                                                "L%hu-R%hu: dequeue() returned an empty data packet (buffering compression?) with FLAGs %hhu; ignoring\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, pTCPSegment->getTCPFlags());
                                delete pTCPSegment;
                                continue;
                            }

                            if (0 != (rc = pConnection->sendTCPDataToRemoteHost (pEntry, pTCPSegment->getData(), pTCPSegment->getItemLength(), pTCPSegment->getTCPFlags()))) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                                "L%hu-R%hu: sendTCPDataToRemoteHost() failed with rc = %d; sending RST packet to local host and clearing connection\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                                    "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }
                                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                delete pTCPSegment;
                                break;
                            }
                            else {
                                pEntry->i64RemoteActionTime = i64CurrTime;
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MediumDetailDebug,
                                                "L%hu-R%hu: transmitted %hu bytes of TCP data with FLAGs %hhu via %s to remote proxy\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, pTCPSegment->getItemLength(),
                                                pTCPSegment->getTCPFlags(), pEntry->getConnector()->getConnectorTypeAsString());
                                delete pTCPSegment;
                            }
                        }
                        else if (pEntry->remoteStatus == TCTRS_ConnRequested) {
                            if (pConnector->isConnectedToRemoteAddr (&pEntry->remoteProxyAddr) || pConnector->isConnectingToRemoteAddr (&pEntry->remoteProxyAddr)) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_LowDetailDebug,
                                                "L%hu-R%hu: there are %u bytes in the input buffer and %u bytes ready to transmit, but connection"
                                                " to remote proxy is not yet established: local status is %d, remote status is %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getOutgoingTotalBytesCount(),
                                                pEntry->getOutgoingReadyBytesCount(), pEntry->localStatus, pEntry->remoteStatus);
                                break;
                            }
                            else if (pConnector->hasFailedConnectionToRemoteAddr (&pEntry->remoteProxyAddr)) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Info,
                                                "L%hu-R%hu: there are %u bytes in the input buffer and %u bytes ready to transmit, but connection"
                                                " attempt to remote proxy failed; resetting connection\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getOutgoingTotalBytesCount(),
                                                pEntry->getOutgoingReadyBytesCount());
                                if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                                    "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }
                                pEntry->reset();
                                break;
                            }
                            else {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_HighDetailDebug,
                                                "L%hu-R%hu: there are %u bytes in the input buffer and %u bytes ready to transmit, "
                                                "but connection attempt still has to be performed\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                pEntry->getOutgoingTotalBytesCount(), pEntry->getOutgoingReadyBytesCount());
                            }
                        }
                        else if (pEntry->getOutgoingTotalBytesCount() > 0) {
                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                            "L%hu-R%hu: there are %u bytes in the input buffer and %u bytes ready to transmit, but the status "
                                            "of remote connection to remote proxy is %d; resetting connection\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getOutgoingTotalBytesCount(),
                                            pEntry->getOutgoingReadyBytesCount(), pEntry->remoteStatus);
                            if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                                "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            break;
                        }
                        else {
                            const TCPSegment * const pTCPSegment = pEntry->dequeueLocallyReceivedData (NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
                            if (pTCPSegment == NULL) {
                                checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                                "L%hu-R%hu: dequeue() failed; sending RST packet to local host and clearing connection\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID);
                                if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                                    "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }
                                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                delete pTCPSegment;
                            }

                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                            "L%hu-R%hu: dequeueLocallyReceivedData returned a (duplicate?) packet with FLAGs %hhu "
                                            "and %hu bytes of data; deleting packet and continuing execution\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pTCPSegment->getTCPFlags(),
                                            pTCPSegment->getItemLength());
                            delete pTCPSegment;
                        }
                    }

                    // If enough space in the TCP window has been freed, host is going to be notified
                    if ((beginningTCPWindowPercentage < 30.0) && (pEntry->getOutgoingBufferRemainingSpacePercentage() >= 30.0) &&
                        ((pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_FIN_WAIT_1) ||
                        (pEntry->localStatus == TCTLS_FIN_WAIT_2))) {
                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_Warning,
                                            "L%hu-R%hu: failed to send ACK packet to signal present TCP Window Size to host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                        }
                        else {
                            pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                            checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MediumDetailDebug,
                                            "L%hu-R%hu: sent notification of present free TCP Window Size (new: %lf% free, old: %lf% free)\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID,
                                            pEntry->getOutgoingBufferRemainingSpacePercentage(), beginningTCPWindowPercentage);
                        }
                    }
                    pEntry->unlock();
                }
            }
            _pTCPConnTable->unlock();
            if (PacketRouter::isTerminationRequested()) {
                break;
            }
            _cvRemoteTCPTransmitter.wait (RTT_TIME_BETWEEN_ITERATIONS);
        }

        _mRemoteTCPTransmitter.unlock();
        _bRemoteTCPTransmitterThreadRunning = false;
    }

    void PacketRouter::AutoConnectionManager::run (void)
    {
        int rc;
        uint32 ui32RemoteProxyID = 0;
        uint16 ui16Port = 0;
        uint64 i64CurrTime = 0, i64NextCycleTime = 0, i64TimeToWait = 0;
        const uint64 i64ShortWait = 2000;
        ConnectorType connectorType (CT_UNDEF);
        String sIPAddress;
        const Connector *pConnector = NULL;
        const NPDArray2<AutoConnectionEntry> * const pAutoConnectionTable = _pConnectionManager->getAutoConnectionTable();

        _bAutoConnectionManagerThreadRunning = true;
        _mAutoConnectionManager.lock();
        while (!PacketRouter::isTerminationRequested()) {
            // Check the AutoConnection Table
            i64CurrTime = getTimeInMilliseconds();
            i64NextCycleTime = i64CurrTime + ACM_TIME_BETWEEN_ITERATIONS;

            for (int i = 0; i <= pAutoConnectionTable->getHighestIndex() && !PacketRouter::isTerminationRequested(); ++i) {
                // Save values in local variables to avoid problems in case of memory deallocation at program exit
                AutoConnectionEntry &rAutoConnectionEntry = pAutoConnectionTable->get (i);
                connectorType = rAutoConnectionEntry.getConnectorType();
                ui32RemoteProxyID = rAutoConnectionEntry.getRemoteProxyID();
                sIPAddress = rAutoConnectionEntry.getRemoteProxyInetAddress()->getIPAsString();
                ui16Port = rAutoConnectionEntry.getRemoteProxyInetAddress()->getPort();
                if (pConnector = _pConnectionManager->getConnectorForType (connectorType)) {
                    if (_pConnectionManager->isConnectionToRemoteProxyOpenedForConnector (connectorType, ui32RemoteProxyID) && rAutoConnectionEntry.isSynchronized()) {
                        // Connection is already established
                        continue;
                    }
                    else if (pConnector->isConnectingToRemoteAddr (_pConnectionManager->getInetAddressForProtocolAndProxyWithID (connectorType, ui32RemoteProxyID))) {
                        // Connection is already being established
                        checkAndLogMsg ("PacketRouter::AutoConnectionManager", Logger::L_HighDetailDebug,
                                        "%sConnection to the remote NetProxy at address %s:%hu is not yet established\n",
                                        connectorTypeToString (connectorType), sIPAddress.c_str(), ui16Port);
                        i64NextCycleTime = std::min (i64NextCycleTime, i64CurrTime + i64ShortWait);
                        continue;
                    }
                    else if (!pConnector->isEnqueueingAllowed() ||
                        ((i64CurrTime - rAutoConnectionEntry.getLastConnectionAttemptTime()) < rAutoConnectionEntry.getAutoReconnectTimeInMillis())) {
                        // Timeout not yet expired
                        i64NextCycleTime = std::min (i64NextCycleTime, rAutoConnectionEntry.getLastConnectionAttemptTime() + rAutoConnectionEntry.getAutoReconnectTimeInMillis());
                        continue;
                    }

                    // Proceed with attempting connection with the remote NetProxy
                    if (0 != (rc = rAutoConnectionEntry.synchronize())) {
                        checkAndLogMsg ("PacketRouter::AutoConnectionManager", Logger::L_Warning,
                                        "synchronize() attempt performed via %s to the remote NetProxy with UniqueID %u at address %s:%hu failed with rc = %d\n",
                                        connectorTypeToString (connectorType), ui32RemoteProxyID, sIPAddress.c_str(), ui16Port, rc);
                    }
                    else if (rAutoConnectionEntry.isSynchronized()) {
                        checkAndLogMsg ("PacketRouter::AutoConnectionManager", Logger::L_MediumDetailDebug,
                                        "synchronize() attempt successfully performed via %s to the remote NetProxy with UniqueID %u at address %s:%hu\n",
                                        connectorTypeToString (connectorType), ui32RemoteProxyID, sIPAddress.c_str(), ui16Port);
                    }
                    else {
                        checkAndLogMsg ("PacketRouter::AutoConnectionManager", Logger::L_HighDetailDebug,
                                        "waiting for the connection to the remote NetProxy with UniqueID %u and address %s:%hu to be established via %s\n",
                                        ui32RemoteProxyID, sIPAddress.c_str(), ui16Port, connectorTypeToString (connectorType));
                        i64NextCycleTime = std::min (i64NextCycleTime, i64CurrTime + i64ShortWait);
                        continue;       // Avoids updating the last connection attempt time
                    }
                    rAutoConnectionEntry.updateLastConnectionAttemptTime (i64CurrTime);
                }
                else {
                    checkAndLogMsg ("PacketRouter::AutoConnectionManager", Logger::L_SevereError,
                                    "impossible to retrieve %s Connector from the ConnectionManager; terminating thread!\n",
                                    connectorTypeToString (connectorType));

                    _mAutoConnectionManager.unlock();
                    _bAutoConnectionManagerThreadRunning = false;
                    return;
                }
            }

            if (PacketRouter::isTerminationRequested()) {
                break;
            }
            i64TimeToWait = i64NextCycleTime - i64CurrTime;
            if (i64TimeToWait > 0) {
                _cvAutoConnectionManager.wait (i64TimeToWait);
            }
        }

        _mAutoConnectionManager.unlock();
        _bAutoConnectionManagerThreadRunning = false;
    }

    void PacketRouter::CleanerThread::run (void)
    {
        int rc;
        bool bWorkDone = false;
        uint32 ui32EntriesNum = 0, ui32BufferSize = 0;
        int64 i64CurrTime = 0, i64RefTime = getTimeInMilliseconds();
        Entry *pEntry = NULL;

        _bCleanerThreadRunning = true;
        _mCleaner.lock();
        while (!PacketRouter::isTerminationRequested()) {
            bWorkDone = false;
            _pTCPConnTable->lock();
            _pTCPConnTable->resetGet();
            i64CurrTime = getTimeInMilliseconds();
            while (NULL != (pEntry = _pTCPConnTable->getNextEntry())) {
                if (pEntry->tryLock() == Mutex::RC_Ok) {
                    // Check the entry to see if it needs to be cleaned up
                    if ((pEntry->localStatus == TCTLS_TIME_WAIT) && ((i64CurrTime - pEntry->i64LocalActionTime) >= TCPConnTable::MSL)) {
                        checkAndLogMsg ("PacketRouter::cleanerThread", Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: cleaning communication entry in status TIME_WAIT\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        pEntry->clear();
                        bWorkDone = true;
                    }
                    else if ((pEntry->localStatus == TCTLS_CLOSED) && ((i64CurrTime - pEntry->i64LocalActionTime) >= CT_TIME_BETWEEN_ITERATIONS)) {
                        checkAndLogMsg ("PacketRouter::cleanerThread", Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: cleaning communication entry in status CLOSED\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        pEntry->clear();
                        bWorkDone = true;
                    }
                    else if ((pEntry->localStatus == TCTLS_SYN_SENT) && ((i64CurrTime - pEntry->i64LastAckTime) >= NetProxyApplicationParameters::SYN_SENT_FAILED_TIMEOUT)) {
                        checkAndLogMsg ("PacketRouter::cleanerThread", Logger::L_Info,
                                        "L%hu-R%hu: SYN_SENT timeout expired; sending an RST to the local application and a RemoteReset request to remote proxy\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MildError,
                                            "L%hu-R%hu: failed to send RST packet; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        bWorkDone = true;
                    }
                    else if ((pEntry->localStatus == TCTLS_SYN_RCVD) && ((pEntry->remoteStatus == TCTRS_WaitingConnEstablishment) || (pEntry->remoteStatus == TCTRS_Unknown))&&
                        ((i64CurrTime - pEntry->i64RemoteActionTime) > NetProxyApplicationParameters::DEFAULT_REMOTE_CONN_ESTABLISHMENT_TIMEOUT)) {
                        checkAndLogMsg ("PacketRouter::cleanerThread", Logger::L_Info,
                                        "L%hu-R%hu: remote connection establishment timeout expired; sending an RST to the local "
                                        "application and a RemoteTCPResetRequest ProxyMessage to remote proxy at address <%s:%hu>\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString(),
                                        pEntry->remoteProxyAddr.getPort());
                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MildError,
                                            "L%hu-R%hu: failed to send RST packet; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        bWorkDone = true;
                    }
                    else if ((pEntry->localStatus == TCTLS_SYN_RCVD) && (pEntry->remoteStatus == TCTRS_ConnRequested) &&
                        ((i64CurrTime - pEntry->i64RemoteActionTime) > (2 * TCPConnTable::MSL))) {
                        checkAndLogMsg ("PacketRouter::cleanerThread", Logger::L_Info,
                                        "L%hu-R%hu: remote connection status is ConnRequested, but the no answer was received before the timeout expired; "
                                        "sending an RST local application and a RemoteTCPResetRequest ProxyMessage to remote proxy at address <%s:%hu>\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort());
                        if (0 != (rc = TCPManager::sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MildError,
                                            "L%hu-R%hu: failed to send RST packet; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        bWorkDone = true;
                    }
                    pEntry->unlock();
                }
            }

            if (bWorkDone) {
                i64RefTime = getTimeInMilliseconds();
            }
            else {
                ui32EntriesNum = _pTCPConnTable->getEntriesNum();
                _pTCPConnTable->cleanMemory();
                if (ui32EntriesNum != _pTCPConnTable->getEntriesNum()) {
                        checkAndLogMsg ("PacketRouter::cleanerThread", Logger::L_MediumDetailDebug,
                                        "TCP connections table shrinked to %u entries\n",
                                        _pTCPConnTable->getEntriesNum());
                }
            }

            _pTCPConnTable->unlock();
            if (PacketRouter::isTerminationRequested()) {
                break;
            }
            _cvCleaner.wait (CT_TIME_BETWEEN_ITERATIONS);
        }

        _mCleaner.unlock();
        _bCleanerThreadRunning = false;
    }

    int PacketRouter::UpdateGUIThread::init (const char * const pszNotificationAddressList)
    {
        StringTokenizer stAddressList (pszNotificationAddressList, ',');

        // Parse notification address list
        while (const char * const pszPairToken = stAddressList.getNextToken()) {
            StringTokenizer stPair (pszPairToken, ':');
            String sIP = stPair.getNextToken();
            String sPort = stPair.getNextToken();

            _notificationAddressList.add (std::make_pair (sIP, static_cast<uint16> (atoi (sPort))));
        }

        return 0;
    }

    void PacketRouter::UpdateGUIThread::run (void)
    {
        int rc;
        const char *pcUpdateGUIMessage = NULL, *pszIPAddress = NULL;
        uint16 ui16UpdateGUIMessageLen = 0, ui16Port = 0;

        _bUpdateGUIThreadRunning = true;
        _mGUIUpdater.lock();
        _cvGUIUpdater.wait (UGT_TIME_BETWEEN_ITERATIONS);
        while (!PacketRouter::isTerminationRequested()) {
            _pGUIStatsManager->packGUIUpdateMessage();
            _pGUIStatsManager->getGUIUpdateMessage (pcUpdateGUIMessage, ui16UpdateGUIMessageLen);

            // Send an UpdateGUI message to all addresses in the Notification Address List
            for (int i = 0; i <= _notificationAddressList.getHighestIndex(); ++i) {
                pszIPAddress = _notificationAddressList.get (i).first;
                ui16Port = _notificationAddressList.get (i).second;
                if (ui16UpdateGUIMessageLen != (rc = _udpSocket.sendTo (pszIPAddress, ui16Port, pcUpdateGUIMessage, ui16UpdateGUIMessageLen))) {
                    checkAndLogMsg ("PacketRouter::UpdateGUIThread::run", Logger::L_Warning,
                                    "sendTo() failed to send an UpdateGUI message of %hu bytes to address %s:%hu; rc = %d\n",
                                    ui16UpdateGUIMessageLen, pszIPAddress, ui16Port, rc);
                }
                else {
                    checkAndLogMsg ("PacketRouter::UpdateGUIThread::run", Logger::L_HighDetailDebug,
                                    "successfully sent an UpdateGUI message of %hu bytes to %s:%hu\n",
                                    ui16UpdateGUIMessageLen, pszIPAddress, ui16Port);
                }
            }

            if (PacketRouter::isTerminationRequested()) {
                break;
            }
            _cvGUIUpdater.wait (UGT_TIME_BETWEEN_ITERATIONS);
        }

        _mGUIUpdater.unlock();
        _bUpdateGUIThreadRunning = false;
    }

    int PacketRouter::handlePacketFromInternalInterface (const uint8 * const pPacket, uint16 ui16PacketLen)
    {
        static int rc = 0;
        EtherFrameHeader * const pEthHeader = (EtherFrameHeader*) pPacket;
        pEthHeader->ntoh();

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            if (pEthHeader->src == NetProxyApplicationParameters::NETPROXY_INTERNAL_INTERFACE_MAC_ADDR) {
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                "ignoring packet generated by this host on the internal interface\n");
                return 0;
            }
            if (hostBelongsToTheExternalNetwork (pEthHeader->src)) {
                // Packet originated on the external network was just reinjected on the internal one --> ignore it
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                "ignoring ethernet packet originated on the external network (source MAC address %2x:%2x:%2x:%2x:%2x:%2x) and previously "
                                "forwarded on the internal one by the NetProxy (destination MAC address %2x:%2x:%2x:%2x:%2x:%2x)\n", (int) pEthHeader->src.ui8Byte1,
                                (int) pEthHeader->src.ui8Byte2, (int) pEthHeader->src.ui8Byte3, (int) pEthHeader->src.ui8Byte4, (int) pEthHeader->src.ui8Byte5,
                                (int) pEthHeader->src.ui8Byte6, (int) pEthHeader->dest.ui8Byte1, (int) pEthHeader->dest.ui8Byte2, (int) pEthHeader->dest.ui8Byte3,
                                (int) pEthHeader->dest.ui8Byte4, (int) pEthHeader->dest.ui8Byte5, (int) pEthHeader->dest.ui8Byte6);
                return 0;
            }

            if (!hostBelongsToTheInternalNetwork (pEthHeader->src)) {
                // Add new host to the list of hosts in the internal network
                _daInternalHosts[_daInternalHosts.getHighestIndex() + 1] = pEthHeader->src;
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                "added source MAC address %2x:%2x:%2x:%2x:%2x:%2x to the set of hosts belonging to the internal network\n",
                                (int) pEthHeader->src.ui8Byte1, (int) pEthHeader->src.ui8Byte2, (int) pEthHeader->src.ui8Byte3,
                                (int) pEthHeader->src.ui8Byte4, (int) pEthHeader->src.ui8Byte5, (int) pEthHeader->src.ui8Byte6);
            }
        }

        if (pEthHeader->ui16EtherType == ET_ARP) {
            register ARPPacket *pARPPacket = (ARPPacket*) (pPacket + sizeof (EtherFrameHeader));
            pARPPacket->ntoh();
            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                            "ARP Packet: operation %d - source %d.%d.%d.%d (%2x:%2x:%2x:%2x:%2x:%2x) - target %d.%d.%d.%d (%2x:%2x:%2x:%2x:%2x:%2x)\n",
                            (int) pARPPacket->ui16Oper, (int) pARPPacket->spa.ui8Byte1, (int) pARPPacket->spa.ui8Byte2, (int) pARPPacket->spa.ui8Byte3,
                            (int) pARPPacket->spa.ui8Byte4, (int) pARPPacket->sha.ui8Byte1, (int) pARPPacket->sha.ui8Byte2, (int) pARPPacket->sha.ui8Byte3,
                            (int) pARPPacket->sha.ui8Byte4, (int) pARPPacket->sha.ui8Byte5, (int) pARPPacket->sha.ui8Byte6, (int) pARPPacket->tpa.ui8Byte1,
                            (int) pARPPacket->tpa.ui8Byte2, (int) pARPPacket->tpa.ui8Byte3, (int) pARPPacket->tpa.ui8Byte4,
                            (int) pARPPacket->tha.ui8Byte1, (int) pARPPacket->tha.ui8Byte2, (int) pARPPacket->tha.ui8Byte3,
                            (int) pARPPacket->tha.ui8Byte4, (int) pARPPacket->tha.ui8Byte5, (int) pARPPacket->tha.ui8Byte6);

            if (pARPPacket->spa.ui32Addr != 0) {
                // This is not an ARP probe --> process it
                if (EndianHelper::htonl (pARPPacket->spa.ui32Addr) != NetProxyApplicationParameters::NETPROXY_IP_ADDR &&
                    pARPPacket->sha != NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR) {
                    // The sender is not this machine
                    _pARPCache.insert (pARPPacket->spa.ui32Addr, pARPPacket->sha);          // Opportunistically cache the MAC address of the sender
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_LowDetailDebug,
                                    "caching ARP address %2x:%2x:%2x:%2x:%2x:%2x for IP address %d.%d.%d.%d on the internal network\n",
                                    (int) pARPPacket->sha.ui8Byte1, (int) pARPPacket->sha.ui8Byte2, (int) pARPPacket->sha.ui8Byte3, (int) pARPPacket->sha.ui8Byte4,
                                    (int) pARPPacket->sha.ui8Byte5, (int) pARPPacket->sha.ui8Byte6, (int) pARPPacket->spa.ui8Byte1, (int) pARPPacket->spa.ui8Byte2,
                                    (int) pARPPacket->spa.ui8Byte3, (int) pARPPacket->spa.ui8Byte4);
                }
            }
            else if (NetProxyApplicationParameters::GATEWAY_MODE) {
                // ARP probe (that is: spa == 0.0.0.0)
                if (EndianHelper::htonl (pARPPacket->tpa.ui32Addr) != NetProxyApplicationParameters::NETPROXY_IP_ADDR) {
                    // Forwarding ARP probe to the external network
                    pARPPacket->sha = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                    pARPPacket->hton();
                    pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                    pEthHeader->hton();
                    if (0 != (rc = sendPacketToHost (_pExternalInterface, pPacket, ui16PacketLen))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_SevereError,
                                        "sendPacketToHost() of an ARP probe packet (target protocol address: %s) "
                                        "of %hu bytes long on the external interface failed with rc = %d\n",
                                        InetAddr (pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen, rc);
                        return -1;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "successfully forwarded an ARP probe packet (target protocol address: %s) of %hu bytes long on the "
                                    "external network interface; both the source Ethernet MAC address and the source hardware address of "
                                    "the ARP packet were changed to be the MAC address of the external network interface of this host\n",
                                    InetAddr (pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen);
                }
                else {
                    // Another host is trying to use our own address! --> reply with a gratuitous ARP announcement on both interfaces
                    if (0 != (rc = sendARPAnnouncement (_pInternalInterface, pARPPacket, NetProxyApplicationParameters::NETPROXY_IP_ADDR,
                                                        NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_SevereError,
                                        "sendARPAnnouncement() on the internal network interface failed with rc = %d\n", rc);
                        return -2;
                    }
                    if (0 != (rc = sendARPAnnouncement (_pExternalInterface, pARPPacket, NetProxyApplicationParameters::NETPROXY_IP_ADDR,
                                                        NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_SevereError,
                                        "sendARPAnnouncement() on the external network interface failed with rc = %d\n", rc);
                        return -2;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_LowDetailDebug,
                                    "successfully sent a gratuitous ARP Announcement packet of %hu "
                                    "bytes long on both interfaces\n", ui16PacketLen);
                }
                return 0;
            }

            if (pARPPacket->ui16Oper == 1) {
                // ARP Request
                if ((NetProxyApplicationParameters::GATEWAY_MODE) && !_pConnectionManager->isRemoteHostIPMapped (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)) &&
                    (EndianHelper::htonl (pARPPacket->tpa.ui32Addr) != NetProxyApplicationParameters::NETPROXY_IP_ADDR)) {
                    /* The NetProxy is running in gateway mode and the requested address is not an entry in the config file, nor the
                     * address of the local host --> the received ARP packet needs to be forwarded on the external network. */
                    pARPPacket->sha = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                    pARPPacket->hton();
                    pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                    pEthHeader->dest = NetProxyApplicationParameters::BROADCAST_MAC_ADDR;                   // The destination MAC address of ARP Requests should always be broadcast
                    pEthHeader->hton();
                    if (0 != (rc = sendPacketToHost (_pExternalInterface, pPacket, ui16PacketLen))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_SevereError,
                                        "sendPacketToHost() of an ARP request packet (source protocol address: %s - destination protocol "
                                        "address: %s) of %hu bytes long on the external network interface failed with rc = %d\n",
                                        InetAddr(pARPPacket->spa.ui32Addr).getIPAsString(), InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(),
                                        ui16PacketLen, rc);
                        return -3;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "successfully forwarded an ARP request packet with target IP address %s (which could not be mapped "
                                    "to a remote proxy address) on the external network interface; both the source Ethernet MAC address "
                                    "and the source hardware address of the ARP packet were changed to be the MAC address of the "
                                    "external network interface of this host\n", InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString());
                }
                else if (NetProxyApplicationParameters::GATEWAY_MODE || _pConnectionManager->isRemoteHostIPMapped (EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                    /* If we end up here and the NetProxy is running in gateway mode, then we need to reply to the ARP request because
                    * either we are mapping the remote address or the ARP requested was the IP of this machine or of the network gateway.
                    * If we end up here and the NetProxy is running in host mode, then we need to reply to the ARP request only
                    * if we are mapping the remote address (ARP requests for this same host will be handled by the kernel). */
                    EtherMACAddr targetMACAddr;
                    if (ACMNetProxy::NetProxyApplicationParameters::GATEWAY_MODE) {
                        // Operating in Gateway mode - we return our MAC address for the remote address in the ARP Response
                        targetMACAddr = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                    }
                    else {
                        // Operating in Host mode - return a constructed MAC address for the remote address in the ARP Response
                        buildVirtualNetProxyEthernetMACAddress (targetMACAddr, pARPPacket->tpa.ui8Byte3, pARPPacket->tpa.ui8Byte4);
                    }
                    if (0 != (rc = sendARPReplyToHost (_pInternalInterface, pARPPacket, targetMACAddr))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_SevereError,
                                        "sendARPReplyToHost() on the internal network interface failed with rc = %d\n", rc);
                        return -4;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MediumDetailDebug,
                                    "successfully sent an ARP Response (Source Protocol Address: %s - Source Hardware "
                                    "Address: %2x:%2x:%2x:%2x:%2x:%2x) on the internal network interface\n",
                                    InetAddr(EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(), targetMACAddr.ui8Byte1,
                                    targetMACAddr.ui8Byte2, targetMACAddr.ui8Byte3, targetMACAddr.ui8Byte4,
                                    targetMACAddr.ui8Byte5, targetMACAddr.ui8Byte6);
                }
                else {
                    // NetProxy running in host mode and IP address is not mapped --> ignore ARP packet
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "received an ARP Request with Target Protocol Address %s that cannot "
                                    "be remapped to any remote instance of NetProxy; ignoring\n",
                                    InetAddr(EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                }
            }
            else if (NetProxyApplicationParameters::GATEWAY_MODE && (pARPPacket->ui16Oper == 2)) {
                // ARP Response when running in Gateway Mode
                if (EndianHelper::htonl (pARPPacket->tpa.ui32Addr) != NetProxyApplicationParameters::NETPROXY_IP_ADDR) {
                    // The target protocol address is not the address of this machine
                    if (pARPPacket->tha == NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR) {
                        /* The Target Hardware Address is the MAC address of this machine. This means that the NetProxy has
                         * forwarded an ARP request from the external to the internal interface in the past and changed its
                         * source hardware address to be the one of this machine --> change it back to the original MAC address */
                        const EtherMACAddr * const pDestEther = _pARPCache.lookup (pARPPacket->tpa.ui32Addr);
                        if (!pDestEther) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_Warning,
                                            "received an ARP Response with source protocol address %s, target protocol address not of this machine "
                                            "(%s), but with the target hardware address of this machine; however, it was not possible to find "
                                            "the real hardware address in the local ARP cache\n",
                                            InetAddr (EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                            InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                            return -5;
                        }
                        pARPPacket->tha = *pDestEther;
                        pEthHeader->dest = *pDestEther;
                    }
                    else {
                        // The Target Hardware Address is not the MAC address of this machine --> cache it
                        _pARPCache.insert (pARPPacket->tpa.ui32Addr, pARPPacket->tha);
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_LowDetailDebug,
                                        "caching ARP address %2x:%2x:%2x:%2x:%2x:%2x for IP address %d.%d.%d.%d\n",
                                        (int) pARPPacket->tha.ui8Byte1, (int) pARPPacket->tha.ui8Byte2, (int) pARPPacket->tha.ui8Byte3,
                                        (int) pARPPacket->tha.ui8Byte4, (int) pARPPacket->tha.ui8Byte5, (int) pARPPacket->tha.ui8Byte6,
                                        (int) pARPPacket->tpa.ui8Byte1, (int) pARPPacket->tpa.ui8Byte2,
                                        (int) pARPPacket->tpa.ui8Byte3, (int) pARPPacket->tpa.ui8Byte4);
                    }
                }
                else {
                    // ARP Response addressed to this host --> it is not necessary to forward it on the external network
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                    "received an ARP Response with source protocol address %s and addressed to this host (target "
                                    "protocol address: %s); it is not necessary to forward it on the internal network\n",
                                    InetAddr (EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                    InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                    return 0;
                }

                // The destination MAC address of ARP Requests should always be broadcast
                pARPPacket->sha = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                pEthHeader->dest = (pARPPacket->ui16Oper == 1) ? NetProxyApplicationParameters::BROADCAST_MAC_ADDR : pEthHeader->dest;
                pARPPacket->hton();
                pEthHeader->hton();
                if (0 != (rc = sendPacketToHost (_pExternalInterface, pPacket, ui16PacketLen))) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_SevereError,
                                    "sendPacketToHost() of an ARP Response packet (source protocol address: %s - destination protocol "
                                    "address: %s) of %hu bytes long on the external network interface failed with rc = %d\n",
                                    InetAddr(pARPPacket->spa.ui32Addr).getIPAsString(),
                                    InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen, rc);
                    return -6;
                }
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MediumDetailDebug,
                                "successfully forwarded an ARP Response packet (opcode: %hu - source protocol address: %s - destination protocol "
                                "address: %s) of %hu bytes long on the external network interface; both the source Ethernet MAC "
                                "address and the source hardware address of the ARP packet were changed to be the MAC address "
                                "of the external network interface of this host\n", EndianHelper::ntohs (pARPPacket->ui16Oper),
                                InetAddr(pARPPacket->spa.ui32Addr).getIPAsString(), InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen);
            }
        }
        else if (pEthHeader->ui16EtherType == ET_IP) {
            register IPHeader *pIPHeader = (IPHeader*) (pPacket + sizeof (EtherFrameHeader));
            uint32 ui32SrcAddr = pIPHeader->srcAddr.ui32Addr;
            uint32 ui32DestAddr = pIPHeader->destAddr.ui32Addr;
            pIPHeader->ntoh();
            pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
            pIPHeader->destAddr.ui32Addr = ui32DestAddr;
            uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;

            if (NetProxyApplicationParameters::GATEWAY_MODE) {
                // NetProxy running in Gateway Mode
                if (pEthHeader->dest == NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR) {
                    // The packet was sent to this node; check if the NetProxy needs to process it
                    if (ui32DestAddr == NetProxyApplicationParameters::NETPROXY_IP_ADDR) {
                        // The packet is addressed to our host --> forward it onto the external interface so that the OS kernel can process it
                        pIPHeader->hton();
                        pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                        pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                        //pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                        pEthHeader->hton();
                        if (0 != (rc = sendPacketToHost (_pExternalInterface, pPacket, ui16PacketLen))) {
                            if ((ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF) && (EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                                // Send back on the external interface an ICMP Packet Fragmentation Needed (Type 3, Code 4) message
                                pEthHeader->ntoh();
                                pIPHeader->ntoh();
                                pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                                pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                                if (0 != (rc = buildAndSendICMPMessageToHost (_pInternalInterface, ICMPHeader::T_Destination_Unreachable, ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                                                              NetProxyApplicationParameters::NETPROXY_IP_ADDR, pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                                    "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation "
                                                    "Needed (Type 3, Code 4) message to the internal network; rc = %d\n", rc);
                                    return -7;
                                }
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_LowDetailDebug,
                                                "IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu bytes long could not be "
                                                "forwarded on the external network because its size exceeds the MTU of %hu bytes; successfully "
                                                "sent back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                                pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(),
                                                ui16PacketLen, NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF);
                                return 1;
                            }
                            else if (ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF) {
                                // Fragmentation of IP packets not yet implemented --> drop packet!
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_Warning,
                                                "Fragmentation of IP packets not yet implemented: packet will be dropped\n");
                            }
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                            "sendPacketToHost() of an IP packet (IP protocol type: %hhu - source: %s - destination: %s) "
                                            "of %hu bytes long on the external interface failed with rc = %d\n", pIPHeader->ui8Proto,
                                            InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, rc);
                            return -8;
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                        "received a packet addressed to this host (source IP: %s - destination: IP %s): NetProxy "
                                        "forwarded the packet on the external interface so that the OS can process it\n",
                                        InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString());
                        return 0;
                    }

                    if (!_pConnectionManager->isRemoteHostIPMapped (pIPHeader->destAddr.ui32Addr)) {
                        /* IP packet with the MAC address of this machine, but with an IP address that cannot be mapped to any
                        * remote NetProxy --> change destination Ethernet MAC address and forward it to the network gateway. */
                        pIPHeader->hton();
                        pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                        pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                        pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                        const EtherMACAddr *pDestEther = _pARPCache.lookup (EndianHelper::ntohl (pIPHeader->destAddr.ui32Addr));
                        pDestEther = pDestEther ? pDestEther : &NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_MAC_ADDR;
                        pEthHeader->dest = *pDestEther;
                        pEthHeader->hton();
                        if (0 != (rc = sendPacketToHost (_pExternalInterface, pPacket, ui16PacketLen))) {
                            if ((ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF) && (EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                                // Send back on the external interface an ICMP Packet Fragmentation Needed (Type 3, Code 4) message
                                pEthHeader->ntoh();
                                pIPHeader->ntoh();
                                pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                                pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                                if (0 != (rc = buildAndSendICMPMessageToHost (_pInternalInterface, ICMPHeader::T_Destination_Unreachable, ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                                                              NetProxyApplicationParameters::NETPROXY_IP_ADDR, pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                                    "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation "
                                                    "Needed (Type 3, Code 4) message to the internal network; rc = %d\n", rc);
                                    return -9;
                                }
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_LowDetailDebug,
                                                "IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu bytes long could not be "
                                                "forwarded on the external network because its size exceeds the MTU of %hu bytes; successfully "
                                                "sent back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                                pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(),
                                                ui16PacketLen, NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF);
                                return 2;
                            }
                            else if (ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF) {
                                // Fragmentation of IP packets not yet implemented --> drop packet!
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_Warning,
                                                "Fragmentation of IP packets not yet implemented: packet will be dropped\n");
                            }
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                            "sendPacketToHost() of an IP packet (IP protocol type: %hhu - source: %s - destination: %s) "
                                            "of %hu bytes long on the external interface failed with rc = %d\n", pIPHeader->ui8Proto,
                                            InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, rc);
                            return -10;
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                        "successfully forwarded an IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu bytes "
                                        "long on the external network interface; both the source and destination Ethernet MAC addresses of the "
                                        "Ethernet packet were changed to be the MAC address of the external network interface of this host and "
                                        "%2x:%2x:%2x:%2x:%2x:%2x, respectively\n", pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(),
                                        InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, pDestEther->ui8Byte1, pDestEther->ui8Byte2,
                                        pDestEther->ui8Byte3, pDestEther->ui8Byte4, pDestEther->ui8Byte5, pDestEther->ui8Byte6);
                        return 0;
                    }
                    // If we reach this point, packet needs further processing. It will be done below.
                }
                else if (isMACAddrBroadcast (pEthHeader->dest) || isMACAddrMulticast (pEthHeader->dest)) {
                    // Forward broadcast packets on the external interface
                    pIPHeader->hton();
                    pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                    pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                    pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                    pEthHeader->hton();
                    if (0 != (rc = sendPacketToHost (_pExternalInterface, pPacket, ui16PacketLen))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_SevereError,
                                        "sendPacketToHost() of an IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu bytes "
                                        "long and with a %s destination MAC address on the external interface failed with rc = %d\n",
                                        pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(),
                                        ui16PacketLen, isMACAddrBroadcast (pEthHeader->dest) ? "Broadcast" : "Multicast", rc);
                        return -11;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "successfully forwarded an IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu bytes long "
                                    "and with a %s destination MAC address on the external network interface; the source Ethernet MAC address "
                                    "of the Ethernet packet was changed to be the MAC address of the external network interface of this host\n",
                                    pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(),
                                    ui16PacketLen, isMACAddrBroadcast (pEthHeader->dest) ? "Broadcast" : "Multicast");
                    if (!_pConnectionManager->isRemoteHostIPMapped (pIPHeader->destAddr.ui32Addr)) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                        "no mapping found for the destination address of the %s IP packet (source: %s - destination: %s): "
                                        "dropping the packet\n", isMACAddrBroadcast (pEthHeader->dest) ? "Broadcast" : "Multicast",
                                        InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(), InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString());
                        return 0;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "found a mapping for the destination address of the %s IP packet (source: %s - destination: %s): "
                                    "processing it for forwading\n", isMACAddrBroadcast (pEthHeader->dest) ? "Broadcast" : "Multicast",
                                    InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(), InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString());
                }
                else {
                    // Packet not addressed to this host: change source MAC address and forward it to the external network
                    pIPHeader->hton();
                    pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                    pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                    pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                    pEthHeader->hton();
                    if (0 != (rc = sendPacketToHost (_pExternalInterface, pPacket, ui16PacketLen))) {
                        if ((ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF) && (EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                            // Send back on the external interface an ICMP Packet Fragmentation Needed (Type 3, Code 4) message
                            pEthHeader->ntoh();
                            pIPHeader->ntoh();
                            pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                            pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                            if (0 != (rc = buildAndSendICMPMessageToHost (_pInternalInterface, ICMPHeader::T_Destination_Unreachable, ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                                                          NetProxyApplicationParameters::NETPROXY_IP_ADDR, pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                                "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation "
                                                "Needed (Type 3, Code 4) message to the internal network; rc = %d\n", rc);
                                return -12;
                            }
                            else {
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_LowDetailDebug,
                                                "IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu bytes long could not be "
                                                "forwarded to the external network because its size exceeds the MTU of %hu bytes; successfully "
                                                "sent back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                                pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(),
                                                ui16PacketLen, NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF);
                                return 3;
                            }
                        }
                        else if (ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF) {
                            // Fragmentation of IP packets not yet implemented --> drop packet!
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_Warning,
                                            "Fragmentation of IP packets not yet implemented: packet will be dropped\n");
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                        "sendPacketToHost() of an IP packet (IP protocol type: %hhu - source: %s - destination: %s) "
                                        "of %hu bytes long on the external interface failed with rc = %d\n", pIPHeader->ui8Proto,
                                        InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, rc);
                        return -13;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "successfully forwarded an IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu bytes "
                                    "long on the external network interface; the source Ethernet MAC address of the Ethernet packet was "
                                    "changed to be the MAC address of the external network interface of this host\n", pIPHeader->ui8Proto,
                                    InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen);
                    return 0;
                }
            }
            else {
                // NetProxy running in host mode
                if (pIPHeader->srcAddr.ui32Addr != NetProxyApplicationParameters::NETPROXY_IP_ADDR) {
                    // Packet does not come from the TAP interface - ignore it
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "ignoring IP packet whose source address %s does not match the virtual "
                                    "interface IP address %s while the NetProxy is running in host mode\n",
                                    InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(),
                                    InetAddr(NetProxyApplicationParameters::NETPROXY_IP_ADDR).getIPAsString());
                    return 0;
                }
            }

            // If we reach this point, the NetProxy needs to process the packet
            if (pIPHeader->ui8Proto == IP_PROTO_ICMP) {
                ICMPHeader *pICMPHeader = (ICMPHeader*) (((uint8*) pIPHeader) + ui16IPHeaderLen);
                uint8 *pICMPData = (uint8*) (((uint8*) pICMPHeader) + sizeof (ICMPHeader));
                uint16 ui16ICMPDataLen = ui16PacketLen - (sizeof (EtherFrameHeader) + ui16IPHeaderLen + sizeof (ICMPHeader));
                pICMPHeader->ntoh();
                const ProtocolSetting * const pProtocolSetting = _pConfigurationManager->mapAddrToProtocol (pIPHeader->srcAddr.ui32Addr, pIPHeader->destAddr.ui32Addr, IP_PROTO_ICMP);
                ProxyMessage::Protocol currentProtocol = pProtocolSetting ? pProtocolSetting->getProxyMessageProtocol() : ProtocolSetting::DEFAULT_ICMP_MAPPING_PROTOCOL;
                ConnectorType connectorType = ProtocolSetting::protocolToConnectorType (currentProtocol);
                if (!pProtocolSetting) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "received an ICMP packet destined to address %s which could not be mapped to any specific protocol - using standard protocol %s",
                                    InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(),
                                    ProtocolSetting::getProxyMessageProtocolAsString (ProtocolSetting::DEFAULT_UDP_MAPPING_PROTOCOL));
                }

                QueryResult query (_pConnectionManager->queryConnectionToRemoteHostForConnectorType (connectorType, pIPHeader->destAddr.ui32Addr, 0));
                const InetAddr * const pRemoteProxyAddr = query.getBestConnectionSolution();
                if (!pRemoteProxyAddr || !query.isValid()) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "received an ICMP message with type %hhu and code %hhu destined for %s which could not be mapped to a proxy address "
                                    "- replying with a destination unreachable:host unreachable ICMP message\n", pICMPHeader->ui8Type,
                                    pICMPHeader->ui8Code, InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString());
                    if (0 != (rc = buildAndSendICMPMessageToHost (_pInternalInterface, ICMPHeader::T_Destination_Unreachable, ICMPHeader::CDU_Host_Unreachable,
                                                                  NetProxyApplicationParameters::NETPROXY_IP_ADDR, pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_Warning,
                                        "buildAndSendICMPMessageToHost() failed with rc = %d\n", rc);
                        return -14;
                    }

                    return 0;
                }
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                "received an ICMP message from %s with type %hhu, code %hhu and %hu bytes addressed to %s that will remapped over %s; "
                                "rest of header is %hhu|%hhu|%hhu|%hhu\n", InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(), pICMPHeader->ui8Type,
                                pICMPHeader->ui8Code, ui16ICMPDataLen, InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(),
                                connectorTypeToString (connectorType), ((uint8*) &pICMPHeader->ui32RoH)[3], ((uint8*) &pICMPHeader->ui32RoH)[2],
                                ((uint8*) &pICMPHeader->ui32RoH)[1], ((uint8*) &pICMPHeader->ui32RoH)[0]);

                Connector * pConnector = _pConnectionManager->getConnectorForType (connectorType);
                if (pConnector == NULL) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                    "could not retrieve the connector to remote proxy with address %s for protocol %s\n",
                                    InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(),
                                    ProtocolSetting::getProxyMessageProtocolAsString (currentProtocol));
                    return -15;
                }
                if (!pConnector->isEnqueueingAllowed()) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_Warning,
                                    "could not enqueue packet in the %s connector; ignoring received ICMP packet\n",
                                    pConnector->getConnectorTypeAsString());
                    return 0;
                }

                Connection *pConnection = query.getActiveConnectionToRemoteProxy();
                if (!pConnection) {
                    pConnection = pConnector->openNewConnectionToRemoteProxy (query, false);
                    if (!pConnection) {
                        pConnection = pConnector->getAvailableConnectionToRemoteProxy (pRemoteProxyAddr);
                        if (!pConnection) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_SevereError,
                                            "could not create a new %sConnection to send an ICMP packet to the remote NetProxy\n",
                                            pConnector->getConnectorTypeAsString());
                            return -16;
                        }
                    }
                }

                bool bReachable = _pConnectionManager->getReachabilityFromRemoteHost (pIPHeader->destAddr.ui32Addr, 0);
                String mocketsConfFile = _pConnectionManager->getMocketsConfigFileForConnectionsToRemoteHost (pIPHeader->destAddr.ui32Addr, 0);
                if (0 != (rc = pConnection->sendICMPProxyMessageToRemoteHost (pRemoteProxyAddr, pICMPHeader->ui8Type, pICMPHeader->ui8Code, pICMPHeader->ui32RoH,
                                                                              pIPHeader->srcAddr.ui32Addr, pIPHeader->destAddr.ui32Addr, const_cast<const uint8 * const> (pICMPData),
                                                                              ui16ICMPDataLen, currentProtocol, bReachable))) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                    "could not send the ICMP Proxy Message with type %hhu and code %hhu to remote proxy with address %s via %s; rc = %d\n",
                                    pICMPHeader->ui8Type, pICMPHeader->ui8Code, InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(),
                                    pConnector->getConnectorTypeAsString(), rc);
                    return -17;
                }
            }
            else if (pIPHeader->ui8Proto == IP_PROTO_UDP) {
                UDPHeader *pUDPHeader = (UDPHeader*) (((uint8*) pIPHeader) + ui16IPHeaderLen);
                pUDPHeader->ntoh();
                if (!_pConnectionManager->isRemoteHostIPMapped (pIPHeader->destAddr.ui32Addr, pUDPHeader->ui16DPort)) {
                    if (NetProxyApplicationParameters::GATEWAY_MODE) {
                        /* TCP packet with an IP address that is mapped to a remote NetProxy, but the port number does
                         * not match --> change destination Ethernet MAC address and forward it to the network gateway. */
                        pUDPHeader->hton();
                        pIPHeader->hton();
                        pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                        pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                        pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                        const EtherMACAddr *pDestEther = _pARPCache.lookup (EndianHelper::ntohl (pIPHeader->destAddr.ui32Addr));
                        pDestEther = pDestEther ? pDestEther : &NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_MAC_ADDR;
                        pEthHeader->hton();
                        if (0 != (rc = sendPacketToHost (_pExternalInterface, pPacket, ui16PacketLen))) {
                            pUDPHeader->ntoh();
                            if ((ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF) && (EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                                // Send back on the external interface an ICMP Packet Fragmentation Needed (Type 3, Code 4) message
                                pEthHeader->ntoh();
                                pIPHeader->ntoh();
                                pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                                pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                                if (0 != (rc = buildAndSendICMPMessageToHost (_pInternalInterface, ICMPHeader::T_Destination_Unreachable, ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                    NetProxyApplicationParameters::NETPROXY_IP_ADDR, pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                                    "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation "
                                                    "Needed (Type 3, Code 4) message to the internal network; rc = %d\n", rc);
                                    return -18;
                                }
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_LowDetailDebug,
                                                "UDP packet (source: %s:%hu - destination: %s:%hu) of %hu bytes long could not be forwarded "
                                                "on the external network because its size exceeds the MTU of %hu bytes; successfully sent "
                                                "back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                                InetAddr (ui32SrcAddr).getIPAsString(), pUDPHeader->ui16SPort, InetAddr (ui32DestAddr).getIPAsString(),
                                                pUDPHeader->ui16DPort, ui16PacketLen, NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF);
                                return 4;
                            }
                            else if (ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF) {
                                // Fragmentation of IP packets not yet implemented --> drop packet!
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_Warning,
                                                "Fragmentation of IP packets not yet implemented: packet will be dropped!\n");
                            }
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                            "sendPacketToHost() of a UDP packet (source: %s:%hu - destination: %s:%hu) of %hu bytes long on the external "
                                            "interface failed with rc = %d\n", InetAddr (ui32SrcAddr).getIPAsString(), pUDPHeader->ui16SPort,
                                            InetAddr (ui32DestAddr).getIPAsString(), pUDPHeader->ui16DPort, ui16PacketLen, rc);
                            return -19;
                        }
                        pUDPHeader->ntoh();
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                        "successfully forwarded a UDP packet (source: %s:%hu - destination: %s:%hu) of %hu bytes long whose port did "
                                        "not match the entry in the Address Mapping Table on the external network interface; both the source and "
                                        "destination Ethernet MAC addresses of the Ethernet packet were changed to be the MAC address of the "
                                        "external network interface of this host and %2x:%2x:%2x:%2x:%2x:%2x, respectively\n",
                                        InetAddr (ui32SrcAddr).getIPAsString(), pUDPHeader->ui16SPort, InetAddr (ui32DestAddr).getIPAsString(),
                                        pUDPHeader->ui16DPort, ui16PacketLen, pDestEther->ui8Byte1, pDestEther->ui8Byte2,
                                        pDestEther->ui8Byte3, pDestEther->ui8Byte4, pDestEther->ui8Byte5, pDestEther->ui8Byte6);
                        return 0;
                    }
                    else {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MediumDetailDebug,
                                        "received a UDP packet destined for %s which could not be mapped to a proxy address - replying with a "
                                        "<Destination Unreachable:Host Unreachable> ICMP message\n", InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString());
                        if (0 != (rc = buildAndSendICMPMessageToHost (_pInternalInterface, ICMPHeader::T_Destination_Unreachable, NetProxyApplicationParameters::GATEWAY_MODE ?
                                                                      ICMPHeader::CDU_Port_Unreachable : ICMPHeader::CDU_Host_Unreachable,
                                                                      NetProxyApplicationParameters::NETPROXY_IP_ADDR, pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_Warning,
                                            "buildAndSendICMPMessageToHost() failed with rc = %d\n", rc);
                            return -20;
                        }
                        return 0;
                    }
                }

                uint16 ui16UDPPacketLen = pIPHeader->ui16TLen - ui16IPHeaderLen;
                const ProtocolSetting * const pProtocolSetting = _pConfigurationManager->mapAddrToProtocol (pIPHeader->srcAddr.ui32Addr, pUDPHeader->ui16SPort, pIPHeader->destAddr.ui32Addr,
                                                                                                            pUDPHeader->ui16DPort, IP_PROTO_UDP);
                ProxyMessage::Protocol currentProtocol = pProtocolSetting ? pProtocolSetting->getProxyMessageProtocol() : ProtocolSetting::DEFAULT_UDP_MAPPING_PROTOCOL;
                ConnectorType connectorType = ProtocolSetting::protocolToConnectorType (currentProtocol);
                if (!pProtocolSetting) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "received a UDP packet destined to address %s:%hu which could not be mapped to any specific protocol - using standard protocol %s",
                                    InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(), pUDPHeader->ui16DPort,
                                    ProtocolSetting::getProxyMessageProtocolAsString (ProtocolSetting::DEFAULT_UDP_MAPPING_PROTOCOL));
                }

                if (isMACAddrBroadcast (pEthHeader->dest)) {
                    // For now, do not use DisService for Broadcast and Multicast traffic
                    //if (0 != (rc = sendBCastMCastPacketToDisService ((const uint8*) pEthHeader, sizeof (EtherFrameHeader) + pIPHeader->ui16TLen))) {
                    //    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                    //                    "sendUDPPacketToDisService() failed with rc = %d\n", rc);
                    //    return -2;
                    //}

                    // For the moment, sendBroadcastPacket() method forwards the packets only to remote NetProxies specified in the configuration file
                    if (0 != (rc = sendBroadcastPacket ((const uint8*) pEthHeader, sizeof (EtherFrameHeader) + pIPHeader->ui16TLen, pIPHeader->srcAddr.ui32Addr,
                                                        pIPHeader->destAddr.ui32Addr, pUDPHeader->ui16DPort, &pProtocolSetting->getCompressionSetting()))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                        "sendBroadcastPacket() failed with rc = %d\n", rc);
                        return -21;
                    }
                    return 0;
                }
                else if (isMACAddrMulticast (pEthHeader->dest)) {
                    if (0 != (rc = sendMulticastPacket ((const uint8*) pEthHeader, sizeof (EtherFrameHeader) + pIPHeader->ui16TLen, pIPHeader->srcAddr.ui32Addr,
                                                        pIPHeader->destAddr.ui32Addr, pUDPHeader->ui16DPort, &pProtocolSetting->getCompressionSetting()))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                        "sendMulticastPacket() failed with rc = %d\n", rc);
                        return -22;
                    }
                }
                else {
                    QueryResult query (_pConnectionManager->queryConnectionToRemoteHostForConnectorType (connectorType, pIPHeader->destAddr.ui32Addr, pUDPHeader->ui16DPort));
                    const InetAddr * const pRemoteProxyAddr = query.getBestConnectionSolution();
                    if (!pRemoteProxyAddr || !query.isValid()) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                        "received a UDP packet destined for %s which could not be mapped to a proxy address - replying with a "
                                        "<Destination Unreachable:%s> ICMP message\n", InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(),
                                        NetProxyApplicationParameters::GATEWAY_MODE ? "Port Unreachable" : "Host Unreachable");
                        if (0 != (rc = buildAndSendICMPMessageToHost (_pInternalInterface, ICMPHeader::T_Destination_Unreachable, NetProxyApplicationParameters::GATEWAY_MODE ?
                                                                      ICMPHeader::CDU_Port_Unreachable : ICMPHeader::CDU_Host_Unreachable,
                                                                      NetProxyApplicationParameters::NETPROXY_IP_ADDR, pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_Warning,
                                            "buildAndSendICMPMessageToHost() failed with rc = %d\n", rc);
                            return -23;
                        }
                        return 0;
                    }
                    Connector * pConnector = _pConnectionManager->getConnectorForType (connectorType);
                    if (pConnector == NULL) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                        "could not retrieve the connector to remote proxy with address %s for protocol %s\n",
                                        InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(),
                                        ProtocolSetting::getProxyMessageProtocolAsString (currentProtocol));
                        return -24;
                    }

                    Connection *pConnection = query.getActiveConnectionToRemoteProxy();
                    if (!pConnection) {
                        pConnection = pConnector->openNewConnectionToRemoteProxy (query, false);
                        if (!pConnection) {
                            pConnection = pConnector->getAvailableConnectionToRemoteProxy (pRemoteProxyAddr);
                            if (!pConnection) {
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_SevereError,
                                                "could not create a new %sConnection to send a UDP packet to the remote NetProxy\n",
                                                pConnector->getConnectorTypeAsString());
                                return -25;
                            }
                        }
                    }

                    if ((pIPHeader->ui16FlagsAndFragOff & (IP_MF_FLAG_FILTER | IP_OFFSET_FILTER)) == 0) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                        "UDP Unicast packet received: payload size %u; source address: %s:%hu - destination address: %s:%hu\n",
                                        static_cast<unsigned int> (ui16UDPPacketLen) - sizeof (UDPHeader), InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(),
                                        pUDPHeader->ui16SPort, InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(), pUDPHeader->ui16DPort);
                    }
                    else {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MediumDetailDebug,
                                        "UDP Unicast fragmented packet received: IPIdent %hu - offset %hu - payload size %hu; "
                                        "source address: %s:%hu - destination address: %s:%hu\n", pIPHeader->ui16Ident,
                                        (pIPHeader->ui16FlagsAndFragOff & IP_OFFSET_FILTER) * 8, ui16UDPPacketLen,
                                        InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(), pUDPHeader->ui16SPort,
                                        InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(), pUDPHeader->ui16DPort);
                    }

                    if (0 > (rc = _localUDPDatagramsManagerThread.addDatagramToOutgoingQueue (pRemoteProxyAddr, pConnection, pConnector, &(pProtocolSetting->getCompressionSetting()),
                                                                                              currentProtocol, pIPHeader, pUDPHeader))) {
                        if (rc == -2) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MediumDetailDebug,
                                            "impossible to reassemble received UDP fragment; dropping fragment\n", rc);
                        }
                        else {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                            "addDatagramToOutgoingQueue() failed with rc = %d\n", rc);
                            return -26;
                        }
                    }
                }
            }
            else if (pIPHeader->ui8Proto == IP_PROTO_TCP) {
                TCPHeader *pTCPHeader = (TCPHeader*) (((uint8*) pIPHeader) + ui16IPHeaderLen);
                pTCPHeader->ntoh();
                if (!_pConnectionManager->isRemoteHostIPMapped (pIPHeader->destAddr.ui32Addr, pTCPHeader->ui16DPort)) {
                    /* TCP packet with an IP address that is mapped to a remote NetProxy, but the port number does
                     * not match --> change destination Ethernet MAC address and forward it to the network gateway. */
                    pTCPHeader->hton();
                    pIPHeader->hton();
                    pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                    pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                    pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                    const EtherMACAddr *pDestEther = _pARPCache.lookup (EndianHelper::ntohl (pIPHeader->destAddr.ui32Addr));
                    pDestEther = pDestEther ? pDestEther : &NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_MAC_ADDR;
                    pEthHeader->hton();
                    if (0 != (rc = sendPacketToHost (_pExternalInterface, pPacket, ui16PacketLen))) {
                        pTCPHeader->ntoh();
                        if ((ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF) && (EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                            // Send back on the external interface an ICMP Packet Fragmentation Needed (Type 3, Code 4) message
                            pEthHeader->ntoh();
                            pIPHeader->ntoh();
                            pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                            pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                            if (0 != (rc = buildAndSendICMPMessageToHost (_pInternalInterface, ICMPHeader::T_Destination_Unreachable, ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                                                          NetProxyApplicationParameters::NETPROXY_IP_ADDR, pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                                "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation "
                                                "Needed (Type 3, Code 4) message to the internal network; rc = %d\n", rc);
                                return -27;
                            }
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_LowDetailDebug,
                                            "TCP packet (source: %s:%hu - destination: %s:%hu) of %hu bytes long could not be forwarded "
                                            "on the external network because its size exceeds the MTU of %hu bytes; successfully sent "
                                            "back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                             InetAddr(ui32SrcAddr).getIPAsString(), pTCPHeader->ui16SPort, InetAddr(ui32DestAddr).getIPAsString(),
                                             pTCPHeader->ui16DPort, ui16PacketLen, NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF);
                            return 5;
                        }
                        else if (ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF) {
                            // Fragmentation of IP packets not yet implemented --> drop packet!
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_Warning,
                                            "Fragmentation of IP packets not yet implemented: packet will be dropped!\n");
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                        "sendPacketToHost() of a TCP packet (source: %s:%hu - destination: %s:%hu) of %hu bytes long on the external "
                                        "interface failed with rc = %d\n", InetAddr(ui32SrcAddr).getIPAsString(), pTCPHeader->ui16SPort,
                                        InetAddr(ui32DestAddr).getIPAsString(), pTCPHeader->ui16DPort, ui16PacketLen, rc);
                        return -28;
                    }
                    pTCPHeader->ntoh();
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "successfully forwarded a TCP packet (source: %s:%hu - destination: %s:%hu) of %hu bytes long whose port did "
                                    "not match the entry in the Address Mapping Table on the external network interface; both the source and "
                                    "destination Ethernet MAC addresses of the Ethernet packet were changed to be the MAC address of the "
                                    "external network interface of this host and %2x:%2x:%2x:%2x:%2x:%2x, respectively\n",
                                    InetAddr(ui32SrcAddr).getIPAsString(), pTCPHeader->ui16SPort, InetAddr(ui32DestAddr).getIPAsString(),
                                    pTCPHeader->ui16DPort, ui16PacketLen, pDestEther->ui8Byte1, pDestEther->ui8Byte2,
                                    pDestEther->ui8Byte3, pDestEther->ui8Byte4, pDestEther->ui8Byte5, pDestEther->ui8Byte6);
                    return 0;
                }
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                "received a TCP Packet with SEQ number %u, ACK number %u, and flags %hhu from %s:%hu to %s:%hu "
                                "that needs to be processed by the NetProxy\n", pTCPHeader->ui32SeqNum, pTCPHeader->ui32AckNum,
                                pTCPHeader->ui8Flags, InetAddr (pIPHeader->srcAddr.ui32Addr).getIPAsString(), pTCPHeader->ui16SPort,
                                InetAddr (pIPHeader->destAddr.ui32Addr).getIPAsString(), pTCPHeader->ui16DPort);
                if (0 != (rc = TCPManager::handleTCPPacketFromHost ((const uint8*) pIPHeader, pIPHeader->ui16TLen))) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_MildError,
                                    "handleTCPPacketFromHost() failed with rc = %d\n", rc);
                    return -29;
                }
            }
            else {
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_LowDetailDebug,
                                "received a packet with IP protocol type %hhu - ignoring it\n", pIPHeader->ui8Proto);
            }
        }
        else {
            // Non-IP, Non-ARP ethernet packet received
            if (NetProxyApplicationParameters::GATEWAY_MODE) {
                // NetProxy running in gateway mode --> forward it to the external interface
                pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                pEthHeader->hton();
                if (0 != (rc = sendPacketToHost (_pExternalInterface, pPacket, ui16PacketLen))) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_SevereError,
                                    "sendPacketToHost() of an unknown ethernet packet (ethernet protocol type %hu) of "
                                    "%hu bytes long on the external interface failed with rc = %d\n",
                                    EndianHelper::ntohs (pEthHeader->ui16EtherType), ui16PacketLen, rc);
                    return -30;
                }
                if (pEthHeader->ui16EtherType == ET_IP_v6) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "successfully forwarded IPv6 packet of %hu bytes long on the external network (source MAC address "
                                    "%2x:%2x:%2x:%2x:%2x:%2x - destination MAC address %2x:%2x:%2x:%2x:%2x:%2x); the source Ethernet MAC "
                                    "address of the Ethernet packet was changed to be the MAC address of the external network interface "
                                    "of this host\n", ui16PacketLen, (int) pEthHeader->src.ui8Byte2, (int) pEthHeader->src.ui8Byte1,
                                    (int) pEthHeader->src.ui8Byte4, (int) pEthHeader->src.ui8Byte3, (int) pEthHeader->src.ui8Byte6,
                                    (int) pEthHeader->src.ui8Byte5, (int) pEthHeader->dest.ui8Byte2, (int) pEthHeader->dest.ui8Byte1,
                                    (int) pEthHeader->dest.ui8Byte4, (int) pEthHeader->dest.ui8Byte3,
                                    (int) pEthHeader->dest.ui8Byte6, (int) pEthHeader->dest.ui8Byte5);
                }
                else {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", Logger::L_HighDetailDebug,
                                    "successfully forwarded an unknown ethernet packet (ethernet protocol type %hu) of %hu bytes long on the external "
                                    "network (source MAC address %2x:%2x:%2x:%2x:%2x:%2x - destination MAC address %2x:%2x:%2x:%2x:%2x:%2x); the "
                                    "source Ethernet MAC address of the Ethernet packet was changed to be the MAC address of the external network "
                                    "interface of this host\n", EndianHelper::ntohs (pEthHeader->ui16EtherType), ui16PacketLen,
                                    (int) pEthHeader->src.ui8Byte2, (int) pEthHeader->src.ui8Byte1, (int) pEthHeader->src.ui8Byte4,
                                    (int) pEthHeader->src.ui8Byte3, (int) pEthHeader->src.ui8Byte6, (int) pEthHeader->src.ui8Byte5,
                                    (int) pEthHeader->dest.ui8Byte2, (int) pEthHeader->dest.ui8Byte1, (int) pEthHeader->dest.ui8Byte4,
                                    (int) pEthHeader->dest.ui8Byte3, (int) pEthHeader->dest.ui8Byte6, (int) pEthHeader->dest.ui8Byte5);
                }
            }
        }

        return 0;
    }

    int PacketRouter::handlePacketFromExternalInterface (const uint8 * const pPacket, uint16 ui16PacketLen)
    {
        int rc;
        EtherFrameHeader * const pEthHeader = (EtherFrameHeader*) pPacket;
        pEthHeader->ntoh();

        if (hostBelongsToTheInternalNetwork (pEthHeader->src)) {
            // Packet originated on the internal network was just reinjected on the external one --> ignore it
            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                            "ignoring ethernet packet originated on the internal network (source MAC address %2x:%2x:%2x:%2x:%2x:%2x) and previously "
                            "forwarded on the external one by the NetProxy (destination MAC address %2x:%2x:%2x:%2x:%2x:%2x)\n", (int) pEthHeader->src.ui8Byte1,
                            (int) pEthHeader->src.ui8Byte2, (int) pEthHeader->src.ui8Byte3, (int) pEthHeader->src.ui8Byte4, (int) pEthHeader->src.ui8Byte5,
                            (int) pEthHeader->src.ui8Byte6, (int) pEthHeader->dest.ui8Byte1, (int) pEthHeader->dest.ui8Byte2, (int) pEthHeader->dest.ui8Byte3,
                            (int) pEthHeader->dest.ui8Byte4, (int) pEthHeader->dest.ui8Byte5, (int) pEthHeader->dest.ui8Byte6);
            return 0;
        }
        if (!hostBelongsToTheExternalNetwork (pEthHeader->src)) {
            // Add new host to the list of hosts in the internal network
            _daExternalHosts[_daExternalHosts.getHighestIndex() + 1] = pEthHeader->src;
            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                            "added source MAC address %2x:%2x:%2x:%2x:%2x:%2x to the set of hosts belonging to the external network\n",
                            (int) pEthHeader->src.ui8Byte1, (int) pEthHeader->src.ui8Byte2, (int) pEthHeader->src.ui8Byte3,
                            (int) pEthHeader->src.ui8Byte4, (int) pEthHeader->src.ui8Byte5, (int) pEthHeader->src.ui8Byte6);
        }

        if (pEthHeader->ui16EtherType == ET_ARP) {
            register ARPPacket *pARPPacket = (ARPPacket*) (pPacket + sizeof (EtherFrameHeader));
            pARPPacket->ntoh();
            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                            "ARP Packet: operation %d - source %d.%d.%d.%d (%2x:%2x:%2x:%2x:%2x:%2x) - target %d.%d.%d.%d (%2x:%2x:%2x:%2x:%2x:%2x)\n",
                            (int) pARPPacket->ui16Oper, (int) pARPPacket->spa.ui8Byte1, (int) pARPPacket->spa.ui8Byte2, (int) pARPPacket->spa.ui8Byte3,
                            (int) pARPPacket->spa.ui8Byte4, (int) pARPPacket->sha.ui8Byte1, (int) pARPPacket->sha.ui8Byte2, (int) pARPPacket->sha.ui8Byte3,
                            (int) pARPPacket->sha.ui8Byte4, (int) pARPPacket->sha.ui8Byte5, (int) pARPPacket->sha.ui8Byte6, (int) pARPPacket->tpa.ui8Byte1,
                            (int) pARPPacket->tpa.ui8Byte2, (int) pARPPacket->tpa.ui8Byte3, (int) pARPPacket->tpa.ui8Byte4,
                            (int) pARPPacket->tha.ui8Byte1, (int) pARPPacket->tha.ui8Byte2, (int) pARPPacket->tha.ui8Byte3,
                            (int) pARPPacket->tha.ui8Byte4, (int) pARPPacket->tha.ui8Byte5, (int) pARPPacket->tha.ui8Byte6);

            if (pARPPacket->spa.ui32Addr != 0) {
                // This is not an ARP probe --> process it
                if ((EndianHelper::htonl (pARPPacket->spa.ui32Addr) != NetProxyApplicationParameters::NETPROXY_IP_ADDR) &&
                    (pARPPacket->sha != NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR)) {
                    // The sender of the ARP message is not this machine --> proceed and cache the MAC address
                    _pARPCache.insert (pARPPacket->spa.ui32Addr, pARPPacket->sha);              // Opportunistically cache the MAC address of the sender
                    if (EndianHelper::htonl (pARPPacket->spa.ui32Addr) == NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR) {
                        // Cache MAC address of the gateway
                        NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_MAC_ADDR = pARPPacket->sha;
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_LowDetailDebug,
                                        "caching ARP address %2x:%2x:%2x:%2x:%2x:%2x for gateway node at IP %d.%d.%d.%d\n",
                                        (int) pARPPacket->sha.ui8Byte1, (int) pARPPacket->sha.ui8Byte2, (int) pARPPacket->sha.ui8Byte3,
                                        (int) pARPPacket->sha.ui8Byte4, (int) pARPPacket->sha.ui8Byte5, (int) pARPPacket->sha.ui8Byte6,
                                        (int) pARPPacket->spa.ui8Byte1, (int) pARPPacket->spa.ui8Byte2,
                                        (int) pARPPacket->spa.ui8Byte3, (int) pARPPacket->spa.ui8Byte4);
                    }
                    else {
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_MediumDetailDebug,
                                        "caching ARP address %2x:%2x:%2x:%2x:%2x:%2x for IP address %d.%d.%d.%d\n",
                                        (int) pARPPacket->sha.ui8Byte1, (int) pARPPacket->sha.ui8Byte2, (int) pARPPacket->sha.ui8Byte3,
                                        (int) pARPPacket->sha.ui8Byte4, (int) pARPPacket->sha.ui8Byte5, (int) pARPPacket->sha.ui8Byte6,
                                        (int) pARPPacket->spa.ui8Byte1, (int) pARPPacket->spa.ui8Byte2,
                                        (int) pARPPacket->spa.ui8Byte3, (int) pARPPacket->spa.ui8Byte4);
                    }
                    if (EndianHelper::htonl (pARPPacket->tpa.ui32Addr) != NetProxyApplicationParameters::NETPROXY_IP_ADDR) {
                        // The Target Protocol Address is not the address of this machine
                        if (pARPPacket->ui16Oper == 2) {
                            // ARP Response
                            if (pARPPacket->tha == NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR) {
                                /* The Target Hardware Address is the MAC address of this machine. This means that the NetProxy had
                                 * forwarded an ARP Request from the internal to the external interface in the past and changed its
                                 * source hardware address to be the one of this machine --> change it back to the original MAC address. */
                                const EtherMACAddr * const pDestEther = _pARPCache.lookup (pARPPacket->tpa.ui32Addr);
                                if (!pDestEther) {
                                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_Warning,
                                                    "received an ARP Response with source protocol address %s, target protocol address not of this machine "
                                                    "(%s), but with the target hardware address of this machine; however, it was not possible to find "
                                                    "the real hardware address in the local ARP cache\n",
                                                    InetAddr (EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                                    InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                                    return -1;
                                }
                                pARPPacket->tha = *pDestEther;
                                pEthHeader->dest = *pDestEther;
                            }
                            else {
                                // ARP Response with a Target Hardware Address that is not the MAC address of this machine --> cache it
                                _pARPCache.insert (pARPPacket->tpa.ui32Addr, pARPPacket->tha);
                                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_LowDetailDebug,
                                                "caching ARP address %2x:%2x:%2x:%2x:%2x:%2x for IP address %s\n",
                                                (int) pARPPacket->tha.ui8Byte1, (int) pARPPacket->tha.ui8Byte2, (int) pARPPacket->tha.ui8Byte3,
                                                (int) pARPPacket->tha.ui8Byte4, (int) pARPPacket->tha.ui8Byte5, (int) pARPPacket->tha.ui8Byte6,
                                                InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                            }

                            if (EndianHelper::htonl (pARPPacket->tpa.ui32Addr) == NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR) {
                                // The Target Protocol Address is the Network Gateway --> do not forward the packet on the internal interface
                                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                                "dropping ARP Response with Target Protocol Address of the network gateway (address: %s)\n",
                                                InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                                return 0;
                            }
                        }
                        // This packet needs to be forwarded on the internal network --> will be done below
                    }
                    else {
                        // ARP message with the Target Protocol Address of this host --> it is not necessary to forward it on the internal network
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                        "received an ARP %s with Source Protocol Address %s addressed to this host (Target Protocol Address: %s); "
                                        "avoid forwarding it on the internal network\n", (pARPPacket->ui16Oper == 1) ? "Request": "Response",
                                        InetAddr (EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                        InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                        return 0;
                    }
                }
                else if (pARPPacket->sha == NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR) {
                    if (EndianHelper::htonl (pARPPacket->spa.ui32Addr) == NetProxyApplicationParameters::NETPROXY_IP_ADDR) {
                        // The sender of the ARP message is this machine
                        if (pARPPacket->ui16Oper == 1) {
                            // ARP Request
                            if (EndianHelper::htonl (pARPPacket->tpa.ui32Addr) == NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR) {
                                // ARP Request generated by this host and addressed to the network gateway --> avoid forwarding it on the internal network
                                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                                "ARP Request generated by this machine and addressed to the network gateway (Source Protocol "
                                                "Address: %s - Target Protocol Address: %s); avoid forwarding packet on the internal network\n",
                                                InetAddr (EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                                InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                                return 0;
                            }

                            const EtherMACAddr * const pSrcEther = _pARPCache.lookup (pARPPacket->tpa.ui32Addr);
                            if (pSrcEther && hostBelongsToTheInternalNetwork (*pSrcEther)) {
                                /* The NetProxy knows that the destination belongs to the internal network --> immediately reply to it.
                                 * If this is not done, the OS will see the ARP Response coming from both the internal and external interface,
                                 * in this order, and it will not generate packets, since the internal interface does not have an IP address. */
                                if (0 != (rc = sendARPReplyToHost (_pExternalInterface, pARPPacket, *pSrcEther))) {
                                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_SevereError,
                                                    "sendARPReplyToHost() on the external network interface failed with rc = %d\n", rc);
                                    return -2;
                                }
                                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_MediumDetailDebug,
                                                "successfully sent an ARP Response to this same host with the pair <Source Protocol Address %s "
                                                "- Source Hardware Address: %2x:%2x:%2x:%2x:%2x:%2x> on the external network interface\n",
                                                InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString(),
                                                pSrcEther->ui8Byte1, pSrcEther->ui8Byte2, pSrcEther->ui8Byte3,
                                                pSrcEther->ui8Byte4, pSrcEther->ui8Byte5, pSrcEther->ui8Byte6);
                                return 0;
                            }
                            else if (pSrcEther) {
                                // The host belongs to the external network --> avoid forwarding the ARP Request on the internal network
                                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                                "successfully sent an ARP Response to this same host with the pair <Source Protocol Address %s "
                                                "- Source Hardware Address: %2x:%2x:%2x:%2x:%2x:%2x> on the external network interface\n",
                                                InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString(),
                                                pSrcEther->ui8Byte1, pSrcEther->ui8Byte2, pSrcEther->ui8Byte3,
                                                pSrcEther->ui8Byte4, pSrcEther->ui8Byte5, pSrcEther->ui8Byte6);
                                return 0;
                            }
                            // The ARP Cache does not contain the requested address --> forward the ARP Request on the internal interface
                        }
                        else {
                            // ARP Response
                            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                            "ARP Response generated by this machine (Source Protocol Address: %s - Target Protocol Address: %s); "
                                            "this is a reply to an ARP Request generated by a host in the external network - avoid forwarding "
                                            "the packet on the internal network\n", InetAddr (EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                            InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                        }
                    }
                    else {
                        // ARP message with the source hardware address of this machine but a different IP
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                        "ARP %s with the source Hardware Address of this machine, but a different Source Protocol Address (%s), "
                                        "and Target Protocol Address %s; the NetProxy forwarded the packet from the internal to the external "
                                        "network in the past - ignore it\n", (pARPPacket->ui16Oper == 1) ? "Request" : "Response",
                                        InetAddr (EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                        InetAddr (EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                        return 0;
                    }
                }
            }
            else {
                // ARP Probe
                if (EndianHelper::htonl (pARPPacket->tpa.ui32Addr) != NetProxyApplicationParameters::NETPROXY_IP_ADDR) {
                    // Forwarding ARP probe to the internal network
                    pARPPacket->sha = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                    pARPPacket->hton();
                    pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                    pEthHeader->hton();
                    if (0 != (rc = sendPacketToHost (_pInternalInterface, pPacket, ui16PacketLen))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_SevereError,
                                        "sendPacketToHost() of an ARP probe packet (target protocol address: %s) "
                                        "of %hu bytes long on the internal interface failed with rc = %d\n",
                                        InetAddr (pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen, rc);
                        return -3;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                    "successfully forwarded an ARP probe packet (target protocol address: %s) of %hu bytes long on the "
                                    "internal network interface; both the source Ethernet MAC address and the source hardware address of "
                                    "the ARP packet were changed to be the MAC address of the external network interface of this host\n",
                                    InetAddr (pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen);
                }
                else {
                    // Another host is trying to use our own address! --> reply with a gratuitous ARP announcement on both interfaces
                    if (0 != (rc = sendARPAnnouncement (_pInternalInterface, pARPPacket, NetProxyApplicationParameters::NETPROXY_IP_ADDR,
                                                        NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_SevereError,
                                        "sendARPAnnouncement() on the internal network interface failed with rc = %d\n", rc);
                        return -4;
                    }
                    if (0 != (rc = sendARPAnnouncement (_pExternalInterface, pARPPacket, NetProxyApplicationParameters::NETPROXY_IP_ADDR,
                                                        NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_SevereError,
                                        "sendARPAnnouncement() on the external network interface failed with rc = %d\n", rc);
                        return -4;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_LowDetailDebug,
                                    "successfully sent a gratuitous ARP Announcement on both interfaces\n");
                }

                return 0;
            }

            // In any case (both an ARP request or an ARP Response), change the source Ethernet MAC address to be the one of this host
            pARPPacket->sha = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
            pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
            // The destination MAC address of ARP Requests should always be broadcast
            pEthHeader->dest = (pARPPacket->ui16Oper == 1) ? NetProxyApplicationParameters::BROADCAST_MAC_ADDR : pEthHeader->dest;
            pARPPacket->hton();
            pEthHeader->hton();
            if (0 != (rc = sendPacketToHost (_pInternalInterface, pPacket, ui16PacketLen))) {
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_SevereError,
                                "sendPacketToHost() of an ARP packet (opcode: %hu - source protocol address: %s - destination "
                                "protocol address: %s) of %hu bytes long on the internal network interface failed with rc = %d\n",
                                EndianHelper::ntohs (pARPPacket->ui16Oper), InetAddr(pARPPacket->spa.ui32Addr).getIPAsString(),
                                InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen, rc);
                return -5;
            }
            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_MediumDetailDebug,
                            "successfully forwarded an ARP packet (opcode: %hu - source protocol address: %s - destination "
                            "protocol address: %s) of %hu bytes long on the internal network interface; both the source "
                            "Ethernet MAC address and the source hardware address of the ARP packet were changed to be "
                            "the MAC address of the external network interface of this host\n",
                            EndianHelper::ntohs (pARPPacket->ui16Oper), InetAddr(pARPPacket->spa.ui32Addr).getIPAsString(),
                            InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen);
            return 0;
        }
        else if (pEthHeader->ui16EtherType == ET_IP) {
            register IPHeader *pIPHeader = (IPHeader*) (pPacket + sizeof (EtherFrameHeader));
            uint32 ui32SrcAddr = pIPHeader->srcAddr.ui32Addr;
            uint32 ui32DestAddr = pIPHeader->destAddr.ui32Addr;
            pIPHeader->ntoh();
            pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
            pIPHeader->destAddr.ui32Addr = ui32DestAddr;

            if (pEthHeader->dest == NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR) {
                // The packet was sent to this node; check if the NetProxy needs to process it
                if (ui32DestAddr == NetProxyApplicationParameters::NETPROXY_IP_ADDR) {
                    // The packet is addressed to our host: the kernel will deliver it to the interested application -- just ignore it
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                    "received a packet addressed to this host (source IP: %s - destination IP: %s): this method will ignore the "
                                    "packet as the kernel will take care of it (it might be a packet coming from a remote NetProxy, or a packet "
                                    "coming from a host in the internal network that the NetProxy had forwarded on the external interface)\n",
                                    InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString());
                    return 0;
                }

                const EtherMACAddr * const pDestEther = _pARPCache.lookup (EndianHelper::ntohl (pIPHeader->destAddr.ui32Addr));
                if (!pDestEther) {
                    // No entry in the ARP Cache matches the destination IP address of the received IP packet --> drop it!
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_Warning,
                                    "received an IP packet (source IP: %s - destination IP: %s) with the destination Ethernet "
                                    "MAC address of this machine, but no matching entry was found in the ARP Cache table\n",
                                    InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString());
                    return -6;
                }
                pIPHeader->hton();
                pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                pEthHeader->dest = *pDestEther;
                pEthHeader->hton();
                if (0 != (rc = sendPacketToHost (_pInternalInterface, pPacket, ui16PacketLen))) {
                    if ((ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF) && (EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                        // Send back on the external interface an ICMP Packet Fragmentation Needed (Type 3, Code 4) message
                        pEthHeader->ntoh();
                        pIPHeader->ntoh();
                        pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                        pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                        if (0 != (rc = buildAndSendICMPMessageToHost (_pExternalInterface, ICMPHeader::T_Destination_Unreachable, ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                                                      NetProxyApplicationParameters::NETPROXY_IP_ADDR, ui32SrcAddr, pIPHeader))) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_SevereError,
                                            "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation Needed (Type %hhu, Code %hhu) message "
                                            "because an IP packet (IP protocol type: %hhu - source IP: %s - destination IP: %s) of %hu bytes long was received; rc = %d\n",
                                            static_cast<uint8> (ICMPHeader::T_Destination_Unreachable), static_cast<uint8> (ICMPHeader::CDU_Fragmentation_needed_and_DF_set),
                                            pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, rc);
                            return -7;
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_LowDetailDebug,
                                        "IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu bytes long could not be "
                                        "forwarded to the internal network because its size exceeds the MTU of %hu bytes; successfully "
                                        "sent back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                        pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(),
                                        ui16PacketLen, NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF);
                        return 1;
                    }
                    else if (ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF) {
                        // Fragmentation of IP packets not yet implemented --> drop packet!
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_Warning,
                                        "Fragmentation of IP packets not yet implemented: packet will be dropped!\n");
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_SevereError,
                                    "sendPacketToHost() of an IP packet (IP protocol type: %hhu - source: %s - destination: "
                                    "%s) of %hu bytes long on the internal interface failed with rc = %d\n",
                                    pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(),
                                    InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, rc);
                    return -8;
                }
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                "successfully forwarded an IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu "
                                "bytes long on the internal network interface; the source Ethernet MAC address of the Ethernet "
                                "packet was changed to be the MAC address of the external network interface of this host\n",
                                pIPHeader->ui8Proto, InetAddr (ui32SrcAddr).getIPAsString(),
                                InetAddr (ui32DestAddr).getIPAsString(), ui16PacketLen);
                return 0;
            }
            else if (hostBelongsToTheInternalNetwork (pEthHeader->dest)) {
                // Such packets are generated by the current node or by services such as a DHCP server --> forward them on the internal interface
                pIPHeader->hton();
                pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                pEthHeader->hton();
                if (0 != (rc = sendPacketToHost (_pInternalInterface, pPacket, ui16PacketLen))) {
                    if ((ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF) && (EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                        // Send back on the external interface an ICMP Packet Fragmentation Needed (Type 3, Code 4) message
                        pEthHeader->ntoh();
                        pIPHeader->ntoh();
                        pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                        pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                        if (0 != (rc = buildAndSendICMPMessageToHost (_pExternalInterface, ICMPHeader::T_Destination_Unreachable, ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                                                      NetProxyApplicationParameters::NETPROXY_IP_ADDR, ui32SrcAddr, pIPHeader))) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_SevereError,
                                            "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation Needed (Type %hhu, Code %hhu) message "
                                            "because an IP packet (IP protocol type: %hhu - source IP: %s - destination IP: %s) of %hu bytes long was received; rc = %d\n",
                                            static_cast<uint8> (ICMPHeader::T_Destination_Unreachable), static_cast<uint8> (ICMPHeader::CDU_Fragmentation_needed_and_DF_set),
                                            pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, rc);
                            return -9;
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_LowDetailDebug,
                                        "IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu bytes long could not be "
                                        "forwarded to the internal network because its size exceeds the MTU of %hu bytes; successfully "
                                        "sent back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                        pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(),
                                        ui16PacketLen, NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF);
                        return 2;
                    }
                    else if (ui16PacketLen > NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF) {
                        // Fragmentation of IP packets not yet implemented --> drop packet!
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_Warning,
                                        "Fragmentation of IP packets not yet implemented: packet will be dropped!\n");
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_SevereError,
                                    "sendPacketToHost() of an IP packet (IP protocol type: %hhu - source: %s - destination: "
                                    "%s) of %hu bytes long on the internal interface failed with rc = %d\n",
                                    pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(),
                                    InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, rc);
                    return -10;
                }
            }
            else {
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                "IP packet (IP protocol type: %hhu - source: %s - destination: %s) of %hu bytes long and with "
                                "destination Ethernet MAC address %2x:%2x:%2x:%2x:%2x:%2x does not need to be processed\n",
                                pIPHeader->ui8Proto, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(),
                                ui16PacketLen, (int) pEthHeader->dest.ui8Byte1, (int) pEthHeader->dest.ui8Byte2,
                                (int) pEthHeader->dest.ui8Byte3, (int) pEthHeader->dest.ui8Byte4,
                                (int) pEthHeader->dest.ui8Byte5, (int) pEthHeader->dest.ui8Byte6);
            }
        }
        else {
            // Non-IP, Non-ARP ethernet packet received
            if (NetProxyApplicationParameters::GATEWAY_MODE) {
                // NetProxy running in gateway mode --> forward it to the external interface
                EtherMACAddr oldEtherMACAddr = pEthHeader->src;
                pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
                pEthHeader->hton();
                if (0 != (rc = sendPacketToHost (_pInternalInterface, pPacket, ui16PacketLen))) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_SevereError,
                                    "sendPacketToHost() of an unknown ethernet packet (ethernet protocol type %hu) of "
                                    "%hu bytes long on the internal interface failed with rc = %d\n",
                                    EndianHelper::ntohs (pEthHeader->ui16EtherType), ui16PacketLen, rc);
                    return -11;
                }
                if (pEthHeader->ui16EtherType == ET_IP_v6) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                    "successfully forwarded IPv6 packet of %hu bytes long on the internal network (source MAC address "
                                    "%2x:%2x:%2x:%2x:%2x:%2x - destination MAC address %2x:%2x:%2x:%2x:%2x:%2x)\n",
                                    ui16PacketLen, (int) pEthHeader->src.ui8Byte2, (int) pEthHeader->src.ui8Byte1, (int) pEthHeader->src.ui8Byte4,
                                    (int) pEthHeader->src.ui8Byte3, (int) pEthHeader->src.ui8Byte6, (int) pEthHeader->src.ui8Byte5,
                                    (int) pEthHeader->dest.ui8Byte2, (int) pEthHeader->dest.ui8Byte1, (int) pEthHeader->dest.ui8Byte4,
                                    (int) pEthHeader->dest.ui8Byte3, (int) pEthHeader->dest.ui8Byte6, (int) pEthHeader->dest.ui8Byte5);
                }
                else {
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", Logger::L_HighDetailDebug,
                                    "successfully forwarded an unknown Ethernet packet (Ethernet protocol type %hu) of %hu bytes long on the internal "
                                    "network (source MAC address %2x:%2x:%2x:%2x:%2x:%2x - destination MAC address %2x:%2x:%2x:%2x:%2x:%2x); the "
                                    "source Ethernet MAC address of the Ethernet packet was changed to be the MAC address of the external "
                                    "network interface of this host\n", EndianHelper::ntohs (pEthHeader->ui16EtherType), ui16PacketLen,
                                    (int) oldEtherMACAddr.ui8Byte2, (int) oldEtherMACAddr.ui8Byte1, (int) oldEtherMACAddr.ui8Byte4,
                                    (int) oldEtherMACAddr.ui8Byte3, (int) oldEtherMACAddr.ui8Byte6, (int) oldEtherMACAddr.ui8Byte5,
                                    (int) pEthHeader->dest.ui8Byte2, (int) pEthHeader->dest.ui8Byte1, (int) pEthHeader->dest.ui8Byte4,
                                    (int) pEthHeader->dest.ui8Byte3, (int) pEthHeader->dest.ui8Byte6, (int) pEthHeader->dest.ui8Byte5);
                }
            }
        }

        return 0;
    }

    int PacketRouter::sendPacketToHost (NetworkInterface * const pNI, const uint8 * const pPacket, int iSize)
    {
        int rc;

        if ((rc = pNI->writePacket (pPacket, iSize)) != iSize) {
            checkAndLogMsg ("PacketRouter::sendPacketToHost", Logger::L_MildError,
                            "writePacket() failed with rc = %d\n", rc);
            return -1;
        }

        return 0;
    }

    int PacketRouter::wrapEtherAndSendPacketToHost (NetworkInterface * const pNI, uint8 *pui8Buf, uint16 ui16PacketLen)
    {
        int rc;
        EtherFrameHeader *pEthHeader = (EtherFrameHeader*) pui8Buf;
        IPHeader *pIPHeader = (IPHeader*) (pui8Buf + sizeof (EtherFrameHeader));
        uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
        if (ACMNetProxy::NetProxyApplicationParameters::GATEWAY_MODE) {
            const EtherMACAddr *pMACAddr = _pARPCache.lookup (pIPHeader->destAddr.ui32Addr);
            if (pMACAddr == NULL) {
                if ((pNI == _pInternalInterface) || NetUtils::areInSameNetwork (NetProxyApplicationParameters::NETPROXY_IP_ADDR,
                                                                                NetProxyApplicationParameters::NETPROXY_NETWORK_NETMASK,
                                                                                EndianHelper::htonl (pIPHeader->destAddr.ui32Addr),
                                                                                NetProxyApplicationParameters::NETPROXY_NETWORK_NETMASK)) {
                    checkAndLogMsg ("PacketRouter::wrapEtherAndSendPacketToHost", Logger::L_Warning,
                                    "do not have the MAC address associated to the IP address %s, which belongs to the same network of this "
                                    "machine - sending an ARP request to retrieve the MAC address and then returning an error code\n",
                                    InetAddr(EndianHelper::htonl (pIPHeader->destAddr.ui32Addr)).getIPAsString());
                    if (0 != (rc = sendARPRequest (pNI, EndianHelper::htonl (pIPHeader->srcAddr.ui32Addr), EndianHelper::htonl (pIPHeader->destAddr.ui32Addr)))) {
                        checkAndLogMsg ("PacketRouter::wrapEtherAndSendPacketToHost", Logger::L_MildError,
                                        "could not send ARP request for IP address %s; sendARPRequest() failed with rc = %d\n",
                                        InetAddr(EndianHelper::htonl (pIPHeader->srcAddr.ui32Addr)).getIPAsString(), rc);
                    }
                    return -1;
                }
                else {
                    pMACAddr = &ACMNetProxy::NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_MAC_ADDR;
                    if (*pMACAddr == NetProxyApplicationParameters::INVALID_MAC_ADDR) {
                        checkAndLogMsg ("PacketRouter::wrapEtherAndSendPacketToHost", Logger::L_Warning,
                                        "do not have the MAC address of the network gateway, necessary to reach IP address %s (with Netmask %s) - "
                                        "sending an ARP request to retrieve the gateway MAC address and then returning an error code\n",
                                        InetAddr(EndianHelper::htonl (pIPHeader->destAddr.ui32Addr)).getIPAsString(),
                                        InetAddr(NetProxyApplicationParameters::NETPROXY_NETWORK_NETMASK).getIPAsString());
                        if (0 != (rc = sendARPRequestForGatewayMACAddress())) {
                            checkAndLogMsg ("PacketRouter::wrapEtherAndSendPacketToHost", Logger::L_MildError,
                                            "sendARPRequestForGatewayMACAddress() failed with rc = %d\n", rc);
                        }
                        return -1;
                    }
                    checkAndLogMsg ("PacketRouter::wrapEtherAndSendPacketToHost", Logger::L_MediumDetailDebug,
                                    "IP address %s does not belong to the same network of this machine - using MAC address of the gateway\n",
                                    InetAddr(EndianHelper::htonl (pIPHeader->destAddr.ui32Addr)).getIPAsString());
                }
            }

            pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
            pEthHeader->dest = *pMACAddr;
        }
        else {
            buildVirtualNetProxyEthernetMACAddress (pEthHeader->src, pIPHeader->srcAddr.ui8Byte3, pIPHeader->srcAddr.ui8Byte4);
            buildVirtualNetProxyEthernetMACAddress (pEthHeader->dest, pIPHeader->destAddr.ui8Byte3, pIPHeader->destAddr.ui8Byte4);
        }
        pEthHeader->ui16EtherType = ET_IP;

        switch (pIPHeader->ui8Proto) {
            case IP_PROTO_ICMP:
            {
                ICMPHeader *pICMPHeader = (ICMPHeader*) (((uint8*)pIPHeader) + ui16IPHeaderLen);
                pICMPHeader->hton();
                break;
            }
            case IP_PROTO_TCP:
            {
                TCPHeader *pTCPHeader = (TCPHeader*) (((uint8*)pIPHeader) + ui16IPHeaderLen);
                pTCPHeader->hton();
                break;
            }
            case IP_PROTO_UDP:
            {
                UDPHeader *pUDPHeader = (UDPHeader*) (((uint8*)pIPHeader) + ui16IPHeaderLen);
                pUDPHeader->hton();
                break;
            }
            default:
            {
                checkAndLogMsg ("PacketRouter::wrapEtherAndSendPacketToHost", Logger::L_MildError,
                                "received a non-ICMP/non-TCP/non-UDP packet; protocol = %hhu\n",
                                pIPHeader->ui8Proto);
                return -2;
            }
        }
        pIPHeader->hton();
        pEthHeader->hton();

        if (0 != (rc = sendPacketToHost (pNI, pui8Buf, sizeof (EtherFrameHeader) + ui16PacketLen))) {
            checkAndLogMsg ("PacketRouter::wrapEtherAndSendPacketToHost", Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            return -3;
        }

        pEthHeader->ntoh();
        pIPHeader->ntoh();
        switch (pIPHeader->ui8Proto) {
            case IP_PROTO_ICMP:
            {
                ICMPHeader *pICMPHeader = (ICMPHeader*) (((uint8*)pIPHeader) + ui16IPHeaderLen);
                pICMPHeader->ntoh();
                break;
            }
            case IP_PROTO_TCP:
            {
                TCPHeader *pTCPHeader = (TCPHeader*) (((uint8*)pIPHeader) + ui16IPHeaderLen);
                pTCPHeader->ntoh();
                break;
            }
            case IP_PROTO_UDP:
            {
                UDPHeader *pUDPHeader = (UDPHeader*) (((uint8*)pIPHeader) + ui16IPHeaderLen);
                pUDPHeader->ntoh();
                break;
            }
        }

        return 0;
    }

    int PacketRouter::sendARPRequest (NetworkInterface * const pNI, uint32 ui32OriginatingIPAddr, uint32 ui32TargetIPAddr)
    {
        int rc;
        if (!ACMNetProxy::NetProxyApplicationParameters::GATEWAY_MODE) {
            // ARP Request is only supported in Gateway mode
            checkAndLogMsg ("PacketRouter::sendARPRequest", Logger::L_MildError,
                            "sendARPRequest() is only supported in Gateway mode\n");
            return -1;
        }

        uint8 *pui8Buf = (uint8 *) _pPBM->getAndLockWriteBuf();
        EtherFrameHeader *pEthHeader = (EtherFrameHeader*) pui8Buf;
        ARPPacket *pARPPacket = (ARPPacket*) (pui8Buf + sizeof (EtherFrameHeader));
        pEthHeader->src = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
        pEthHeader->dest = NetProxyApplicationParameters::BROADCAST_MAC_ADDR;
        pEthHeader->ui16EtherType = ET_ARP;
        pARPPacket->ui16HType = 1;
        pARPPacket->ui16PType = ET_IP;
        pARPPacket->ui8HLen = 6;
        pARPPacket->ui8PLen = 4;
        pARPPacket->sha = NetProxyApplicationParameters::NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;
        pARPPacket->spa.ui32Addr = EndianHelper::ntohl (ui32OriginatingIPAddr);
        pARPPacket->tha.ui8Byte1 = 0;
        pARPPacket->tha.ui8Byte2 = 0;
        pARPPacket->tha.ui8Byte3 = 0;
        pARPPacket->tha.ui8Byte4 = 0;
        pARPPacket->tha.ui8Byte5 = 0;
        pARPPacket->tha.ui8Byte6 = 0;
        pARPPacket->tpa.ui32Addr = EndianHelper::ntohl (ui32TargetIPAddr);
        pARPPacket->ui16Oper = 1;
        pARPPacket->hton();
        pEthHeader->hton();

        if (0 != (rc = sendPacketToHost (pNI, pui8Buf, sizeof (EtherFrameHeader) + sizeof (ARPPacket)))) {
            checkAndLogMsg ("PacketRouter::sendARPRequest", Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            if (0 != _pPBM->findAndUnlockWriteBuf (pui8Buf)) {
                    checkAndLogMsg ("PacketRouter::sendARPRequest", Logger::L_SevereError,
                                    "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -2;
        }

        if (0 != _pPBM->findAndUnlockWriteBuf (pui8Buf)) {
                checkAndLogMsg ("PacketRouter::sendARPRequest", Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
                return -3;
        }

        return 0;
    }

    int PacketRouter::sendARPReplyToHost (NetworkInterface * const pNI, const ARPPacket * const pARPReqPacket, const EtherMACAddr &rTargetMACAddr)
    {
        int rc;
        if (!pARPReqPacket) {
            return -1;
        }
        if (pARPReqPacket->ui16Oper != 1) {
            checkAndLogMsg ("PacketRouter::sendARPReplyToHost", Logger::L_Warning,
                            "sendARPReplyToHost() called passing an ARP packet that is not a "
                            "Request (ARP Operation code is %hu)\n", pARPReqPacket->ui16Oper);
            return -2;
        }

        uint8 *pui8Buf = (uint8 *) _pPBM->getAndLockWriteBuf();
        EtherFrameHeader *pEthHeader = (EtherFrameHeader*) pui8Buf;
        ARPPacket *pARPPacket = (ARPPacket*) (pui8Buf + sizeof (EtherFrameHeader));
        pEthHeader->dest = pARPReqPacket->sha;
        pEthHeader->src = rTargetMACAddr;
        pEthHeader->ui16EtherType = ET_ARP;
        pARPPacket->ui16HType = pARPReqPacket->ui16HType;
        pARPPacket->ui16PType = pARPReqPacket->ui16PType;
        pARPPacket->ui8HLen = pARPReqPacket->ui8HLen;
        pARPPacket->ui8PLen = pARPReqPacket->ui8PLen;
        pARPPacket->sha = rTargetMACAddr;
        pARPPacket->spa = pARPReqPacket->tpa;
        pARPPacket->tha = pARPReqPacket->sha;
        pARPPacket->tpa = pARPReqPacket->spa;
        pARPPacket->ui16Oper = 2;
        pARPPacket->hton();
        pEthHeader->hton();

        if (0 != (rc = sendPacketToHost (pNI, pui8Buf, sizeof (EtherFrameHeader) + sizeof (ARPPacket)))) {
            checkAndLogMsg ("PacketRouter::sendARPReplyToHost", Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            if (0 != _pPBM->findAndUnlockWriteBuf (pui8Buf)) {
                    checkAndLogMsg ("PacketRouter::sendARPReplyToHost", Logger::L_SevereError,
                                    "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -3;
        }

        if (0 != _pPBM->findAndUnlockWriteBuf (pui8Buf)) {
                checkAndLogMsg ("PacketRouter::sendARPReplyToHost", Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
                return -4;
        }

        return 0;
    }

    int PacketRouter::sendARPAnnouncement (NetworkInterface * const pNI, const ARPPacket * const pARPReqPacket, uint32 ui32IPAddr, const NOMADSUtil::EtherMACAddr &rMACAddr)
    {
        if (!pARPReqPacket) {
            return -1;
        }

        int rc;
        uint8 *pui8Buf = (uint8 *) _pPBM->getAndLockWriteBuf();
        EtherFrameHeader *pEthHeader = (EtherFrameHeader*) pui8Buf;
        ARPPacket *pARPPacket = (ARPPacket*) (pui8Buf + sizeof (EtherFrameHeader));
        pEthHeader->dest = NetProxyApplicationParameters::BROADCAST_MAC_ADDR;
        pEthHeader->src = rMACAddr;
        pEthHeader->ui16EtherType = ET_ARP;
        pARPPacket->ui16HType = pARPReqPacket->ui16HType;
        pARPPacket->ui16PType = pARPReqPacket->ui16PType;
        pARPPacket->ui8HLen = pARPReqPacket->ui8HLen;
        pARPPacket->ui8PLen = pARPReqPacket->ui8PLen;
        pARPPacket->ui16Oper = 2;
        pARPPacket->sha = rMACAddr;
        pARPPacket->spa.ui32Addr = EndianHelper::ntohl (ui32IPAddr);
        pARPPacket->tha = rMACAddr;
        pARPPacket->tpa = pARPPacket->spa;
        pARPPacket->hton();
        pEthHeader->hton();

        if (0 != (rc = sendPacketToHost (pNI, pui8Buf, sizeof (EtherFrameHeader) + sizeof (ARPPacket)))) {
            checkAndLogMsg ("PacketRouter::sendARPAnnouncement", Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            if (0 != _pPBM->findAndUnlockWriteBuf (pui8Buf)) {
                    checkAndLogMsg ("PacketRouter::sendARPAnnouncement", Logger::L_SevereError,
                                    "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -2;
        }

        if (0 != _pPBM->findAndUnlockWriteBuf (pui8Buf)) {
                checkAndLogMsg ("PacketRouter::sendARPAnnouncement", Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
                return -3;
        }

        return 0;
    }

    int PacketRouter::buildAndSendICMPMessageToHost (NetworkInterface * const pNI, ICMPHeader::Type ICMPType, ICMPHeader::Code_Destination_Unreachable ICMPCode,
                                                     uint32 ui32SourceIP, uint32 ui32DestinationIP, IPHeader * const pRcvdIPPacket)
    {
        int rc;
        uint8 *pui8Packet = (uint8 *) _pPBM->getAndLockWriteBuf();
        IPHeader *pIPHeader = (IPHeader*) (pui8Packet + sizeof (EtherFrameHeader));
        size_t uiICMPDataLen = ((pRcvdIPPacket->ui8VerAndHdrLen & 0x0F) * 4) + 8;
        uint16 ui16IPPacketLen = sizeof (IPHeader) + sizeof (ICMPHeader) + uiICMPDataLen;
        uint32 ui32IPSourceAddr = pRcvdIPPacket->srcAddr.ui32Addr;
        uint32 ui32IPDestinationAddr = pRcvdIPPacket->destAddr.ui32Addr;
        if (ui16IPPacketLen > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", Logger::L_MildError,
                            "ICMP packet length with %hu bytes of data exceeds maximum packet size (%hu)\n",
                            ui16IPPacketLen, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
            if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -1;
        }

        pIPHeader->ui8VerAndHdrLen = 0x40 | (sizeof (IPHeader) / 4);
        pIPHeader->ui8TOS = 0;
        pIPHeader->ui16TLen = ui16IPPacketLen;
        pIPHeader->ui16Ident = PacketRouter::getMutexCounter()->tick();
        pIPHeader->ui16FlagsAndFragOff = 0;
        pIPHeader->ui8TTL = 128;
        pIPHeader->ui8Proto = IP_PROTO_ICMP;
        pIPHeader->srcAddr.ui32Addr = EndianHelper::ntohl (ui32SourceIP);
        pIPHeader->destAddr.ui32Addr = EndianHelper::ntohl (ui32DestinationIP);
        pIPHeader->computeChecksum();

        ICMPHeader *pICMPHeader = (ICMPHeader*) ((uint8*) pIPHeader + sizeof (IPHeader));
        uint8 *pICMPData = ((uint8*) pICMPHeader + sizeof (ICMPHeader));
        pICMPHeader->ui8Type = (uint8) ICMPType;
        pICMPHeader->ui8Code = ICMPCode;
        pRcvdIPPacket->hton();
        pRcvdIPPacket->srcAddr.ui32Addr = ui32IPSourceAddr;
        pRcvdIPPacket->destAddr.ui32Addr = ui32IPDestinationAddr;

        switch (ICMPType) {
            case ICMPHeader::T_Destination_Unreachable:
            {
                switch (ICMPCode) {
                    case ICMPHeader::CDU_Host_Unreachable:
                    {
                        // Rest of Header is unused
                        pICMPHeader->ui32RoH = 0;
                        memcpy ((void*) pICMPData, (void*) pRcvdIPPacket, uiICMPDataLen);
                        pICMPHeader->computeChecksum (sizeof (ICMPHeader) + sizeof (IPHeader) + 8);
                        break;
                    }
                    case ICMPHeader::CDU_Fragmentation_needed_and_DF_set:
                    {
                        // First word of Rest of Header is unused, the second word is the Next-hop MTU
                        pICMPHeader->ui16RoHWord1 = 0;
                        pICMPHeader->ui16RoHWord2 = NetProxyApplicationParameters::ETHERNET_DEFAULT_MTU;        // Next-hop MTU
                        memcpy ((void*) pICMPData, (void*) pRcvdIPPacket, uiICMPDataLen);
                        pICMPHeader->computeChecksum (sizeof (ICMPHeader) + sizeof (IPHeader) + 8);
                        break;
                    }
                    default:
                    {
                        checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", Logger::L_Warning,
                                        "could not find an entry for ICMP Message Type Destination Unreachable with Code %d\n", ICMPCode);
                        if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
                            checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", Logger::L_SevereError,
                                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
                        }
                        return -3;
                    }
                }
                break;
            }
            default:
            {
                checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", Logger::L_Warning,
                                "could not find an entry for ICMP Message Type %d\n", ICMPType);
                if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
                    checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", Logger::L_SevereError,
                                    "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
                }
                return -2;
            }
        }

        if (0 != (rc = wrapEtherAndSendPacketToHost (pNI, pui8Packet, ui16IPPacketLen))) {
            checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", Logger::L_MildError,
                            "wrapEtherAndSendPacketToHost() failed with rc = %d - could not send an ICMP message to host\n", rc);
            if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -4;
        }
        if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
            checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -5;
        }

        return 0;
    }

    int PacketRouter::forwardICMPMessageToHost (uint32 ui32LocalTargetIP, uint32 ui32RemoteOriginationIP, uint32 ui32RemoteProxyIP, ICMPHeader::Type ICMPType,
                                                ICMPHeader::Code_Destination_Unreachable ICMPCode, uint32 ui32RoH, const uint8 * const pICMPData, uint16 ui16PayloadLen)
    {
        int rc;
        uint8 *pui8Packet = (uint8 *) _pPBM->getAndLockWriteBuf();
        IPHeader *pIPHeader = (IPHeader*) (pui8Packet + sizeof (EtherFrameHeader));
        uint16 ui16IPPacketLen = sizeof (IPHeader) + sizeof (ICMPHeader) + ui16PayloadLen;
        if (ui16IPPacketLen > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", Logger::L_MildError,
                            "ICMP packet length with %hu bytes of data exceeds maximum packet size (%hu)\n",
                            ui16IPPacketLen, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
            if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -1;
        }

        pIPHeader->ui8VerAndHdrLen = 0x40 | (sizeof (IPHeader) / 4);
        pIPHeader->ui8TOS = 0;
        pIPHeader->ui16TLen = ui16IPPacketLen;
        pIPHeader->ui16Ident = PacketRouter::getMutexCounter()->tick();
        pIPHeader->ui16FlagsAndFragOff = 0;
        pIPHeader->ui8TTL = 128;
        pIPHeader->ui8Proto = IP_PROTO_ICMP;
        pIPHeader->srcAddr.ui32Addr = EndianHelper::ntohl (ui32RemoteOriginationIP);
        pIPHeader->destAddr.ui32Addr = EndianHelper::ntohl (ui32LocalTargetIP);
        pIPHeader->computeChecksum();

        ICMPHeader *pICMPHeader = (ICMPHeader*) ((uint8*) pIPHeader + sizeof (IPHeader));
        pICMPHeader->ui8Type = (uint8) ICMPType;
        pICMPHeader->ui8Code = ICMPCode;
        pICMPHeader->ui32RoH = ui32RoH;
        memcpy ((void *) ((uint8*) pICMPHeader + sizeof (ICMPHeader)), (void *) pICMPData, ui16PayloadLen);
        pICMPHeader->computeChecksum (sizeof (ICMPHeader) + ui16PayloadLen);

        if (0 != (rc = wrapEtherAndSendPacketToHost (_pInternalInterface, pui8Packet, ui16IPPacketLen))) {
            checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", Logger::L_MildError,
                            "wrapEtherAndSendPacketToHost() failed trying to send an ICMP message to host with "
                            "IP address %s; rc = %d\n", InetAddr (ui32LocalTargetIP).getIPAsString(), rc);
            if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -2;
        }
        else {
            checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", Logger::L_HighDetailDebug,
                            "successfully sent to the host with IP address %s an ICMP message of type %d, code %d and %hu bytes of "
                            "data coming from address %s; RoH is %hhu|%hhu|%hhu|%hhu\n", InetAddr(ui32LocalTargetIP).getIPAsString(),
                            ICMPType, ICMPCode, ui16PayloadLen, InetAddr(ui32RemoteOriginationIP).getIPAsString(),
                            ((uint8*) &pICMPHeader->ui32RoH)[3], ((uint8*) &pICMPHeader->ui32RoH)[2],
                            ((uint8*) &pICMPHeader->ui32RoH)[1], ((uint8*) &pICMPHeader->ui32RoH)[0]);
        }
        if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
            checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -3;
        }

        return 0;
    }

    int PacketRouter::initializeRemoteConnection (uint32 ui32RemoteProxyID, ConnectorType connectorType)
    {
        if (ui32RemoteProxyID == 0) {
            checkAndLogMsg ("PacketRouter::initializeRemoteConnection", Logger::L_MildError,
                            "impossible to initialize connection to remote proxy: remote NetProxy UniqueID not valid\n");
            return -1;
        }
        if (connectorType == CT_UNDEF) {
            checkAndLogMsg ("PacketRouter::initializeRemoteConnection", Logger::L_MildError,
                            "impossible to initialize connection to remote proxy: ConnectorType unspecified\n");
            return -2;
        }

        int rc;
        AutoConnectionEntry * const pAutoConnectionEntry = _pConnectionManager->getAutoConnectionEntryToRemoteProxyID (ui32RemoteProxyID);
        ConnectorType currentConnectorType = pAutoConnectionEntry ? pAutoConnectionEntry->getConnectorType() : connectorType;
        QueryResult query (_pConnectionManager->queryConnectionToRemoteProxyIDForConnectorType (currentConnectorType, ui32RemoteProxyID));
        const InetAddr *pRemoteProxyAddr = pAutoConnectionEntry ? pAutoConnectionEntry->getRemoteProxyInetAddress() : query.getBestConnectionSolution();
        if (!pRemoteProxyAddr) {
            checkAndLogMsg ("PacketRouter::initializeRemoteConnection", Logger::L_MildError,
                            "impossible to find the necessary information to connect to the remote NetProxy with ID %u\n",
                            ui32RemoteProxyID);
            return -3;
        }
        if (!pAutoConnectionEntry) {
            checkAndLogMsg ("PacketRouter::initializeRemoteConnection", Logger::L_LowDetailDebug,
                            "no protocol specification for AutoConnections to remote NetProxy with address %s:%hu was found; "
                            "using the same protocol as the remote proxy (%s)\n", pRemoteProxyAddr->getIPAsString(),
                            pRemoteProxyAddr->getPort(), connectorTypeToString (connectorType));
        }

        Connector *pConnector = _pConnectionManager->getConnectorForType (currentConnectorType);
        if (!pConnector) {
            checkAndLogMsg ("PacketRouter::initializeRemoteConnection", Logger::L_MildError,
                            "impossible to retrieve the connector for protocol %s\n",
                            connectorTypeToString (currentConnectorType));
            return -4;
        }
        Connection *pConnection = query.getActiveConnectionToRemoteProxy();
        if (!pConnection) {
            pConnection = pConnector->openNewConnectionToRemoteProxy (query, false);
            if (!pConnection) {
                pConnection = pConnector->getAvailableConnectionToRemoteProxy (pRemoteProxyAddr);
                if (!pConnection) {
                    checkAndLogMsg ("PacketRouter::initializeRemoteConnection", Logger::L_MildError,
                                    "impossible to retrieve the Connection to remote NetProxy at address %s:%hu\n",
                                    pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort());
                    return -5;
                }
            }
        }

        bool bReachable = _pConnectionManager->getReachabilityFromRemoteProxyWithID (ui32RemoteProxyID);
        String sMocketConfigFile = _pConnectionManager->getMocketsConfigFileForProxyWithID (ui32RemoteProxyID);
        if ((rc = pConnection->confirmOpenedConnectionWithRemoteProxy (pRemoteProxyAddr, bReachable)) != 0) {
            checkAndLogMsg ("PacketRouter::initializeRemoteConnection", Logger::L_MildError,
                            "confirmOpenedConnectionWithRemoteProxy() failed with rc = %d\n", rc);
            return -6;
        }
        if (pAutoConnectionEntry) {
            pAutoConnectionEntry->synchronized();
        }
        checkAndLogMsg ("PacketRouter::initializeRemoteConnection", Logger::L_MediumDetailDebug,
                        "successfully initialized connection with the remote NetProxy at address %s:%hu and UniqueID %u\n",
                        pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort(), ui32RemoteProxyID);

        return 0;
    }

    int PacketRouter::sendUDPUniCastPacketToHost (uint32 ui32RemoteOriginationIP, uint32 ui32LocalTargetIP, const UDPHeader * const pUDPPacket)
    {
        static const uint16 ui16MaxUDPPacketLen = NetProxyApplicationParameters::TAP_INTERFACE_MTU - sizeof (IPHeader);

        int rc;
        uint8 *pui8Packet = (uint8 *) _pPBM->getAndLockWriteBuf();
        IPHeader *pIPHeader = (IPHeader*) (pui8Packet + sizeof (EtherFrameHeader));
        UDPHeader *pUDPHeader = (UDPHeader*) (((uint8*)pIPHeader) + sizeof (IPHeader));
        uint16 ui16IPPacketLen = sizeof (IPHeader) + pUDPPacket->ui16Len;
        uint16 ui16EthPacketLen = sizeof (EtherFrameHeader) + ui16IPPacketLen;

        pIPHeader->ui8VerAndHdrLen = 0x40 | (sizeof (IPHeader) / 4);
        pIPHeader->ui8TOS = 0;
        pIPHeader->ui16Ident = PacketRouter::getMutexCounter()->tick();
        pIPHeader->ui8TTL = 128;
        pIPHeader->ui8Proto = IP_PROTO_UDP;
        pIPHeader->srcAddr.ui32Addr = EndianHelper::ntohl (ui32RemoteOriginationIP);
        pIPHeader->destAddr.ui32Addr = EndianHelper::ntohl (ui32LocalTargetIP);

        // It might be necessary to split UDP Packet at the level of the IP protocol
        uint16 ui16WrittenBytes = 0;
        while (ui16WrittenBytes < pUDPPacket->ui16Len) {
            if ((pUDPPacket->ui16Len - ui16WrittenBytes) > ui16MaxUDPPacketLen) {
                pIPHeader->ui16TLen = ((ui16MaxUDPPacketLen / 8) * 8) + sizeof (IPHeader);
                pIPHeader->ui16FlagsAndFragOff = IP_MF_FLAG_FILTER | (((ui16WrittenBytes / 8) & IP_OFFSET_FILTER));
            }
            else {
                pIPHeader->ui16TLen = (pUDPPacket->ui16Len - ui16WrittenBytes) + sizeof (IPHeader);
                pIPHeader->ui16FlagsAndFragOff = ((ui16WrittenBytes / 8) & IP_OFFSET_FILTER);
            }
            pIPHeader->computeChecksum();
            memcpy (pUDPHeader, ((uint8*) pUDPPacket) + ui16WrittenBytes, pIPHeader->ui16TLen - sizeof (IPHeader));
            if (0 != (rc = wrapEtherAndSendPacketToHost (_pInternalInterface, pui8Packet, pIPHeader->ui16TLen))) {
                checkAndLogMsg ("PacketRouter::sendUDPUniCastPacketToHost", Logger::L_MildError,
                                "wrapEtherAndSendPacketToHost() failed with rc = %d - could not send a UDP message to host\n", rc);
                if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
                    checkAndLogMsg ("PacketRouter::sendUDPUniCastPacketToHost", Logger::L_SevereError,
                                    "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
                }
                return -1;
            }
            ui16WrittenBytes += pIPHeader->ui16TLen - sizeof (IPHeader);
        }
        checkAndLogMsg ("PacketRouter::sendUDPUniCastPacketToHost", Logger::L_MediumDetailDebug,
                        "successfully forwarded UDP Packet of %hu bytes from %s:%hu to %s:%hu\n",
                        pUDPPacket->ui16Len, InetAddr(ui32RemoteOriginationIP).getIPAsString(), pUDPPacket->ui16SPort,
                        InetAddr(ui32LocalTargetIP).getIPAsString(), pUDPPacket->ui16DPort);
        if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
            checkAndLogMsg ("PacketRouter::sendUDPUniCastPacketToHost", Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -2;
        }

        return 0;
    }

    int PacketRouter::sendUDPBCastMCastPacketToHost (const uint8 * const pPacket, uint16 ui16PacketLen)
    {
        int rc;
        uint8* pui8Packet = (uint8*) _pPBM->getAndLockWriteBuf();
        memcpy (pui8Packet, pPacket, ui16PacketLen);
        EtherFrameHeader *pEthHeader = (EtherFrameHeader*) pui8Packet;
        IPHeader *pIPHeader = (IPHeader*) (pui8Packet + sizeof (EtherFrameHeader));
        pIPHeader->srcAddr.ntoh();
        pIPHeader->destAddr.ntoh();
        uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
        UDPHeader *pUDPHeader = (UDPHeader*) (((uint8*) pIPHeader) + ui16IPHeaderLen);
        checkAndLogMsg ("PacketRouter::sendUDPBCastMCastPacketToHost", Logger::L_MediumDetailDebug,
                        "Sending UDP Packet: size %d from %d.%d.%d.%d:%d to %d.%d.%d.%d:%d\n",
                        (int) pUDPHeader->ui16Len - sizeof (UDPHeader),
                        (int) pIPHeader->srcAddr.ui8Byte1, (int) pIPHeader->srcAddr.ui8Byte2, (int) pIPHeader->srcAddr.ui8Byte3, (int) pIPHeader->srcAddr.ui8Byte4,
                        (int) pUDPHeader->ui16SPort,
                        (int) pIPHeader->destAddr.ui8Byte1, (int) pIPHeader->destAddr.ui8Byte2, (int) pIPHeader->destAddr.ui8Byte3, (int) pIPHeader->destAddr.ui8Byte4,
                        (int) pUDPHeader->ui16DPort);
        pUDPHeader->hton();
        pIPHeader->hton();
        pEthHeader->hton();

        if (0 != (rc = sendPacketToHost (_pInternalInterface, const_cast<const uint8 * const> (pui8Packet), ui16PacketLen))) {
            checkAndLogMsg ("PacketRouter::sendUDPBCastMCastPacketToHost", Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("PacketRouter::sendUDPBCastMCastPacketToHost", Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -1;
        }
        if (0 != _pPBM->findAndUnlockWriteBuf (pui8Packet)) {
            checkAndLogMsg ("PacketRouter::sendUDPBCastMCastPacketToHost", Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -2;
        }

        return 0;
    }

    int PacketRouter::sendRemoteResetRequestIfNeeded (Entry * const pEntry)
    {
        int rc;
        Connection * const pConnection = pEntry->getConnection();
        if (!pConnection || !pConnection->isConnected()) {
            checkAndLogMsg ("PacketRouter::sendRemoteResetRequestIfNeeded", Logger::L_MildError,
                            "L%hu-R%hu: could not send a RemoteTCPResetRequest to remote proxy with address <%s:%hu>; "
                            "impossible to retrieve Connection (Connection deleted or disconnected, or Entry already reset)\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString(),
                            pEntry->remoteProxyAddr.getPort());
            return -1;
        }

        // Look if we need to reset remote connection
        if (((pEntry->remoteStatus == TCTRS_ConnEstablished) || (pEntry->remoteStatus == TCTRS_ConnRequested) ||
            (pEntry->remoteStatus == TCTRS_DisconnRequestSent) || (pEntry->remoteStatus == TCTRS_DisconnRequestReceived)) &&
            (pEntry->ui16RemoteID != 0)) {
            if (0 != (rc = pConnection->sendResetTCPConnectionRequest (pEntry))) {
                checkAndLogMsg ("PacketRouter::sendRemoteResetRequestIfNeeded", Logger::L_MildError,
                                "L%hu-R%hu: could not send a remoteResetRequest to remote proxy with address %s; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString(), rc);
                return -2;
            }
        }

        pEntry->remoteStatus = TCTRS_Disconnected;
        return 0;
    }

    int PacketRouter::flushAndSendCloseConnectionRequest (Entry * const pEntry)
    {
        // Flush any data left in compressor buffer
        int rc;
        unsigned int uiBytesToSend = 0, uiSentBytes = 0;
        unsigned char *pDest[1];
        *pDest = NULL;
        if (0 != (rc = pEntry->getConnectorWriter()->flush (pDest, uiBytesToSend))) {
            checkAndLogMsg ("PacketRouter::flushAndSendCloseConnectionRequest", Logger::L_MildError,
                            "L%hu-R%hu: flushWriter() failed with rc = %d; sending RST packet to local host and clearing connection\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            return -1;
        }

        if (pEntry->getConnection() == NULL) {
            checkAndLogMsg ("PacketRouter::flushAndSendCloseConnectionRequest", Logger::L_MildError,
                            "L%hu-R%hu: could not retrieve the Connection to remote proxy with address %s for protocol %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString(), pEntry->getProtocol());
            return -2;
        }

        while (uiBytesToSend > uiSentBytes) {
            uint32 bytesToWriteToPacket = std::min ((uint32) (uiBytesToSend - uiSentBytes), (uint32) NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
            uint8 ui8Flags = (uiBytesToSend <= uiSentBytes) ? (TCPHeader::TCPF_ACK | TCPHeader::TCPF_PSH) : TCPHeader::TCPF_ACK;
            if (0 != (rc = pEntry->getConnection()->sendTCPDataToRemoteHost (pEntry, *pDest + uiSentBytes, bytesToWriteToPacket, ui8Flags))) {
                checkAndLogMsg ("PacketRouter::flushAndSendCloseConnectionRequest", Logger::L_MildError,
                                "L%hu-R%hu: sendTCPDataToRemoteHost() failed with rc = %d; couldn't send final (flushed) bytes;\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                break;
            }
            uiSentBytes += bytesToWriteToPacket;
        }
        if (uiBytesToSend > uiSentBytes) {
            // Unable to flush all data --> connection already resetted
            checkAndLogMsg ("PacketRouter::flushAndSendCloseConnectionRequest", Logger::L_Warning,
                            "L%hu-R%hu: unable to flush all data to remote host; flushed only %u/%u; connection has been resetted\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, uiSentBytes, uiBytesToSend);
            return -3;
        }
        else {
            checkAndLogMsg ("PacketRouter::flushAndSendCloseConnectionRequest", Logger::L_MediumDetailDebug,
                            "L%hu-R%hu: flushed all data to remote host; amount of flushed bytes is %u\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, uiSentBytes);
        }

        // The outgoing queue is empty, so send close request to the remote side
        if (0 != (rc = pEntry->getConnection()->sendCloseTCPConnectionRequest (pEntry))) {
            checkAndLogMsg ("PacketRouter::flushAndSendCloseConnectionRequest", Logger::L_MildError,
                            "L%hu-R%hu: could not send close connection request to remote proxy %s; rc = %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString(), rc);
            return -4;
        }

        return 0;
    }

    int PacketRouter::sendBroadcastPacket (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32BroadcastSrcIP, uint32 ui32BroadcastDestIP,
                                            uint16 ui16DestPort, const CompressionSetting * const pCompressionSetting)
    {
        int rc;
        const InetAddr broadcastSourceIP (ui32BroadcastSrcIP), broadcastDestinationIP (ui32BroadcastDestIP);
        const String mocketsConfFile = _pConnectionManager->getMocketsConfigFileForConnectionsToRemoteHost (ui32BroadcastDestIP, ui16DestPort);
        const ProtocolSetting *const pProtocolSetting = _pConfigurationManager->mapAddrToProtocol (ui32BroadcastSrcIP, ui32BroadcastDestIP, IP_PROTO_UDP);
        const ProxyMessage::Protocol currentProtocol = pProtocolSetting ? pProtocolSetting->getProxyMessageProtocol() : ProtocolSetting::DEFAULT_UDP_MAPPING_PROTOCOL;
        const ConnectorType connectorType = ProtocolSetting::protocolToConnectorType (currentProtocol);
        Connector * const pConnector = _pConnectionManager->getConnectorForType (connectorType);
        if (!pConnector) {
            checkAndLogMsg ("PacketRouter::sendBroadcastPacket", Logger::L_Warning,
                            "could not retrieve the connector for protocol %s\n",
                            ProtocolSetting::getProxyMessageProtocolAsString (currentProtocol));
            return -1;
        }
        if (!pConnector->isEnqueueingAllowed()) {
            checkAndLogMsg ("PacketRouter::sendBroadcastPacket", Logger::L_Warning,
                            "could not enqueue packet in the %s connector; dropping forwarding of UDP Broadcast packet\n",
                            pConnector->getConnectorTypeAsString());
            return -2;
        }

        const NPDArray2<QueryResult> daQueryList (_pConnectionManager->queryAllConnectionsToRemoteHostForConnectorType (connectorType, ui32BroadcastDestIP, ui16DestPort));
        for (int i = 0; i <= daQueryList.getHighestIndex(); ++i) {
            const QueryResult &query = daQueryList.get (i);
            if (!query.isValid()) {
                continue;
            }

            const InetAddr * const pRemoteProxyAddr = query.getBestConnectionSolution();
            if (!pRemoteProxyAddr) {
                continue;
            }

            Connection *pConnection = query.getActiveConnectionToRemoteProxy();
            if (!pConnection) {
                pConnection = pConnector->openNewConnectionToRemoteProxy (query, false);
                if (!pConnection) {
                    pConnection = pConnector->getAvailableConnectionToRemoteProxy (query.getRemoteProxyServerAddress());
                    if (!pConnection) {
                        checkAndLogMsg ("PacketRouter::sendBroadcastPacket", Logger::L_MildError,
                                        "impossible to retrieve the Connection to remote NetProxy at address %s:%hu\n",
                                        pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort());
                        return -3;
                    }
                }
            }

            if (0 != (rc = pConnection->sendUDPBCastMCastPacketToRemoteHost (pRemoteProxyAddr, ui32BroadcastDestIP, pPacket, ui16PacketLen,
                                                                             pCompressionSetting, currentProtocol))) {
                checkAndLogMsg ("PacketRouter::sendBroadcastPacket", Logger::L_MildError,
                                "sendUDPBCastMCastPacketToRemoteHost() failed when trying to send a message to the "
                                "remote proxy with address %s; rc = %d\n", pRemoteProxyAddr->getIPAsString(), rc);

                return -4;
            }
            checkAndLogMsg ("PacketRouter::sendBroadcastPacket", Logger::L_HighDetailDebug,
                            "UDP Broadcast packet of size %hu addressed to address %s successfully forwarded to remote proxy at address %s\n",
                            ui16PacketLen, broadcastDestinationIP.getIPAsString(), pRemoteProxyAddr->getIPAsString());
        }

        return 0;
    }

    int PacketRouter::sendMulticastPacket (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32MulticastSrcIP, uint32 ui32MulticastDestIP,
                                            uint16 ui16DestPort, const CompressionSetting * const pCompressionSetting)
    {
        int rc;
        const InetAddr broadcastSourceIP (ui32MulticastSrcIP), broadcastDestinationIP (ui32MulticastDestIP);
        const String mocketsConfFile = _pConnectionManager->getMocketsConfigFileForConnectionsToRemoteHost (ui32MulticastDestIP, ui16DestPort);
        const ProtocolSetting *pProtocolSetting = _pConfigurationManager->mapAddrToProtocol (ui32MulticastSrcIP, ui32MulticastDestIP, IP_PROTO_UDP);
        const ProxyMessage::Protocol currentProtocol = pProtocolSetting ? pProtocolSetting->getProxyMessageProtocol() : ProtocolSetting::DEFAULT_UDP_MAPPING_PROTOCOL;
        const ConnectorType connectorType = ProtocolSetting::protocolToConnectorType (currentProtocol);
        Connector * const pConnector = _pConnectionManager->getConnectorForType (connectorType);
        if (!pConnector) {
            checkAndLogMsg ("PacketRouter::sendMulticastPacket", Logger::L_Warning,
                            "could not retrieve the connector for protocol %s\n",
                            ProtocolSetting::getProxyMessageProtocolAsString (currentProtocol));
            return -1;
        }
        if (!pConnector->isEnqueueingAllowed()) {
            checkAndLogMsg ("PacketRouter::sendMulticastPacket", Logger::L_Warning,
                            "could not enqueue packet in the %s connector; dropping forwarding of UDP Broadcast packet\n",
                            pConnector->getConnectorTypeAsString());
            return -2;
        }

        const NPDArray2<QueryResult> daQueryList (_pConnectionManager->queryAllConnectionsToRemoteHostForConnectorType (connectorType, ui32MulticastDestIP, ui16DestPort));
        for (int i = 0; i <= daQueryList.getHighestIndex(); ++i) {
            const QueryResult &query = daQueryList.get (i);
            if (!query.isValid()) {
                continue;
            }

            const InetAddr * const pRemoteProxyAddr = query.getBestConnectionSolution();
            if (!pRemoteProxyAddr) {
                continue;
            }

            Connection *pConnection = query.getActiveConnectionToRemoteProxy();
            if (!pConnection) {
                pConnection = pConnector->openNewConnectionToRemoteProxy (query, false);
                if (!pConnection) {
                    pConnection = pConnector->getAvailableConnectionToRemoteProxy (query.getRemoteProxyServerAddress());
                    if (!pConnection) {
                        checkAndLogMsg ("PacketRouter::sendMulticastPacket", Logger::L_MildError,
                                        "impossible to retrieve the Connection to remote NetProxy at address %s:%hu\n",
                                        pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort());
                        return -3;
                    }
                }
            }

            if (0 != (rc = pConnection->sendUDPBCastMCastPacketToRemoteHost (pRemoteProxyAddr, ui32MulticastDestIP, pPacket, ui16PacketLen,
                                                                             pCompressionSetting, currentProtocol))) {
                checkAndLogMsg ("PacketRouter::sendMulticastPacket", Logger::L_MildError,
                                "sendUDPBCastMCastPacketToRemoteHost() failed when trying to send a message to the "
                                "remote proxy with address %s; rc = %d\n", pRemoteProxyAddr->getIPAsString(), rc);
                return -4;
            }
            checkAndLogMsg ("PacketRouter::sendMulticastPacket", Logger::L_HighDetailDebug,
                            "UDP multicast packet of size %hu coming from host with IP address %s and addressed "
                            "to address %s successfully forwarded to remote proxy at address %s\n",
                            ui16PacketLen, broadcastSourceIP.getIPAsString(), broadcastDestinationIP.getIPAsString(),
                            pRemoteProxyAddr->getIPAsString());
        }

        return 0;
    }

    int PacketRouter::sendBCastMCastPacketToDisService (const uint8 * const pPacket, uint16 ui16PacketLen)
    {
        #if defined (USE_DISSERVICE)
            int rc;
            if (_pDisService == NULL) {
                checkAndLogMsg ("PacketRouter::sendBCastMCastPacketToDisService", Logger::L_MildError,
                                "ignoring BCastMCast packet since DisService has not been initialized\n");
                return -2;
            }
            if (0 != (rc = _pDisService->push (0, "netproxy.unreliable", "", 1, pPacket, ui16PacketLen, 0, 0, 0, 0, NULL, 0))) {
                checkAndLogMsg ("PacketRouter::sendBCastMCastPacketToDisService", Logger::L_MildError,
                                "push() on DisService failed with rc = %d\n", rc);
                return -3;
            }
            else {
                checkAndLogMsg ("PacketRouter::sendBCastMCastPacketToDisService", Logger::L_MediumDetailDebug,
                                "sent a packet of size %d\n", (int) ui16PacketLen);
            }
            return 0;
        #else
            checkAndLogMsg ("PacketRouter::sendBCastMCastPacketToDisService", Logger::L_MildError,
                            "DisService has not been included in build\n");
            return -1;
        #endif
    }

    #if defined (USE_DISSERVICE)
        bool PacketRouter::dataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const void *pData, uint32 ui32Length, uint32 ui32MetadataLength, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority)
        {
            int rc;
            checkAndLogMsg ("PacketRouter::dataArrived", Logger::L_MediumDetailDebug,
                            "dataArrived: length = %d\n", (int) ui32Length);
            if (ui32Length < (sizeof (EtherFrameHeader) + sizeof (IPHeader) + sizeof (UDPHeader))) {
                checkAndLogMsg ("PacketRouter::dataArrived", Logger::L_MildError,
                                "received a message that is smaller than a UDP packet - ignoring; size of message = %u; size must be at least %u\n",
                                ui32Length, sizeof (EtherFrameHeader) + sizeof (IPHeader) + sizeof (UDPHeader));
                return false;
            }
            else if (ui32Length > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
                checkAndLogMsg ("PacketRouter::dataArrived", Logger::L_MildError,
                                "received a message of size %lu that is too large - maximum MTU is %lu\n",
                                ui32Length, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
            }
            uint8 ui8Buf[NetProxyApplicationParameters::PROXY_MESSAGE_MTU];
            EtherFrameHeader *pEthHeader = (EtherFrameHeader*) ui8Buf;
            IPHeader *pIPHeader = (IPHeader*) (ui8Buf + sizeof (EtherFrameHeader));      /*!!*/ // Check if header for TUN/TAP is the size of an Ethernet header
            memcpy (pEthHeader, pData, ui32Length);
            uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
            UDPHeader *pUDPHeader = (UDPHeader*) (((uint8*)pIPHeader) + ui16IPHeaderLen);
            checkAndLogMsg ("PacketRouter::dataArrived", Logger::L_MediumDetailDebug,
                            "sending UDP Packet: size %d from %d.%d.%d.%d:%d to %d.%d.%d.%d:%d\n",
                            (int) pUDPHeader->ui16Len - sizeof (UDPHeader),
                            (int) pIPHeader->srcAddr.ui8Byte1, (int) pIPHeader->srcAddr.ui8Byte2, (int) pIPHeader->srcAddr.ui8Byte3, (int) pIPHeader->srcAddr.ui8Byte4,
                            (int) pUDPHeader->ui16SPort,
                            (int) pIPHeader->destAddr.ui8Byte1, (int) pIPHeader->destAddr.ui8Byte2, (int) pIPHeader->destAddr.ui8Byte3, (int) pIPHeader->destAddr.ui8Byte4,
                            (int) pUDPHeader->ui16DPort);
            pEthHeader->hton();
            pIPHeader->hton();
            pUDPHeader->hton();
            if (0 != (rc = sendPacketToHost (ui8Buf, ui32Length + sizeof (EtherFrameHeader)))) {
                checkAndLogMsg ("PacketRouter::dataArrived", Logger::L_MildError,
                                "sendPacketToHost() failed with rc = %d\n", rc);
                return false;
            }
            return true;
        }

        bool PacketRouter::chunkArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const void *pChunk, uint32 ui32Length, uint8 ui8NChunks, uint8 ui8TotNChunks, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority)
        {
            return true;
        }

        bool PacketRouter::metadataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const void *pMetadata, uint32 ui32MetadataLength, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority)
        {
            return true;
        }

        bool PacketRouter::dataAvailable (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const char * pszId, const void *pMetadata, uint32 ui32MetadataLength, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority)
        {
            return true;
        }

    #endif

    bool PacketRouter::hostBelongsToTheInternalNetwork (const EtherMACAddr & emaHost)
    {
        static const EtherMACAddr * pInternalEthHostAddr = NULL;
        pInternalEthHostAddr = _daInternalHosts.getData();

        for (int i = 0; i <= _daInternalHosts.getHighestIndex(); i++) {
            if (pInternalEthHostAddr[i] == emaHost) {
                return true;
            }
        }

        return false;
    }

    bool PacketRouter::hostBelongsToTheExternalNetwork (const EtherMACAddr & emaHost)
    {
        static const EtherMACAddr * pExternalEthHostAddr = NULL;
        pExternalEthHostAddr = _daExternalHosts.getData();

        for (int i = 0; i <= _daExternalHosts.getHighestIndex(); i++) {
            if (pExternalEthHostAddr[i] == emaHost) {
                return true;
            }
        }

        return false;
    }

}
