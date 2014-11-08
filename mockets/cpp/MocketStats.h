#ifndef INCL_MOCKET_STATS_H
#define INCL_MOCKET_STATS_H

/*
 * MocketStats.h
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
 
 * MocketStats
 *
 * The class that contains statistics about an active mocket connection
 * Obtained by calling getStatistics() on a Mocket object.
 */

#include "DArray2.h"
#include "FTypes.h"
#include "Mutex.h"


class MocketStats
{
    public:
        virtual ~MocketStats (void);

        // MessageStats
        // Contains statistics about messages sent and received for the four different classes of service
        // Objects of this type are returned by getOverallMessageStatistics() and getMessageStatisticsForType()
        // method - see below
        class MessageStats {
            public:
                MessageStats (void);
                uint32 ui32SentReliableSequencedMsgs;
                uint32 ui32SentReliableUnsequencedMsgs;
                uint32 ui32SentUnreliableSequencedMsgs;
                uint32 ui32SentUnreliableUnsequencedMsgs;
                uint32 ui32ReceivedReliableSequencedMsgs;
                uint32 ui32ReceivedReliableUnsequencedMsgs;
                uint32 ui32ReceivedUnreliableSequencedMsgs;
                uint32 ui32ReceivedUnreliableUnsequencedMsgs;
                uint32 ui32CancelledPackets;
            private:
                friend class MocketStats;
                int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
                int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);
        };

        // Returns the number of retransmitted packets
        uint32 getRetransmitCount (void);

        // Returns the number of sent packets
        uint32 getSentPacketCount (void);

        // Returns the number of bytes transmitted
        uint32 getSentByteCount (void);

        // Returns the number of packets received
        uint32 getReceivedPacketCount (void);

        // Returns the number of bytes received
        uint32 getReceivedByteCount (void);

        // Returns the number of incoming packets that were discarded because they were duplicates
        uint32 getDuplicatedDiscardedPacketCount (void);

        // Returns the number of incoming packets that were discarded because there was no room to buffer them
        uint32 getNoRoomDiscardedPacketCount (void);

        // Returns the number of incoming packets that are discarded because a message was not reassembled
        // This occurs when reassembly of a message from packet fragments is abandoned due to a timeout
        // The packets discarded are the fragments of the message that were received
        uint32 getReassemblySkippedDiscardedPacketCount (void);

        // Returns the estimated round-trip-time in milliseconds
        float getEstimatedRTT (void);

        // Returns the estimated bandwidth in bytes per second
	// return value is -2.0 if bandwidth estimation was not activate
	// return value is -1.0 if bandwidth estimation is active but no estimate is available yet
        int32 getEstimatedBandwidth (void);

        void setEstimatedBandwidth (int32 dBandwidthEstimation);

        // Returns the size (in bytes) of the data that is enqueued in the pending packet queue awaiting transmission
        uint32 getPendingDataSize (void);

        // Returns the number of packets in the pending packet queue awaiting retransmission
        uint32 getPendingPacketQueueSize (void);

        // Returns the size (in bytes) of the data that is in the reliable, sequenced packet queue awaiting acknowledgement
        uint32 getReliableSequencedDataSize (void);

        // Returns the number of packets in the reliable, sequenced packet queue awaiting acknowledgement
        uint32 getReliableSequencedPacketQueueSize (void);

        // Returns the size (in bytes) of the data that is in the reliable, unsequenced packet queue awaiting acknowledgement
        uint32 getReliableUnsequencedDataSize (void);

        // Returns the number of packets in the reliable, unsequenced packet queue awaiting acknowledgement
        uint32 getReliableUnsequencedPacketQueueSize (void);

        // Returns statistics at the level of messages (instead of packets)
        // Includes cumulative numbers for messages of all types
        MessageStats * getOverallMessageStatistics (void);

        // Returns the highest tag value that has been used by the application
        // for identifying a particular message type
        // (Useful when iterating and obtaining statistics per type of message)
        uint16 getHighestTag (void);

        // Returns true if the application has sent any messages so far using the
        // specified tag value to identify a message
        // (Useful when iterating and obtaining statistics per type of message)
        bool isTagUsed (uint16 ui16Tag);

        // Returns the statistics at the level of messages (instead of packets)
        // for messages of the specified type
        MessageStats * getMessageStatisticsForType (uint16 ui16Tag);

    protected:
        MocketStats (void);
        void lock();
        void unlock();

    private:
        friend class Mocket;
        friend class PacketProcessor;
        friend class Receiver;
        friend class StreamMocket;
        friend class Transmitter;
        friend class CongestionController;
        friend class BandwidthEstimator;
        uint32 _ui32Retransmits;
        uint32 _ui32SentPackets;
        uint32 _ui32SentBytes;
        uint32 _ui32ReceivedPackets;
        uint32 _ui32ReceivedBytes;
        uint32 _ui32DuplicatedDiscardedPackets;
        uint32 _ui32NoRoomDiscardedPackets;
        uint32 _ui32ReassemblySkippedDiscardedPackets;
        float _fSRTT;
        uint32 _ui32PendingDataSize;
        uint32 _ui32PendingPacketQueueSize;
        uint32 _ui32ReliableSequencedDataSize;
        uint32 _ui32ReliableSequencedPacketQueueSize;
        uint32 _ui32ReliableUnsequencedDataSize;
        uint32 _ui32ReliableUnsequencedPacketQueueSize;
        MessageStats _globalMessageStats;
        NOMADSUtil::DArray2<MessageStats> _perTypeMessageStats;
        NOMADSUtil::Mutex _m;    // NOTE: Currently, the mutex is not used when reading or when just incrementing a value

	    int32 _i32BandwidthEstimation;

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);
};

inline MocketStats::MocketStats (void)
{
    _ui32Retransmits = 0;
    _ui32SentPackets = 0;
    _ui32SentBytes = 0;
    _ui32ReceivedPackets = 0;
    _ui32ReceivedBytes = 0;
    _ui32DuplicatedDiscardedPackets = 0;
    _ui32NoRoomDiscardedPackets = 0;
    _ui32ReassemblySkippedDiscardedPackets = 0;
    _fSRTT = -1.0f;
    _ui32PendingDataSize = 0;
    _ui32PendingPacketQueueSize = 0;
    _ui32ReliableSequencedDataSize = 0;
    _ui32ReliableSequencedPacketQueueSize = 0;
    _ui32ReliableUnsequencedDataSize = 0;
    _ui32ReliableUnsequencedPacketQueueSize = 0;

    _i32BandwidthEstimation = -2;
}

inline MocketStats::~MocketStats (void)
{
}

inline uint32 MocketStats::getRetransmitCount (void)
{
    return _ui32Retransmits;
}

inline uint32 MocketStats::getSentPacketCount (void)
{
    return _ui32SentPackets;
}

inline uint32 MocketStats::getSentByteCount (void)
{
    return _ui32SentBytes;
}

inline uint32 MocketStats::getReceivedPacketCount (void)
{
    return _ui32ReceivedPackets;
}

inline uint32 MocketStats::getReceivedByteCount (void)
{
    return _ui32ReceivedBytes;
}

inline uint32 MocketStats::getDuplicatedDiscardedPacketCount (void)
{
    return _ui32DuplicatedDiscardedPackets;
}

inline uint32 MocketStats::getNoRoomDiscardedPacketCount (void)
{
    return _ui32NoRoomDiscardedPackets;
}

inline uint32 MocketStats::getReassemblySkippedDiscardedPacketCount (void)
{
    return _ui32ReassemblySkippedDiscardedPackets;
}

inline float MocketStats::getEstimatedRTT (void)
{
    return _fSRTT;
}

inline int32 MocketStats::getEstimatedBandwidth (void)
{
    return _i32BandwidthEstimation;
}

inline void MocketStats::setEstimatedBandwidth (int32 i32BandwidthEstimation)
{
    _i32BandwidthEstimation = i32BandwidthEstimation;
}

inline void MocketStats::lock (void)
{
    _m.lock();
}

inline void MocketStats::unlock (void)
{
    _m.unlock();
}

inline uint32 MocketStats::getPendingDataSize (void)
{
    return _ui32PendingDataSize;
}

inline uint32 MocketStats::getPendingPacketQueueSize (void)
{
    return _ui32PendingPacketQueueSize;
}

inline uint32 MocketStats::getReliableSequencedDataSize (void)
{
    return _ui32ReliableSequencedDataSize;
}

inline uint32 MocketStats::getReliableSequencedPacketQueueSize (void)
{
    return _ui32ReliableSequencedPacketQueueSize;
}

inline uint32 MocketStats::getReliableUnsequencedDataSize (void)
{
    return _ui32ReliableUnsequencedDataSize;
}

inline uint32 MocketStats::getReliableUnsequencedPacketQueueSize (void)
{
    return _ui32ReliableUnsequencedPacketQueueSize;
}

inline MocketStats::MessageStats * MocketStats::getOverallMessageStatistics (void)
{
    return &_globalMessageStats;
}

inline uint16 MocketStats::getHighestTag (void)
{
    return (uint16) _perTypeMessageStats.getHighestIndex();
}

inline bool MocketStats::isTagUsed (uint16 ui16Tag)
{
    return (_perTypeMessageStats.used (ui16Tag) != 0);
}

inline MocketStats::MessageStats * MocketStats::getMessageStatisticsForType (uint16 ui16Tag)
{
    return &(_perTypeMessageStats[ui16Tag]);
}

inline int MocketStats::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    objectFreezer.putUInt32 (_ui32Retransmits);
    objectFreezer.putUInt32 (_ui32SentPackets);
    objectFreezer.putUInt32 (_ui32SentBytes);
    objectFreezer.putUInt32 (_ui32ReceivedPackets);
    objectFreezer.putUInt32 (_ui32ReceivedBytes);
    objectFreezer.putUInt32 (_ui32DuplicatedDiscardedPackets);
    objectFreezer.putUInt32 (_ui32NoRoomDiscardedPackets);
    objectFreezer.putUInt32 (_ui32ReassemblySkippedDiscardedPackets);
    objectFreezer.putFloat (_fSRTT);
    objectFreezer.putUInt32 (_ui32PendingDataSize);
    objectFreezer.putUInt32 (_ui32PendingPacketQueueSize);
    objectFreezer.putUInt32 (_ui32ReliableSequencedDataSize);
    objectFreezer.putUInt32 (_ui32ReliableSequencedPacketQueueSize);
    objectFreezer.putUInt32 (_ui32ReliableUnsequencedDataSize);
    objectFreezer.putUInt32 (_ui32ReliableUnsequencedPacketQueueSize);
    // Do not freeze _dBandwidthEstimation since the connection from a new node will have a different bandwidth value
    
/*    printf ("MocketStats\n");
    printf ("_ui32Retransmits %lu\n", _ui32Retransmits);
    printf ("_ui32SentPackets %lu\n", _ui32SentPackets);
    printf ("_ui32SentBytes %lu\n", _ui32SentBytes);
    printf ("_ui32ReceivedPackets %lu\n", _ui32ReceivedPackets);
    printf ("_ui32ReceivedBytes %lu\n", _ui32ReceivedBytes);
    printf ("_ui32DuplicatedDiscardedPackets %lu\n", _ui32DuplicatedDiscardedPackets);
    printf ("_ui32NoRoomDiscardedPackets %lu\n", _ui32NoRoomDiscardedPackets);
    printf ("_ui32ReassemblySkippedDiscardedPackets %lu\n", _ui32ReassemblySkippedDiscardedPackets);
    printf ("_fSRTT %.2f\n", _fSRTT);
    printf ("_ui32PendingDataSize %lu\n", _ui32PendingDataSize);
    printf ("_ui32PendingPacketQueueSize %lu\n", _ui32PendingPacketQueueSize);
    printf ("_ui32ReliableSequencedDataSize %lu\n", _ui32ReliableSequencedDataSize);
    printf ("_ui32ReliableSequencedPacketQueueSize %lu\n", _ui32ReliableSequencedPacketQueueSize);
    printf ("_ui32ReliableUnsequencedDataSize %lu\n", _ui32ReliableUnsequencedDataSize);
    printf ("_ui32ReliableUnsequencedPacketQueueSize %lu\n", _ui32ReliableUnsequencedPacketQueueSize);*/
    
    if (0 != _globalMessageStats.freeze (objectFreezer)) {
        // return -1 is if objectFreezer.endObject() don't end with success
        return -2;
    }
    
    // DArray2<MessageStats> _perTypeMessageStats;
    //printf ("PerTypeMessageStats\n");
    for (uint16 ui16Tag = 1; ui16Tag <= _perTypeMessageStats.getHighestIndex(); ui16Tag++) {
        if (_perTypeMessageStats.used (ui16Tag) != 0) {
            // Insert a control char to signal that another object follow
            objectFreezer << (unsigned char) 1;
            // Insert the tag
            objectFreezer.putUInt16 (ui16Tag);
            //printf ("Tag = %d\n", (int) ui16Tag);
            // Insert an object from the array
            if (0 != _perTypeMessageStats[ui16Tag].freeze (objectFreezer)) {
                return -3;
            }
        }
    }
    // Insert a control char to signal that there are no more data
    objectFreezer << (unsigned char) 0;
    return 0;
}

inline int MocketStats::defrost (NOMADSUtil::ObjectDefroster &objectDefroster)
{
    objectDefroster >> _ui32Retransmits;
    objectDefroster >> _ui32SentPackets;
    objectDefroster >> _ui32SentBytes;
    objectDefroster >> _ui32ReceivedPackets;
    objectDefroster >> _ui32ReceivedBytes;
    objectDefroster >> _ui32DuplicatedDiscardedPackets;
    objectDefroster >> _ui32NoRoomDiscardedPackets;
    objectDefroster >> _ui32ReassemblySkippedDiscardedPackets;
    objectDefroster >> _fSRTT;
    objectDefroster >> _ui32PendingDataSize;
    objectDefroster >> _ui32PendingPacketQueueSize;
    objectDefroster >> _ui32ReliableSequencedDataSize;
    objectDefroster >> _ui32ReliableSequencedPacketQueueSize;
    objectDefroster >> _ui32ReliableUnsequencedDataSize;
    objectDefroster >> _ui32ReliableUnsequencedPacketQueueSize;
    
/*    printf ("MocketStats\n");
    printf ("_ui32Retransmits %lu\n", _ui32Retransmits);
    printf ("_ui32SentPackets %lu\n", _ui32SentPackets);
    printf ("_ui32SentBytes %lu\n", _ui32SentBytes);
    printf ("_ui32ReceivedPackets %lu\n", _ui32ReceivedPackets);
    printf ("_ui32ReceivedBytes %lu\n", _ui32ReceivedBytes);
    printf ("_ui32DuplicatedDiscardedPackets %lu\n", _ui32DuplicatedDiscardedPackets);
    printf ("_ui32NoRoomDiscardedPackets %lu\n", _ui32NoRoomDiscardedPackets);
    printf ("_ui32ReassemblySkippedDiscardedPackets %lu\n", _ui32ReassemblySkippedDiscardedPackets);
    printf ("_fSRTT %.2f\n", _fSRTT);
    printf ("_ui32PendingDataSize %lu\n", _ui32PendingDataSize);
    printf ("_ui32PendingPacketQueueSize %lu\n", _ui32PendingPacketQueueSize);
    printf ("_ui32ReliableSequencedDataSize %lu\n", _ui32ReliableSequencedDataSize);
    printf ("_ui32ReliableSequencedPacketQueueSize %lu\n", _ui32ReliableSequencedPacketQueueSize);
    printf ("_ui32ReliableUnsequencedDataSize %lu\n", _ui32ReliableUnsequencedDataSize);
    printf ("_ui32ReliableUnsequencedPacketQueueSize %lu\n", _ui32ReliableUnsequencedPacketQueueSize);*/
    
    if (_globalMessageStats.defrost (objectDefroster)) {
        return -2;
    }
    
    //printf ("PerTypeMessageStats\n");
    unsigned char moreData;
    uint16 ui16Tag;
    MessageStats messageStat;
    objectDefroster >> moreData;
    while (moreData) {
        objectDefroster >> ui16Tag;
        //printf ("Tag = %d\n", (int) ui16Tag);
        messageStat.defrost (objectDefroster);
        // TODO: check this code //
        _perTypeMessageStats[ui16Tag]=messageStat;
        objectDefroster >> moreData;
    }
    /*if (objectDefroster.endObject()) {
        return -3;
    }
    
    if (objectDefroster.endObject()) {
        return -1;
    }*/
    return 0;
}

inline MocketStats::MessageStats::MessageStats (void)
{
    ui32SentReliableSequencedMsgs = 0;
    ui32SentReliableUnsequencedMsgs = 0;
    ui32SentUnreliableSequencedMsgs = 0;
    ui32SentUnreliableUnsequencedMsgs = 0;
    ui32ReceivedReliableSequencedMsgs = 0;
    ui32ReceivedReliableUnsequencedMsgs = 0;
    ui32ReceivedUnreliableSequencedMsgs = 0;
    ui32ReceivedUnreliableUnsequencedMsgs = 0;
    ui32CancelledPackets = 0;
}

inline int MocketStats::MessageStats::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    objectFreezer.putUInt32 (ui32SentReliableSequencedMsgs);
    objectFreezer.putUInt32 (ui32SentReliableUnsequencedMsgs);
    objectFreezer.putUInt32 (ui32SentUnreliableSequencedMsgs);
    objectFreezer.putUInt32 (ui32SentUnreliableUnsequencedMsgs);
    objectFreezer.putUInt32 (ui32ReceivedReliableSequencedMsgs);
    objectFreezer.putUInt32 (ui32ReceivedReliableUnsequencedMsgs);
    objectFreezer.putUInt32 (ui32ReceivedUnreliableSequencedMsgs);
    objectFreezer.putUInt32 (ui32ReceivedUnreliableUnsequencedMsgs);
    objectFreezer.putUInt32 (ui32CancelledPackets);
    
/*    printf ("MessageStats\n");
    printf ("ui32SentReliableSequencedMsgs %lu\n", ui32SentReliableSequencedMsgs);
    printf ("ui32SentReliableUnsequencedMsgs %lu\n", ui32SentReliableUnsequencedMsgs);
    printf ("ui32SentUnreliableSequencedMsgs %lu\n", ui32SentUnreliableSequencedMsgs);
    printf ("ui32SentUnreliableUnsequencedMsgs %lu\n", ui32SentUnreliableUnsequencedMsgs);
    printf ("ui32ReceivedReliableSequencedMsgs %lu\n", ui32ReceivedReliableSequencedMsgs);
    printf ("ui32ReceivedReliableUnsequencedMsgs %lu\n", ui32ReceivedReliableUnsequencedMsgs);
    printf ("ui32ReceivedUnreliableSequencedMsgs %lu\n", ui32ReceivedUnreliableSequencedMsgs);
    printf ("ui32ReceivedUnreliableUnsequencedMsgs %lu\n", ui32ReceivedUnreliableUnsequencedMsgs);
    printf ("ui32CancelledPackets %lu\n", ui32CancelledPackets);*/
      
    return 0;
}

inline int MocketStats::MessageStats::defrost (NOMADSUtil::ObjectDefroster &objectDefroster)
{
    objectDefroster >> ui32SentReliableSequencedMsgs;
    objectDefroster >> ui32SentReliableUnsequencedMsgs;
    objectDefroster >> ui32SentUnreliableSequencedMsgs;
    objectDefroster >> ui32SentUnreliableUnsequencedMsgs;
    objectDefroster >> ui32ReceivedReliableSequencedMsgs;
    objectDefroster >> ui32ReceivedReliableUnsequencedMsgs;
    objectDefroster >> ui32ReceivedUnreliableSequencedMsgs;
    objectDefroster >> ui32ReceivedUnreliableUnsequencedMsgs;
    objectDefroster >> ui32CancelledPackets;
    
/*    printf ("MessageStats\n");
    printf ("ui32SentReliableSequencedMsgs %lu\n", ui32SentReliableSequencedMsgs);
    printf ("ui32SentReliableUnsequencedMsgs %lu\n", ui32SentReliableUnsequencedMsgs);
    printf ("ui32SentUnreliableSequencedMsgs %lu\n", ui32SentUnreliableSequencedMsgs);
    printf ("ui32SentUnreliableUnsequencedMsgs %lu\n", ui32SentUnreliableUnsequencedMsgs);
    printf ("ui32ReceivedReliableSequencedMsgs %lu\n", ui32ReceivedReliableSequencedMsgs);
    printf ("ui32ReceivedReliableUnsequencedMsgs %lu\n", ui32ReceivedReliableUnsequencedMsgs);
    printf ("ui32ReceivedUnreliableSequencedMsgs %lu\n", ui32ReceivedUnreliableSequencedMsgs);
    printf ("ui32ReceivedUnreliableUnsequencedMsgs %lu\n", ui32ReceivedUnreliableUnsequencedMsgs);
    printf ("ui32CancelledPackets %lu\n", ui32CancelledPackets);*/
    
    return 0;
}

#endif   // #ifndef INCL_MOCKET_STATS_H
