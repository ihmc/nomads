#ifndef INCL_RECEIVER_H
#define INCL_RECEIVER_H

/*
 * Receiver.h
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

#include "SequencedPacketQueue.h"

#include "FTypes.h"
#include "Mutex.h"
#include "Thread.h"


class CommInterface;
class Mocket;
class PacketProcessor;
class Transmitter;

namespace NOMADSUtil
{
    class UDPDatagramSocket;
}

class Receiver : public NOMADSUtil::Thread
{
    public:
        Receiver (Mocket *pMocket, bool bEnableRecvLogging);
        ~Receiver (void);

        uint32 getWindowSize (void);
        void incrementQueuedDataSize (uint32 ui32Delta);
        void decrementQueuedDataSize (uint32 ui32Delta);
        
        void resetRemoteAddress (uint32 ui32NewRemoteAddress, uint16 ui16NewRemotePort);
        
        void run (void);

        // Used when estimating the bandwidth receiver side
        int64 getLastRecTime (void);
        uint32 getBytesReceived (void);

    private:
        int processCancelledChunk (CancelledChunkAccessor cancelledChunkAccessor);

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);
        
    private:
        friend class Mocket;
        friend class PacketProcessor;
        SequencedPacketQueue * getControlPacketQueue (void);
        SequencedPacketQueue * getReliableSequencedPacketQueue (void);
        SequencedPacketQueue * getUnreliableSequencedPacketQueue (void);

    private:
        NOMADSUtil::Mutex _m;

        Mocket *_pMocket;
        CommInterface *_pCommInterface;
        uint32 _ui32RemoteAddress;
        uint16 _ui16RemotePort;
        PacketProcessor *_pPacketProcessor;
        uint32 _ui32IncomingValidation;

        char * _pszRemoteAddress;
        int64 _i64LastRecvTime;
        int64 _i64LastSentPacketTime;

        // Used when estimating the bandwidth receiver side
        uint32 _ui32BytesReceived;

        uint32 _ui32QueuedDataSize;
        SequencedPacketQueue _ctrlPacketQueue;
        SequencedPacketQueue _reliableSequencedPacketQueue;
        SequencedPacketQueue _unreliableSequencedPacketQueue;

        // Variables for logging received packets
        FILE *_filePacketRecvLog;
        int64 _i64LogStartTime;
        int64 _i64LastRecvLogTime;
};

inline void Receiver::resetRemoteAddress (uint32 ui32NewRemoteAddress, uint16 ui16NewRemotePort)
{
    _ui32RemoteAddress = ui32NewRemoteAddress;
    _ui16RemotePort = ui16NewRemotePort;
}

inline SequencedPacketQueue * Receiver::getControlPacketQueue (void)
{
    return &_ctrlPacketQueue;
}

inline SequencedPacketQueue * Receiver::getReliableSequencedPacketQueue (void)
{
    return &_reliableSequencedPacketQueue;
}

inline SequencedPacketQueue * Receiver::getUnreliableSequencedPacketQueue (void)
{
    return &_unreliableSequencedPacketQueue;
}

inline int64 Receiver::getLastRecTime (void)
{
    return _i64LastRecvTime;
}

inline uint32 Receiver::getBytesReceived (void)
{
    return _ui32BytesReceived;
}

#endif   // #ifndef INCL_RECEIVER_H
