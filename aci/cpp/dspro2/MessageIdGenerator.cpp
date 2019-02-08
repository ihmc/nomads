/*
 * MessageIdGenerator.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 */

#include "MessageIdGenerator.h"

#include "Defs.h"
#include "DSSFLib.h"

#include "DArray2.h"
#include "Logger.h"
#include "NLFLib.h"
#include "PropertyStoreInterface.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const char * MessageIdGenerator::LATEST_SEQ_ID_PROPERTY_PREFIX = "dspro.latestMsgSeqId.";

MessageIdGenerator::MessageIdGenerator (const char *pszNodeId,
                                        const char *pszGroupName,
                                        PropertyStoreInterface *pPropertyStore)
    : _nodeId (pszNodeId),
      _groupName (pszGroupName),
      _pPropertyStore (pPropertyStore),
      _seqIdsByGrp (true, // bCaseSensitiveKeys
                    true, // bCloneKeys
                    true, // bDeleteKeys
                    true) // bDeleteValues
{
}

MessageIdGenerator::~MessageIdGenerator()
{
}

char * MessageIdGenerator::getMsgId (const char *pszGroupName, bool bDisseminate)
{
    if (pszGroupName == nullptr) {
        return nullptr;
    }
    String grp (bDisseminate ? pszGroupName : _groupName.c_str());
    if (!bDisseminate) {
        grp += ".";
        grp += pszGroupName;
    }

    return getIdInternal (grp);
}

char * MessageIdGenerator::chunkId (const char *pszGroupName, bool bDisseminate)
{
    if (pszGroupName == nullptr) {
        return nullptr;
    }
    String grp (bDisseminate ? pszGroupName : _groupName.c_str());
    if (!bDisseminate) {
        grp += ".";
        grp += pszGroupName;
    }

    char *pszChunkGrp = getOnDemandDataGroupName (grp);
    if (pszChunkGrp == nullptr) {
        return nullptr;
    }

    String chunGrp (pszChunkGrp);
    free (pszChunkGrp);

    return getIdInternal (chunGrp);
}

char * MessageIdGenerator::getIdInternal (NOMADSUtil::String &group)
{
    String property (LATEST_SEQ_ID_PROPERTY_PREFIX);
    property += group;

    // get seq id for the group
    uint32 *pui32SeqId = _seqIdsByGrp.get (group.c_str());
    if (pui32SeqId == nullptr) {
        // Look whether there's a message sequence id value in the property store
        String sSeqId = _pPropertyStore->get (_nodeId, property);

        pui32SeqId = (uint32*) malloc (sizeof (uint32));
        if (pui32SeqId == nullptr) {
            checkAndLogMsg ("MessageIdGenerator::getMsgId", memoryExhausted);
            return nullptr;
        }
        if (sSeqId.length() <= 0) {
            *pui32SeqId = 0;
        }
        else {
            *pui32SeqId = (atoui32 (sSeqId.c_str()) + 1);
        }

        _seqIdsByGrp.put (group.c_str(), pui32SeqId);
    }

    // Store sequence id in the property store
    char seqId[23];
    i64toa (seqId, *pui32SeqId);
    if (*pui32SeqId == 0) {
        // it's the seq ID for the group, it needs to be inserted
        _pPropertyStore->set (_nodeId, property, seqId);
    }
    else {
        // a seq ID was found, I just need to update the value if the highest seq ID
        _pPropertyStore->update (_nodeId, property, seqId);
    }

    char *pszId = convertFieldToKey (group.c_str(), _nodeId, *pui32SeqId);
    if (pszId != nullptr) {
        (*pui32SeqId)++;
    }
    return pszId;
}

String MessageIdGenerator::extractSubgroupFromMsgId (const char *pszMsgId)
{
    if (pszMsgId == nullptr) {
        return String();
    }
    DArray2<String> fields (1U);
    int rc = convertKeyToField (pszMsgId, fields, 1, MSG_ID_GROUP);
    if (rc < 0 || !fields.used (MSG_ID_GROUP)) {
        return String();
    }
    return extractSubgroupFromMsgGroup (fields[MSG_ID_GROUP]);
}

String MessageIdGenerator::extractSubgroupFromMsgGroup (const char *pszGroupName)
{
    if (pszGroupName == nullptr) {
        return String();
    }
    String sMsgId (pszGroupName);
    int iRootGrpDelimiterPos = sMsgId.indexOf (".");
    if (iRootGrpDelimiterPos < 0) {
        return String (pszGroupName);
    }
    int iGrpLen = sMsgId.length();
    if (iGrpLen <= 1) {
        return String();
    }
    return sMsgId.substring (iRootGrpDelimiterPos + 1, iGrpLen);
}

