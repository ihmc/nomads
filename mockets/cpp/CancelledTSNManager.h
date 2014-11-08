#ifndef INCL_CANCELLED_TSN_MANAGER_H
#define INCL_CANCELLED_TSN_MANAGER_H

/*
 * CancelledTSNManager.h
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

#include "PacketAccessors.h"
#include "TSNRangeHandler.h"

#include "Mutex.h"
#include "ObjectDefroster.h"
#include "ObjectFreezer.h"


class Packet;

class CancelledTSNManager
{
    public:
        CancelledTSNManager (void);

        bool haveInformation (void);

        int startCancellingReliableSequencedPackets (void);
        int startCancellingReliableUnsequencedPackets (void);
        int startCancellingUnreliableSequencedPacket (void);
        int addCancelledPacketTSN (uint32 ui32TSN);
        int endCancellingPackets (void);

        void cancelReliableSequencedPacket (uint32 ui32TSN);
        void cancelReliableUnsequencedPacket (uint32 ui32TSN);
        void cancelUnreliableSequencedPacket (uint32 ui32TSN);

        int processSAckChunk (SAckChunkAccessor sackChunkAccessor);

        void deliveredReliableSequencedPacket (uint32 ui32TSN);
        void deliveredReliableUnsequencedPacket (uint32 ui32TSN);
        void deliveredUnreliableSequencedPacket (uint32 ui32TSN);

        int appendCancelledTSNInformation (Packet *pPacket);

    private:
        enum SelectedFlow {
            SF_None,
            SF_ReliableSequenced,
            SF_ReliableUnsequenced,
            SF_UnreliableSequenced
        };
        SelectedFlow _sf;

    private:
        friend class Mocket;
        
        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

        NOMADSUtil::Mutex _m;
        CancelledTSNRangeHandler _reliableSequencedTSNHandler;
        CancelledTSNRangeHandler _reliableUnsequencedTSNHandler;
        CancelledTSNRangeHandler _unreliableSequencedTSNHandler;
};

#endif   // #ifndef INCL_CANCELLED_PACKET_MANAGER_H
