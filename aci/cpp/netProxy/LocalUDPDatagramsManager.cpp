/*
 * LocalUDPDatagramsManager.cpp
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
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#if defined (WIN32)
#define NOMINMAX    // For interference with std::min and std::max
#endif

#include <mutex>
#include <condition_variable>
#include <algorithm>

#include "LocalUDPDatagramsManager.h"
#include "Connection.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    void LocalUDPDatagramsManagerThread::run (void)
    {
        started();

        int rc;
        MutexUDPQueue readyUDPDatagramPacketQueue;
        std::unique_lock<std::mutex> ul{_mtx};
        if (NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT == 0) {
            // Nagle's-like algorithm for UDP disabled
            while (!terminationRequested()) {
                UDPDatagramPacket * pUDPDatagramPacket;
                UDPDatagramPacket * pReferenceUDPDatagramPacket;
                const auto i64CurrentCycleTime = NOMADSUtil::getTimeInMilliseconds();

                _muqUDPReassembledDatagramsQueue.lock();
                _muqUDPReassembledDatagramsQueue.resetGet();
                while ((pUDPDatagramPacket = _muqUDPReassembledDatagramsQueue.getNext())) {
                    if (!pUDPDatagramPacket->isDatagramComplete()) {
                        if ((i64CurrentCycleTime - pUDPDatagramPacket->getCreationTime()) >= I64_LUDMT_TIME_BETWEEN_ITERATIONS) {
                            // Timeout expired --> UDP Datagram could not be reassembled --> deleting incomplete fragment
                            delete _muqUDPReassembledDatagramsQueue.remove (pUDPDatagramPacket);
                            NOMADSUtil::InetAddr iaUDPDatagramDestinationAddress{pUDPDatagramPacket->getDestinationIPAddr()};
                            checkAndLogMsg ("LocalUDPDatagramsManagerThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "packet still incomplete after reassembly timeout expired; dropping fragment addressed to <%s:%hu>\n",
                                            iaUDPDatagramDestinationAddress.getIPAsString(), pUDPDatagramPacket->getDestinationPortNum());
                            pUDPDatagramPacket = nullptr;
                        }
                        continue;
                    }

                    if (pUDPDatagramPacket->getPacketLen() >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                        // Send packet by itself (since data was sent using UDP, it's not necessary to maintain the order)
                        if (pUDPDatagramPacket->getConnection()->isEnqueueingAllowed()) {
                            pUDPDatagramPacket->getConnection()->
                                sendUDPUnicastPacketToRemoteHost (pUDPDatagramPacket->getSourceIPAddr(), pUDPDatagramPacket->getDestinationIPAddr(),
                                                                  pUDPDatagramPacket->getIPPacketTTL(), pUDPDatagramPacket->getUDPPacket(),
                                                                  pUDPDatagramPacket->getPacketLen(), pUDPDatagramPacket->getCompressionSetting(),
                                                                  Protocol{static_cast<Protocol> (pUDPDatagramPacket->getPMProtocol())});
                        }
                        else {
                            NOMADSUtil::InetAddr iaUDPDatagramDestinationAddress{pUDPDatagramPacket->getDestinationIPAddr()};
                            checkAndLogMsg ("LocalUDPDatagramsManagerThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                            "connector buffer is full; dropping a UDP Datagram of %u bytes with destination <%s:%hu>\n",
                                            pUDPDatagramPacket->getPacketLen(), iaUDPDatagramDestinationAddress.getIPAsString(),
                                            pUDPDatagramPacket->getDestinationPortNum());
                        }
                        delete _muqUDPReassembledDatagramsQueue.remove (pUDPDatagramPacket);
                        pUDPDatagramPacket = nullptr;
                    }
                    else if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() == 0) {
                        // No packets had previously been enqueued and packet size is less than the configured threshold
                        pReferenceUDPDatagramPacket = pUDPDatagramPacket;
                        readyUDPDatagramPacketQueue.enqueue (_muqUDPReassembledDatagramsQueue.remove (pUDPDatagramPacket));
                    }
                    else {
                        /* Some packets have already been enqueued and packet size is less than
                         * the configured threshold --> check if wrapping together is possible */
                        if (pReferenceUDPDatagramPacket->canBeWrappedTogether (pUDPDatagramPacket)) {
                            // It's possible to wrap packets together
                            readyUDPDatagramPacketQueue.enqueue (_muqUDPReassembledDatagramsQueue.remove (pUDPDatagramPacket));
                        }
                    }

                    if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() >=
                        NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                        // The configured threshold has been reached --> creating and sending a new MultipleUDPDatagrams ProxyMessage
                        if (0 > (rc = sendEnqueuedDatagramsToRemoteProxy (&readyUDPDatagramPacketQueue))) {
                            checkAndLogMsg ("LocalUDPDatagramsManagerThread::run", NOMADSUtil::Logger::L_MildError,
                                            "sendEnqueuedDatagramsToRemoteProxy() failed when trying to transmit one "
                                            "or more aggregated packets whose size reached the configured threshold; "
                                            "Nagle'like algorithm for UDP is disabled; rc = %d\n", rc);
                        }
                        readyUDPDatagramPacketQueue.removeAll (true);
                        pReferenceUDPDatagramPacket = nullptr;
                        _muqUDPReassembledDatagramsQueue.resetGet();     // Need to reconsider any skipped packet
                    }
                }

                if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() > 0) {
                    // Transmitting any enqueued UDP Datagrams
                    if (0 > (rc = sendEnqueuedDatagramsToRemoteProxy (&readyUDPDatagramPacketQueue))) {
                        checkAndLogMsg ("LocalUDPDatagramsManagerThread::run", NOMADSUtil::Logger::L_MildError,
                                        "sendEnqueuedDatagramsToRemoteProxy() failed when trying to transmit one or more "
                                        "aggregated packets; Nagle'like algorithm for UDP is disabled; rc = %d\n", rc);
                    }
                    readyUDPDatagramPacketQueue.removeAll (true);
                    pReferenceUDPDatagramPacket = nullptr;
                }
                _muqUDPReassembledDatagramsQueue.unlock();

                const int64 i64NextCycleTime = NOMADSUtil::getTimeInMilliseconds() + I64_LUDMT_TIME_BETWEEN_ITERATIONS;
                if (!terminationRequested()) {
                    _cv.wait_for (ul, std::chrono::milliseconds{I64_LUDMT_TIME_BETWEEN_ITERATIONS},
                                  [this, i64NextCycleTime] {
                                    return _bNotified || (NOMADSUtil::getTimeInMilliseconds() >= i64NextCycleTime) ||
                                        terminationRequested();
                                  });
                }
                _bNotified = false;
            }
        }
        else {
            // Nagle's-like algorithm for UDP enabled
            while (!terminationRequested()) {
                int64 i64NextCycleTime = 0;
                MutexUDPQueue * pUDPDatagramsQueue;
                UDPDatagramPacket * pReferenceUDPDatagramPacket;
                const auto i64CurrentCycleTime = NOMADSUtil::getTimeInMilliseconds();

                _mtxUDPDatagramsQueueHashTable.lock();
                NOMADSUtil::UInt32Hashtable<MutexUDPQueue>::Iterator udpDatagramsQueueIterator = _ui32UDPDatagramsQueueHashTable.getAllElements();
                _mtxUDPDatagramsQueueHashTable.unlock();
                while ((pUDPDatagramsQueue = udpDatagramsQueueIterator.getValue())) {
                    if (!pUDPDatagramsQueue->isEmpty()) {
                        UDPDatagramPacket * pUDPDatagramPacket;

                        pUDPDatagramsQueue->lock();
                        pUDPDatagramsQueue->resetGet();
                        while ((pUDPDatagramPacket = pUDPDatagramsQueue->getNext())) {
                            if (!pUDPDatagramPacket->isDatagramComplete()) {
                                // Fragment incomplete
                                if ((i64CurrentCycleTime - pUDPDatagramPacket->getCreationTime()) >=
                                    NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT) {
                                    // Timeout expired --> UDP Datagram could not be reassembled --> deleting incomplete fragment
                                    NOMADSUtil::InetAddr iaUDPDatagramDestinationAddress{pUDPDatagramPacket->getDestinationIPAddr()};
                                    checkAndLogMsg ("LocalUDPDatagramsManagerThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                                    "packet still incomplete after reassembly timeout expired; dropping fragment "
                                                    "addressed to <%s:%hu>\n", iaUDPDatagramDestinationAddress.getIPAsString(),
                                                    pUDPDatagramPacket->getDestinationPortNum());
                                    delete pUDPDatagramsQueue->remove (pUDPDatagramPacket);
                                    pUDPDatagramPacket = nullptr;
                                }
                                continue;
                            }
                            if (pUDPDatagramPacket->getPacketLen() >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                                // Send packet by itself; since the used protocol is UDP, it is not necessary to keep the receiving order
                                if (pUDPDatagramPacket->getConnection()->isEnqueueingAllowed()) {
                                    pUDPDatagramPacket->getConnection()->
                                        sendUDPUnicastPacketToRemoteHost (pUDPDatagramPacket->getSourceIPAddr(), pUDPDatagramPacket->getDestinationIPAddr(),
                                                                          pUDPDatagramPacket->getIPPacketTTL(), pUDPDatagramPacket->getUDPPacket(),
                                                                          pUDPDatagramPacket->getPacketLen(), pUDPDatagramPacket->getCompressionSetting(),
                                                                          Protocol{static_cast<Protocol> (pUDPDatagramPacket->getPMProtocol())});
                                }
                                else {
                                    NOMADSUtil::InetAddr iaUDPDatagramDestinationAddress{pUDPDatagramPacket->getDestinationIPAddr()};
                                    checkAndLogMsg ("LocalUDPDatagramsManagerThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                    "connector buffer is full; dropping a UDP Datagram of %u bytes with destination <%s:%hu>\n",
                                                    pUDPDatagramPacket->getPacketLen(), iaUDPDatagramDestinationAddress.getIPAsString(),
                                                    pUDPDatagramPacket->getDestinationPortNum());
                                }
                                delete pUDPDatagramsQueue->remove (pUDPDatagramPacket);
                                pUDPDatagramPacket = nullptr;
                            }
                            else if ((readyUDPDatagramPacketQueue.getEnqueuedBytesCount() == 0) &&
                                ((i64CurrentCycleTime - pUDPDatagramPacket->getCreationTime()) < NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT)) {
                                // Check if the necessary size to send out a packet can be reached by aggregating multiple packets
                                pReferenceUDPDatagramPacket = pUDPDatagramPacket;
                                readyUDPDatagramPacketQueue.enqueue (pUDPDatagramPacket);
                                while ((readyUDPDatagramPacketQueue.getEnqueuedBytesCount() < NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) &&
                                    (pUDPDatagramPacket = pUDPDatagramsQueue->getNext())) {
                                    // Check if packet is complete, or continue to the next one
                                    if (pUDPDatagramPacket->isDatagramComplete()) {
                                        // Check whether packet has to be sent by itself or it can be wrapped together with other packets
                                        if (pUDPDatagramPacket->getPacketLen() >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                                            // Send packet by itself; since the used protocol is UDP, it's not necessary to maintain any order
                                            if (pUDPDatagramPacket->getConnection()->isEnqueueingAllowed()) {
                                                pUDPDatagramPacket->getConnection()->
                                                    sendUDPUnicastPacketToRemoteHost (pUDPDatagramPacket->getSourceIPAddr(), pUDPDatagramPacket->getDestinationIPAddr(),
                                                                                      pUDPDatagramPacket->getIPPacketTTL(), pUDPDatagramPacket->getUDPPacket(),
                                                                                      pUDPDatagramPacket->getPacketLen(), pUDPDatagramPacket->getCompressionSetting(),
                                                                                      Protocol{static_cast<Protocol> (pUDPDatagramPacket->getPMProtocol())});
                                            }
                                            else {
                                                NOMADSUtil::InetAddr iaUDPDatagramDestinationAddress{pUDPDatagramPacket->getDestinationIPAddr()};
                                                checkAndLogMsg ("LocalUDPDatagramsManagerThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                                "connector buffer is full; dropping a UDP Datagram of %u bytes addressed to address <%s:%hu>\n",
                                                                pUDPDatagramPacket->getPacketLen(), iaUDPDatagramDestinationAddress.getIPAsString(),
                                                                pUDPDatagramPacket->getDestinationPortNum());
                                            }
                                            delete pUDPDatagramsQueue->remove (pUDPDatagramPacket);
                                            pUDPDatagramPacket = nullptr;
                                        }
                                        else if (pReferenceUDPDatagramPacket->canBeWrappedTogether (pUDPDatagramPacket)) {
                                            // Enqueue packet for multiple UDP datagrams later sending
                                            readyUDPDatagramPacketQueue.enqueue (pUDPDatagramPacket);
                                        }
                                    }
                                }

                                if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() >=
                                    NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                                    // The configured threshold has been reached --> creating and sending a new MultipleUDPDatagrams ProxyMessage
                                    readyUDPDatagramPacketQueue.resetGet();
                                    while ((pUDPDatagramPacket = readyUDPDatagramPacketQueue.getNext())) {
                                        pUDPDatagramsQueue->remove (pUDPDatagramPacket);
                                    }
                                    if (0 > (rc = sendEnqueuedDatagramsToRemoteProxy (&readyUDPDatagramPacketQueue))) {
                                        checkAndLogMsg ("LocalUDPDatagramsManagerThread::run", NOMADSUtil::Logger::L_MildError,
                                                        "sendEnqueuedDatagramsToRemoteProxy() failed when trying to transmit one or more "
                                                        "packets aggregated to an unexpired packet whose size reached the configured "
                                                        "threshold; Nagle'like algorithm for UDP is enabled; rc = %d\n", rc);
                                    }
                                    pUDPDatagramsQueue->resetGet();                 // Need to reconsider any skipped packet
                                    readyUDPDatagramPacketQueue.removeAll (true);
                                }
                                else {
                                    // Required size NOT reached --> wait until the timeout expires or more packets arrive
                                    readyUDPDatagramPacketQueue.removeAll (false);
                                }
                                pReferenceUDPDatagramPacket = nullptr;
                            }
                            else if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() == 0) {
                                /* Configured timeout is expired, no packets had previously been enqueued, and the
                                 * packet size is less than the configured threshold --> enqueue for transmission */
                                pReferenceUDPDatagramPacket = pUDPDatagramPacket;
                                readyUDPDatagramPacketQueue.enqueue (pUDPDatagramsQueue->remove (pUDPDatagramPacket));
                            }
                            else {
                                /* Some packets have already been enqueued and packet size is less than
                                 * the configured threshold --> check if wrapping together is possible */
                                if (pReferenceUDPDatagramPacket->canBeWrappedTogether (pUDPDatagramPacket)) {
                                    readyUDPDatagramPacketQueue.enqueue (pUDPDatagramsQueue->remove (pUDPDatagramPacket));
                                }
                            }

                            /* If the threshold has been reached, send multiple UDP datagram packets to the
                             * remote NetProxy and reset the pUDPDatagramsQueue to continue processing */
                            if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() >=
                                NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                                if (0 > (rc = sendEnqueuedDatagramsToRemoteProxy (&readyUDPDatagramPacketQueue))) {
                                    checkAndLogMsg ("LocalUDPDatagramsManagerThread::run", NOMADSUtil::Logger::L_MildError,
                                                    "sendEnqueuedDatagramsToRemoteProxy() failed when trying to transmit one "
                                                    "or more aggregated packets whose size reached the configured threshold; "
                                                    "Nagle'like algorithm for UDP is enabled; rc = %d\n", rc);
                                }
                                readyUDPDatagramPacketQueue.removeAll (true);
                                pReferenceUDPDatagramPacket = nullptr;
                                pUDPDatagramsQueue->resetGet();     // Need to reconsider any skipped packet
                            }
                        }

                        if (readyUDPDatagramPacketQueue.getEnqueuedBytesCount() > 0) {
                            // Transmitting any expired UDP Datagrams
                            if (0 > (rc = sendEnqueuedDatagramsToRemoteProxy (&readyUDPDatagramPacketQueue))) {
                                checkAndLogMsg ("LocalUDPDatagramsManagerThread::run", NOMADSUtil::Logger::L_MildError,
                                                "sendEnqueuedDatagramsToRemoteProxy() failed when trying to transmit one or "
                                                "more packets aggregated among the whole queue of received UDP datagrams; "
                                                "Nagle'like algorithm for UDP is enabled; rc = %d\n", rc);
                            }
                            readyUDPDatagramPacketQueue.removeAll (true);
                            pReferenceUDPDatagramPacket = nullptr;
                        }

                        // Calculate next time when a timeout for a certain UDP datagram will expire
                        if ((pUDPDatagramPacket = pUDPDatagramsQueue->peek())) {
                            i64NextCycleTime = (i64NextCycleTime == 0) ?
                                (pUDPDatagramPacket->getCreationTime() + NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT) :
                                std::min (i64NextCycleTime, (pUDPDatagramPacket->getCreationTime() +
                                                             NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT));
                        }
                        pUDPDatagramsQueue->unlock();
                    }
                    udpDatagramsQueueIterator.nextElement();
                }

                const int64 i64TimeToWait = (i64NextCycleTime == 0) ?
                    I64_LUDMT_TIME_BETWEEN_ITERATIONS : (i64NextCycleTime - i64CurrentCycleTime);
                i64NextCycleTime = (i64NextCycleTime == 0) ?
                    (NOMADSUtil::getTimeInMilliseconds() + I64_LUDMT_TIME_BETWEEN_ITERATIONS) : i64NextCycleTime;
                if (!terminationRequested()) {
                    _cv.wait_for (ul, std::chrono::milliseconds{i64TimeToWait},
                                  [this, i64NextCycleTime] {
                                    return _bNotified || (NOMADSUtil::getTimeInMilliseconds() >= i64NextCycleTime) ||
                                        terminationRequested();
                                  });
                }
                _bNotified = false;
            }
        }

        terminating();
    }

    int LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue (Connection * const pConnection, const CompressionSettings & compressionSettings,
                                                                    const Protocol protocol, const NOMADSUtil::IPHeader * const pIPHeader,
                                                                    const NOMADSUtil::UDPHeader * const pUDPHeader)
    {
        if (!pConnection || !pIPHeader || !pUDPHeader) {
            // All parameters are required
            return -1;
        }

        int rc;
        bool bCompleteFragment = false;
        MutexUDPQueue * pUDPDatagramsQueue = nullptr;
        if (NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT == 0) {
            pUDPDatagramsQueue = &_muqUDPReassembledDatagramsQueue;
        }
        else {
            std::lock_guard<std::mutex> lg{_mtxUDPDatagramsQueueHashTable};
            pUDPDatagramsQueue = _ui32UDPDatagramsQueueHashTable.get (pIPHeader->destAddr.ui32Addr);
            if (!pUDPDatagramsQueue) {
                // New destination IP Address --> adding Queue to the hash table
                pUDPDatagramsQueue = new MutexUDPQueue();
                _ui32UDPDatagramsQueueHashTable.put (pIPHeader->destAddr.ui32Addr, pUDPDatagramsQueue);
            }
        }

        pUDPDatagramsQueue->lock();
        if ((pIPHeader->ui16FlagsAndFragOff & IP_OFFSET_FILTER) != 0) {
            // Fragment received --> looking for relative incomplete UDP datatgram
            rc = pUDPDatagramsQueue->reassembleUDPDatagram (pIPHeader, pUDPHeader);
            if ((rc == MutexUDPQueue::REASSEMBLING_NULL) || (rc == MutexUDPQueue::REASSEMBLING_IMPOSSIBLE) ||
                (rc == MutexUDPQueue::REASSEMBLING_ERROR)) {
                pUDPDatagramsQueue->unlock();
                return -2;
            }
            else if (rc == MutexUDPQueue::REASSEMBLING_COMPLETE) {
                bCompleteFragment = true;
                const UDPDatagramPacket * const pUDPDatagramPacket = pUDPDatagramsQueue->findPacketFromIPHeader (pIPHeader);
                checkAndLogMsg ("LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "correctly reassembled UDP datagram packet with %hu bytes of data and addressed to address <%s:%hu>\n",
                                (pUDPDatagramPacket->getPacketLen() - sizeof(NOMADSUtil::UDPHeader)),
                                NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(),
                                pUDPDatagramPacket->getDestinationPortNum());
            }
        }
        else {
            // Nagle's algorithm entails buffering received packets by adding them to a different queue, based on the destination IP
            UDPDatagramPacket *pUDPDatagramPacket = new UDPDatagramPacket{pConnection, compressionSettings, protocol, pIPHeader, pUDPHeader};
            if (!pUDPDatagramPacket) {
                checkAndLogMsg ("LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", NOMADSUtil::Logger::L_MildError,
                                "impossible to allocate memory for received packet of %hu bytes in total and addressed to IP address %s\n",
                                pIPHeader->ui16TLen, NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAddress());
                pUDPDatagramsQueue->unlock();
                return -3;
            }
            // If flags and offset are all 0s, a complete packet has just been enqueued
            bCompleteFragment = (pIPHeader->ui16FlagsAndFragOff & IP_MF_FLAG_FILTER) == 0;

            rc = pUDPDatagramsQueue->enqueue (pUDPDatagramPacket);
            if ((rc == MutexUDPQueue::ENQUEUING_NULL) || (rc == MutexUDPQueue::ENQUEUING_ERROR)) {
                // ENQUEUING_NULL should never be returned because of the previous checking
                if (rc == MutexUDPQueue::ENQUEUING_ERROR) {
                    checkAndLogMsg ("LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", NOMADSUtil::Logger::L_Warning,
                                    "impossible to enqueue received UDP packet with %hu bytes of data; rc = %d\n",
                                    pUDPDatagramPacket->getCurrentPacketLen(), rc);
                }
                pUDPDatagramsQueue->unlock();
                delete pUDPDatagramPacket;
                return -4;
            }
            else if (rc == MutexUDPQueue::ENQUEUING_BUFFER_FULL) {
                checkAndLogMsg ("LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", NOMADSUtil::Logger::L_LowDetailDebug,
                                "impossible to enqueue received UDP packet with %hu bytes of data; "
                                "%u bytes are already in the queue (%u bytes left); discarding packet\n",
                                pUDPDatagramPacket->getPacketLen(), pUDPDatagramsQueue->getEnqueuedBytesCount(),
                                pUDPDatagramsQueue->getSpaceLeftInBuffer());
                pUDPDatagramsQueue->unlock();
                delete pUDPDatagramPacket;
                return -5;
            }
            else if (bCompleteFragment) {
                checkAndLogMsg ("LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", NOMADSUtil::Logger::L_HighDetailDebug,
                                "correctly enqueued received UDP packet with %hu bytes of data; %u bytes and %d packets are currently "
                                "in the queue\n", pUDPDatagramPacket->getPacketLen(), pUDPDatagramsQueue->getEnqueuedBytesCount(),
                                pUDPDatagramsQueue->size());
            }
            else {
                checkAndLogMsg ("LocalUDPDatagramsManagerThread::addDatagramToOutgoingQueue", NOMADSUtil::Logger::L_HighDetailDebug,
                                "correctly enqueued an incomplete UDP fragment; %u bytes and %d packets are currently in the queue\n",
                                pUDPDatagramsQueue->getEnqueuedBytesCount(), pUDPDatagramsQueue->size());
            }
        }
        uint32 ui32TotalEnqueuedBytes = pUDPDatagramsQueue->getEnqueuedBytesCount();
        pUDPDatagramsQueue->unlock();

        if ((ui32TotalEnqueuedBytes >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) ||
            ((NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT == 0) && bCompleteFragment)) {
            // Waking up LocalUDPDatagramsManager Thread, if sleeping, since packets might be ready to be sent to remote proxies
            _bNotified = true;
            _cv.notify_one();
            if (ui32TotalEnqueuedBytes >= NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD) {
                // Force yielding to LocalUDPDatagramsManagerThread so that transmission can be performed
                yield();
            }
        }

        return 0;
    }

    int LocalUDPDatagramsManagerThread::sendEnqueuedDatagramsToRemoteProxy (MutexUDPQueue * const pUDPDatagramsQueue) const
    {
        if (!pUDPDatagramsQueue) {
            return -1;
        }
        if (pUDPDatagramsQueue->isEmpty()) {
            return 0;
        }

        const auto * const pReferenceUDPDatagramPacket = pUDPDatagramsQueue->peek();
        auto * const pConnection = pReferenceUDPDatagramPacket->getConnection();
        if (pReferenceUDPDatagramPacket->getConnection()->isEnqueueingAllowed()) {
            int rc;
            if (pUDPDatagramsQueue->size() > 1) {
                if (0 != (rc = pConnection->sendMultipleUDPDatagramsToRemoteHost (pReferenceUDPDatagramPacket->getSourceIPAddr(),
                                                                                  pReferenceUDPDatagramPacket->getDestinationIPAddr(),
                                                                                  pUDPDatagramsQueue, pReferenceUDPDatagramPacket->getCompressionSetting(),
                                                                                  Protocol{static_cast<Protocol> (pReferenceUDPDatagramPacket->getPMProtocol())}))) {
                    checkAndLogMsg ("LocalUDPDatagramsManagerThread::sendEnqueuedDatagramsToRemoteProxy", NOMADSUtil::Logger::L_MildError,
                                    "sendMultipleUDPDatagramsToRemoteHost() failed with rc = %d\n", rc);
                    return -2;
                }
            }
            else {
                if (0 != (rc =
                    pConnection->sendUDPUnicastPacketToRemoteHost (pReferenceUDPDatagramPacket->getSourceIPAddr(), pReferenceUDPDatagramPacket->getDestinationIPAddr(),
                                                                   pReferenceUDPDatagramPacket->getIPPacketTTL(), pReferenceUDPDatagramPacket->getUDPPacket(),
                                                                   pReferenceUDPDatagramPacket->getPacketLen(), pReferenceUDPDatagramPacket->getCompressionSetting(),
                                                                   Protocol{static_cast<Protocol> (pReferenceUDPDatagramPacket->getPMProtocol())}))) {
                    checkAndLogMsg ("LocalUDPDatagramsManagerThread::sendEnqueuedDatagramsToRemoteProxy", NOMADSUtil::Logger::L_MildError,
                                    "sendUDPUnicastPacketToRemoteHost() failed with rc = %d\n", rc);
                    return -3;
                }
            }
        }
        else {
            checkAndLogMsg ("LocalUDPDatagramsManagerThread::sendEnqueuedDatagramsToRemoteProxy", NOMADSUtil::Logger::L_LowDetailDebug,
                            "connector buffer is full; dropping a Proxy Packet of %u bytes (containing %d UDP datagrams)\n",
                            pUDPDatagramsQueue->getEnqueuedBytesCount(), pUDPDatagramsQueue->size());
            return 0;
        }

        return pUDPDatagramsQueue->getEnqueuedBytesCount();
    }

    const int64 LocalUDPDatagramsManagerThread::I64_LUDMT_TIME_BETWEEN_ITERATIONS;
}