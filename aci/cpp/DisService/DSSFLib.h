/*
 * DSSFLib.h
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
 *
 * DisseminationService's Small Function Library
 * 
 * Small function library which gathers some widely
 * used and cross class functions used in the DisService
 * Project.
 */

#ifndef INCL_DSSF_LIB_H
#define INCL_DSSF_LIB_H

#include "DArray2.h"
#include "FTypes.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class String;
}

namespace IHMC_ACI
{
    class MessageHeader;

    enum MessageIdFields {
        MSG_ID_GROUP    = 0x00,
        MSG_ID_SENDER   = 0x01,
        MSG_ID_SEQ_NUM  = 0x02,
        MSG_ID_CHUNK_ID = 0x03,
        MSG_ID_OFFSET   = 0x04,
        MSG_ID_LENGTH   = 0x05
    };

    static NOMADSUtil::String ON_DEMAND_DATA_GROUP_SUFFIX = "[od]";

    bool checkGroupName (const char *pszGroupName);

    char * getOnDemandDataGroupName (const char *pszGroupName);

    /**
     * Returns the message id that represents all the chunks of this message (aka the "complete" message ID)
     */
    const char * getAllChunksMessageID (MessageHeader *pMH);

    char * addChunkIdToCompleteMsgId (const char *pszKey, uint8 ui8ChunkId);

    /**
     * Returns true if the message id specified in pszKey is the ID of either
     * a complete data message or a complete large objects.
     * In other words it's in the form:
     * groupName:senderNodeId:seqId
     */
    char * addOnDemandSuffixToId (const char *pszKey);
    char * removeOnDemandSuffixFromId (const char *pszKey);
    char * removeOnDemandSuffixFromGroupName (const char *pszKey);

    // Returns true if the id only specifies <GroupName>:<OriginatorNodeId>:<MsgSeqId> and nothing else
    bool isAllChunksMessageID (const char *pszKey);

    // Return true if the id completely specifies <GroupName>:<OriginatorNodeId>:<MsgSeqId>:<FragmentOffset>:<FragmentLength>
    bool isFullyQualifiedMessageID (const char *pszKey);

    bool isOnDemandGroupName (const char *pszGroupName);
    bool isOnDemandDataID (const char *pszId);

    /**
     * Given a valid key, the function tokenizes it in the different fields.
     * Returns the number of fields in which the key has been tokenized.
     *
     * - ui8FieldNumber: limits the number of field that are needed.
     *   If ui8FieldNumber = 0 every field is tokenized.
     * - A subset of the flags in MessageIdFields are accepted.
     */
    int convertKeyToField (const char *pszKey, NOMADSUtil::DArray2<NOMADSUtil::String> &aTokenizedKey,
                           unsigned short usFieldNumber=0, ...);
    int tokenizeKey (const char *pszKey, NOMADSUtil::DArray2<NOMADSUtil::String> &aTokenizedKey);

    /**
     * Returns a valid key.
     * NOTE: the returned key must be deleted by the caller.
     *
     * NOTE: for efficiency reasons MessageInfo and SqLiteInterface::execQueryAndReturnKey create
     *       the key without calling convertFieldToKey.
     *       Keep
     *          MessageInfo::read, MessageInfo::MessageInfo, MessageInfo::regenerateMsgId and
     *          SqLiteInterface::execQueryAndReturnKey
     *       updated in compliance with convertFieldToKey.
     */
    char * convertFieldToKey (const char *pszGroupName, const char *pszSenderNodeId,
                              uint32 ui32SeqId);
    char * convertFieldToKey (const char *pszGroupName, const char *pszSenderNodeId,
                              const char *pszMsgSeqId);
    char * convertFieldToKey (const char *pszGroupName, const char *pszSenderNodeId,
                              uint32 ui32SeqId, uint8 ui8ChunkId,
                              uint32 ui32FragmentLength, uint32 ui32FragmentOffest);

    NOMADSUtil::String extractGroupFromKey (const char *pszKey);
    NOMADSUtil::String extractSenderNodeIdFromKey (const char *pszKey);
    char * toAllChunksMessageId (const char *pszKey);
}

#endif // end INCL_DSSF_LIB_H
