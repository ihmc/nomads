/*
 * MessageInfo.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#include "MessageInfo.h"

#include "DSSFLib.h"

#include "NLFLib.h"
#include "StrClass.h"
#include "Writer.h"

#include <assert.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const String MessageHeader::MSG_ID_SEPARATOR = ":";
const int64 MessageHeader::NO_EXPIRATION = 0;
const uint8 MessageHeader::UNDEFINED_CHUNK_ID = 0x00;
const uint8 MessageHeader::MIN_CHUNK_ID = 0x01;
const uint8 MessageHeader::MAX_CHUNK_ID = 0xFF; 
const String MessageHeader::DEFAULT_MIME_TYPE = "application/octet-stream";

//------------------------------------------------------------------------------
// MessageHeader
//------------------------------------------------------------------------------

MessageHeader::MessageHeader (void)
    : _mimeType (DEFAULT_MIME_TYPE)
{
    _ui32SeqId = 0;
    _ui16Tag = 0;
    _ui16ClientId = 0;
    _ui8ClientType = 0;
    _ui32TotalMessageLength = 0;
    _ui32FragmentOffset = 0;
    _ui32FragmentLength = 0;
    _ui16HistoryWindow = 0;
    _ui8Priority = DEFAULT_AVG_PRIORITY;
    _i64Expiration = NO_EXPIRATION;
    _bAcknowledgment = false;
}

MessageHeader::MessageHeader (Type type, const char *pszGroupName, const char *pszPublisherNodeId,
                              uint32 ui32SeqId, uint8 ui8ChunkId, const char *pszObjectId,
                              const char *pszInstanceId, uint16 ui16Tag, uint16 ui16ClientId,
                              uint8 ui8ClientType, const char *pszMimeType, const char *pszChecksum,
                              uint32 ui32FragmentOffset, uint32 ui32FragmentLength,
                              uint32 ui32TotalMessageLength, uint16 ui16HistoryWindow, uint8 ui8Priority,
                              int64 i64Expiration, bool bAcknowledgment)
    : _sGroupName (pszGroupName),               // StrClass makes a copy of pszGroupName
      _sPublisherNodeId (pszPublisherNodeId),   // StrClass makes a copy of pszPublisherNodeId
      _objectId (pszObjectId),
      _instanceId (pszInstanceId),
      _mimeType (pszMimeType),                  // StrClass makes a copy of pszMimeType
      _checksum (pszChecksum)                   // StrClass makes a copy of pszChecksum
{
    _type = type; 
    _ui32SeqId = ui32SeqId;
    _ui8ChunkId = ui8ChunkId;
    _ui16Tag = ui16Tag;
    _ui16ClientId = ui16ClientId;
    _ui8ClientType = ui8ClientType;
    _ui32TotalMessageLength = ui32TotalMessageLength;
    _ui32FragmentOffset = ui32FragmentOffset;
    _ui32FragmentLength = ui32FragmentLength;
    _ui16HistoryWindow = ui16HistoryWindow;
    _ui8Priority = ui8Priority;
    _i64Expiration = i64Expiration;
    _bAcknowledgment = bAcknowledgment;
}

MessageHeader::~MessageHeader()
{
}

const char * MessageHeader::getMsgId() const
{
    return _sMsgId;
}

char * MessageHeader::getIdForCompleteMsg (void) const
{
    String id =  getGroupName();
    id += MSG_ID_SEPARATOR;
    id += getPublisherNodeId();
    char szBuf[24];
    sprintf (szBuf, "%s%lu%s%d", MSG_ID_SEPARATOR.c_str(), (long unsigned int)getMsgSeqId(), MSG_ID_SEPARATOR.c_str(), (int) getChunkId());
    id += szBuf;
    return id.r_str();
}

char * MessageHeader::getLargeObjectId()
{
    String s;
    generateLargeMsgId (s);
    return s.r_str();
}

const char * MessageHeader::getGroupName() const
{
    return _sGroupName;
}
/*!!*/ // NOTE: Having the getPublisherNodeId() as an inline function in the header
       // file was causing the method to return a bad pointer to the
       // string in Visual Studio 2008 - debug later
const char * MessageHeader::getPublisherNodeId() const
{
    return _sPublisherNodeId;
}

const char * MessageHeader::getObjectId (void) const
{
    return _objectId.c_str();
}

const char * MessageHeader::getInstanceId (void) const
{
    return _instanceId.c_str();
}

uint16 MessageHeader::getTag() const
{
    return _ui16Tag;
}

uint16 MessageHeader::getClientId (void) const
{
    return _ui16ClientId;
}

uint8 MessageHeader::getClientType (void) const
{
    return _ui8ClientType;
}

const char * MessageHeader::getMimeType (void) const
{
    return _mimeType.c_str();
}

const char * MessageHeader::getChecksum (void) const
{
    return _checksum.c_str();
}

uint32 MessageHeader::getMsgSeqId() const
{
    return _ui32SeqId;
}

uint32 MessageHeader::getFragmentOffset() const
{
    return _ui32FragmentOffset;
}

uint32 MessageHeader::getFragmentLength() const
{
    return _ui32FragmentLength;
}

uint32 MessageHeader::getTotalMessageLength() const
{
    return _ui32TotalMessageLength;
}

bool MessageHeader::isCompleteMessage() const
{
    return (_ui32TotalMessageLength == _ui32FragmentLength);
}
    
bool MessageHeader::operator == (MessageHeader &rhsMessageHeader)
{
    return ((_sMsgId == (String) rhsMessageHeader.getMsgId()) ? true : false);
}

int MessageHeader::read (Reader *pReader, uint32 ui32MaxSize)
{
    uint8 ui8;
    pReader->read8 (&ui8);
    switch (ui8) {
        case MessageInfo:
            _type = MessageInfo;
            break;

        case ChunkMessageInfo:
            _type = ChunkMessageInfo;
            break;

        default:
            return -1;
    }

    // read groupName
    uint16 ui16;
    pReader->read16 (&ui16);
    char *pszTemp = new char[ui16+1];
    pReader->readBytes (pszTemp, ui16);
    pszTemp[ui16] = '\0';
    _sGroupName = pszTemp;
    delete[] pszTemp;

    // read publisherId
    pReader->read16 (&ui16);
    pszTemp = new char[ui16+1];
    pReader->readBytes (pszTemp, ui16);
    pszTemp[ui16] = '\0';
    _sPublisherNodeId = pszTemp;
    delete[] pszTemp;

    pReader->read32 (&_ui32SeqId);
    pReader->read8 (&_ui8ChunkId);

    // read objectId
    pReader->read16 (&ui16);
    if (ui16 > 0) {
        pszTemp = new char[ui16+1];
        pReader->readBytes (pszTemp, ui16);
        pszTemp[ui16] = '\0';
        _objectId = pszTemp;
        delete[] pszTemp;
    }

    // read instanceId
    pReader->read16 (&ui16);
    if (ui16 > 0) {
        pszTemp = new char[ui16+1];
        pReader->readBytes (pszTemp, ui16);
        pszTemp[ui16] = '\0';
        _instanceId = pszTemp;
        delete[] pszTemp;
    }

    pReader->read16 (&_ui16Tag);
    pReader->read16 (&_ui16ClientId);
    pReader->read8 (&_ui8ClientType);

    // read  mimeType
    pReader->read16 (&ui16);
    pszTemp = new char[ui16+1];
    pReader->readBytes (pszTemp, ui16);
    pszTemp[ui16] = '\0';
    _mimeType = pszTemp;
    delete[] pszTemp;

    pReader->read32 (&_ui32TotalMessageLength);
    pReader->read32 (&_ui32FragmentOffset);
    pReader->read32 (&_ui32FragmentLength);
    pReader->read16 (&_ui16HistoryWindow);
    pReader->read8 (&_ui8Priority);
    pReader->read64 (&_i64Expiration);

    ui8 = 2;
    pReader->read8 (&ui8);
    assert ((ui8 == 0) || (ui8 == 1));
    _bAcknowledgment = (ui8 == 1 ? true : false);

    return 0;
}

int MessageHeader::write (Writer *pWriter, uint32 ui32MaxSize)
{
    uint8 ui8 = _type;
    pWriter->write8 (&ui8);

    uint16 ui16 = _sGroupName.length();
    pWriter->write16 (&ui16);
    pWriter->writeBytes ((const char*) _sGroupName, ui16);

    ui16 = _sPublisherNodeId.length();
    pWriter->write16 (&ui16);
    pWriter->writeBytes ((const char*) _sPublisherNodeId, ui16);

    pWriter->write32 (&_ui32SeqId);
    pWriter->write8 (&_ui8ChunkId);

    int i = _objectId.length();
    ui16 = (i > 0 && i < 0xFFFF) ? (uint16) i : 0;
    pWriter->write16 (&ui16);
    if (ui16 > 0) {
        pWriter->writeBytes ((const char*) _objectId, ui16);
    }

    i = _instanceId.length();
    ui16 = (i > 0 && i < 0xFFFF) ? (uint16) i : 0;
    pWriter->write16 (&ui16);
    if (ui16 > 0) {
        pWriter->writeBytes ((const char*) _instanceId, ui16);
    }

    pWriter->write16 (&_ui16Tag);
    pWriter->write16 (&_ui16ClientId);
    pWriter->write8 (&_ui8ClientType);

    i = _mimeType.length();
    ui16 = (i > 0 && i < 0xFFFF) ? (uint16) i : 0;
    pWriter->write16 (&ui16);
    if (ui16 > 0) {
        pWriter->writeBytes ((const char*) _mimeType, ui16);
    }

    pWriter->write32 (&_ui32TotalMessageLength);
    pWriter->write32 (&_ui32FragmentOffset);
    pWriter->write32 (&_ui32FragmentLength);
    pWriter->write16 (&_ui16HistoryWindow);
    pWriter->write8 (&_ui8Priority);
    pWriter->write64 (&_i64Expiration);

    ui8 = _bAcknowledgment ? 1 : 0;
    pWriter->write8 (&ui8);

    return 0;
}

void MessageHeader::setFragmentOffset (uint32 ui32FragmentOffset)
{
    _ui32FragmentOffset = ui32FragmentOffset;
    generateMsgId ();
}

void MessageHeader::setFragmentLength (uint32 ui32FragmentLength)
{
    _ui32FragmentLength = ui32FragmentLength;
    generateMsgId();
}

void MessageHeader::setMsgSeqId (uint32 ui32MsgSeqId)
{
    _ui32SeqId = ui32MsgSeqId;
    generateMsgId();
}

void MessageHeader::generateLargeMsgId (String &id)
{
    char pszBuf[12];
    char *pszTemp;

    id = _sGroupName;
    id = id + MSG_ID_SEPARATOR + _sPublisherNodeId;
    pszTemp = itoa (pszBuf, _ui32SeqId);
    id = (String) id + MSG_ID_SEPARATOR + pszTemp;
}

void MessageHeader::generateMsgId()
{
    generateLargeMsgId (_sMsgId);

    char pszBuf[12];
    char *pszTemp;

    pszTemp = itoa (pszBuf, _ui8ChunkId);
    _sMsgId = (String) _sMsgId + MSG_ID_SEPARATOR + pszTemp;
    pszTemp = itoa (pszBuf, _ui32FragmentOffset);
    _sMsgId = (String) _sMsgId + MSG_ID_SEPARATOR + pszTemp;
    pszTemp = itoa (pszBuf, _ui32FragmentLength);
    _sMsgId = (String) _sMsgId + MSG_ID_SEPARATOR + pszTemp + "\0";
}

//------------------------------------------------------------------------------
// MessageInfo
//-----------------------------------------------------------------------------

MessageInfo::MessageInfo()
    : MessageHeader()
{
    _bMetaData = false;
}

MessageInfo::MessageInfo (bool bMetaData)
    : MessageHeader()
{
    _bMetaData = bMetaData;
}

MessageInfo::MessageInfo (const char *pszGroupName, const char *pszPublisherNodeId, uint32 ui32SeqId,
                          const char *pszObjectId, const char *pszInstanceId, uint16 ui16Tag, 
                          uint16 ui16ClientId, uint8 ui8ClientType, const char *pszMimeType, const char *pszChecksum,
                          uint32 ui32TotalMessageLength, uint32 ui32FragmentLength, uint32 ui32FragmentOffset,
                          uint32 ui32MetaDataLength, uint16 ui16HistoryWindow, uint8 ui8Priority, int64 i64Expiration,
                          bool bAcknowledgment, bool bMetaData, const char *pszReferredObjectId)
    : MessageHeader (MessageHeader::MessageInfo, pszGroupName, pszPublisherNodeId,
                     ui32SeqId, MessageHeader::UNDEFINED_CHUNK_ID, pszObjectId,
                     pszInstanceId, ui16Tag, ui16ClientId, ui8ClientType, pszMimeType, pszChecksum,
                     ui32FragmentOffset, ui32FragmentLength, ui32TotalMessageLength, ui16HistoryWindow,
                     ui8Priority, i64Expiration, bAcknowledgment)
{
    _ui32MetaDataLength = ui32MetaDataLength;
    _bMetaData = bMetaData;

    if (pszReferredObjectId != NULL) {
        setReferredObject (pszReferredObjectId);
    }

    generateMsgId();
}

MessageInfo::~MessageInfo()
{
}

int MessageInfo::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (MessageHeader::read (pReader, ui32MaxSize) < 0) {
        return -1;
    }

    // read refObj
    uint16 ui16;
    pReader->read16 (&ui16);
    if (ui16 > 0) {
        char *pszTemp = new char[ui16+1];
        pReader->readBytes (pszTemp, ui16);
        pszTemp[ui16] = '\0';
        _sRefObj = pszTemp;
        delete[] pszTemp;
    }

    pReader->read32 (&_ui32MetaDataLength);

    uint8 ui8 = 2;
    pReader->read8 (&ui8);
    assert ((ui8 == 0) || (ui8 == 1));
    _bMetaData = (ui8 == 1 ? true : false);

    generateMsgId();
    
    return 0;
}

int MessageInfo::write (Writer *pWriter, uint32 ui32MaxSize)
{
    if (MessageHeader::write (pWriter, ui32MaxSize) < 0) {
        return -1;
    }

    uint16 ui16 = (uint16) maximum (_sRefObj.length(), 0);
    pWriter->write16 (&ui16);
    if (ui16 > 0) {
        pWriter->writeBytes ((const char*) _sRefObj, ui16);
    }

    pWriter->write32 (&_ui32MetaDataLength);

    uint8 ui8 = _bMetaData ? 1 : 0;
    pWriter->write8 (&ui8);

    return 0;
}

MessageInfo * MessageInfo::clone (void)
{
    return new MessageInfo ((const char *)_sGroupName, (const char *) _sPublisherNodeId,
                            _ui32SeqId, getObjectId(), getInstanceId(), getTag(), getClientId(),
                            getClientType(), _mimeType.c_str(), _checksum.c_str(),
                            getTotalMessageLength(), _ui32FragmentLength,
                            _ui32FragmentOffset, _ui32MetaDataLength, getHistoryWindow(),
                            getPriority(), getExpiration(), getAcknowledgment(), _bMetaData,
                            _sRefObj.length() > 0 ? (const char *)_sRefObj : NULL);
}

void MessageInfo::display (FILE *pFileOut)
{
    if (pFileOut == NULL) {
        return;
    }
    fprintf (pFileOut, "GroupName = %s\nPublisherNodeId = %s\nSeqId = %u\n", (const char *)_sGroupName, (const char *)_sPublisherNodeId, _ui32SeqId);
    fprintf (pFileOut, "Referes to = %s\n", (const char *)_sRefObj);
    fprintf (pFileOut, "Tag = %d\n", getTag());
    fprintf (pFileOut, "ClientID = %d\n", getClientId());
    fprintf (pFileOut, "ClientType = %d\n", getClientType());
    fprintf (pFileOut, "TotalMessageLength = %u\n", getTotalMessageLength());
    fprintf (pFileOut, "FragmentOffset = %u\n", _ui32FragmentOffset);
    fprintf (pFileOut, "FragmentLength = %u\n", _ui32FragmentLength);
    fprintf (pFileOut, "MetadataLength = %u\n", _ui32MetaDataLength);
    fprintf (pFileOut, "HistoryWindow = %u\n", getHistoryWindow());
    fprintf (pFileOut, "Priority = %u\n", getPriority());
    fprintf (pFileOut, "Expiration = %lld\n", getExpiration());
    fprintf (pFileOut, "Ack = %s", getAcknowledgment() ? "true" : "false");
    fprintf (pFileOut, "Metadata = %s", _bMetaData ? "true" : "false");
}

const char * MessageInfo::getReferredObject()
{
    if (_sRefObj.length() <= 0) {
        if ((_sGroupName.length() > 0) && (_sPublisherNodeId.length() > 0)) {
            char *pszBuf = (char*) malloc (_sGroupName.length() + _sPublisherNodeId.length() + 15); // +15 for the two ':', for the maximum value of a uint32, and the null terminator (+1 for good measure)
            sprintf (pszBuf, "%s:%s:%u", (const char*) _sGroupName, (const char *) _sPublisherNodeId, _ui32SeqId);
            _sRefObj = pszBuf;
            free (pszBuf);
        }
    }
    return _sRefObj;
}

void MessageInfo::setReferredObject (const char *pszMessageId)
{
    _sRefObj = pszMessageId;
}

//------------------------------------------------------------------------------
// ChunkMsgInfo
//------------------------------------------------------------------------------

ChunkMsgInfo::ChunkMsgInfo()
    : MessageHeader()
{
    _ui8ChunkId = 0;
    _ui8TotalNumOfChunks = 0;
}

ChunkMsgInfo::ChunkMsgInfo (const char *pszGroupName, const char *pszPublisherNodeId,
                            uint32 ui32SeqId, uint8 ui8ChunkId, const char *pszObjectId,
                            const char *pszInstanceId, uint16 ui16Tag, uint16 ui16ClientId,
                            uint8 ui8ClientType, const char *pszMimeType, const char *pszChecksum,
                            uint32 ui32FragmentOffset, uint32 ui32FragmentLength,
                            uint32 ui32TotalMessageLength, uint8 ui8TotalNumOfChunks,
                            uint16 ui16HistoryWindow, uint8 ui8Priority,
                            int64 i64Expiration, bool bAcknowledgment)
    : MessageHeader (MessageHeader::ChunkMessageInfo, pszGroupName, pszPublisherNodeId,
                     ui32SeqId, ui8ChunkId, pszObjectId, pszInstanceId, ui16Tag,
                     ui16ClientId, ui8ClientType, pszMimeType, pszChecksum, ui32FragmentOffset,
                     ui32FragmentLength, ui32TotalMessageLength, ui16HistoryWindow, ui8Priority,
                     i64Expiration, bAcknowledgment)
{
    assert (MIN_CHUNK_ID <= ui8ChunkId);

    _ui8TotalNumOfChunks = ui8TotalNumOfChunks;

    generateMsgId();
}

ChunkMsgInfo::~ChunkMsgInfo()
{
}


int ChunkMsgInfo::read (Reader *pReader, uint32 ui32MaxSize)
{
    if (MessageHeader::read (pReader, ui32MaxSize) < 0) {
        return -1;
    }

    pReader->read8 (&_ui8TotalNumOfChunks);

    generateMsgId();

    return 0;
}

int ChunkMsgInfo::write (Writer *pWriter, uint32 ui32MaxSize)
{
    if (MessageHeader::write (pWriter, ui32MaxSize) < 0) {
        return -1;
    }

    pWriter->write8 (&_ui8TotalNumOfChunks);

    return 0;
}

ChunkMsgInfo * ChunkMsgInfo::clone (void)
{
    return new ChunkMsgInfo ((const char *) _sGroupName, (const char *) _sPublisherNodeId,
                              _ui32SeqId, _ui8ChunkId, getObjectId(), getInstanceId(), getTag(),
                              getClientId(), getClientType(), _mimeType.c_str(), _checksum.c_str(),
                              _ui32FragmentOffset, _ui32FragmentLength, getTotalMessageLength(),
                              _ui8TotalNumOfChunks, getHistoryWindow(), getPriority(), getExpiration(),
                              getAcknowledgment());
}

void ChunkMsgInfo::setFragmentLength (uint32 ui32FragmentLength)
{
    _ui32FragmentLength = ui32FragmentLength;
    generateMsgId();
}

void ChunkMsgInfo::setFragmentOffset (uint32 ui32FragmentOffset)
{
    _ui32FragmentOffset = ui32FragmentOffset;
    generateMsgId();
}

//------------------------------------------------------------------------------
// MessageHeaderHelper
//------------------------------------------------------------------------------

MessageHeader * MessageHeaderHelper::getMessageHeader (const char *pszGroupName, const char *pszPublisherNodeId,
                                                       uint32 ui32MsgSeqId, uint8 ui8ChunkId, const char *pszObjectId,
                                                       const char *pszInstanceId, uint16 ui16Tag, uint16 ui16ClientId,
                                                       uint8 ui8ClientType, const char *pszMimeType, const char *pszChecksum,
                                                       uint32 ui32TotalMessageLength, uint32 ui32FragmentOffset,
                                                       uint32 ui32FragmentLength, uint32 ui32MetaDataLength, uint16 ui16HistoryWindow,
                                                       uint8 ui8Priority, int64 i64Expiration, bool bAcknowledgment,
                                                       bool bMetaData, uint8 ui8TotalNumOfChunks, const char *pszReferredObjectId)
{
    if (ui8TotalNumOfChunks == 0) {
        assert (ui8ChunkId == MessageHeader::UNDEFINED_CHUNK_ID);

        return new MessageInfo (pszGroupName, pszPublisherNodeId, ui32MsgSeqId, pszObjectId,
                                pszInstanceId, ui16Tag, ui16ClientId, ui8ClientType, pszMimeType,
                                pszChecksum, ui32TotalMessageLength, ui32FragmentLength,
                                ui32FragmentOffset, ui32MetaDataLength, ui16HistoryWindow,
                                ui8Priority, i64Expiration, bAcknowledgment, bMetaData, pszReferredObjectId);
    }
    else {
        assert (MessageHeader::MIN_CHUNK_ID <= ui8ChunkId);

        return new ChunkMsgInfo (pszGroupName, pszPublisherNodeId, ui32MsgSeqId, ui8ChunkId,
                                 pszObjectId, pszInstanceId, ui16Tag, ui16ClientId, ui8ClientType,
                                 pszMimeType, pszChecksum, ui32FragmentOffset, ui32FragmentLength,
                                 ui32TotalMessageLength, ui8TotalNumOfChunks, ui16HistoryWindow,
                                 ui8Priority, i64Expiration, bAcknowledgment);
    }
    return NULL;
}

