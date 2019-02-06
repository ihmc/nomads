/*
 * Entry.h
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
 *
 * Data structure that maintains all necessary information about
 * a single TCP connection that the NetProxy is remapping.
 */

#ifndef INCL_ENTRY_H
#define INCL_ENTRY_H

#include <limits>
#include <string>
#include <deque>
#include <mutex>

#include "FTypes.h"
#include "net/NetworkHeaders.h"

#include "ConfigurationParameters.h"
#include "ProxyMessages.h"
#include "CircularOrderedBuffer.h"

#ifdef max
#undef max
#endif


namespace ACMNetProxy
{
    class MutexBuffer;
    class TCPSegment;
    class ConnectorReader;
    class ConnectorWriter;
    class Connection;


    enum LocalState
    {
        TCTLS_LISTEN,           // 0
        TCTLS_SYN_RCVD,         // 1
        TCTLS_SYN_SENT,         // 2
        TCTLS_ESTABLISHED,      // 3
        TCTLS_CLOSE_WAIT,       // 4
        TCTLS_LAST_ACK,         // 5
        TCTLS_FIN_WAIT_1,       // 6
        TCTLS_FIN_WAIT_2,       // 7
        TCTLS_CLOSING,          // 8
        TCTLS_TIME_WAIT,        // 9
        TCTLS_CLOSED            // 10
    };


    enum RemoteState
    {
        TCTRS_Unknown,                      // 0
        TCTRS_WaitingConnEstablishment,     // 1
        TCTRS_ConnRequestSent,              // 2
        TCTRS_ConnRequestReceived,          // 3
        TCTRS_ConnEstablished,              // 4
        TCTRS_DisconnRequestSent,           // 5
        TCTRS_DisconnRequestReceived,       // 6
        TCTRS_Disconnected                  // 7
    };


    struct DataCompressors
    {
        DataCompressors (void);
        ~DataCompressors (void);

        void reset (void);

        ConnectorReader * pConnectorReader;
        ConnectorWriter * pConnectorWriter;
    };


    // Handles a single connection, both the local and the remote sides
    class Entry
    {
    public:
        Entry (void);
        ~Entry (void);

        bool operator== (const Entry & rEntry) const;

        // NOTE: For the next methods we assume that the caller has already locked the entry
        void clear (void);
        void reset (void);

        void prepareNewConnection (void);  // Generates outgoing SEQ number and set the local state to LISTEN
        uint32 ackOutgoingDataUpto (uint32 ui32AckNum);
        void updateOutgoingWindow (const NOMADSUtil::TCPHeader * const pTCPHeader);
        void calculateRTO (int64 rtt);
        const bool isRemoteConnectionFlushed (void) const;
        TCPSegment * const dequeueLocallyReceivedData (uint16 ui16RequestedBytesNum);

        bool isOutgoingBufferEmpty (void) const;
        bool areDataAvailableInTheIncomingBuffer (void);
        bool areThereHolesInTheIncomingDataBuffer (void) const;
        unsigned int getOutgoingReadyBytesCount (void) const;
        unsigned int getOutgoingTotalBytesCount (void) const;
        unsigned int getOutgoingBufferedBytesCount (void) const;
        unsigned int getOutgoingBufferRemainingSpace (void) const;
        uint16 getCurrentTCPWindowSize (void) const;
        double getOutgoingBufferRemainingSpacePercentage (void) const;
        void setNextExpectedInSeqNum (unsigned int uiNextExpectedInSeqNum);
        int insertTCPSegmentIntoOutgoingBuffer (TCPSegment * const pTCPSegment);
        const TCPSegment * const getLastOutgoingQueuedPacket (void);

        bool areThereUntransmittedPacketsInUDPTransmissionQueue (void) const;

        Connection * const getConnection (void) const;
        ConnectorReader * const getConnectorReader (void) const;
        ConnectorWriter * const getConnectorWriter (void) const;
        Connection * const setConnection (Connection * const pConnection);
        ConnectorReader * const setProperConnectorReader (ConnectorReader * const pNewConnectorReader);
        ConnectorWriter * const setProperConnectorWriter (ConnectorWriter * const pNewConnectorWriter);
        void resetConnectors (void);

        Protocol getProtocol (void) const;
        void setProtocol (Protocol protocol);

        std::mutex & getMutexRef (void) const;

        uint16 ui16ID;
        uint16 ui16RemoteID;
        uint32 ui32RemoteProxyUniqueID;
        NOMADSUtil::InetAddr iaLocalInterfaceAddr;
        NOMADSUtil::InetAddr iaRemoteProxyAddr;
        LocalState localState;
        RemoteState remoteState;
        uint32 ui32LocalIP;
        uint16 ui16LocalPort;
        uint32 ui32RemoteIP;
        uint16 ui16RemotePort;

        uint32 ui32StartingInSeqNum;
        uint32 ui32StartingOutSeqNum;
        uint32 ui32NextExpectedInSeqNum;                            // Expected SEQ number of the next incoming packet
        uint32 ui32LastACKedSeqNum;                                 // SEQ number of the last packet which was ACKed
        uint32 ui32LastAckSeqNum;
        int64 i64LastAckTime;
        uint32 ui32ReceivedDataSeqNum;
        uint32 ui32OutSeqNum;                                       // Next sequence number to be used when sending new data to the host
        uint16 ui16ReceiverWindowSize;
        uint16 ui16RTO;                                             // Retransmission TimeOut
        uint16 ui16SRTT;                                            // Smoothed Round Trip Time
        uint8 ui8RetransmissionAttempts;                            // Number of retransmitted packets sent
        int64 i64LastCalculatedRTOTime;                             // Time when RTO has been calculated
        int64 i64LocalActionTime;                                   // Time when a local action, such as a request to connect, was taken
        int64 i64RemoteActionTime;                                  // Time when a remote action, such as a request to connect, was taken
        int64 i64IdleTime;                                          // Last time the connection was detected as idle

        uint32 assignedPriority;
        uint32 currentPriority;
        TCPSegment * pTCPSegment;

        std::deque<ReceivedData> dqLocalHostOutBuf;                 // Queue of packets received from the remote application and ready to be sent to the local one

        static const uint32 STANDARD_MSL = 2 * 60 * 1000;           // Standard Maximum Segment Lifetime of 2 minutes (RFC 793)
        static const uint16 LB_RTO = 1 * 100;                       // Retransmission TimeOut Lower Bound of 100 milliseconds (in RFC 793 is 1 second)
        static const uint16 UB_RTO = 3 * 1000;                      // Retransmission TimeOut Upper Bound of 60 seconds (RFC 793)
        static const uint16 RTO_RECALCULATION_TIME = 1 * 1000;      // Time that has to pass before recalculating RTO (RFC 793)
        static const double ALPHA_RTO;                              // Alpha constant for RTO calculation (RFC 793)
        static const double BETA_RTO;                               // Beta constant for RTO calculation (RFC 793)


    private:
        void releaseMemory (void);

        int removePacketsFromUDPTransmissionQueue (void) const;

        Protocol _protocol;
        DataCompressors _dataCompressors;
        MutexBuffer * _pMutexBuffer;                                // Always nullptr, except when data is remapped over a UDP Connection
        TCPSegment * _pAvailableData;
        TCPSegment * _pTempTCPSegment;
        Connection * _pRemoteConnection;                            // The Connection to the remote NetProxy to which this entry is remapped

        CircularOrderedBuffer inQueue;                              // To handle the TCP window of incoming packets with support for out-of-order packets

        mutable std::mutex _mtx;

        static const uint16 MAX_TCP_WINDOW_SIZE = std::numeric_limits<uint16>::max();
    };


    inline DataCompressors::DataCompressors (void) :
        pConnectorReader{nullptr}, pConnectorWriter{nullptr}
    { }

    inline Entry::Entry (void) :
        pTCPSegment{nullptr}, _pMutexBuffer{nullptr}, _pAvailableData{nullptr}, _pTempTCPSegment{nullptr},
        _pRemoteConnection{nullptr}, inQueue{ui32NextExpectedInSeqNum, NetworkConfigurationSettings::TCP_WINDOW_SIZE}
    {
        clear();
    }

    inline bool Entry::operator== (const Entry &rEntry) const
    {
        return (ui16ID == rEntry.ui16ID) && (ui16RemoteID == rEntry.ui16RemoteID);
    }

    inline bool Entry::isOutgoingBufferEmpty (void) const
    {
        return (inQueue.isEmpty() && (_pAvailableData ? (_pAvailableData->getItemLength() == 0) : true));
    }

    inline bool Entry::areDataAvailableInTheIncomingBuffer (void)
    {
        return (inQueue.isDataReady() || (_pAvailableData ? (_pAvailableData->getItemLength() > 0) : false));
    }

    inline bool Entry::areThereHolesInTheIncomingDataBuffer (void) const
    {
        return inQueue.isDataOutOfOrder();
    }

    inline unsigned int Entry::getOutgoingReadyBytesCount (void) const
    {
        return (inQueue.getAvailableBytesCount() + (_pAvailableData ? _pAvailableData->getItemLength() : 0));
    }

    inline unsigned int Entry::getOutgoingTotalBytesCount (void) const
    {
        return (inQueue.getTotalBytesCount() + (_pAvailableData ? _pAvailableData->getItemLength() : 0));
    }

    inline unsigned int Entry::getOutgoingBufferedBytesCount (void) const
    {
        return (_pAvailableData ? _pAvailableData->getItemLength() : 0);
    }

    inline unsigned int Entry::getOutgoingBufferRemainingSpace (void) const
    {
        return inQueue.getRemainingSpace();
    }

    inline uint16 Entry::getCurrentTCPWindowSize(void) const
    {
        return static_cast<uint16> ((inQueue.getCurrentBufferSize() > MAX_TCP_WINDOW_SIZE) ?
                                    MAX_TCP_WINDOW_SIZE : inQueue.getCurrentBufferSize());
    }

    inline double Entry::getOutgoingBufferRemainingSpacePercentage (void) const
    {
        return inQueue.getFreeSpacePercentage();
    }

    inline void Entry::setNextExpectedInSeqNum (unsigned int uiNextExpectedInSeqNum)
    {
        inQueue.setBeginningSequenceNumber (uiNextExpectedInSeqNum);
    }

    inline int Entry::insertTCPSegmentIntoOutgoingBuffer (TCPSegment * const pTCPSegment)
    {
        return pTCPSegment ? inQueue.insertData (pTCPSegment) : -1;
    }

    inline const TCPSegment * const Entry::getLastOutgoingQueuedPacket (void)
    {
        return inQueue.getLastSegment() ? inQueue.getLastSegment() : _pAvailableData;
    }

    inline Connection * const Entry::getConnection (void) const
    {
        return _pRemoteConnection;
    }

    inline ConnectorReader * const Entry::getConnectorReader (void) const
    {
        return _dataCompressors.pConnectorReader;
    }

    inline ConnectorWriter * const Entry::getConnectorWriter (void) const
    {
        return _dataCompressors.pConnectorWriter;
    }

    inline Connection * const Entry::setConnection (Connection * const pConnection)
    {
        Connection * const pOldConnection = _pRemoteConnection;
        _pRemoteConnection = pConnection;

        return pOldConnection;
    }

    inline ConnectorReader * const Entry::setProperConnectorReader (ConnectorReader * const pNewConnectorReader)
    {
        ConnectorReader * const pOldConnectorReader = _dataCompressors.pConnectorReader;
        _dataCompressors.pConnectorReader = pNewConnectorReader;
        return pOldConnectorReader;
    }

    inline ConnectorWriter * const Entry::setProperConnectorWriter (ConnectorWriter * const pNewConnectorWriter)
    {
        ConnectorWriter * const pOldConnectorWriter = _dataCompressors.pConnectorWriter;
        _dataCompressors.pConnectorWriter = pNewConnectorWriter;
        return pOldConnectorWriter;
    }

    inline void Entry::resetConnectors (void)
    {
        _dataCompressors.reset();
    }

    inline Protocol Entry::getProtocol (void) const
    {
        return _protocol;
    }

    inline void Entry::setProtocol (Protocol protocol)
    {
        _protocol = protocol;
    }

    inline std::mutex & Entry::getMutexRef (void) const
    {
        return _mtx;
    }
}

#endif  // INCL_ENTRY_H
