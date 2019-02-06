/*
 * Entry.cpp
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

#include <algorithm>

#include "ISAACRand.h"
#include "Logger.h"

#include "Entry.h"
#include "ProxyMessages.h"
#include "MutexBuffer.h"
#include "TCPSegment.h"
#include "ConnectorReader.h"
#include "ConnectorWriter.h"
#include "Connection.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    DataCompressors::~DataCompressors (void)
    {
        delete pConnectorReader;
        delete pConnectorWriter;
    }

    void DataCompressors::reset (void)
    {
        delete pConnectorReader;
        pConnectorReader = nullptr;
        delete pConnectorWriter;
        pConnectorWriter = nullptr;
    }

    Entry::~Entry (void)
    {
        std::lock_guard<std::mutex> lg{_mtx};

        delete _pMutexBuffer;
        delete _pAvailableData;
        delete _pTempTCPSegment;
        delete pTCPSegment;
    }

    void Entry::clear (void)
    {
        if ((localState == TCTLS_LISTEN) && (remoteState == TCTRS_Unknown)) {
            // Nothing to do
            return;
        }

        localState = TCTLS_LISTEN;
        remoteState = TCTRS_Unknown;
        ui16RemoteID = 0;
        ui32RemoteProxyUniqueID = 0;
        ui32LocalIP = 0;
        ui16LocalPort = 0;
        ui32RemoteIP = 0;
        ui16RemotePort = 0;

        ui32StartingInSeqNum = 0;
        ui32StartingOutSeqNum = 0;
        ui32NextExpectedInSeqNum = 0;
        ui32LastACKedSeqNum = 0;
        ui32LastAckSeqNum = 0;
        ui32OutSeqNum = 0;
        ui32ReceivedDataSeqNum = 0;
        ui16ReceiverWindowSize = 0;

        i64LastAckTime = 0;
        i64LastCalculatedRTOTime = 0;
        ui16RTO = Entry::LB_RTO;
        ui16SRTT = Entry::LB_RTO;
        ui8RetransmissionAttempts = 0;
        i64LocalActionTime = 0;
        i64RemoteActionTime = 0;
        i64IdleTime = 0;

        assignedPriority = 0;
        currentPriority = 0;

        _protocol = Protocol::PMP_UNDEF_PROTOCOL;

        iaLocalInterfaceAddr.clear();
        iaRemoteProxyAddr.clear();

        releaseMemory();
    }

    void Entry::reset (void)
    {
        if ((localState == TCTLS_LISTEN) ||
            ((localState == TCTLS_CLOSED) && (remoteState == TCTRS_Disconnected))) {
            // Nothing to do
            return;
        }

        localState = TCTLS_CLOSED;
        remoteState = TCTRS_Disconnected;

        i64LocalActionTime = NOMADSUtil::getTimeInMilliseconds();

        releaseMemory();
    }

    // This method prepares the Entry to be used
    void Entry::prepareNewConnection (void)
    {
        ui32OutSeqNum = NOMADSUtil::ISAACRand::getRnd ((uint32)NOMADSUtil::getTimeInMilliseconds());
        ui32StartingOutSeqNum = ui32OutSeqNum;

        delete _pMutexBuffer;
        _pMutexBuffer = new MutexBuffer{false, NetProxyApplicationParameters::MAX_ENTRY_BUF_SIZE, NetProxyApplicationParameters::MIN_ENTRY_BUF_SIZE};
        delete pTCPSegment;
        _pTempTCPSegment = new TCPSegment{0, _pMutexBuffer->getBufferSize(), const_cast<const unsigned char*> (_pMutexBuffer->getBuffer())};
    }

    uint32 Entry::ackOutgoingDataUpto (uint32 ui32AckNum)
    {
        /* We should delete from the buffer only the packets that satisfy (pData->getFollowingSequenceNumber() == ui32AckNum),
         * but RFC allows applications to send one single ACK to acknowledge all the packets received from the last ACK NUM sent
         * up to the one for which the equality (pData->getFollowingSequenceNumber() == ui32AckNum) is fulfilled.
         * For the reason above, the follwing inequality is verified.
         */
        uint32 ui16ACKedPackets = 0;
        int64 lastPacketTime = 0;
        while (dqLocalHostOutBuf.size() > 0) {
            auto & rReceivedData = dqLocalHostOutBuf.front();
            if ((rReceivedData.getItemLength() > 0) &&              // Do not dequeue 0-sized items
                NOMADSUtil::SequentialArithmetic::lessThanOrEqual (rReceivedData.getFollowingSequenceNumber(), ui32AckNum)) {
                lastPacketTime = rReceivedData.getLastTransmitTime();
                dqLocalHostOutBuf.pop_front();
                ui16ACKedPackets++;
            }
            else {
                if ((rReceivedData.getItemLength() > 0) && NOMADSUtil::SequentialArithmetic::lessThan (rReceivedData.getSequenceNumber(), ui32AckNum)) {
                    // Some bytes in the packet, but not all of them, have been acknowledged
                    uint16 ui16AckedBytes = NOMADSUtil::SequentialArithmetic::delta (ui32AckNum, rReceivedData.getSequenceNumber());
                    rReceivedData.incrementValues (ui16AckedBytes);
                    lastPacketTime = rReceivedData.getLastTransmitTime();
                }
                break;
            }
        }

        if ((lastPacketTime > 0) && ((i64LastAckTime - i64LastCalculatedRTOTime) >= Entry::RTO_RECALCULATION_TIME)) {
            // i64LastAckTime is the time at which the last packet with an ACK flag was received
            // lastPacketTime is the transmission time of the last sent packet that is covered by the received ACK packet
            calculateRTO (i64LastAckTime - lastPacketTime);
            i64LastCalculatedRTOTime = NOMADSUtil::getTimeInMilliseconds();
        }

        return ui16ACKedPackets;
    }

    void Entry::updateOutgoingWindow (const NOMADSUtil::TCPHeader * const pTCPHeader)
    {
        /* The difference between ui32LastAckSeqNum and ui32ReceivedDataSeqNum is the following:
         * ui32ReceivedDataSeqNum is the greatest SEQ NUM we received, i.e. the most recent packet sent from the local application;
         * ui32LastAckSeqNum is the greatest ACK we have received, i.e. the most recent packet sent by the NetProxy that was ACKed by the local application.
         */

        // Check if received packet is new or remote window info needs to be updated
        if (NOMADSUtil::SequentialArithmetic::lessThan (ui32ReceivedDataSeqNum, pTCPHeader->ui32SeqNum) ||
            ((ui32ReceivedDataSeqNum == pTCPHeader->ui32SeqNum) && NOMADSUtil::SequentialArithmetic::lessThan (ui32LastAckSeqNum, pTCPHeader->ui32AckNum)) ||
            ((ui32ReceivedDataSeqNum == pTCPHeader->ui32SeqNum) && (ui32LastAckSeqNum == pTCPHeader->ui32AckNum) && (pTCPHeader->ui16Window > ui16ReceiverWindowSize))) {
                ui32ReceivedDataSeqNum = pTCPHeader->ui32SeqNum;
                ui32LastAckSeqNum = pTCPHeader->ui32AckNum;
                ui16ReceiverWindowSize = pTCPHeader->ui16Window;
        }
    }

    const bool Entry::isRemoteConnectionFlushed (void) const
    {
        return _dataCompressors.pConnectorWriter->isFlushed() && (_pAvailableData ? (_pAvailableData->getItemLength() == 0) : true);
    }

    void Entry::calculateRTO (int64 rtt)
    {
        uint16 ui16rtt = (uint16) rtt;
        double tempSRTT = (Entry::ALPHA_RTO * ui16SRTT) + ((1 - Entry::ALPHA_RTO) * ui16rtt);
        ui16SRTT = (uint16) tempSRTT;

        ui16RTO = std::min (Entry::UB_RTO, std::max (Entry::LB_RTO, (uint16) (Entry::BETA_RTO * tempSRTT)));
        checkAndLogMsg ("Entry::calculateRTO", NOMADSUtil::Logger::L_HighDetailDebug,
                        "new calculated RTO: %hums; SRTT: %hums\n", ui16RTO, ui16SRTT);
    }

    TCPSegment * const Entry::dequeueLocallyReceivedData (uint16 ui16RequestedBytesNum)
    {
        int rc, extractedData = 0;
        uint8 *ppBuf[1];
        uint32 uiBufLen = 0;
        TCPSegment *pDataToReturn = _pAvailableData;

        static const uint8 TCP_DATA_FLAGS_FILTER = 0xF8;                // Nasty filter to avoid sending packets with control flags in a TCPDataProxyMessages
        static const uint8 TCP_DATA_FLAGS_FILTER_NO_PSH = 0xF0;         // Nasty filter to avoid sending packets with control or PSH flags in a TCPDataProxyMessages

        ppBuf[0] = nullptr;
        if (pDataToReturn) {
            // We have data previously processed --> check if there are already enough bytes in the buffer
            checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "L%hu-R%hu: a packet with SEQ number %u and %hu bytes of data is already in the buffer; "
                            "a total of %hu bytes was requested\n", ui16ID, ui16RemoteID,
                            pDataToReturn->getSequenceNumber(), pDataToReturn->getItemLength(), ui16RequestedBytesNum);
            if (pDataToReturn->getItemLength() > ui16RequestedBytesNum) {
                // Available data is more than needed --> create and return a new packet with only requested data
                pDataToReturn = new TCPSegment (_pAvailableData->getSequenceNumber(), ui16RequestedBytesNum,
                                                _pAvailableData->getData(), NOMADSUtil::TCPHeader::TCPF_ACK);
                _pAvailableData->incrementValues (ui16RequestedBytesNum);
                checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_HighDetailDebug,
                                "L%hu-R%hu: all needed data was already in the buffer; packet with SEQ number %u "
                                "and %hu bytes of data is still in the buffer\n", ui16ID, ui16RemoteID,
                                _pAvailableData->getSequenceNumber(), _pAvailableData->getItemLength());
                return pDataToReturn;
            }

            _pAvailableData = nullptr;
            if (pDataToReturn->getItemLength() == ui16RequestedBytesNum) {
                // We have exactly the requested amount of data --> return the available packet
                checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_HighDetailDebug,
                                "L%hu-R%hu: all needed data was already in the buffer; buffer is now empty\n",
                                ui16ID, ui16RemoteID);
                return pDataToReturn;
            }
        }

        if (inQueue.isDataReady()) {
            // There is data available and requested bytes are more than buffered (or there is none in the buffer)
            if (pDataToReturn) {
                // Copying already extracted data
                TCPSegment *pTempDataToReturn = pDataToReturn;
                pDataToReturn = new ReceivedData (pDataToReturn->getSequenceNumber(), pDataToReturn->getItemLength(),
                                                  pDataToReturn->getData(), pDataToReturn->getTCPFlags());
                delete pTempDataToReturn;
            }

            // Resizing buffer, if necessary
            _pMutexBuffer->lock();
            if (inQueue.getAvailableBytesCount() > _pMutexBuffer->getBufferSize()) {
                _pMutexBuffer->resizeBuffer (inQueue.getAvailableBytesCount(), false);
            }
            _pTempTCPSegment->setData (_pMutexBuffer->getBuffer());
            _pTempTCPSegment->setItemLength (_pMutexBuffer->getBufferSize());
            _pTempTCPSegment->setTCPFlags (0U);

            // Extracting available data
            if ((extractedData = inQueue.extractData (_pTempTCPSegment)) < 0) {
                checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_MildError,
                                "L%hu-R%hu: impossible to extract data from the CircularOrderedBuffer; "
                                "buffer size is %u bytes and there are %u available bytes in buffer\n",
                                ui16ID, ui16RemoteID, _pMutexBuffer->getBufferSize(),
                                inQueue.getAvailableBytesCount());
                _pMutexBuffer->unlock();
                return nullptr;
            }

            if (!pDataToReturn) {
                // No packets were available in the buffer --> reading new data from scratch
                checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: no buffered data found; retrieved %d bytes of data starting from "
                                "packet with SEQ number %u - %u bytes still available in buffer\n",
                                ui16ID, ui16RemoteID, extractedData, _pTempTCPSegment->getSequenceNumber(),
                                inQueue.getAvailableBytesCount());
                bool bLocalFlush = ((_pTempTCPSegment->getTCPFlags() & NOMADSUtil::TCPHeader::TCPF_PSH) ||
                                    (_pTempTCPSegment->getTCPFlags() & NOMADSUtil::TCPHeader::TCPF_FIN)) != 0;
                if ((rc = getConnectorWriter()->writeData (_pTempTCPSegment->getData(), extractedData, ppBuf, uiBufLen, bLocalFlush)) < 0) {
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_MildError,
                                    "L%hu-R%hu: error manipulating %d bytes of data from the packet with SEQ NUM %u through the Connector Writer; "
                                    "writeData() returned rc = %d\n", ui16ID, ui16RemoteID, extractedData, _pTempTCPSegment->getSequenceNumber(), rc);
                    _pMutexBuffer->unlock();
                    return nullptr;
                }

                if (ui16RequestedBytesNum < uiBufLen) {
                    // Available data is more than needed --> create and return a new packet with only requested data
                    pDataToReturn = new TCPSegment (_pTempTCPSegment->getSequenceNumber(), ui16RequestedBytesNum,
                                                    *ppBuf, NOMADSUtil::TCPHeader::TCPF_ACK);
                    _pAvailableData = new TCPSegment (_pTempTCPSegment->getSequenceNumber() + ui16RequestedBytesNum,
                                                      uiBufLen - ui16RequestedBytesNum, *ppBuf + ui16RequestedBytesNum,
                                                      _pTempTCPSegment->getTCPFlags());
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: retrieved all requested bytes from packet with SEQ number %u; "
                                    "a new packet with SEQ %u and %u bytes of data has been built and buffered\n",
                                    ui16ID, ui16RemoteID, pDataToReturn->getSequenceNumber(),
                                    _pAvailableData->getSequenceNumber(), _pAvailableData->getItemLength());
                }
                else {
                    // Creating a new packet with necessary data and storing it as temporary buffer (maybe we can enqueue more data later)
                    pDataToReturn = new TCPSegment (_pTempTCPSegment->getSequenceNumber(), uiBufLen,
                                                    *ppBuf, _pTempTCPSegment->getTCPFlags());
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: retrieved a packet with SEQ number %u, FLAGs %hhu and %u bytes long (%d before compression); "
                                    "%u bytes can still be retrieved\n", ui16ID, ui16RemoteID, pDataToReturn->getSequenceNumber(),
                                    pDataToReturn->getTCPFlags(), pDataToReturn->getItemLength(), extractedData,
                                    (ui16RequestedBytesNum - pDataToReturn->getItemLength()));
                }
            }
            else {
                // Some data had already been received --> retrieve missing data and copy requested one to pDataToReturn
                bool bLocalFlush = ((_pTempTCPSegment->getTCPFlags() & NOMADSUtil::TCPHeader::TCPF_PSH) ||
                                    (_pTempTCPSegment->getTCPFlags() & NOMADSUtil::TCPHeader::TCPF_FIN)) != 0;
                if ((rc = getConnectorWriter()->writeData (_pTempTCPSegment->getData(), extractedData, ppBuf, uiBufLen, bLocalFlush)) < 0) {
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_MildError,
                                    "L%hu-R%hu: error manipulating %u bytes of data from packet with SEQ number %u "
                                    "through the Connector Writer (compression is <%s>); rc = %d\n",
                                    ui16ID, ui16RemoteID, _pTempTCPSegment->getItemLength(),
                                    _pTempTCPSegment->getSequenceNumber(), getConnectorWriter()->getCompressionName(), rc);
                    _pMutexBuffer->unlock();
                    return pDataToReturn;
                }

                uint32 ui32BytesToReadFromPacket = std::min (uiBufLen, ui16RequestedBytesNum - pDataToReturn->getItemLength());
                static_cast<ReceivedData *> (pDataToReturn)->appendDataToBuffer (const_cast<unsigned char *> (*ppBuf),
                                                                                 ui32BytesToReadFromPacket);
                if ((uiBufLen - ui32BytesToReadFromPacket) > 0) {
                    static_cast<ReceivedData *> (pDataToReturn)->addTCPFlags (_pTempTCPSegment->getTCPFlags() & TCP_DATA_FLAGS_FILTER_NO_PSH);
                    _pAvailableData = new TCPSegment (_pTempTCPSegment->getSequenceNumber() + ui32BytesToReadFromPacket,
                                                      uiBufLen - ui32BytesToReadFromPacket, *ppBuf + ui32BytesToReadFromPacket,
                                                      _pTempTCPSegment->getTCPFlags());
                    if (!_pAvailableData) {
                        // Error allocating new TCPSegment
                        checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_MildError,
                                        "L%hu-R%hu: error buffering the remaining %u bytes of data extracted from the ConnectionWriter; "
                                        "this method will return the %u remaining bytes from packet with SEQ NUM %u and %u bytes long "
                                        "(%u before going through the ConnectorWriter)\n", ui16ID, ui16RemoteID,
                                        (uiBufLen - ui32BytesToReadFromPacket), ui32BytesToReadFromPacket, _pTempTCPSegment->getSequenceNumber(),
                                        uiBufLen, _pTempTCPSegment->getItemLength(), _pAvailableData->getItemLength());
                        _pMutexBuffer->unlock();
                        return pDataToReturn;
                    }
                    // Enqueued to pDataToReturn only necessary data from compressed packet --> shrink remaining data
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: retrieved the %u remaining bytes from packet with SEQ number %u and %u bytes long "
                                    "(%u before processing through the Connector Writer) - %u buffered bytes left\n",
                                    ui16ID, ui16RemoteID, ui32BytesToReadFromPacket, _pTempTCPSegment->getSequenceNumber(),
                                    uiBufLen, _pTempTCPSegment->getItemLength(), _pAvailableData->getItemLength());
                }
                else {
                    // Enqueued the whole (compressed) data packet to pDataToReturn (PSH allowed to reduce latency)
                    (static_cast<ReceivedData*> (pDataToReturn))->addTCPFlags (_pTempTCPSegment->getTCPFlags() & TCP_DATA_FLAGS_FILTER);
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: retrieved the whole packet with SEQ number %u, FLAGs %hhu and %u bytes long "
                                    "(%u before processing through the Connector Writer); %u bytes could still be retrieved\n",
                                    ui16ID, ui16RemoteID, _pTempTCPSegment->getSequenceNumber(), _pTempTCPSegment->getTCPFlags(),
                                    ui32BytesToReadFromPacket, _pTempTCPSegment->getItemLength(),
                                    (ui16RequestedBytesNum - pDataToReturn->getItemLength()));
                    _pAvailableData = nullptr;
                }
            }
            _pMutexBuffer->unlock();
        }

        return pDataToReturn;
    }

    bool Entry::areThereUntransmittedPacketsInUDPTransmissionQueue (void) const
    {
        return getConnection() ?
            getConnection()->areThereTCPTypePacketsInUDPTransmissionQueue (ui16ID, ui16RemoteID) : 0;
    }

    void Entry::releaseMemory (void)
    {
        removePacketsFromUDPTransmissionQueue();

        inQueue.resetBuffer();
        dqLocalHostOutBuf.clear();
        _dataCompressors.reset();

        delete _pMutexBuffer;
        _pMutexBuffer = nullptr;
        delete _pAvailableData;
        _pAvailableData = nullptr;
        delete _pTempTCPSegment;
        _pTempTCPSegment = nullptr;
        delete pTCPSegment;
        pTCPSegment = nullptr;
        _pRemoteConnection = nullptr;
    }

    int Entry::removePacketsFromUDPTransmissionQueue (void) const
    {
        return getConnection() ?
            getConnection()->removeTCPTypePacketsFromTransmissionQueue (ui16ID, ui16RemoteID) : 0;
    }

    const uint32 Entry::STANDARD_MSL;                   // Standard Maximum Segment Lifetime of 2 minutes (RFC 793)
    const uint16 Entry::LB_RTO;                         // Retransmission TimeOut Lower Bound of 100 milliseconds (in RFC 793 is 1 second)
    const uint16 Entry::UB_RTO;                         // Retransmission TimeOut Upper Bound of 60 seconds (RFC 793)
    const uint16 Entry::RTO_RECALCULATION_TIME;         // Time that has to pass before recalculating RTO (RFC 793)
    const double Entry::ALPHA_RTO = 0.85;               // Alpha constant for RTO calculation (RFC 793)
    const double Entry::BETA_RTO = 0.85;                // Beta constant for RTO calculation (RFC 793)
}
