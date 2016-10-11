/*
 * NetworkMessageV1.cpp
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

#include "NetworkMessageV1.h"

#include "Logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXSIZE 65535
#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

NetworkMessageV1::NetworkMessageV1 (void)
{
    _pBuf = (char*) malloc (MAX_BUF_SIZE);
    _ui16NetMsgLen = 0;
}

NetworkMessageV1::NetworkMessageV1 (const NetworkMessage& nm) //copy constructor
{
    if (nm.getVersion() == 1) {
        _pBuf = (char *) malloc (nm.getLength());
        memcpy (_pBuf, nm.getBuf(), nm.getLength());
        _ui16NetMsgLen = nm.getLength();
    }
    else {
        uint8 ui8Version = nm.getVersion();
        uint8 ui8MsgType = nm.getMsgType();
        uint32 ui32SourceAddress = nm.getSourceAddr();
        uint32 ui32TargetAddress = nm.getDestinationAddr();
        uint16 ui16SessionId = nm.getSessionId();
        uint16 ui16MsgId = nm.getMsgId();
        uint8 ui8HopCount = nm.getHopCount();
        uint8 ui8TTL = nm.getTTL();
        ChunkType chunkType = (ChunkType) nm.getChunkType();
        bool bReliable = nm.isReliableMsg();
        const void *pMsgMetaData = nm.getMetaData();
        uint16 ui16MsgMetaDataLen = nm.getMetaDataLen();
        const void *pMsg = nm.getMsg();
        uint16 ui16MsgLen = nm.getMsgLen();

        create (ui8MsgType, ui32SourceAddress, ui32TargetAddress, ui16SessionId,
                ui16MsgId, ui8HopCount, ui8TTL, chunkType, bReliable,
                pMsgMetaData, ui16MsgMetaDataLen, pMsg, ui16MsgLen);
    }
}

NetworkMessageV1::NetworkMessageV1 (const void *pBuf, uint16 ui16BufSize)
{
    if ((ui16BufSize > FIXED_HEADER_LENGTH) && (NetworkMessage::getVersionFromBuffer ((char*) pBuf, ui16BufSize) == 1)) {
        _pBuf = (char *) malloc (ui16BufSize);
        memcpy (_pBuf, (const char*) pBuf, ui16BufSize);
        _ui16NetMsgLen = ui16BufSize;
    }
    else {
        _ui16NetMsgLen = 0;
        _pBuf = NULL;
        if (ui16BufSize <= FIXED_HEADER_LENGTH) {
            checkAndLogMsg ("NetworkMessageV1::NetworkMessageV1", Logger::L_Warning,
                            "short message of size %d received - ignoring\n", (int) ui16BufSize);
        }
        else if (NetworkMessage::getVersionFromBuffer ((const char*) pBuf, ui16BufSize) != 1) {
            checkAndLogMsg ("NetworkMessageV1::NetworkMessageV1", Logger::L_MildError, "message version mismatch\n");
        }
    }
}

NetworkMessageV1::NetworkMessageV1 (uint8 ui8MsgType, uint32 ui32SourceAddress, uint32 ui32TargetAddress,
                                    uint16 ui16SessionId, uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                    ChunkType chunkType, bool bReliable,
                                    const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                    const void *pMsg, uint16 ui16MsgLen)
{
    create (ui8MsgType, ui32SourceAddress, ui32TargetAddress, ui16SessionId,
            ui16MsgId, ui8HopCount, ui8TTL, chunkType, bReliable, pMsgMetaData,
            ui16MsgMetaDataLen, pMsg, ui16MsgLen);
}

NetworkMessageV1::~NetworkMessageV1 (void)
{
}

void NetworkMessageV1::create (uint8 ui8MsgType, uint32 ui32SourceAddress, uint32 ui32TargetAddress,
                               uint16 ui16SessionId, uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                               ChunkType chunkType, bool bReliable,
                               const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                               const void *pMsg, uint16 ui16MsgLen)
{
    _pBuf = (char *) malloc (FIXED_HEADER_LENGTH + ui16MsgMetaDataLen + ui16MsgLen);
    *((uint8*)(_pBuf + VERSION_AND_TYPE_FIELD_OFFSET)) = VERSION_AND_TYPE;
    *((uint16*)(_pBuf + LENGTH_FIELD_OFFSET)) = FIXED_HEADER_LENGTH + ui16MsgMetaDataLen + ui16MsgLen;
    uint32 ui32NetMsgLen = FIXED_HEADER_LENGTH + ui16MsgMetaDataLen + ui16MsgLen;
    _ui16NetMsgLen = (uint16) ui32NetMsgLen;
    if (ui32NetMsgLen >  MAXSIZE) {
        checkAndLogMsg ("NetworkMessage::NetworkMessage", Logger::L_SevereError, "Message exceed maximum MTU %u\n", ui32NetMsgLen);
    }
    *((uint8*)(_pBuf + MSG_TYPE_FIELD_OFFSET)) = ui8MsgType;
    *((uint32*)(_pBuf + SOURCE_ADDRESS_FIELD_OFFSET)) = ui32SourceAddress;
    *((uint32*)(_pBuf + TARGET_ADDRESS_FIELD_OFFSET)) = ui32TargetAddress;
    *((uint16*)(_pBuf + SESSION_ID_FIELD_OFFSET)) = ui16SessionId;
    *((uint16*)(_pBuf + MSG_ID_FIELD_OFFSET)) = ui16MsgId;
    *((uint8*)(_pBuf + HOP_COUNT_FIELD_OFFSET)) = ui8HopCount;
    *((uint8*)(_pBuf + TTL_FIELD_OFFSET)) = ui8TTL;
    *((uint8*)(_pBuf + CHUNK_TYPE_FIELD)) = (uint8) chunkType;

    uint8 ui8Rel = (bReliable ? 1 : 0);
    *((uint8*)(_pBuf + RELIABLE_FIELD)) = ui8Rel;

    *((uint16*)(_pBuf + MSG_META_DATA_LENGTH_FIELD_OFFSET)) = ui16MsgMetaDataLen;
    *((uint16*)(_pBuf + MSG_DATA_LENGTH_FIELD_OFFSET)) = ui16MsgLen;

    if (pMsgMetaData != NULL) {
        memcpy (_pBuf + MSG_META_DATA_FIELD_OFFSET, pMsgMetaData, ui16MsgMetaDataLen);
    }

    if (pMsg != NULL) {
        memcpy (_pBuf + MSG_META_DATA_FIELD_OFFSET + ui16MsgMetaDataLen, pMsg, ui16MsgLen);
    }
    else {
        checkAndLogMsg ("NetworkMessage::NetworkMessage", Logger::L_Warning,
                        "warning: 0 byte data message\n");
    }
}

uint16 NetworkMessageV1::getFixedHeaderLength (void) const
{
    return FIXED_HEADER_LENGTH;
}

void* NetworkMessageV1::getMetaData (void) const
{
    if (getMetaDataLen() == 0) {
        return NULL;
    }
    return (void*) (_pBuf + MSG_META_DATA_FIELD_OFFSET);
}

uint16 NetworkMessageV1::getMetaDataLen (void) const
{
    return *((uint16*)(_pBuf + MSG_META_DATA_LENGTH_FIELD_OFFSET));
}

void* NetworkMessageV1::getMsg (void) const
{
    if (getMsgLen()==0) {
        return NULL;
    }
    return (void*) (_pBuf + MSG_META_DATA_FIELD_OFFSET + getMetaDataLen());
}

uint16 NetworkMessageV1::getMsgLen (void) const
{
    return *((uint16*)(_pBuf + MSG_DATA_LENGTH_FIELD_OFFSET));
}

