/*
 * Reassembler.h
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_REASSEMBLER_H
#define INCL_REASSEMBLER_H

#include "FTypes.h"
#include "PtrLList.h"
#include "TSNRangeHandler.h"
#include "UInt32Hashtable.h"

namespace NOMADSUtil
{
    class NetworkMessage;
}

namespace NOMADSUtil
{
    class Reassembler
    {
        public:
            Reassembler (uint32 ui32RetransmissionTime, bool bSequenced=false);
            virtual ~Reassembler (void);

            uint16 getCumulativeTSN (uint32 ui32SourceAddress);
            uint32 * getNeighborsToBeAcknowledged (uint32 &ui32NumOfNeighbors);

            /**
             * Returns the serialized list of the Selective Acknowledgments
             */
            void * getSacks (uint32 ui32SourceAddress, uint32 ui32MaxLength, uint32 &ui32Lentgh);
            void * getAllSacks (uint32 ui32MaxLength, uint32 &ui32Lentgh);
            bool hasTSN (uint32 ui32SourceAddress, uint16 ui16TSN);
            bool isNewSessionId (uint32 ui32SourceAddress, uint16 ui16SessionId);

            NetworkMessage * pop (uint32 ui32SourceAddress);

            /**
             * Return values:
             *  0 the message has been added successfully.
             *  1 the message had already been received.
             * -1 error.
             *
             * NOTE: Reassemble does not make a copy of the stored
             * NetworkMessageV1.  Therefore the caller must be careful when
             * deleting them. (The can be safely deleted only when push()
             * returns a value != 0.
             */
            int push (uint32 ui32SourceAddress, NetworkMessage *pNetMsg);

            /**
             * Every time a message is received refresh should be called.
             * It updates the arrival time of the latest message for the peer.
             * (If there's any interest in keeping this value).
             */
            int refresh (uint32 ui32SourceAddress);

        private:
            static const uint8 MAX_NUM_OF_LOST_RETRANSMISSIONS = 0;
            struct MsgWrapper
            {
                MsgWrapper (NetworkMessage *pNetMsg, uint64 ui64ArrivalTime);
                ~MsgWrapper();

                bool operator == (const MsgWrapper &rhsMsgWrapper) const;
                bool operator > (const MsgWrapper &rhsMsgWrapper) const;
                bool operator < (const MsgWrapper &rhsMsgWrapper) const;

                NetworkMessage *_pNetMsg;
                uint64 _ui64ArrivalTime;
            };

            struct MsgQueue
            {
                MsgQueue();
                ~MsgQueue();

                SAckTSNRangeHandler _sAcks;
                PtrLList<MsgWrapper> _msgs;
                uint16 _ui16SessionId;
                int64 _i64LastMsgRcvdTime;
            };

            void addRange (uint16 ui16PreviousSeqId, uint16 ui16CurrentSeqId);
            NetworkMessage * reassemble (MsgQueue *pMsgQueue, uint16 ui16FirstSeqId,
                                         uint16 ui16LatestSeqId, uint32 ui32MetaDataLen,
                                         uint32 ui32DataLen);

            uint32 _ui32RetransmissionTime;
            uint8 _ui8MaxTimeOfLostRetransmissions;

            UInt32Hashtable<MsgQueue> _msgsBySourceAddress;
            bool _bSequenced;
            Mutex _m;
    };

    inline uint16 Reassembler::getCumulativeTSN (uint32 ui32SourceAddress)
    {
        MsgQueue *pMQ = _msgsBySourceAddress.get (ui32SourceAddress);
        uint16 ui16CumulativeTSN;
        if (pMQ != NULL) {
            ui16CumulativeTSN = (pMQ->_sAcks).getCumulativeTSN();
        }
        else {
            ui16CumulativeTSN = 0;
            ui16CumulativeTSN--;
        }
        return ui16CumulativeTSN;
    }

    inline bool Reassembler::isNewSessionId (uint32 ui32SourceAddress, uint16 ui16SessionId)
    {
        MsgQueue *pMQ = _msgsBySourceAddress.get (ui32SourceAddress);
        if (pMQ == NULL) {
            return false;
        }
        return pMQ->_ui16SessionId != ui16SessionId;
    }

    inline bool Reassembler::hasTSN (uint32 ui32SourceAddress, uint16 ui16TSN)
    {
        MsgQueue *pMQ = _msgsBySourceAddress.get (ui32SourceAddress);
        return ((pMQ != NULL) ? pMQ->_sAcks.hasTSN (ui16TSN) : false);
    }
}

#endif  // INCL_REASSEMBLER_H
