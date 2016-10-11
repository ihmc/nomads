/*
 * NetworkMessageV2.h
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
 *
 *  Created on: Jun 15, 2011
 *  Author: Andrea Rossi    (arossi@ihmc.us)
 */

#ifndef INCL_NETWORK_MESSAGE_V2_H
#define INCL_NETWORK_MESSAGE_V2_H

#include "NetworkMessage.h"

namespace NOMADSUtil
{
    class NetworkMessageV2 : public NetworkMessage
    {
        public:

            NetworkMessageV2 (void);

            // Constructor to de-serialize
            NetworkMessageV2 (const void *pBuf, uint16 ui16BufSize);
            NetworkMessageV2 (const NetworkMessage& nm);

            // Constructor to serialize
            NetworkMessageV2 (uint8 ui8MsgType, uint32 ui32SourceAddress, uint32 ui32TargetAddress,
                              uint16 ui16SessionId, uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                              ChunkType chunkType, bool bReliable,
                              const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                              const void *pMsg, uint16 ui16MsgLen, uint8 ui8QueueSize);
            virtual ~NetworkMessageV2 (void);

            bool isMalformed (void) const;

            uint16 getFixedHeaderLength (void) const;

            void * getMetaData (void) const;
            uint16 getMetaDataLen (void) const;
            void * getMsg (void) const;
            uint16 getMsgLen (void) const;

            uint8 getQueueLength (void) const;
            void setQueueLength (uint8 ui8QueueLength);

        public:
            static const uint16 FIXED_HEADER_LENGTH = 25;

        private:
            void create (uint8 ui8MsgType, uint32 ui32SourceAddress, uint32 ui32TargetAddress,
                         uint16 ui16SessionId, uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                         ChunkType chunkType, bool bReliable,
                         const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                         const void *pMsg, uint16 ui16MsgLen, uint8 ui8QueueSize);

        private:
            static const uint8 VERSION_AND_TYPE = 0x20 | 0x0C;       // Version 2, type C

            static const uint16 QUEUE_LENGTH_FIELD_OFFSET = 20;
            static const uint16 MSG_META_DATA_LENGTH_FIELD_OFFSET = 21;
            static const uint16 MSG_DATA_LENGTH_FIELD_OFFSET = 23;
            static const uint16 MSG_META_DATA_FIELD_OFFSET = 25;
    };

    inline bool NetworkMessageV2::isMalformed () const
    {
        // NOTE: _ui16NetMsgLen >= FIXED_HEADER_LENGTH is necessary because this
        // ensures getMetaDataLen() getMsgLen() are set.
        // NOTE: the second condition should be == I guess...
        if ((_ui16NetMsgLen >= FIXED_HEADER_LENGTH) && (_ui16NetMsgLen >= (FIXED_HEADER_LENGTH + getMetaDataLen() + getMsgLen()))) {
            return false;
        }
        return true;
    }
}

#endif /* INCL_NETWORK_MESSAGE_V2_H */
