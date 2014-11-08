/*
 * MessageId.cpp
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

#include "MessageId.h"

#include "NLFLib.h"
#include <string.h>

using namespace IHMC_ACI;

MessageId::MessageId (void)
{
    _ui32SeqId = 0;
    _ui8ChunkId = 0;
}

MessageId::MessageId (const char *pszMsgId)
    : _msgId (pszMsgId)
{
    parseAndInitFromString (pszMsgId);
}

MessageId::MessageId (const char *pszGroupName, const char *pszOriginatorNodeId, uint32 ui32SeqId, uint8 ui8ChunkId)
{
    init (pszGroupName, pszOriginatorNodeId, ui32SeqId, ui8ChunkId);
}

MessageId::MessageId (const MessageId &srcId)
{
    _msgId = srcId._msgId;
    _groupName = srcId._groupName;
    _originatorNodeId = srcId._originatorNodeId;
    _ui32SeqId = srcId._ui32SeqId;
    _ui8ChunkId = srcId._ui8ChunkId;
}

MessageId::~MessageId (void)
{
}

int MessageId::init (const char *pszMsgId)
{
    return parseAndInitFromString (pszMsgId);
}

int MessageId::init (const char *pszGroupName, const char *pszOriginatorNodeId, uint32 ui32SeqId, uint8 ui8ChunkId)
{
    if ((pszGroupName == NULL) || (pszOriginatorNodeId == NULL)) {
        return -1;
    }
    _groupName = pszGroupName;
    _originatorNodeId = pszOriginatorNodeId;
    _ui32SeqId = ui32SeqId;
    _ui8ChunkId = ui8ChunkId;
    buildMessageIdString();
    return 0;
}

const MessageId & MessageId::operator = (const MessageId &rhsId)
{
    _msgId = rhsId._msgId;
    _groupName = rhsId._groupName;
    _originatorNodeId = rhsId._originatorNodeId;
    _ui32SeqId = rhsId._ui32SeqId;
    _ui8ChunkId = rhsId._ui8ChunkId;
    return *this;
}

int MessageId::setGroupName (const char *pszGroupName)
{
    if (pszGroupName == NULL) {
        return -1;
    }
    _groupName = pszGroupName;
    buildMessageIdString();
    return 0;
}

int MessageId::setOriginatorNodeId (const char *pszOriginatorNodeId)
{
    if (pszOriginatorNodeId == NULL) {
        return -1;
    }
    _originatorNodeId = pszOriginatorNodeId;
    buildMessageIdString();
    return 0;
}

void MessageId::setSeqId (uint32 ui32SeqId)
{
    _ui32SeqId = ui32SeqId;
    buildMessageIdString();
}

void MessageId::setChunkId (uint8 ui8ChunkId)
{
    _ui8ChunkId = ui8ChunkId;
    buildMessageIdString();
}

void MessageId::buildMessageIdString (void)
{
    char szBuf[20];     // Should be large enough for a :, a uint32 (10 bytes), a :, a uint8 (3 bytes), and the null terminator
    _msgId = _groupName;
    _msgId += ":";
    _msgId += _originatorNodeId;
    sprintf (szBuf, ":%u:%d", _ui32SeqId, (int) _ui8ChunkId);
    _msgId += szBuf;
}

int MessageId::parseAndInitFromString (const char *pszMsgId)
{
    if (pszMsgId == NULL) {
        return -1;
    }
    char *pszCopy = NOMADSUtil::strDup (pszMsgId);
    char *pszTemp = pszCopy;
    char *pszSep = strchr (pszTemp, ':');
    if (pszSep == NULL) {
        // Did not find the group name
        return -2;
    }
    *pszSep++ = '\0';
    _groupName = pszTemp;
    pszTemp = pszSep;
    pszSep = strchr (pszTemp, ':');
    if (pszSep == NULL) {
        // Did not find the originator node id
        return -3;
    }
    *pszSep++ = '\0';
    _originatorNodeId = pszTemp;
    pszTemp = pszSep;
    pszSep = strchr (pszTemp, ':');
    if (pszSep == NULL) {
        // Did not find the seq id
        return -4;
    }
    *pszSep++ = '\0';
    _ui32SeqId = NOMADSUtil::atoui32 (pszTemp);
    pszTemp = pszSep;
    if (strlen (pszTemp) <= 0) {
        // Did not find the chunk id
        return -5;
    }
    pszSep = strchr (pszTemp, ':');
    if (pszSep == NULL) { 
        _ui8ChunkId = (uint8) NOMADSUtil::atoui32 (pszTemp);
    }
    else {
        *pszSep++ = '\0';
        _ui8ChunkId = (uint8) NOMADSUtil::atoui32 (pszTemp);
    }

    return 0;
}
