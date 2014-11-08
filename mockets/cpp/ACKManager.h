#ifndef INCL_ACK_MANAGER_H
#define INCL_ACK_MANAGER_H

/*
 * ACKManager.h
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

#include "TSNRangeHandler.h"

#include "FTypes.h"
#include "Mutex.h"
#include "ObjectDefroster.h"
#include "ObjectFreezer.h"


class Packet;

class ACKManager
{
    public:
        ACKManager (void);
        int init (uint32 ui32InitialControlTSN,
                  uint32 ui32InitialReliableSequencedTSN,
                  uint32 ui32InitialReliableUnsequencedID);
        bool haveNewInformationSince (int64 i64Time);
        void receivedControlPacket (uint32 ui32TSN);
        void receivedReliableSequencedPacket (uint32 ui32TSN);
        void receivedReliableUnsequencedPacket (uint32 ui32TSN);
        int appendACKInformation (Packet *pPacket);
        // This SAck is also used to pass the info to estimate the bandwidth receiver side
        int appendACKInformation (Packet *pPacket, int64 i64Timestamp, uint32 ui32BytesReceived);
        void receivedMoreRecentSentPacket (int64 i64Timestamp, uint32 ui32SequenceNumber); //Erika: this method is not implemented
        void setLastSentPacketReceivedTime (int64 i64Timestamp); //Erika: this method is not implemented

    private:
        friend class Mocket;

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

        NOMADSUtil::Mutex _m;
        int64 _i64LastUpdateTime;
        bool _bNewUpdateAtSameTimeStamp;
        SAckTSNRangeHandler _controlTSNHandler;
        SAckTSNRangeHandler _reliableSequencedTSNHandler;
        SAckTSNRangeHandler _reliableUnsequencedTSNHandler;
};

inline bool ACKManager::haveNewInformationSince (int64 i64Time)
{
    if (_i64LastUpdateTime > i64Time) {
        return true;
    }
    else if ((_i64LastUpdateTime == i64Time) && (_bNewUpdateAtSameTimeStamp)) {
        return true;
    }
    else {
        return false;
    }
}

#endif   // #ifndef INCL_ACK_MANAGER_H
