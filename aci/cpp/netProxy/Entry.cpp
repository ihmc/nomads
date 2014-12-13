/*
 * Entry.cpp
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

#include "ISAACRand.h"
#include "Logger.h"

#include "Entry.h"
#include "TCPConnTable.h"
#include "MutexBuffer.h"
#include "ConnectorReader.h"
#include "ConnectorWriter.h"
#include "UDPConnector.h"
#include "ConnectionManager.h"
#include "NetProxyConfigManager.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    void Connectors::reset (Entry * const pEntry)
    {
        pConnector = NULL;
        if (pConnection && pEntry) {
            pConnection->removeTCPEntry (pEntry);
        }
        pConnection = NULL;

        if (pConnectorReader) {
            delete pConnectorReader;
            pConnectorReader = NULL;
        }
        if (pConnectorWriter) {
            delete pConnectorWriter;
            pConnectorWriter = NULL;
        }
    }

    Entry::~Entry (void)
    {
        _m.lock();
        outBuf.removeAll (true);
        _connectors.reset (this);

        if (_pAvailableData) {
            delete _pAvailableData;
            _pAvailableData = NULL;
        }
        if (_pMutexBuffer) {
            delete _pMutexBuffer;
            _pMutexBuffer = NULL;
        }
        if (_pTempTCPSegment) {
            delete _pTempTCPSegment;
            _pTempTCPSegment = NULL;
        }
        _m.unlock();
    }

    void Entry::clear (void)
    {
        localStatus = TCTLS_Unused;
        remoteStatus = TCTRS_Unknown;
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
        ui16RTO = TCPConnTable::LB_RTO;
        ui16SRTT = TCPConnTable::LB_RTO;
        ui8RetransmissionAttempts = 0;
        i64LocalActionTime = 0;
        i64RemoteActionTime = 0;
        i64IdleTime = 0;

        _protocol = ProxyMessage::PMP_UNDEF_PROTOCOL;

        remoteProxyAddr.clear();
        inQueue.resetBuffer();
        outBuf.removeAll (true);
        removePacketsFromUDPTransmissionQueue();
        _connectors.reset (this);

        if (_pAvailableData) {
            delete _pAvailableData;
            _pAvailableData = NULL;
        }
        if (_pMutexBuffer) {
            delete _pMutexBuffer;
            _pMutexBuffer = NULL;
        }
        if (_pTempTCPSegment) {
            delete _pTempTCPSegment;
            _pTempTCPSegment = NULL;
        }
    }

    void Entry::reset (void)
    {
        if (localStatus == TCTLS_Unused) {
            return;
        }
        localStatus = TCTLS_CLOSED;
        remoteStatus = TCTRS_Disconnected;

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
        ui16RTO = TCPConnTable::LB_RTO;
        ui16SRTT = TCPConnTable::LB_RTO;
        i64LocalActionTime = getTimeInMilliseconds();

        inQueue.resetBuffer();
        remoteProxyAddr.clear();
        outBuf.removeAll (true);
        removePacketsFromUDPTransmissionQueue();
        _connectors.reset (this);

        if (_pAvailableData) {
            delete _pAvailableData;
            _pAvailableData = NULL;
        }
        if (_pMutexBuffer) {
            delete _pMutexBuffer;
            _pMutexBuffer = NULL;
        }
        if (_pTempTCPSegment) {
            delete _pTempTCPSegment;
            _pTempTCPSegment = NULL;
        }
    }

    void Entry::prepareNewConnection (void)
    {
        _m.lock();

        localStatus = TCTLS_LISTEN;
        ui32OutSeqNum = ISAACRand::getRnd ((uint32) getTimeInMilliseconds());
        ui32StartingOutSeqNum = ui32OutSeqNum;

        _connectors.reset (this);
        if (_pAvailableData) {
            delete _pAvailableData;
            _pAvailableData = NULL;
        }
        if (_pMutexBuffer) {
            delete _pMutexBuffer;
            _pMutexBuffer = NULL;
        }
        if (_pTempTCPSegment) {
            delete _pTempTCPSegment;
            _pTempTCPSegment = NULL;
        }

        _m.unlock();
    }

    uint32 Entry::ackOutgoingDataUpto (uint32 ui32AckNum)
    {
        /* ALE:
         * We should delete from the buffer only the packet which has (pData->getFollowingSequenceNumber() == ui32AckNum),
         * but RFC allows applications to send one single ACK to acknowledge all the packets received from the last ACK NUM sent
         * up to the one for which the equality (pData->getFollowingSequenceNumber() == ui32AckNum) is fulfilled.
         * For the reason above, the inequality below is applied.
         */
        ReceivedData *pData;
        uint32 ui16ACKedPackets = 0;
        int64 lastPacketTime = 0;
        while (NULL != (pData = outBuf.peek())) {
            if ((pData->getItemLength() > 0) && SequentialArithmetic::lessThanOrEqual (pData->getFollowingSequenceNumber(), ui32AckNum)) {        // Do not dequeue 0 length items
                outBuf.dequeue();
                lastPacketTime = pData->_i64LastTransmitTime;
                delete pData;
                ui16ACKedPackets++;
            }
            else {
                if ((pData->getItemLength() > 0) && SequentialArithmetic::lessThan (pData->getSequenceNumber(), ui32AckNum)) {
                    // Some bytes in the packet, but not all of them, have been acknowledged
                    uint16 ui16AckedBytes = SequentialArithmetic::delta (ui32AckNum, pData->getSequenceNumber());
                    pData->incrementValues (ui16AckedBytes);
                }
                break;
            }
        }

        if ((lastPacketTime > 0) && ((i64LastAckTime - i64LastCalculatedRTOTime) >= TCPConnTable::RTO_RECALCULATION_TIME)) {
            // i64LastAckTime is the time at which the last packet with an ACK flag was received
            // lastPacketTime is the transmission time of the last sent packet that is covered by the received ACK packet
            calculateRTO (i64LastAckTime - lastPacketTime);
            i64LastCalculatedRTOTime = getTimeInMilliseconds();
        }

        return ui16ACKedPackets;
    }

    void Entry::updateOutgoingWindow (const TCPHeader *pTCPHeader)
    {
        /* ALE:
         * The difference between ui32LastAckSeqNum and ui32ReceivedDataSeqNum is the following:
         * ui32ReceivedDataSeqNum is the greatest SEQ number we received, ie the most recent packet sent from the application running on the local host;
         * ui32LastAckSeqNum is the greatest ACK we have received, i.e. the most recent ACK to a packet which the proxy sent to an application running on the local host.
         */

        // Check if received packet is new or remote window info needs to be updated
        if (SequentialArithmetic::lessThan (ui32ReceivedDataSeqNum, pTCPHeader->ui32SeqNum) ||
            ((ui32ReceivedDataSeqNum == pTCPHeader->ui32SeqNum) && SequentialArithmetic::lessThan (ui32LastAckSeqNum, pTCPHeader->ui32AckNum)) ||
            ((ui32ReceivedDataSeqNum == pTCPHeader->ui32SeqNum) && (ui32LastAckSeqNum == pTCPHeader->ui32AckNum) && (pTCPHeader->ui16Window > ui16ReceiverWindowSize))) {
                ui32ReceivedDataSeqNum = pTCPHeader->ui32SeqNum;
                ui32LastAckSeqNum = pTCPHeader->ui32AckNum;
                ui16ReceiverWindowSize = pTCPHeader->ui16Window;
                i64LastAckTime = getTimeInMilliseconds();
        }
    }

    void Entry::calculateRTO (int64 rtt)
    {
        uint16 ui16rtt = (uint16) rtt;
        double tempSRTT = (TCPConnTable::ALPHA_RTO * ui16SRTT) + ((1 - TCPConnTable::ALPHA_RTO) * ui16rtt);
        ui16SRTT = (uint16) tempSRTT;

        ui16RTO = std::min (TCPConnTable::UB_RTO, std::max (TCPConnTable::LB_RTO, (uint16) (TCPConnTable::BETA_RTO * tempSRTT)));
        checkAndLogMsg ("Entry::calculateRTO", Logger::L_HighDetailDebug,
                        "new calculated RTO: %hums; SRTT: %hums\n", ui16RTO, ui16SRTT);
    }

    TCPSegment * const Entry::dequeueLocallyReceivedData (uint16 ui16RequestedBytesNum)
    {
        int rc, extractedData = 0;
        uint8 *ppBuf[1], ui8Flags = 0;
        ppBuf[0] = NULL;
        uint32 uiBufLen = 0, freedBytes = 0;
        TCPSegment *pDataToReturn = _pAvailableData;

        static const uint8 TCP_DATA_FLAGS_FILTER = 0xF8;                // Nasty filter to avoid sending packets with control flags in a TCPDataProxyMessages
        static const uint8 TCP_DATA_FLAGS_FILTER_NO_PSH = 0xF0;         // Nasty filter to avoid sending packets with control or PSH flags in a TCPDataProxyMessages

        if (pDataToReturn) {
            // We have data previously processed --> check if there are already enough bytes in the buffer
            checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_MediumDetailDebug,
                            "L%hu-R%hu: a packet with SEQ number %u and %hu bytes of data is already in the buffer;"
                            " a total of %hu bytes was requested\n", ui16ID, ui16RemoteID,
                            pDataToReturn->getSequenceNumber(), pDataToReturn->getItemLength(), ui16RequestedBytesNum);
            if (pDataToReturn->getItemLength() > ui16RequestedBytesNum) {
                // Available data is more than needed --> create and return a new packet with only requested data
                pDataToReturn = new TCPSegment (_pAvailableData->getSequenceNumber(), ui16RequestedBytesNum, _pAvailableData->getData(), TCPHeader::TCPF_ACK);
                _pAvailableData->incrementValues (ui16RequestedBytesNum);
                checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_HighDetailDebug,
                                "L%hu-R%hu: all needed data was already in the buffer; packet with SEQ number %u"
                                " and %hu bytes of data is still in the buffer\n", ui16ID, ui16RemoteID,
                                _pAvailableData->getSequenceNumber(), _pAvailableData->getItemLength());
                return pDataToReturn;
            }

            _pAvailableData = NULL;
            if (pDataToReturn->getItemLength() == ui16RequestedBytesNum) {
                // We have exactly the requested amount of data --> return the available packet
                checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_HighDetailDebug,
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
                checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_MildError,
                                "L%hu-R%hu: impossible to extract data from the CircularOrderedBuffer; "
                                "buffer size is %u bytes and there are %u available bytes in buffer\n",
                                ui16ID, ui16RemoteID, _pMutexBuffer->getBuffer(),
                                inQueue.getAvailableBytesCount());
                _pMutexBuffer->unlock();
                return NULL;
            }

            if (!pDataToReturn) {
                // No packets were available in the buffer --> reading new data from scratch
                checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: no buffered data found; retrieved %d bytes of data starting from"
                                " packet with SEQ number %u - %u bytes still available in buffer\n",
                                ui16ID, ui16RemoteID, extractedData, _pTempTCPSegment->getSequenceNumber(),
                                inQueue.getAvailableBytesCount());
                bool bLocalFlush = ((_pTempTCPSegment->getTCPFlags() & TCPHeader::TCPF_PSH) || (_pTempTCPSegment->getTCPFlags() & TCPHeader::TCPF_FIN)) != 0;
                if ((rc = getConnectorWriter()->writeData (_pTempTCPSegment->getData(), extractedData, ppBuf, uiBufLen, bLocalFlush)) < 0) {
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_MildError,
                                    "L%hu-R%hu: error manipulating %d bytes of data from packet with SEQ number %u through the Connector Writer; rc = %d\n",
                                    ui16ID, ui16RemoteID, extractedData, _pTempTCPSegment->getSequenceNumber(), rc);
                    _pMutexBuffer->unlock();
                    return NULL;
                }

                if (ui16RequestedBytesNum < uiBufLen) {
                    // Available data is more than needed --> create and return a new packet with only requested data
                    pDataToReturn = new TCPSegment (_pTempTCPSegment->getSequenceNumber(), ui16RequestedBytesNum, *ppBuf, TCPHeader::TCPF_ACK);
                    _pAvailableData = new TCPSegment (_pTempTCPSegment->getSequenceNumber() + ui16RequestedBytesNum, uiBufLen - ui16RequestedBytesNum,
                                                        *ppBuf + ui16RequestedBytesNum, _pTempTCPSegment->getTCPFlags());
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: retrieved all requested bytes from packet with SEQ number %u; "
                                    "a new packet with SEQ %u and %u bytes of data has been built and buffered\n",
                                    ui16ID, ui16RemoteID, pDataToReturn->getSequenceNumber(),
                                    _pAvailableData->getSequenceNumber(), _pAvailableData->getItemLength());
                }
                else {
                    // Creating a new packet with necessary data and storing it as temporary buffer (maybe we can enqueue more data later)
                    pDataToReturn = new TCPSegment (_pTempTCPSegment->getSequenceNumber(), uiBufLen, *ppBuf, _pTempTCPSegment->getTCPFlags());
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: retrieved a packet with SEQ number %u, FLAGs %hhu and %u bytes long"
                                    " (%d before compression); %u bytes can still be retrieved\n",
                                    ui16ID, ui16RemoteID, pDataToReturn->getSequenceNumber(),
                                    pDataToReturn->getTCPFlags(), pDataToReturn->getItemLength(), extractedData,
                                    (ui16RequestedBytesNum - pDataToReturn->getItemLength()));
                }
            }
            else {
                // Some data had already been received --> retrieve missing data and copy requested one to pDataToReturn
                bool bLocalFlush = ((_pTempTCPSegment->getTCPFlags() & TCPHeader::TCPF_PSH) || (_pTempTCPSegment->getTCPFlags() & TCPHeader::TCPF_FIN)) != 0;
                if ((rc = getConnectorWriter()->writeData (_pTempTCPSegment->getData(), extractedData, ppBuf, uiBufLen, bLocalFlush)) < 0) {
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_MildError,
                                    "L%hu-R%hu: error manipulating %u bytes of data from packet with SEQ number %u"
                                    " through the Connector Writer (compression is <%s>); rc = %d\n",
                                    ui16ID, ui16RemoteID, _pTempTCPSegment->getItemLength(),
                                    _pTempTCPSegment->getSequenceNumber(), getConnectorWriter()->getCompressionName(), rc);
                    _pMutexBuffer->unlock();
                    return pDataToReturn;
                }

                uint32 ui32BytesToReadFromPacket = std::min (uiBufLen, ui16RequestedBytesNum - pDataToReturn->getItemLength());
                (static_cast<ReceivedData*> (pDataToReturn))->appendDataToBuffer (const_cast<unsigned char*> (*ppBuf), ui32BytesToReadFromPacket);
                if ((uiBufLen - ui32BytesToReadFromPacket) > 0) {
                    (static_cast<ReceivedData*> (pDataToReturn))->addTCPFlags (_pTempTCPSegment->getTCPFlags() & TCP_DATA_FLAGS_FILTER_NO_PSH);
                    _pAvailableData = new TCPSegment (_pTempTCPSegment->getSequenceNumber() + ui32BytesToReadFromPacket, uiBufLen - ui32BytesToReadFromPacket,
                                                        *ppBuf + ui32BytesToReadFromPacket, _pTempTCPSegment->getTCPFlags());
                    if (!_pAvailableData) {
                        // Error allocating new TCPSegment
                        checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_MildError,
                                        "L%hu-R%hu: error buffering remaining %u bytes of data extracted from the connection writer"
                                        " - the %u remaining bytes from packet with SEQ number %u and %u bytes long "
                                        "(%u before processing through the Connector Writer) will be returned\n",
                                        ui16ID, ui16RemoteID, (uiBufLen - ui32BytesToReadFromPacket),
                                        ui32BytesToReadFromPacket, _pTempTCPSegment->getSequenceNumber(), uiBufLen,
                                        _pTempTCPSegment->getItemLength(), _pAvailableData->getItemLength());
                        _pMutexBuffer->unlock();
                        return pDataToReturn;
                    }
                    // Enqueued to pDataToReturn only necessary data from compressed packet --> shrink remaining data
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: retrieved the %u remaining bytes from packet with SEQ number %u and %u bytes long "
                                    "(%u before processing through the Connector Writer) - %u buffered bytes left\n",
                                    ui16ID, ui16RemoteID, ui32BytesToReadFromPacket, _pTempTCPSegment->getSequenceNumber(),
                                    uiBufLen, _pTempTCPSegment->getItemLength(), _pAvailableData->getItemLength());
                }
                else {
                    // Enqueued the whole (compressed) data packet to pDataToReturn (PSH allowed to reduce latency)
                    (static_cast<ReceivedData*> (pDataToReturn))->addTCPFlags (_pTempTCPSegment->getTCPFlags() & TCP_DATA_FLAGS_FILTER);
                    checkAndLogMsg ("Entry::dequeueLocallyReceivedData", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: retrieved the whole packet with SEQ number %u, FLAGs %hhu and %u bytes long "
                                    "(%u before processing through the Connector Writer); %u bytes could still be retrieved\n",
                                    ui16ID, ui16RemoteID, _pTempTCPSegment->getSequenceNumber(),
                                    _pTempTCPSegment->getTCPFlags(), ui32BytesToReadFromPacket, _pTempTCPSegment->getItemLength(),
                                    ui16RequestedBytesNum - pDataToReturn->getItemLength());
                    _pAvailableData = NULL;
                }
            }
            _pMutexBuffer->unlock();
        }

        return pDataToReturn;
    }

    Connector * const Entry::setConnector (const Connector * const pConnector)
    {
        Connector * const pOldConnector = _connectors.pConnector;
        _connectors.pConnector = const_cast<Connector * const> (pConnector);

        // Setting _pMutexBuffer pointer
        if (_pMutexBuffer) {
            delete _pMutexBuffer;
        }
        if (_pTempTCPSegment) {
            delete _pTempTCPSegment;
        }
        _pMutexBuffer = new MutexBuffer (false, NetProxyApplicationParameters::MAX_ENTRY_BUF_SIZE, NetProxyApplicationParameters::MIN_ENTRY_BUF_SIZE);
        _pTempTCPSegment = new TCPSegment (0, _pMutexBuffer->getBufferSize(), const_cast<const unsigned char*> (_pMutexBuffer->getBuffer()));

        return pOldConnector;
    }

    Connection * const Entry::setConnection (Connection * const pConnection)
    {
        if (pConnection && !pConnection->isTCPEntryInTable (this)) {
            Connection * const pOldConnection = _connectors.pConnection;
            _connectors.pConnection = pConnection;
            if (pOldConnection) {
                pOldConnection->removeTCPEntry (this);
            }
            pConnection->addTCPEntry (this);

            return pOldConnection;
        }
        else {
            Connection * const pOldConnection = _connectors.pConnection;
            _connectors.pConnection = pConnection;

            return pOldConnection;
        }

        return NULL;
    }

    int Entry::removePacketsFromUDPTransmissionQueue (void) const
    {
        static ConnectionManager * const P_CONNECTION_MANAGER = ConnectionManager::getConnectionManagerInstance();
        static Connector * const pConnector =  P_CONNECTION_MANAGER->getConnectorForType (CT_UDP);
        if (pConnector) {
            UDPConnector * const pUDPConnector = dynamic_cast<UDPConnector*> (pConnector);
            if (!pUDPConnector) {
                checkAndLogMsg ("Entry::removePacketsFromUDPTransmissionQueue", Logger::L_Warning,
                                "L%hu-R%hu: error performing dynamic cast on connector %s to UDPConnector\n",
                                ui16ID, ui16RemoteID, pConnector->getConnectorTypeAsString());
                return -1;
            }

            return pUDPConnector->removeTCPTypePacketsFromTransmissionQueue (ui16ID, ui16RemoteID);
        }

        // Connector not instantiated
        return 0;
    }

}
