#ifndef INCL_PACKET_WRAPPER_H
#define INCL_PACKET_WRAPPER_H

/*
 * PacketWrapper.h
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
 *
 * PacketWrapper class is used to maintain meta information about packets that are being sent or have been received
 * While awaiting transmission, the following information is used:
 *     i64EnqueueTime - the time in milliseconds when the packet was enqueued for transmission
 *     i64LastIOTime - the time in milliseconds when the packet was last transmitted over the wire
 *     ui8Priority - the priority of the packet - 0 being the lowest and 255 being the highest priority
 *     ui32RetryTimeout - the maximum length of time in milliseconds for which this packet will be kept in the
 *                        UnacknowledgedPacketQueue awaiting acknowledgement
 *                        If this length of time is exceeded, the packet is dropped from the queue and will never be
 *                        retransmitted again
 *     ui32RetransmitTimeout - the length of time in milliseconds the transmitter should wait to receive an
 *                             acknowledgement for the packet before retransmitting the packet
 *
 * After a packet has been received, the following information is used:
 *     i64LastIOTime - the time in milliseconds when the packet was received over the wire
 * If a packet has been cancelled, the following information is used:
 *     ui32CancelledSequenceNum - the sequence number of the packet that was cancelled
 *     i64LastIOTime - the time in milliseconds when the cancelled packet notification was received over the wire
 */

#include "FTypes.h"
#include "NLFLib.h"

#include <stddef.h>


class Packet;

class PacketWrapper
{
    public:
        PacketWrapper (Packet *pPacket, int64 i64LastIOTime);
        PacketWrapper (uint32 ui32CancelledSequenceNum, int64 i64LastIOTime);
        PacketWrapper (Packet *pPacket, int64 i64LastIOTime, uint8 ui8Priority, uint32 ui32MessageTSN, uint32 ui32RetryTimeout, uint32 ui32RetransmitTimeout);
        Packet * getPacket (void);
        uint32 getSequenceNum (void);
        int64 getEnqueueTime (void);
        void setEnqueueTime (int64 i64EnqueueTime);
        int64 getLastIOTime (void);
        void setLastIOTime (int64 i64LastIOTime);
        uint8 getPriority (void);
        void setPriority (uint8 ui8Priority);
        uint32 getMessageTSN (void);
        void setMessageTSN (uint32 ui32MessageTSN);
        uint32 getRetryTimeout (void);
        void setRetryTimeout (uint32 ui32RetryTimeout);
        uint32 getRetransmitTimeout (void);
        void setRetransmitTimeout (uint32 ui32RetransmitTimeout);
        uint16 getRetransmitCount (void);
        void resetRetransmitCount (void);
        void incrementRetransmitCount (void);
        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

    private:
        Packet *_pPacket;
        uint32 _ui32CancelledSequenceNum;
        int64 _i64EnqueueTime;
        int64 _i64LastIOTime;
        uint8 _ui8Priority;
        uint32 _ui32MessageTSN;   // Used to keep fragments of the same message together
        uint32 _ui32RetryTimeout;
        uint32 _ui32RetransmitTimeout;
        uint16 _ui16RetransmitCount;
};

inline PacketWrapper::PacketWrapper (Packet *pPacket, int64 i64LastIOTime)
{
    _pPacket = pPacket;
    _ui32CancelledSequenceNum = 0;
    _i64EnqueueTime = 0;
    _i64LastIOTime = i64LastIOTime;
    _ui8Priority = 0;
    _ui32MessageTSN = 0;
    _ui32RetryTimeout = 0;
    _ui32RetransmitTimeout = 0;
    _ui16RetransmitCount = 0;
}

inline PacketWrapper::PacketWrapper (uint32 ui32CancelledSequenceNum, int64 i64LastIOTime)
{
    _pPacket = NULL;
    _ui32CancelledSequenceNum = ui32CancelledSequenceNum;
    _i64EnqueueTime = 0;
    _i64LastIOTime = i64LastIOTime;
    _ui8Priority = 0;
    _ui32MessageTSN = 0;
    _ui32RetryTimeout = 0;
    _ui32RetransmitTimeout = 0;
    _ui16RetransmitCount = 0;
}

inline PacketWrapper::PacketWrapper (Packet *pPacket, int64 i64LastIOTime, uint8 ui8Priority, uint32 ui32MessageTSN, uint32 ui32RetryTimeout, uint32 ui32RetransmitTimeout)
{
    _pPacket = pPacket;
    _ui32CancelledSequenceNum = 0;
    _i64EnqueueTime = 0;
    _i64LastIOTime = i64LastIOTime;
    _ui8Priority = ui8Priority;
    _ui32MessageTSN = ui32MessageTSN;
    _ui32RetryTimeout = ui32RetryTimeout;
    _ui32RetransmitTimeout = ui32RetransmitTimeout;
    _ui16RetransmitCount = 0;
}

inline Packet * PacketWrapper::getPacket (void)
{
    return _pPacket;
}

inline uint32 PacketWrapper::getSequenceNum (void)
{
    if (_pPacket) {
        return _pPacket->getSequenceNum();
    }
    else {
        return _ui32CancelledSequenceNum;
    }
}

inline int64 PacketWrapper::getEnqueueTime (void)
{
    return _i64EnqueueTime;
}

inline void PacketWrapper::setEnqueueTime (int64 i64EnqueueTime)
{
    _i64EnqueueTime = i64EnqueueTime;
}

inline int64 PacketWrapper::getLastIOTime (void)
{
    return _i64LastIOTime;
}

inline void PacketWrapper::setLastIOTime (int64 i64LastIOTime)
{
    _i64LastIOTime = i64LastIOTime;
}

inline uint8 PacketWrapper::getPriority (void)
{
    return _ui8Priority;
}

inline void PacketWrapper::setPriority (uint8 ui8Priority)
{
    _ui8Priority = ui8Priority;
}

inline uint32 PacketWrapper::getMessageTSN (void)
{
    return _ui32MessageTSN;
}

inline void PacketWrapper::setMessageTSN (uint32 ui32MessageTSN)
{
    _ui32MessageTSN = ui32MessageTSN;
}

inline uint32 PacketWrapper::getRetryTimeout (void)
{
    return _ui32RetryTimeout;
}

inline void PacketWrapper::setRetryTimeout (uint32 ui32RetryTimeout)
{
    _ui32RetryTimeout = ui32RetryTimeout;
}

inline uint32 PacketWrapper::getRetransmitTimeout (void)
{
    return _ui32RetransmitTimeout;
}

inline void PacketWrapper::setRetransmitTimeout (uint32 ui32RetransmitTimeout)
{
    _ui32RetransmitTimeout = ui32RetransmitTimeout;
}

inline uint16 PacketWrapper::getRetransmitCount (void)
{
	return _ui16RetransmitCount;
}

inline void PacketWrapper::resetRetransmitCount (void)
{
    _ui16RetransmitCount = 0;
}

inline void PacketWrapper::incrementRetransmitCount (void)
{
	_ui16RetransmitCount++;
}

inline int PacketWrapper::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    //Data from _pPacket
    _pPacket->freeze (objectFreezer);
    
    objectFreezer.putUInt32 (_ui32CancelledSequenceNum);
    // We need to freeze _i64EnqueueTime and _i64LastIOTime
    // Since it is a time we freeze the time elapsed from these times to now
    // and during the defrost we compute a time consistent with the time in the new node.
    // We can use a uint32
    // Anyway this time will not be accurate since from freeze to defrost some time will elapse
    uint32 ui32ElapsedTime;
    ui32ElapsedTime = (uint32) (NOMADSUtil::getTimeInMilliseconds() - _i64EnqueueTime);
    objectFreezer.putUInt32 (ui32ElapsedTime);
    ui32ElapsedTime = (uint32) (NOMADSUtil::getTimeInMilliseconds() - _i64LastIOTime);
    objectFreezer.putUInt32 (ui32ElapsedTime);
    
    objectFreezer.putUInt16 (_ui8Priority); // putUInt8 does not exist
    objectFreezer.putUInt32 (_ui32MessageTSN);
    objectFreezer.putUInt32 (_ui32RetryTimeout);
    objectFreezer.putUInt32 (_ui32RetransmitTimeout);
    objectFreezer.putUInt16 (_ui16RetransmitCount);
    
/*    printf ("_ui32CancelledSequenceNum %lu\n", _ui32CancelledSequenceNum);
    printf ("_ui8Priority %d\n", (int) _ui8Priority);
    printf ("_ui32RetryTimeout %lu\n", _ui32RetryTimeout);
    printf ("_ui32RetransmitTimeout %lu\n", _ui32RetransmitTimeout);
    printf ("_ui16RetransmitCount %d\n", (int)_ui16RetransmitCount );*/

    return 0;
}

inline int PacketWrapper::defrost (NOMADSUtil::ObjectDefroster &objectDefroster)
{
    // Recontract the object Packet
    Packet *pPacket = new Packet (objectDefroster);
    _pPacket = pPacket;
    
    objectDefroster >> _ui32CancelledSequenceNum;
    uint32 ui32ElapsedTime;
    objectDefroster >> ui32ElapsedTime;
    _i64EnqueueTime = NOMADSUtil::getTimeInMilliseconds() - (int64) ui32ElapsedTime;
    objectDefroster >> ui32ElapsedTime;
    _i64LastIOTime = NOMADSUtil::getTimeInMilliseconds() - (int64) ui32ElapsedTime;
    
    uint16 ui16Priority;
    objectDefroster >> ui16Priority;
    _ui8Priority = (uint8) ui16Priority;
    objectDefroster >> _ui32MessageTSN;
    objectDefroster >> _ui32RetryTimeout;
    objectDefroster >> _ui32RetransmitTimeout;
    objectDefroster >> _ui16RetransmitCount;
    
/*    printf ("_ui32CancelledSequenceNum %lu\n", _ui32CancelledSequenceNum);
    printf ("_ui8Priority %d\n", (int) _ui8Priority);
    printf ("_ui32RetryTimeout %lu\n", _ui32RetryTimeout);
    printf ("_ui32RetransmitTimeout %lu\n", _ui32RetransmitTimeout);
    printf ("_ui16RetransmitCount %d\n", (int)_ui16RetransmitCount );*/
    
    return 0;
}

#endif   // #ifndef INCL_PACKET_WRAPPER_H
