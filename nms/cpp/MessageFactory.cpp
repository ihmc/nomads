/*
 * MessageFactory.cpp
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

#include "MessageFactory.h"
#include "NetworkMessageV2.h"
#include "NetworkMessageV1.h"

#include <stdlib.h>

using namespace NOMADSUtil;

MessageFactory::MessageFactory (uint8 ui8MessageVersion)
{
    srand ((uint32)getTimeInMilliseconds());
    // Set session id
    // I want to limit the session id to 65535 since
    // this is the greatest number which is rapresentable
    // with 16 bits.
    _ui16SessionID = (rand() % 65535);
    _ui8MessageVersion = ui8MessageVersion;
}

MessageFactory::~MessageFactory()
{
    _seqIdsByNodeAddress.removeAll();
}

NetworkMessage * MessageFactory::getDataMessage (uint32 ui32SourceAddress, uint16 ui16MsgId,
                                                 NetworkMessage::ChunkType chunkType,
                                                 TransmissionInfo &trInfo, MessageInfo &msgInfo)
{
    return createNetworkMessageFromFields (trInfo.ui8MsgType, ui32SourceAddress, trInfo.ui32DestinationAddress,
                                           _ui16SessionID, ui16MsgId, trInfo.ui8HopCount, trInfo.ui8TTL, chunkType, false,
                                           msgInfo.pMsgMetaData, msgInfo.ui16MsgMetaDataLen, msgInfo.pMsg, msgInfo.ui16MsgLen,
                                           _ui8MessageVersion);
}

NetworkMessage * MessageFactory::getReliableDataMessage (uint32 ui32SourceAddress,
                                                         NetworkMessage::ChunkType chunkType,
                                                         TransmissionInfo &trInfo,
                                                         const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                         const void *pMsg, uint16 ui16MsgLen)
{
    return createNetworkMessageFromFields (trInfo.ui8MsgType, ui32SourceAddress, trInfo.ui32DestinationAddress,
                                           _ui16SessionID, getNodeSeqId (trInfo.ui32DestinationAddress), trInfo.ui8HopCount,
                                           trInfo.ui8TTL, chunkType, true, pMsgMetaData, ui16MsgMetaDataLen,
                                           pMsg, ui16MsgLen, _ui8MessageVersion);
}

NetworkMessage * MessageFactory::getSAckMessage (uint8 ui8MsgType, uint32 ui32SourceAddress, uint32 ui32TargetAddress,
                                                 uint8 ui8HopCount, uint8 ui8TTL,
                                                 const void *, uint16 ,
                                                 const void *pMsg, uint16 ui16MsgLen)
{
    return createNetworkMessageFromFields (ui8MsgType, ui32SourceAddress, ui32TargetAddress,
                                           _ui16SessionID, 0, ui8HopCount, ui8TTL,
                                           NetworkMessage::CT_SAck, false, NULL, 0, pMsg,
                                           ui16MsgLen, _ui8MessageVersion);
}

uint16 MessageFactory::getNodeSeqId (uint32 ui32TargetAddress)
{
    SequenceID * pSeqID = _seqIdsByNodeAddress.get (ui32TargetAddress);
    if (pSeqID == NULL) {
        pSeqID = new SequenceID();
        _seqIdsByNodeAddress.put (ui32TargetAddress, pSeqID);
    }
    else {
        pSeqID->seqID = pSeqID->seqID + 1;
    }
    return pSeqID->seqID;
}

NetworkMessage * MessageFactory::createNetworkMessage (uint8 ui8Version)
{
    switch (ui8Version) {
        case 1 :
            return new NetworkMessageV1();
        case 2 :
            return new NetworkMessageV2();
        default :
            return NULL;
    }
}

// Constructor to deserealize
NetworkMessage * MessageFactory::createNetworkMessageFromBuffer (const void *pBuf, uint16 ui16BufSize, uint8 ui8Version)
{
    switch (ui8Version) {
        case 1 :
            return new NetworkMessageV1 (pBuf, ui16BufSize);
        case 2 :
            return new NetworkMessageV2 (pBuf, ui16BufSize);
        default :
            return NULL;
    }

}

NetworkMessage * MessageFactory::createNetworkMessageFromMessage (const NetworkMessage& nmi, uint8 ui8Version)
{
    switch (ui8Version) {
        case 1 :
            return new NetworkMessageV1 (nmi);
        case 2 :
            return new NetworkMessageV2 (nmi);
        default :
            return NULL;
    }
}

// Constructor to serialize
NetworkMessage * MessageFactory::createNetworkMessageFromFields (uint8 ui8MsgType, uint32 ui32SourceAddress,
                                                                 uint32 ui32TargetAddress, uint16 ui16SessionId,
                                                                 uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                                                 NetworkMessage::ChunkType chunkType, bool bReliable,
                                                                 const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                                 const void *pMsg, uint16 ui16MsgLen, uint8 ui8Version)
{
    switch (ui8Version) {
        case 1 :
            return new NetworkMessageV1 (ui8MsgType, ui32SourceAddress, ui32TargetAddress,
                                         ui16SessionId, ui16MsgId, ui8HopCount, ui8TTL,
                                         chunkType, bReliable, pMsgMetaData, ui16MsgMetaDataLen,
                                         pMsg, ui16MsgLen);
        case 2 :
            return new NetworkMessageV2 (ui8MsgType, ui32SourceAddress, ui32TargetAddress, ui16SessionId,
                                         ui16MsgId, ui8HopCount, ui8TTL, chunkType, bReliable, pMsgMetaData,
                                         ui16MsgMetaDataLen, pMsg, ui16MsgLen, 0);
        default :
            return NULL;
    }
}
