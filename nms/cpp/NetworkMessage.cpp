/*
 * NetworkMessage.cpp
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

#include "NetworkMessage.h"

#include "Logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXSIZE 65535
#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

NetworkMessage::NetworkMessage (void)
{
    _pBuf = NULL;
    _ui16NetMsgLen = 0;
}

NetworkMessage::NetworkMessage (const NetworkMessage& nm) //copy constructor
{
    memcpy (_pBuf, nm._pBuf, nm._ui16NetMsgLen);;
    _ui16NetMsgLen = nm._ui16NetMsgLen;
}

NetworkMessage::NetworkMessage (const void *pBuf, uint16 ui16BufSize)
{
    memcpy (_pBuf, pBuf, ui16BufSize);
    _ui16NetMsgLen = ui16BufSize;
}

NetworkMessage::~NetworkMessage (void)
{
    if (_pBuf != NULL) {
        free (_pBuf);
        _pBuf = NULL;
    }
    _ui16NetMsgLen = 0;
}

uint8 NetworkMessage::getVersionFromBuffer (const char* pBuf, uint32 ui32BufSize)
{
    if (ui32BufSize > 0) {
        return (*((uint8*)(pBuf + VERSION_AND_TYPE_FIELD_OFFSET))) >> 4;
    }
    else {
        return 0;
    }
}

bool NetworkMessage::isMalformed (void) const
{
    return true;
}

uint16 NetworkMessage::getFixedHeaderLength (void) const
{
    return 0;
}

uint8 NetworkMessage::getVersion (void) const
{
    return *((uint8*)(_pBuf + VERSION_AND_TYPE_FIELD_OFFSET))>>4;
}

uint8 NetworkMessage::getType (void) const
{
    return *((uint8*)(_pBuf + VERSION_AND_TYPE_FIELD_OFFSET)) & 0x0F;
}

uint8 NetworkMessage::getMsgType (void) const
{
    return *((uint8*)(_pBuf + MSG_TYPE_FIELD_OFFSET));
}

uint32 NetworkMessage::getSourceAddr (void) const
{
    return *((uint32*)(_pBuf + SOURCE_ADDRESS_FIELD_OFFSET));
}

uint32 NetworkMessage::getDestinationAddr (void) const
{
    return *((uint32*)(_pBuf + TARGET_ADDRESS_FIELD_OFFSET));
}

uint16 NetworkMessage::getSessionId (void) const
{
    return *((uint16*)(_pBuf + SESSION_ID_FIELD_OFFSET));
}

uint16 NetworkMessage::getMsgId (void) const
{
    return *((uint16*)(_pBuf + MSG_ID_FIELD_OFFSET));
}

uint8 NetworkMessage::getHopCount (void) const
{
    return *((uint8*)(_pBuf + HOP_COUNT_FIELD_OFFSET));
}

uint8 NetworkMessage::getTTL (void) const
{
    return *((uint8*)(_pBuf + TTL_FIELD_OFFSET));
}

uint8 NetworkMessage::getChunkType (void) const
{
    return *((uint8*)(_pBuf + CHUNK_TYPE_FIELD));
}

bool NetworkMessage::isReliableMsg (void) const
{
    uint8 ui8Rel = *((uint8*)(_pBuf + RELIABLE_FIELD));
    return (ui8Rel == 1);
}

void * NetworkMessage::getMetaData (void) const
{
    return NULL;
}

uint16 NetworkMessage::getMetaDataLen (void) const
{
    return 0;
}

void * NetworkMessage::getMsg (void) const
{
    return NULL;
}

uint16 NetworkMessage::getMsgLen (void) const
{
    return 0;
}

void * NetworkMessage::getBuf(void) const
{
    return _pBuf;
}

void NetworkMessage::setTargetAddress (uint32 ui32TargetAddress)
{
    *((uint32*)(_pBuf + TARGET_ADDRESS_FIELD_OFFSET)) = ui32TargetAddress;
}

char *returnNullTerminated (char * pchStr, uint16 ui16len)
{
    char * pchTmpMeta = new char[ui16len+1];
    memcpy (pchTmpMeta, pchStr, ui16len);
    pchTmpMeta[ui16len] ='\0';
    return pchTmpMeta;
}

void NetworkMessage::display() const
{
    const char * const pszMethod = "NetworkMessage::display";
    checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Message %u:%u\n", getSourceAddr(), getMsgId());
    checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Version: %u\n", getVersion());
    checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Type: %u\n", getType());
    checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Length: %u\n", getLength());
    checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Message Type: %u\n", getMsgType());
    checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Source add: %u\n", getSourceAddr());
    checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Target add: %u\n", getDestinationAddr());

    switch (getChunkType ()) {
        case CT_SAck: {
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "CHUNK TYPE: CT_SAck\n");
            break;
        }
        case CT_DataMsgComplete: {
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Msg id: %u\n", getMsgId());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Hop count: %u\n", getHopCount());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "TTL: %u\n", getTTL());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "CHUNK TYPE: CT_DataMsgComplete\n");
            break;
        }
        case CT_DataMsgStart: {
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Msg id: %u\n", getMsgId());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Hop count: %u\n", getHopCount());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "TTL: %u\n", getTTL());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "CHUNK TYPE: CT_DataMsgStart\n");
            break;
        }
        case CT_DataMsgInter: {
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Msg id: %u\n", getMsgId());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Hop count: %u\n", getHopCount());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "TTL: %u\n", getTTL());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "CHUNK TYPE: CT_DataMsgInter\n");
            break;
        }
        case CT_DataMsgEnd: {
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Msg id: %u\n", getMsgId());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Hop count: %u\n", getHopCount());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "TTL: %u\n", getTTL());
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "CHUNK TYPE: CT_DataMsgEnd\n");
            break;
        }
    }
    checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Metadata length: %u\n", getMetaDataLen());
    checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Message length: %u\n", getMsgLen());
    if ((getMetaData() != NULL) && (getChunkType() != CT_SAck)) {
        char* pchMeta = (char*)getMetaData();
        char * pszMetaData = returnNullTerminated(pchMeta, getMetaDataLen());
        checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Metadata: %s\n", pszMetaData);
        delete [] pszMetaData;
        pszMetaData = NULL;
    }
    if ((getMsg() != NULL) && (getChunkType() != CT_SAck)) {
        char* pchMsg = (char*)getMsg();

        char * pszData = returnNullTerminated (pchMsg, getMsgLen());
        checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Message: %s\n", pszData);
        delete [] pszData;
        pszData = NULL;
    }
    checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "______________________\n");
}
/*
void NetworkMessage::dump (FILE *file)
{
    fprintf(file, "_____    Message %u:/%u     _____\n", getSourceAddr(), getMsgId());
    fprintf(file, "Version: %u\t", getVersion());
    fprintf(file, "Type: %u\t", getType());
    fprintf(file, "Total Length: %u\t", getLength());
    fprintf(file, "Message Type: %u\t", getMsgType());
    fprintf(file, "Source add: %u\t", getSourceAddr ());
    fprintf(file, "Target add: %u\t", getDestinationAddr ());
    fprintf(file, "Msg id: %u\t", getMsgId ());
    fprintf(file, "Hop count: %u\t", getHopCount  ());
    fprintf(file, "TTL: %u\t", getTTL ());
    switch (getChunkType ()) {
        case CT_SAck: {
            fprintf(file, "CHUNK TYPE: CT_SAck\t");
            break;
        }
        case CT_DataMsgComplete: {
            fprintf(file, "CHUNK TYPE: CT_DataMsgComplete\t");
            break;
        }
        case CT_DataMsgStart: {
            fprintf(file, "CHUNK TYPE: CT_DataMsgStart\t");
            break;
        }
        case CT_DataMsgInter: {
            fprintf(file, "CHUNK TYPE: CT_DataMsgInter\t");
            break;
        }
        case CT_DataMsgEnd: {
            fprintf(file, "CHUNK TYPE: CT_DataMsgEnd\t");
            break;
        }
    }
    fprintf(file, "Metadata length: %u\t", getMetaDataLen ());
    fprintf(file, "Message length: %u\t", getMsgLen ());

    if (getMetaData() != NULL) {
        char* pchMeta = (char*)getMetaData();
        char * pszMetaData = returnNullTerminated(pchMeta, getMetaDataLen());
        fprintf(file, "Metadata: %s\t", pszMetaData);
        delete [] pszMetaData;
        pszMetaData = NULL;
    }
    if (getMsg() != NULL) {
        char* pchMsg = (char*)getMsg();
        char * pszData = returnNullTerminated(pchMsg, getMsgLen());
        fprintf(file, "Message: %s\t", pszData);
        delete [] pszData;
        pszData = NULL;
    }
    fprintf (file, "\n");
}*/

void NetworkMessage::incrementHopCount (void)
{
    (*((uint8*)(_pBuf + HOP_COUNT_FIELD_OFFSET)))++;
}
