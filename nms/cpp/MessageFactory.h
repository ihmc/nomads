/*
 * MessageFactory.h
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

#ifndef INCL_MESSAGE_FACTORY_H
#define	INCL_MESSAGE_FACTORY_H

#include "NetworkMessage.h"
#include "UInt32Hashtable.h"

namespace NOMADSUtil
{
    struct TransmissionInfo
    {
        bool bReliable;
        bool bExpedited;
        uint8 ui8HopCount;
        uint8 ui8TTL;
        uint16 ui16DelayTolerance;
        uint8 ui8MsgType;
        uint32 ui32DestinationAddress;
        const char **ppszOutgoingInterfaces;
        const char *pszHints;
    };

    struct MessageInfo
    {
        uint16 ui16MsgLen;
        uint16 ui16MsgMetaDataLen;
        const void *pMsgMetaData;
        const void *pMsg;
    };

    class MessageFactory
    {
        public:
            MessageFactory (uint8 ui8MessageVersion = 1);
            virtual ~MessageFactory();

            NetworkMessage * getDataMessage (uint32 ui32SourceAddress, uint16 ui16MsgId,
                                             NetworkMessage::ChunkType chunkType,
                                             TransmissionInfo &trInfo, MessageInfo &msgInfo);

            NetworkMessage * getReliableDataMessage (uint32 ui32SourceAddress,
                                                     NetworkMessage::ChunkType chunkType,
                                                     TransmissionInfo &trInfo,
                                                     const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                     const void *pMsg, uint16 ui16MsgLen);

            NetworkMessage * getSAckMessage (uint8 ui8MsgType, uint32 ui32SourceAddress, uint32 ui32TargetAddress,
                                             uint8 ui8HopCount, uint8 ui8TTL,
                                             const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                             const void *pMsg, uint16 ui16MsgLen);

            uint16 getNodeSeqId (uint32 ui32TargetAddress);

            //static methods to create messages of the appropriate version. they call the correspondent constructors
            //on the relative classes
            static NetworkMessage * createNetworkMessage (uint8 ui8Version = 1);

            // Constructor to de-serialize
            static NetworkMessage * createNetworkMessageFromBuffer (const void *pBuf, uint16 ui16BufSize, uint8 ui8Version = 1);
            static NetworkMessage * createNetworkMessageFromMessage (const NetworkMessage &nmi, uint8 ui8Version = 1);

            // Constructor to serialize
            static NetworkMessage * createNetworkMessageFromFields (uint8 ui8MsgType, uint32 ui32SourceAddress, uint32 ui32TargetAddress,
                                                                    uint16 ui16SessionId, uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                                                    NetworkMessage::ChunkType chunkType, bool bReliable,
                                                                    const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                                    const void *pMsg, uint16 ui16MsgLen, uint8 ui8Version = 1);

        private:
            struct SequenceID
            {
                SequenceID();
                ~SequenceID();
                uint16 seqID;
            };

            uint8 _ui8MessageVersion;
            uint16 _ui16SessionID;
            UInt32Hashtable<SequenceID> _seqIdsByNodeAddress;
    };

    inline MessageFactory::SequenceID::SequenceID()
    {
        seqID = 0;
    }

    inline MessageFactory::SequenceID::~SequenceID()
    {
    }
}

#endif	// MESSAGE_FACTORY_H
