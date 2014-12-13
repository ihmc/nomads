/*
 * Entry.h
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
 *
 * Data structure that maintains all necessary information about
 * a single TCP connection that the NetProxy is remapping.
 */

#ifndef INCL_ENTRY_H
#define INCL_ENTRY_H

#include "FTypes.h"
#include "net/NetworkHeaders.h"
#include "PtrQueue.h"
#include "Mutex.h"

#include "ConfigurationParameters.h"
#include "ProxyMessages.h"
#include "TCPSegment.h"
#include "MutexBuffer.h"
#include "CircularOrderedBuffer.h"
#include "ConnectorWriter.h"


namespace ACMNetProxy
{
    class Connector;
    class Connection;
    class ConnectorReader;
    class CompressionSetting;
    class Entry;


    enum LocalStatus
    {
        TCTLS_Unused,       // = 0
        TCTLS_LISTEN,       // = 1
        TCTLS_SYN_RCVD,     // = 2
        TCTLS_SYN_SENT,     // = 3
        TCTLS_ESTABLISHED,  // = 4
        TCTLS_CLOSE_WAIT,   // = 5
        TCTLS_LAST_ACK,     // = 6
        TCTLS_FIN_WAIT_1,   // = 7
        TCTLS_FIN_WAIT_2,   // = 8
        TCTLS_CLOSING,      // = 9
        TCTLS_TIME_WAIT,    // = 10
        TCTLS_CLOSED        // = 11
    };


    enum RemoteStatus
    {
        TCTRS_Unknown,
        TCTRS_WaitingConnEstablishment,
        TCTRS_ConnRequested,
        TCTRS_ConnEstablished,
        TCTRS_DisconnRequestSent,
        TCTRS_DisconnRequestReceived,
        TCTRS_Disconnected
    };


    struct Connectors
    {
        Connectors (void);
        ~Connectors (void);

        void reset (Entry * const pEntry);

        Connector *pConnector;
        Connection *pConnection;
        ConnectorReader *pConnectorReader;
        ConnectorWriter *pConnectorWriter;
    };


    // Handles a single connection, both the local and the remote sides
    class Entry
    {
    public:
        Entry (void);
        ~Entry (void);

        bool operator== (const Entry &rEntry) const;

        // NOTE: For the next methods we assume that the caller has already locked the entry
        void clear (void);
        void reset (void);

        void prepareNewConnection (void);                                               // Generates outgoing SEQ number and set local status to LISTEN
        uint32 ackOutgoingDataUpto (uint32 ui32AckNum);
        void updateOutgoingWindow (const NOMADSUtil::TCPHeader *pTCPHeader);
        void calculateRTO (int64 rtt);
        const bool isRemoteConnectionFlushed (void) const;
        TCPSegment * const dequeueLocallyReceivedData (uint16 ui16RequestedBytesNum);

        bool isOutgoingBufferEmpty (void) const;
        bool isOutgoingDataReady (void);
        bool areThereHolesInOutgoingDataBuffer (void) const;
        unsigned int getOutgoingReadyBytesCount (void) const;
        unsigned int getOutgoingTotalBytesCount (void) const;
        unsigned int getOutgoingBufferedBytesCount (void) const;
        unsigned int getOutgoingBufferRemainingSpace (void) const;
        double getOutgoingBufferRemainingSpacePercentage (void) const;

        void setNextExpectedInSeqNum (unsigned int uiNextExpectedInSeqNum);
        int insertTCPSegmentIntoOutgoingBuffer (TCPSegment *pTCPSegment);
        const TCPSegment * const getLastOutgoingQueuedPacket (void);

        Connector * const getConnector (void) const;
        Connection * const getConnection (void) const;
        ConnectorReader * const getConnectorReader (void) const;
        ConnectorWriter * const getConnectorWriter (void) const;
        Connector * const setConnector (const Connector * const pConnector);
        Connection * const setConnection (Connection * const pConnection);
        ConnectorReader * const setProperConnectorReader (const ConnectorReader * const pNewConnectorReader);
        ConnectorWriter * const setProperConnectorWriter (const CompressionSetting * const pCompressionSetting);
        void resetConnectors (void);

        ProxyMessage::Protocol getProtocol (void) const;
        const NOMADSUtil::String & getMocketConfFile (void) const;
        void setProtocol (ProxyMessage::Protocol prot);
        void setMocketConfFile (const char * const pcConfigFile);
        void setMocketConfFile (const NOMADSUtil::String &configFile);

        int lock (void) const;
        int tryLock (void) const;
        int unlock (void) const;

        uint16 ui16ID;
        uint16 ui16RemoteID;
        uint32 ui32RemoteProxyUniqueID;
        NOMADSUtil::InetAddr remoteProxyAddr;
        LocalStatus localStatus;
        RemoteStatus remoteStatus;
        uint32 ui32LocalIP;
        uint16 ui16LocalPort;
        uint32 ui32RemoteIP;
        uint16 ui16RemotePort;

        uint32 ui32StartingInSeqNum;
        uint32 ui32StartingOutSeqNum;
        uint32 ui32NextExpectedInSeqNum;                        // Expected SEQ number of the next incoming packet
        uint32 ui32LastACKedSeqNum;                             // SEQ number of the last packet which was ACKed
        uint32 ui32LastAckSeqNum;
        int64 i64LastAckTime;
        uint32 ui32ReceivedDataSeqNum;
        uint32 ui32OutSeqNum;                                   // Next sequence number to be used when sending new data to the host
        uint16 ui16ReceiverWindowSize;
        uint16 ui16RTO;                                         // Retransmission TimeOut
        uint16 ui16SRTT;                                        // Smoothed Round Trip Time
        uint8 ui8RetransmissionAttempts;                        // Number of retransmitted packets sent
        int64 i64LastCalculatedRTOTime;                         // Time when RTO has been calculated
        int64 i64LocalActionTime;                               // Time when a local action, such as a request to connect, was taken
        int64 i64RemoteActionTime;                              // Time when a remote action, such as a request to connect, was taken
        int64 i64IdleTime;                                      // Last time the connection was detected as idle

        NOMADSUtil::PtrQueue<ReceivedData> outBuf;              // Packets received from local host and outgoing to the network

    private:
        friend class TCPConnTable;
        int removePacketsFromUDPTransmissionQueue (void) const;

        Connectors _connectors;
        ProxyMessage::Protocol _protocol;
        NOMADSUtil::String _sMocketConfFileName;
        MutexBuffer *_pMutexBuffer;                             // NULL except if the UDP connector is used to send data to remote proxy
        TCPSegment *_pAvailableData;
        TCPSegment *_pTempTCPSegment;

        CircularOrderedBuffer inQueue;                          // To handle TCP window (with support to out-of-order packets)

        mutable NOMADSUtil::Mutex _m;
    };


    inline Connectors::Connectors (void) :
        pConnector (NULL), pConnection (NULL), pConnectorReader (NULL), pConnectorWriter (NULL) { }

    inline Connectors::~Connectors (void)
    {
        reset (NULL);
    }

    inline bool Entry::operator== (const Entry &rEntry) const
    {
        return (ui16ID == rEntry.ui16ID) && (ui16RemoteID == rEntry.ui16RemoteID);
    }

    inline const bool Entry::isRemoteConnectionFlushed (void) const
    {
        return _connectors.pConnectorWriter->isFlushed() && (_pAvailableData ? _pAvailableData->getItemLength() == 0 : true);
    }

    inline Entry::Entry (void) :
        _pMutexBuffer (NULL), _pAvailableData (NULL), _pTempTCPSegment (NULL), inQueue (ui32NextExpectedInSeqNum, NetProxyApplicationParameters::TCP_WINDOW_SIZE)
    {
        clear();
    }

    inline bool Entry::isOutgoingBufferEmpty (void) const
    {
        return (inQueue.isEmpty() && (_pAvailableData ? (_pAvailableData->getItemLength() == 0) : true));
    }

    inline bool Entry::isOutgoingDataReady (void)
    {
        return (inQueue.isDataReady() || (_pAvailableData ? (_pAvailableData->getItemLength() > 0) : false));
    }

    inline bool Entry::areThereHolesInOutgoingDataBuffer (void) const
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

    inline double Entry::getOutgoingBufferRemainingSpacePercentage (void) const
    {
        return inQueue.getFreeSpacePercentage();
    }

    inline void Entry::setNextExpectedInSeqNum (unsigned int uiNextExpectedInSeqNum)
    {
        inQueue.setBeginningSequenceNumber (uiNextExpectedInSeqNum);
    }

    inline int Entry::insertTCPSegmentIntoOutgoingBuffer (TCPSegment *pTCPSegment)
    {
        return pTCPSegment ? inQueue.insertData (pTCPSegment) : -1;
    }

    inline const TCPSegment * const Entry::getLastOutgoingQueuedPacket (void)
    {
        return inQueue.getLastSegment() ? inQueue.getLastSegment() : _pAvailableData;
    }

    inline Connector * const Entry::getConnector (void) const
    {
        return _connectors.pConnector;
    }

    inline Connection * const Entry::getConnection (void) const
    {
        return _connectors.pConnection;
    }

    inline ConnectorReader * const Entry::getConnectorReader (void) const
    {
        return _connectors.pConnectorReader;
    }

    inline ConnectorWriter * const Entry::getConnectorWriter (void) const
    {
        return _connectors.pConnectorWriter;
    }

    inline ConnectorReader * const Entry::setProperConnectorReader (const ConnectorReader * const pNewConnectorReader)
    {
        ConnectorReader * const pOldConnectorReader = _connectors.pConnectorReader;
        _connectors.pConnectorReader = const_cast<ConnectorReader *> (pNewConnectorReader);
        return pOldConnectorReader;
    }

    inline ConnectorWriter * const Entry::setProperConnectorWriter (const CompressionSetting * const pCompressionSetting)
    {
        ConnectorWriter * const pOldConnectorWriter = _connectors.pConnectorWriter;
        _connectors.pConnectorWriter = ConnectorWriter::connectorWriterFactory (pCompressionSetting);
        return pOldConnectorWriter;
    }

    inline void Entry::resetConnectors (void)
    {
        _connectors.reset (this);
    }

    inline ProxyMessage::Protocol Entry::getProtocol (void) const
    {
        return _protocol;
    }

    inline const NOMADSUtil::String & Entry::getMocketConfFile (void) const
    {
        return _sMocketConfFileName;
    }

    inline void Entry::setProtocol (ProxyMessage::Protocol protocol)
    {
        _protocol = protocol;
    }

    inline void Entry::setMocketConfFile (const char * const pcConfigFile)
    {
        _sMocketConfFileName = NOMADSUtil::String (pcConfigFile);
    }

    inline void Entry::setMocketConfFile (const NOMADSUtil::String &configFile)
    {
        _sMocketConfFileName = configFile;
    }

    inline int Entry::lock (void) const
    {
        return _m.lock();
    }

    inline int Entry::tryLock (void) const
    {
        return _m.tryLock();
    }

    inline int Entry::unlock (void) const
    {
        return _m.unlock();
    }
}

#endif  // INCL_ENTRY_H
