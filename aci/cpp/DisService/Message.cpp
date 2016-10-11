/*
 * Message.cpp
 *
 *This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#include "Message.h"

#include "MessageInfo.h"

 #include <stdlib.h>
 #include <string.h>

using namespace IHMC_ACI;

Message::Message()
{
    _pMsgHeader = NULL;
    _pData = NULL;
}

Message::Message (MessageHeader *pMsgHeader, const void *pData)
{
    _pMsgHeader = pMsgHeader;
    _pData = pData;
}

Message::~Message()
{
    _pMsgHeader = NULL;
    _pData = NULL;
}

Message * Message::clone (void)
{
    if (_pMsgHeader == NULL) {
        return NULL;
    }

    MessageHeader *pMH = _pMsgHeader->clone();
    if (pMH == NULL) {
        return NULL;
    }

    void *pData = NULL;
    if (_pData != NULL) {
        pData = malloc (_pMsgHeader->getFragmentLength());
        if (pData == NULL) {
            delete pMH;
            return NULL;
        }
        memcpy (pData, _pData, _pMsgHeader->getFragmentLength());
    }

    Message *pMsg = new Message (pMH, pData);
    if (pMsg == NULL) {
        delete pMH;
        if (pData != NULL) {
            free (pData);
        }
    }

    return pMsg;
}

MessageHeader * Message::getMessageHeader (void) const
{
    return _pMsgHeader;
}

MessageHeader * Message::relinquishMessageHeader (void)
{
    MessageHeader *pMH = _pMsgHeader;
    _pMsgHeader = NULL;
    return pMH;
}

MessageInfo * Message::getMessageInfo (void) const
{
    return (MessageInfo *) _pMsgHeader;
}

MessageInfo * Message::relinquishMessageInfo (void)
{
    MessageInfo *pMI = (MessageInfo *) _pMsgHeader;
    _pMsgHeader = NULL;
    return pMI;
}

ChunkMsgInfo * Message::getChunkMsgInfo (void) const
{
    return (ChunkMsgInfo *) _pMsgHeader;
}

ChunkMsgInfo * Message::relinquishChunkMsgInfo (void)
{
    ChunkMsgInfo *pCMI = (ChunkMsgInfo *) _pMsgHeader;
    _pMsgHeader = NULL;
    return pCMI;
}

const void * Message::getData (void) const
{
    return _pData;
}

const void * Message::relinquishData (void)
{
    const void *pData = _pData;
    _pData = NULL;
    return pData;
}

void Message::setMessageHeader (MessageHeader *pMsgHeader)
{
    _pMsgHeader = pMsgHeader;
}

void Message::setData (const void *pData)
{
    _pData = pData;
}

bool Message::operator > (Message &msgToMatch)
{
    if (_pMsgHeader->getMsgId() > msgToMatch.getMessageHeader()->getMsgId()) {
        return true;
    }
    else if  ((_pMsgHeader->getMsgId() == msgToMatch.getMessageHeader()->getMsgId()) && ((_pMsgHeader->getFragmentOffset()) > (msgToMatch.getMessageHeader()->getFragmentOffset()))) {
        return true;
    }
    return false;
}

bool Message::operator < (Message &msgToMatch)
{
    if (_pMsgHeader->getMsgId() < msgToMatch.getMessageHeader()->getMsgId()) {
        return true;
    }
    else if  ((_pMsgHeader->getMsgId() == msgToMatch.getMessageHeader()->getMsgId()) && ((_pMsgHeader->getFragmentOffset()) < (msgToMatch.getMessageHeader()->getFragmentOffset()))) {
        return true;
    }
    return false;
}

bool Message::operator == (Message &msgToMatch)
{
    return ((_pMsgHeader->getMsgId() == msgToMatch.getMessageHeader()->getMsgId()) && ((_pMsgHeader->getFragmentOffset()) == (msgToMatch.getMessageHeader()->getFragmentOffset())) &&
            (_pMsgHeader->getFragmentLength() == msgToMatch.getMessageHeader()->getFragmentLength()));
}
