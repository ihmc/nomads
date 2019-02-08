/*
 * DSSFLib.cpp
 *
 * This file is part of the IHMC DisService Library/Component
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

#include "DSSFLib.h"

#include "MessageInfo.h"

#include "NLFLib.h"
#include "StringTokenizer.h"
#include "StrClass.h"

#include <stdarg.h>
#include <string.h>

#if defined (UNIX)
    #include <strings.h>
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

using namespace NOMADSUtil;
using namespace IHMC_ACI;

bool IHMC_ACI::checkGroupName (const char *pszGroupName)
{
    if ((strstr (pszGroupName, "[") != NULL) ||
        (strstr (pszGroupName, "]") != NULL)) {
        return false;
    }
    return true;
}

char * IHMC_ACI::getOnDemandDataGroupName (const char *pszGroupName)
{
    if (pszGroupName == NULL) {
        return NULL;
    }
    unsigned int uiLen = strlen (pszGroupName);
    if (uiLen == 0) {
        return NULL;
    }
    unsigned int uiGrpNameLen = uiLen;
    if (pszGroupName[uiLen-1] == '*') { // it's a template
        uiGrpNameLen--;
    }
    String odGrpName (pszGroupName, uiGrpNameLen);
    odGrpName += ".";
    odGrpName += ON_DEMAND_DATA_GROUP_SUFFIX;
    if (pszGroupName[uiLen-1] == '*') {
        odGrpName += ".*";
    }
    return odGrpName.r_str();
}

const char * IHMC_ACI::getAllChunksMessageID (MessageHeader *pMH)
{
    if (pMH == NULL) {
        return NULL;
    }
    return convertFieldToKey (pMH->getGroupName(), pMH->getPublisherNodeId(),
                              pMH->getMsgSeqId(), pMH->getChunkId(),
                              pMH->getFragmentOffset(), pMH->getFragmentLength());
}

char * IHMC_ACI::addChunkIdToCompleteMsgId (const char *pszKey, uint8 ui8ChunkId)
{
    String msgId (pszKey);
    msgId = msgId + MessageHeader::MSG_ID_SEPARATOR;

    char szBuf[12];
    itoa (szBuf, ui8ChunkId);
    msgId = msgId + szBuf;

    return msgId.r_str();
}

char * IHMC_ACI::addOnDemandSuffixToId (const char *pszKey)
{
    if (pszKey == NULL) {
        return NULL;
    }

    DArray2<String> aTokenizedKey (6U);
    if (isAllChunksMessageID (pszKey)) {
        int rc = convertKeyToField (pszKey, aTokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM);
        if (rc != 0) {
            return NULL;
        }
    }
    else if (convertKeyToField (pszKey, aTokenizedKey) != 0) {
        return NULL;
    }

    char *pszNewGroupName = getOnDemandDataGroupName (aTokenizedKey[MSG_ID_GROUP].c_str());
    String msgId (pszNewGroupName);
    for (unsigned int i = MSG_ID_SENDER; i < aTokenizedKey.size(); i++) {
        msgId += MessageHeader::MSG_ID_SEPARATOR;
        msgId += aTokenizedKey[i];
    }
    return msgId.r_str();

    free (pszNewGroupName);
    return msgId.r_str();
}

char * IHMC_ACI::removeOnDemandSuffixFromId (const char *pszKey)
{
    if (pszKey == NULL) {
        return NULL;
    }

    DArray2<String> aTokenizedKey (6U);
    if (isAllChunksMessageID (pszKey)) {
        int rc = convertKeyToField (pszKey, aTokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM);
        if (rc != 0) {
            return NULL;
        }
    }
    else if (convertKeyToField (pszKey, aTokenizedKey) != 0) {
        return NULL;
    }

    char *pszNewGroupName = removeOnDemandSuffixFromGroupName (aTokenizedKey[MSG_ID_GROUP].c_str());
    String msgId (pszNewGroupName);
    for (unsigned int i = MSG_ID_SENDER; i < aTokenizedKey.size(); i++) {
        msgId += MessageHeader::MSG_ID_SEPARATOR;
        msgId += aTokenizedKey[i];
    }
    return msgId.r_str();
}

char * IHMC_ACI::removeOnDemandSuffixFromGroupName (const char *pszGroupName)
{
    if (pszGroupName == NULL) {
        return NULL;
    }
    char *pszGroupNameCopy = strDup (pszGroupName);
    char *pszTemp = strrchr (pszGroupNameCopy, '.');
    if (pszTemp != NULL) {
        if (0 == stricmp (pszTemp+1, ON_DEMAND_DATA_GROUP_SUFFIX)) {
            *pszTemp = '\0';
        }
    }
    return pszGroupNameCopy;
}

bool IHMC_ACI::isAllChunksMessageID (const char *pszKey)
{
    if (pszKey == NULL) {
        return false;
    }
    size_t len = strlen (pszKey);
    const char chSeparator = ((const char *)MessageHeader::MSG_ID_SEPARATOR)[0];
    unsigned short usCount = 0;
    for (size_t i = 0; i < len; i++) {
        if (pszKey[i] == chSeparator) {
            usCount++;
            if (usCount > 2) {
                return false;
            }
        }
    }
    return (usCount == 2);
}

bool IHMC_ACI::isFullyQualifiedMessageID (const char *pszKey)
{
    if (pszKey == NULL) {
        return false;
    }
    uint16 ui16Count = 0;
    char chSeparator = ((const char *)MessageHeader::MSG_ID_SEPARATOR)[0];
    while (*pszKey != '\0') {
        if (*pszKey++ == chSeparator) {
            ui16Count++;
        }
    }
    return (ui16Count == 5);
}

bool IHMC_ACI::isOnDemandGroupName (const char *pszGroupName)
{
    if (pszGroupName == NULL) {
        return false;
    }
    unsigned int uiLen = (unsigned int) strlen (pszGroupName);
    if (uiLen < (unsigned int) ON_DEMAND_DATA_GROUP_SUFFIX.length()) {
        return false;
    }

    for (unsigned int i = ON_DEMAND_DATA_GROUP_SUFFIX.length(); i > 0; i--, uiLen--) {
        if (pszGroupName[uiLen-1] != (ON_DEMAND_DATA_GROUP_SUFFIX.c_str())[i-1]) {
            return false;
        }
    }

    return true;
}

bool IHMC_ACI::isOnDemandDataID (const char *pszId)
{
    if (pszId == NULL) {
        return false;
    }
    DArray2<String> aTokenizedKey (1);
    if (convertKeyToField (pszId, aTokenizedKey, 1, MSG_ID_GROUP) < 0) {
        return false;
    }
    return isOnDemandGroupName (aTokenizedKey[MSG_ID_GROUP].c_str());
}

int IHMC_ACI::convertKeyToField (const char *pszKey, DArray2<String>& aTokenizedKey,
                                 unsigned short usFieldNumber, ...)
{
    if (pszKey == NULL) {
        return -1;
    }
    if (usFieldNumber > 6) {
        return -2;
    }

    uint8 ui8Greatest;
    if (usFieldNumber > 0) {
        va_list vl;
        va_start (vl, usFieldNumber);
        uint8 ui8Val;
        ui8Greatest = 0;
        for (uint8 i = 0; i < usFieldNumber; i++) {
            ui8Val = va_arg (vl, int);
            switch (ui8Val) {
                case MSG_ID_GROUP:
                case MSG_ID_SENDER:
                case MSG_ID_SEQ_NUM:
                case MSG_ID_CHUNK_ID:
                case MSG_ID_OFFSET:
                case MSG_ID_LENGTH:
                    if (ui8Val > ui8Greatest) {
                        ui8Greatest = ui8Val;
                    }
                    break;

                default:
                    // unknown field requested
                    va_end (vl);
                    return -2;
            }
        }
        va_end (vl);
    }
    else {
        ui8Greatest = 5;
    }
    assert (strlen ((const char *)MessageHeader::MSG_ID_SEPARATOR) == 1); // the separator must be a char
    StringTokenizer st (pszKey , ((const char *)MessageHeader::MSG_ID_SEPARATOR)[0]);

    uint8 i = 0;
    for (const char *pszToken; (i <= ui8Greatest) && (NULL != (pszToken = st.getNextToken())); i++) {
        aTokenizedKey[i] = pszToken;
    }

    assert ((i-1) == ui8Greatest);
    return ((i-1) == ui8Greatest ? 0 : -3);
}

int IHMC_ACI::tokenizeKey (const char *pszKey, DArray2<String> &aTokenizedKey)
{
    StringTokenizer st (pszKey , ((const char *)MessageHeader::MSG_ID_SEPARATOR)[0]);
    const char *pszToken;

    for (uint8 i = 0; NULL != (pszToken = st.getNextToken()); i++) {
        aTokenizedKey[i] = pszToken;
    }

    bool bRet = (aTokenizedKey.size() >= 3 && aTokenizedKey.size() <= 6);
    assert (bRet);
    return bRet;
}

char * IHMC_ACI::convertFieldToKey (const char *pszGroupName, const char *pszSenderNodeId,
                                    uint32 ui32SeqId)
{
    if (pszGroupName == NULL || pszSenderNodeId == NULL) {
        return NULL;
    }

    char szBuf[12];
    itoa (szBuf, ui32SeqId);

    return convertFieldToKey (pszGroupName, pszSenderNodeId, szBuf);
}

char * IHMC_ACI::convertFieldToKey (const char *pszGroupName, const char *pszSenderNodeId,
                                    const char *pszMsgSeqId)
{
    if (pszGroupName == NULL || pszSenderNodeId == NULL || pszMsgSeqId == NULL) {
        return NULL;
    }

    String msgId = (pszGroupName);
    msgId = msgId + MessageHeader::MSG_ID_SEPARATOR;
    msgId = msgId + pszSenderNodeId;
    msgId = msgId + MessageHeader::MSG_ID_SEPARATOR;
    msgId = msgId + pszMsgSeqId;

    return msgId.r_str();
}

char * IHMC_ACI::convertFieldToKey (const char *pszGroupName, const char *pszSenderNodeId,
                                    uint32 ui32SeqId, uint8 ui8ChunkId,
                                    uint32 ui32FragmentLength, uint32 ui32FragmentOffest)
{
    if (pszGroupName == NULL || pszSenderNodeId == NULL) {
        return NULL;
    }

    String msgId = "";

    char szBuf[12];

    msgId = msgId + pszGroupName;
    msgId = msgId + MessageHeader::MSG_ID_SEPARATOR;
    msgId = msgId + pszSenderNodeId;

    itoa (szBuf, ui32SeqId);
    msgId = (String) msgId + MessageHeader::MSG_ID_SEPARATOR + szBuf;

    itoa (szBuf, ui8ChunkId);
    msgId = (String) msgId + MessageHeader::MSG_ID_SEPARATOR + szBuf;

    itoa (szBuf, ui32FragmentOffest);
    msgId = (String) msgId + MessageHeader::MSG_ID_SEPARATOR + szBuf;

    itoa (szBuf, ui32FragmentLength);
    msgId = (String) msgId + MessageHeader::MSG_ID_SEPARATOR + szBuf;

    return strDup ((const char *) msgId);
}

String IHMC_ACI::extractGroupFromKey (const char *pszKey)
{
    DArray2<String> aTokenizedKey;
    convertKeyToField (pszKey, aTokenizedKey, 1U, MSG_ID_GROUP);
    return String (aTokenizedKey[MSG_ID_GROUP]);
}

String IHMC_ACI::extractSenderNodeIdFromKey (const char *pszKey)
{
    DArray2<String> aTokenizedKey;
    convertKeyToField (pszKey, aTokenizedKey, 1U, MSG_ID_SENDER);
    return String (aTokenizedKey[MSG_ID_SENDER]);
}

char * IHMC_ACI::toAllChunksMessageId (const char *pszKey)
{
    if (pszKey == NULL) {
        return NULL;
    }
    DArray2<String> tokens (3U);
    if (convertKeyToField (pszKey, tokens, 3,
                           MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) != 0) {
        return NULL;
    }
    return convertFieldToKey (tokens[MSG_ID_GROUP], tokens[MSG_ID_SENDER],
                              atoui32 (tokens[MSG_ID_SEQ_NUM]));
}

