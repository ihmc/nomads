/*
 * SchedulerCache.cpp
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 25, 2014, 5:48 PM
 */

#include "SchedulerCache.h"

#include "DSSFLib.h"
#include "PropertyStoreInterface.h"
#include "Voi.h"

#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

const char * SchedulerCache::LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY = "latestMessagePushedToTarget";
const char * SchedulerCache::LATEST_MESSAGE_TO_TARGET_SOURCE_TIMESTAMP_PROPERTY = "latestMessagePushedToTargetTStamp";
const char * SchedulerCache::LATEST_RESET_MESSAGE_PROPERTY = "latestResetMessage";

SchedulerCache::SchedulerCache (PropertyStoreInterface *pPropertyStore, IHMC_VOI::Voi *pVoi)
    : _pPropertyStore (pPropertyStore),
      _pVoi (pVoi)
{
}

SchedulerCache::~SchedulerCache (void)
{
}

String SchedulerCache::getLatestResetMessageId (void)
{
    _m.lock();
    String tmp (_pPropertyStore->get ("*", LATEST_RESET_MESSAGE_PROPERTY));
    _m.unlock();
    return tmp;
}

String SchedulerCache::getLatestMessageIdPushedToTarget (const char *pszTarget)
{
    if (pszTarget == nullptr) {
        String tmp; // empty string
        return tmp;
    }

    _m.lock();
    String tmp (_pPropertyStore->get (pszTarget, LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY));
    _m.unlock();
    return tmp;
}

int SchedulerCache::setLatestMessageIdPushedToTarget (const char *pszTarget, const char *pszLatestMessageId,
                                                      MetadataInterface *pMetadata)
{
    if (pszTarget == nullptr || pszLatestMessageId == nullptr) {
        return -1;
    }

    _m.lock();

    int rc;
    const String previousMessageId = getLatestMessageIdPushedToTargetInternal (pszTarget);
    if (previousMessageId.length() <= 0) {
        // LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY was never added for pszTarget: set it
        rc = _pPropertyStore->set (pszTarget, LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY, pszLatestMessageId);
    }
    else {
        // LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY was already added for pszTarget: update it
        rc = _pPropertyStore->update (pszTarget, LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY, pszLatestMessageId);
    }

    setLatestResetMessageId (pszLatestMessageId);

    _pVoi->addMetadataForPeer (pszTarget, pMetadata, nullptr, 0);

    _m.unlock();
    return rc;
}

int SchedulerCache::resetLatestMessageIdPushedToTarget (const char *pszTarget)
{
    if (pszTarget == nullptr) {
        return -1;
    }

    _pPropertyStore->remove (pszTarget, LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY);

    return 0;
}

int64 SchedulerCache::getMostRecentMessageTimestamp (const char *pszTarget, const char *pszObjectId)
{
    _m.lock();
    int64 i64Timestamp = getMostRecentMessageTimestampInternal (pszTarget, pszObjectId);
    _m.unlock();
    return i64Timestamp;
}

int SchedulerCache::setMostRecentMessageTimestamp (const char *pszTarget, const char *pszObjectId, int64 i64Timestamp)
{
    if (pszTarget == nullptr || pszObjectId == nullptr || i64Timestamp <= 0) {
        return -1;
    }

    char chTimestamp[22];
    i64toa (chTimestamp, i64Timestamp);

    String property (LATEST_MESSAGE_TO_TARGET_SOURCE_TIMESTAMP_PROPERTY);
    property += ":";
    property += pszObjectId;

    _m.lock();

    int rc;
    const int64 i64OldTimestamp = getMostRecentMessageTimestampInternal (pszTarget, pszObjectId);
    if (i64OldTimestamp <= 0) {
        // LATEST_MESSAGE_TO_TARGET_SOURCE_TIMESTAMP_PROPERTY was never added for pszTarget: set it
        rc = _pPropertyStore->set (pszTarget, property, chTimestamp);
    }
    else {
        // LATEST_MESSAGE_TO_TARGET_SOURCE_TIMESTAMP_PROPERTY was already added for pszTarget: update it
        rc = _pPropertyStore->update (pszTarget, property, chTimestamp);
    }

    _m.unlock();
    return rc;
}

String SchedulerCache::getLatestMessageIdPushedToTargetInternal (const char *pszTarget)
{
    if (pszTarget == nullptr) {
        return String();    // empty string
    }

    return _pPropertyStore->get (pszTarget, LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY);
}

int64 SchedulerCache::getMostRecentMessageTimestampInternal (const char *pszTarget, const char *pszObjectId)
{
    if (pszTarget == nullptr || pszObjectId == nullptr) {
        return -1;
    }

    String property (LATEST_MESSAGE_TO_TARGET_SOURCE_TIMESTAMP_PROPERTY);
    property += ":";
    property += pszObjectId;

    String val = _pPropertyStore->get (pszTarget, property);
    if (val.length() >= 0) {
        return atoi64 (val);
    }
    return -1;
}

int SchedulerCache::setLatestResetMessageId (const char *pszLatestResetMessageId)
{
    if (pszLatestResetMessageId == nullptr) {
        return -1;
    }
    int rc = 0;
    String group (extractGroupFromKey (pszLatestResetMessageId));
    group.convertToLowerCase ();
    if (wildcardStringCompare (group, "*reset")) {
        const String anyNodeId ("*");
        const String oldId (_pPropertyStore->get (anyNodeId, LATEST_RESET_MESSAGE_PROPERTY));
        if (oldId.length () <= 0) {
            // LATEST_MESSAGE_TO_TARGET_SOURCE_TIMESTAMP_PROPERTY was never added for pszTarget: set it
            rc = _pPropertyStore->set (anyNodeId, LATEST_RESET_MESSAGE_PROPERTY, pszLatestResetMessageId);
        }
        else {
            // LATEST_MESSAGE_TO_TARGET_SOURCE_TIMESTAMP_PROPERTY was already added for pszTarget: update it
            rc = _pPropertyStore->update (anyNodeId, LATEST_RESET_MESSAGE_PROPERTY, pszLatestResetMessageId);
        }
    }
    return rc;
}

