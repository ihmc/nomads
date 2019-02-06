#ifndef INCL_TRANSMITTER_H
#define INCL_TRANSMITTER_H

/*
 * Transmitter.h
 *
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

#include "PendingPacketQueue.h"
#include "UnacknowledgedPacketQueue.h"
#include "CongestionController.h"
#include "TransmissionRateModulation.h"

#include "ConditionVariable.h"
#include "Mutex.h"
#include "Thread.h"
#include "TimeIntervalAverage.h"
#include "NLFLib.h"

//#include <stdio.h>

class CommInterface;
class Mocket;
class Receiver;

namespace NOMADSUtil
{
    class UDPDatagramSocket;
    class Mutex;
}

class Transmitter : public NOMADSUtil::Thread
{
    public:
        Transmitter (Mocket *pMocket, bool bEnableXMitLogging = false);
        ~Transmitter (void);

        void enableTransmitLogging (bool bEnableXMitLogging);

        int setRemoteWindowSize (uint32 ui32RemoteWindowSize);

        // Sets the transmit rate limit, which is specified in bytes per second
        // A value of 0 indicates no limit
        int setTransmitRateLimit (uint32 ui32TransmitRateLimit);

        // Returns the number of bytes that can be transmitted with a call to send() without
        // blocking the caller
        uint32 getSpaceAvailable (void);

        // Enqueues the specified data for transmission using the specified reliability and sequencing requirements
        // The tag identifies the type of the packet and the priority indicates the priority for the packet
        // The enqueue timeout indicates the length of time in milliseconds for which the method will wait
        //     if there is no room in the outgoing buffer (a zero value indicates wait forever)
        // The retry timeout indicates the length of time for which the transmitter will retransmit the packet
        //     to ensure successful delivery. By default this is zero which indicates no limit.
        // Returns 0 if successful or a negative value in case of error
        int send (bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
                  uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);

        // Variable argument version of send (to handle a gather write)
        // Note - Need two copies of va_list because the method needs to iterate the first time to count the number of
        // bytes to be sent and then iterate a second time to actually write the data
        int gsend (bool bReliable, bool bSequenced, uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout,
                   const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2);

        // See comments in Mocket
        int cancel (bool bReliable, bool bSequenced, uint16 ui16TagId, uint8 * pui8HigherPriority = nullptr);

        void run (void);

        int processSAckChunk (SAckChunkAccessor sackChunkAccessor);

        int processTimestampChunk (TimestampChunkAccessor tsChunkAccessor);

        int processTimestampAckChunk (TimestampAckChunkAccessor tsaChunkAccessor);

        // Request that a timestamp be sent, which will trigger the other side to send
        // a timestamp ack upon which the RTT will be measured
        int requestTimestampTransmission (void);

        // Request that SAck information be transmitted at the next possible time
        int requestSAckTransmission (void);

        // Check if we need to reduce the sending rate and in that case add a bandwidth limit
        int enforceCongestionControl (void);

        // Activates Bandwidth estimation
        int setBandwidthEstimationActive (uint16 ui16InitialAssumedBandwidth);

        // Activates Congestion Control, which implies the activation of bandwidth estimation
        int setCongestionControlActive (const char *pszCongestionControl);

        // Set the start time of the shutdown process. Called from Mocket::close()
        void setShutdownStartTime (void);

        // Called from Mocket::suspend
        bool waitForFlush (void);

        // Called from Mocket::suspend
        bool suspend (void);

        // Check and resolve simultaneous suspensions and process the simpleSuspend packet
        void processSimpleSuspendPacket (SimpleSuspendChunkAccessor simpleSuspendChunkAccessor);

        void processSimpleSuspendAckPacket (SimpleSuspendAckChunkAccessor simpleSuspendAckChunkAccessor);

        // Called when a suspend packet is received.
        // Check and resolve simultaneous suspensions and process the suspend packet
        void processSuspendPacket (SuspendChunkAccessor suspendChunkAccessor);

        // Extract, decrypt and save nonce (UUID) and Ks
        void processSuspendAckPacket (SuspendAckChunkAccessor suspendAckChunkAccessor);

        // Extract, decrypt and check the nonce (UUID), IP and port
        // Go on with resume if the nonce is correct, abort otherwise
        void processResumePacket (ResumeChunkAccessor resumeChunkAccessor, uint32 ui32NewRemoteAddress, uint16 ui16NewRemotePort);

        // Extract, decrypt and check the nonce (UUID), IP and port
        // Reestablish connection if all the values are correct, abort otherwise
        void processReEstablishPacket (ReEstablishChunkAccessor reEstablishChunkAccessor, uint32 ui32NewRemoteAddress, uint16 ui16NewRemotePort);

    private:
        // An internal class that is used to keep track of resource limits
        struct ResourceLimits
        {
            ResourceLimits (void);
            void reset (void);
            uint32 ui32RateLimit;                // Specified in bytes/sec
            uint32 ui32RateLimitInterval;        // The enforcement interval in milliseconds
            uint32 ui32BytesPerInterval;         // Number of bytes that are allowed to be written in one interval;
                                                 // computed based on the _rateLimit and the _rateLimitInterval
            int64 i64IntervalStartTime;          // Time in milliseconds when interval started
            uint32 ui32BytesWrittenInInterval;   // The number of bytes written so far in this interval
            private:
                friend class Transmitter;
                int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
                int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);
        };

    private:
        // NOTE: The assumption for all the private functions below is that the calling thread
        // has already obtained a lock on the mutex _m

        friend class Mocket;
        friend class CongestionController;
        friend class TransmissionRateModulation;


        // Check to see if there are packets in the pending packet queue that need to be processed
        // Transmits at the most one packet
        // Returns 0 if no packets were sent, 1 if a packet was sent, or a negative value in case of error
        int processPendingPacketQueue (void);

        // Check to see if there are packets in the unacknowledged packet queues that need to be retransmitted due to a timeout
        // Transmits at the most three packets - one from each of the three queues
        // Returns 0 if no packets were sent, a positive value indicating a count of the number of packets sent, or a negative value in case of error
        int processUnacknowledgedPacketQueues (void);

        // Assigns the next control sequence number to this packet, inserts the packet
        // into the unacknowledged packet queue for control packets, and transmits the packet
        // NOTE: The packet is handled right away and not enqueued into the pending packet queue
        // NOTE: Even though the packet is enqueued into the unacknowledged packet queue, the queued data size is not incremented
        //       and the remote window size is not checked
        // Returns 0 if successful and a negative value in case of error
        // NOTE: The caller should not delete pPacket since it will be deleted after the packet is acknowledged
        int sendControlPacket (Packet *pPacket, uint8 ui8Priority);

        int sendHeartbeatPacket (void);
        int sendSAckPacket (void);
        int sendCancelledTSNPacket (void);
        int sendShutdownPacket (void);
        int sendShutdownAckPacket (void);
        int sendShutdownCompletePacket (void);
        int sendSimpleSuspendPacket (void);
        int sendSimpleSuspendAckPacket (void);
        int sendSuspendPacket (void);
        int sendSuspendAckPacket (void);
        int sendResumeAckPacket (void);
        int sendReEstablishAckPacket (void);

        // When a resume message is received this cause to go in ESTABLISHED state,
        // so we need to force mocket to send a resumeAck message
        int requestResumeAckTransmission (void);

        // When a reEstablish message is received we need to force mocket to send a reEstablishAck message
        int requestReEstablishAckTransmission (void);

        // When a suspend is received this cause to go in SUSPEND_RECEIVED state
        // and in this state we want to send a suspend_ack only if we receive a suspend
        // message because this means that the first one was lost.
        // We also don't want to reply to other kind of messages because we are in a suspended state
        int requestSuspendAckTransmission (void);
        int requestSimpleSuspendAckTransmission (void);

        // Appends piggyback information if possible and then transmits the packet by calling transmitPacket()
        // Piggyback information includes SAck and Cancelled chunks
        // The piggyback information is removed after the packet is successfully transmitted
        // If the SAck information is sent, _i64LastSAckTransmitTime is updated to reflect the current time
        // If the Cancelled TSN information is sent, _i64LastCancelledTSNTransmitTime is updated to reflect the current time
        // Returns 0 if successful or a negative value if the transmitPacket() fails or the piggyback chunks were
        //     appended but could not be removed
        // NOTE: Does not return an error if the piggyback chunks could not be appended
        // NOTE: The purpose argument is optional and is only used for logging
        int appendPiggybackDataAndTransmitPacket (Packet *pPacket, const char *pszPurpose);

        // Transmits the specified packet over the datagram socket to the remote endpoint
        // The window size is updated in the packet by querying the receiver before the packet is transmitted
        // After a successful transmission, _i64LastTransmitTime is updated to reflect the current time
        // NOTE: The purpose argument is optional and is only used for logging
        int transmitPacket (Packet *pPacket, const char *pszPurpose);

        uint32 getRetransmissionTimeout (void);

        int computeAckBasedRTT (uint32 ui32MinAckTime);
        int computeTimestampBasedRTT (int64 i64Timestamp);

        // Returns the BandwidthEstimator, or nullptr if bandwidth estimation was not activated
        BandwidthEstimator * getBandwidthEstimator (void);

        // Methods used by congestion control mechanisms
        uint16 getNumberOfAcknowledgedPackets (void);
        uint32 getTransmitRateLimit (void);

        bool allowedToSend (void);

        int resetSRTT (void);
        int resetUnackPacketsRetransmitTimeoutRetransmitCount (uint32 ui32RetransmitTO);

        void notify (void);

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

    private:
        NOMADSUtil::Mutex _m;
        NOMADSUtil::ConditionVariable _cv;
        NOMADSUtil::Mutex _mSend;
        NOMADSUtil::Mutex _mStat;

        Mocket *_pMocket;
        CommInterface *_pCommInterface;
        uint32 _ui32RemoteAddress;
        uint16 _ui16RemotePort;

        FILE *_filePacketXMitLog;
        int64 _i64LogStartTime;
        int64 _i64LastXMitLogTime;

        uint32 _ui32RemoteWindowSize;       // guarded by _lckRemoteWindowSize
        NOMADSUtil::Mutex _lckRemoteWindowSize;

        uint32 _ui32MessageTSN;             // Used to keep fragments in the Pending Packet Queue together
        bool _bSendingFragmentedMsg;        // Used to avoid to send a new msg when fragments of an old one are missing
        uint32 _ui32SendingMessageTSN;      // Used as a double check when sending fragments

        uint32 _ui32ControlTSN;
        uint32 _ui32ReliableSequencedTSN;
        uint32 _ui32UnreliableSequencedTSN;
        uint32 _ui32ReliableUnsequencedID;
        uint32 _ui32UnreliableUnsequencedID;
        int64 _i64LastTransmitTime;
        int64 _i64LastSAckTransmitTime;
        int64 _i64LastCancelledTSNTransmitTime;
        bool _bSendSAckInformationNow;

        // The following four variables are used to handle using timestamps for estimating the RTT
        bool _bSendTimestamp;
        bool _bSendTimestampAck;
        int64 _i64Timestamp;
        int64 _i64TimestampReceiveTime;

        // Condition variable and mutex for flushing data
        NOMADSUtil::ConditionVariable _cvFlushData;
        NOMADSUtil::Mutex _mFlushData;
        // Condition variable and mutex for waiting suspend_ack
        NOMADSUtil::ConditionVariable _cvSuspend;
        NOMADSUtil::Mutex _mSuspend;

        // This variable is used to force mocket to send a resumeAck
        bool _bSendResumeAck;

        // This variable is used to force mocket to send a reEstablishAck
        bool _bSendReEstablishAck;

        // This variable is used to force mocket to send a simpleSuspendAck
        bool _bSendSimpleSuspendAck;

        // This variable is used to force mocket to send a suspendAck
        bool _bSendSuspendAck;

        int64 _i64ShutdownStartTime;        // Time when a shutdown was requested (either by the local side or the remote side)
                                            // Used in conjunction with the linger time to determine how long to wait for data to be flushed

        float _fSRTT;                       // Contains the smoothed RTT measure for this connection
        int64 _i64LastRTTEstimationTime;
        int64 _i64LastStatUpdateTime;

        uint16 _ui16NumberOfAcknowledgedPackets;

        int64 _i64NextTimeToTransmit;

        // Receiver side bandwidth estimation
        int64 _i64LastRecTimeTimestamp;
        uint32 _ui32RecSideBytesReceived;

        PendingPacketQueue _pendingPacketQueue;
        UnacknowledgedPacketQueue _upqControlPackets;
        UnacknowledgedPacketQueue _upqReliableSequencedPackets;
        UnacknowledgedPacketQueue _upqReliableUnsequencedPackets;

        ResourceLimits _resLimits;

        CongestionControl *_pCongestionControl;
        BandwidthEstimator *_pBandwidthEstimator;

        // Lists used in fast retransmit
        DLList<uint32> *_pFastRetransmitControlPackets;
        DLList<uint32> *_pFastRetransmitReliableSequencedPackets;
        DLList<uint32> *_pFastRetransmitReliableUnsequencedPackets;

        TimeIntervalAverage<uint32> *_pByteSentPerInterval;

        // This is 2^32. It is used to check if the variable that contains the
        // number of bytes received so far from the receiver wrapped around
        static const uint32 UINT32_MAX_VALUE = 0xFFFFFFFFUL;
};

inline void Transmitter::setShutdownStartTime (void)
{
    _i64ShutdownStartTime = NOMADSUtil::getTimeInMilliseconds();
}

inline int Transmitter::setRemoteWindowSize (uint32 ui32RemoteWindowSize)
{
    _lckRemoteWindowSize.lock();
    _ui32RemoteWindowSize = ui32RemoteWindowSize;
    _lckRemoteWindowSize.unlock();
    return 0;
}

inline uint32 Transmitter::getSpaceAvailable (void)
{
    return _pendingPacketQueue.getSpaceAvailable();
}

inline int Transmitter::requestTimestampTransmission (void)
{
    _bSendTimestamp = true;
    return 0;
}

inline int Transmitter::requestSAckTransmission (void)
{
    _bSendSAckInformationNow = true;
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
    return 0;
}

inline int Transmitter::requestResumeAckTransmission (void)
{
    _bSendResumeAck = true;
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
    return 0;
}

inline int Transmitter::requestReEstablishAckTransmission (void)
{
    _bSendReEstablishAck = true;
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
    return 0;
}

inline int Transmitter::requestSimpleSuspendAckTransmission (void)
{
    _bSendSimpleSuspendAck = true;
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
    return 0;
}

inline int Transmitter::requestSuspendAckTransmission (void)
{
    _bSendSuspendAck = true;
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
    return 0;
}

inline uint16 Transmitter::getNumberOfAcknowledgedPackets (void)
{
    return _ui16NumberOfAcknowledgedPackets;
}

inline uint32 Transmitter::getTransmitRateLimit (void)
{
    return _resLimits.ui32RateLimit;
}

inline void Transmitter::notify (void)
{
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
}

inline Transmitter::ResourceLimits::ResourceLimits (void)
{
    reset();
}

inline void Transmitter::ResourceLimits::reset (void)
{
    ui32RateLimit = 0;
    ui32RateLimitInterval = 0;
    ui32BytesPerInterval = 0;
    i64IntervalStartTime = 0;
    ui32BytesWrittenInInterval = 0;
}

inline int Transmitter::ResourceLimits::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    //objectFreezer.beginNewObject ("ResourceLimits");
    objectFreezer.putUInt32 (ui32RateLimit);
    objectFreezer.putUInt32 (ui32RateLimitInterval);
    objectFreezer.putUInt32 (ui32BytesPerInterval);
    // i64IntervalStartTime Time in milliseconds when interval started
    // we froze the time elapsed from i64IntervalStartTime
    uint32 ui32ElapsedTime = (uint32) (NOMADSUtil::getTimeInMilliseconds() - i64IntervalStartTime);
    objectFreezer.putUInt32 (ui32ElapsedTime);
    objectFreezer.putUInt32 (ui32BytesWrittenInInterval);

/*    printf ("ResourceLimits\n");
    printf ("ui32RateLimit %lu\n", ui32RateLimit);
    printf ("ui32RateLimitInterval %lu\n", ui32RateLimitInterval);
    printf ("ui32BytesPerInterval %lu\n", ui32BytesPerInterval);
    printf ("ui32BytesWrittenInInterval %lu\n", ui32BytesWrittenInInterval);*/

    //objectFreezer.endObject();
    return 0;
}

inline int Transmitter::ResourceLimits::defrost (NOMADSUtil::ObjectDefroster &objectDefroster)
{
    //objectDefroster.beginNewObject ("ResourceLimits");
    objectDefroster >> ui32RateLimit;
    objectDefroster >> ui32RateLimitInterval;
    objectDefroster >> ui32BytesPerInterval;
    uint32 ui32ElapsedTime;
    objectDefroster >> ui32ElapsedTime;
    i64IntervalStartTime = NOMADSUtil::getTimeInMilliseconds() - (int64) ui32ElapsedTime;
    objectDefroster >> ui32BytesWrittenInInterval;

/*    printf ("ResourceLimits\n");
    printf ("ui32RateLimit %lu\n", ui32RateLimit);
    printf ("ui32RateLimitInterval %lu\n", ui32RateLimitInterval);
    printf ("ui32BytesPerInterval %lu\n", ui32BytesPerInterval);
    printf ("ui32BytesWrittenInInterval %lu\n", ui32BytesWrittenInInterval);*/

    //return objectDefroster.endObject();
    return 0;
}

#endif   // #ifndef INCL_TRANSMITTER_H
