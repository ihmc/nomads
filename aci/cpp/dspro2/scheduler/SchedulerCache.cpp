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

#include "PropertyStoreInterface.h"

#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const char * SchedulerCache::LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY = "latestMessagePushedToTarget";
const char * SchedulerCache::LATEST_MESSAGE_TO_TARGET_SOURCE_TIMESTAMP_PROPERTY = "latestMessagePushedToTargetTStamp";

SchedulerCache::SchedulerCache (PropertyStoreInterface *pPropertyStore)
    : _pPropertyStore (pPropertyStore)
{
}

SchedulerCache::~SchedulerCache()
{
}

String SchedulerCache::getLatestMessageIdPushedToTarget (const char *pszTarget)
{
    if (pszTarget == NULL) {
        String tmp; // empty string
        return tmp;
    }

    _m.lock();
    String tmp (_pPropertyStore->get (pszTarget, LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY));
    _m.unlock();
    return tmp;
}

int SchedulerCache::setLatestMessageIdPushedToTarget (const char *pszTarget, const char *pszLatestMessageId)
{
    if (pszTarget == NULL || pszLatestMessageId == NULL) {
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

    _m.unlock();
    return rc;
}

int SchedulerCache::resetLatestMessageIdPushedToTarget (const char *pszTarget)
{
    if (pszTarget == NULL) {
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
    if (pszTarget == NULL || pszObjectId == NULL || i64Timestamp <= 0) {
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
    if (pszTarget == NULL) {
        return String();    // empty string
    }

    return _pPropertyStore->get (pszTarget, LATEST_MESSAGE_ID_PUSHED_TO_TARGET_PROPERTY);
}

int64 SchedulerCache::getMostRecentMessageTimestampInternal (const char *pszTarget, const char *pszObjectId)
{
    if (pszTarget == NULL || pszObjectId == NULL) {
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

