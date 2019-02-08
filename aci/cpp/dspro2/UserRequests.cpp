/*
 * UserRequests.cpp
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include "UserRequests.h"

#include "Defs.h"

#include "DSSFLib.h"

#include "Logger.h"
#include "NLFLib.h"
#include "StringHashtable.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

UserRequests::UserRequests()
    : _byGroupName (true, true, true, true),
      _m (MutexId::UserRequests_m, LOG_MUTEX)
{
}

UserRequests::~UserRequests()
{
}

bool UserRequests::contains (const char *pszMsgId, String &queryId)
{
    queryId = "";

    if (pszMsgId == nullptr) {
        return false;
    }

    DArray2<String> aTokenizedKey;
    if (convertKeyToField (pszMsgId, aTokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
        checkAndLogMsg ("UserRequests::contains", Logger::L_Warning, "could not parse key\n");
        return false;
    }

    return contains (aTokenizedKey[MSG_ID_GROUP].c_str(), aTokenizedKey[MSG_ID_SENDER].c_str(),
                     atoui32 (aTokenizedKey[MSG_ID_SEQ_NUM].c_str()), queryId);
}

bool UserRequests::contains (const char *pszGroupName, const char*pszSenderNoded, uint32 ui32MsgSeqId, String &queryId)
{
    queryId = "";

    _m.lock (1085);
    BySender *pBySender = _byGroupName.get (pszGroupName);
    if (pBySender == nullptr) {
        _m.unlock (1085);
        return false;
    }

    BySeqId *pBySeqId = pBySender->get (pszSenderNoded);
    if (pBySeqId == nullptr) {
        _m.unlock (1085);
        return false;
    }

    bool bRet = pBySeqId->sedIds.hasTSN (ui32MsgSeqId);
    String *pString = pBySeqId->seqIdToQueryId.get (ui32MsgSeqId);
    if (pString != nullptr) {
        queryId = pString->c_str(); // queryId = operator makes a copy
    }

    _m.unlock (1085);
    return bRet;
}

int UserRequests::put (const char *pszMsgId)
{
    return put (pszMsgId, nullptr);
}

int UserRequests::put (const char *pszGroupName, const char *pszSenderNoded, uint32 ui32MsgSeqId)
{
    return put (pszGroupName, pszSenderNoded, ui32MsgSeqId, nullptr);
}

int UserRequests::put (const char *pszMsgId, const char *pszQueryId)
{
    if (pszMsgId == nullptr) {
        return -1;
    }

    DArray2<String> aTokenizedKey;
    if (convertKeyToField (pszMsgId, aTokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
        checkAndLogMsg ("UserRequests::put", Logger::L_Warning, "could not parse key\n");
        return -2;
    }

    return put (aTokenizedKey[MSG_ID_GROUP].c_str(), aTokenizedKey[MSG_ID_SENDER].c_str(),
                atoui32 (aTokenizedKey[MSG_ID_SEQ_NUM].c_str()), pszQueryId);
}

int UserRequests::put (const char *pszGroupName, const char *pszSenderNoded, uint32 ui32MsgSeqId, const char *pszQueryId)
{
    if (pszGroupName == nullptr || pszSenderNoded == nullptr) {
        return -1;
    }

    _m.lock (1086);
    BySender *pBySender = _byGroupName.get (pszGroupName);
    if (pBySender == nullptr) {
        pBySender = new BySender (true, true, true, true);
        if (pBySender == nullptr) {
            checkAndLogMsg ("UserRequests::put", memoryExhausted);
            _m.unlock (1086);
            return -2;
        }
        _byGroupName.put (pszGroupName, pBySender);
    }

    BySeqId *pBySeqId = pBySender->get (pszSenderNoded);
    if (pBySeqId == nullptr) {
        pBySeqId = new BySeqId();
        if (pBySeqId == nullptr) {
            checkAndLogMsg ("UserRequests::put", memoryExhausted);
            _m.unlock (1086);
            return -3;
        }
        pBySender->put (pszSenderNoded, pBySeqId);
    }

    int rc = pBySeqId->sedIds.addTSN (ui32MsgSeqId);
    if (rc < 0) {
        checkAndLogMsg ("UserRequests::put", Logger::L_Warning, "could not insert TSN\n");
        _m.unlock (1086);
        return -4;
    }
    if (pszQueryId != nullptr) {
        String *pQueryId = new String (pszQueryId);
        if (pQueryId != nullptr) {
            pBySeqId->seqIdToQueryId.put (ui32MsgSeqId, pQueryId);
        }
    }

    _m.unlock (1086);
    return rc == 1 ? 0 : 1;
}

int UserRequests::remove (const char *pszMsgId)
{
    if (pszMsgId == nullptr) {
        return -1;
    }

    DArray2<String> aTokenizedKey;
    if (convertKeyToField (pszMsgId, aTokenizedKey, 3, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM) < 0) {
        checkAndLogMsg ("UserRequests::put", Logger::L_Warning, "could not parse key\n");
        return -2;
    }

    return remove (aTokenizedKey[MSG_ID_GROUP].c_str(), aTokenizedKey[MSG_ID_SENDER].c_str(),
                   atoui32 (aTokenizedKey[MSG_ID_SEQ_NUM].c_str()));
}

int UserRequests::remove (const char *pszGroupName, const char *pszSenderNoded, uint32 ui32MsgSeqId)
{
    if (pszGroupName == nullptr || pszSenderNoded == nullptr) {
        return -1;
    }

    _m.lock (1087);
    BySender *pBySender = _byGroupName.get (pszGroupName);
    if (pBySender == nullptr) {
        _m.unlock (1087);
        return 0;
    }

    BySeqId *pBySeqId = pBySender->get (pszSenderNoded);
    if (pBySeqId == nullptr) {
        _m.unlock (1087);
        return 0;
    }

    pBySeqId->sedIds.removeTSN (ui32MsgSeqId);
    _m.unlock (1087);
    return 0;
}

UserRequests::BySeqId::BySeqId (void)
    : sedIds (true)
{
}

UserRequests::BySeqId::~BySeqId (void)
{
}

