/*
 * History.cpp
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

#include "History.h"

#include "Message.h"
#include "MessageInfo.h"
#include "Subscription.h"

#include "NLFLib.h"
#include "SequentialArithmetic.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//==============================================================================
//  History
//==============================================================================
History::History (int64 i64TimeOut)
{
    _i64TimeOut = i64TimeOut;
    _i64HistoryReqTime = getTimeInMilliseconds();
}

History::~History()
{
}

bool History::isExpired()
{
    return (getTimeInMilliseconds() > (_i64HistoryReqTime + _i64TimeOut));
}

//==============================================================================
//  ShiftHistory
//==============================================================================
ShiftHistory::ShiftHistory (uint16 ui16HistoryLength, int64 i64TimeOut)
    : History (i64TimeOut)
{
    _ui16HistoryLength = ui16HistoryLength;
}

ShiftHistory::~ShiftHistory()
{
}

bool ShiftHistory::isInHistory (Message *pMsg, uint32 ui32LatestMsgRcvdPerSender)
{
    if (ui32LatestMsgRcvdPerSender == 0) {
        return true;
    }
    uint32 ui32 = pMsg->getMessageInfo()->getMsgSeqId();
    uint32 ui32From = ui32LatestMsgRcvdPerSender - _ui16HistoryLength;
    if (SequentialArithmetic::greaterThanOrEqual (ui32, ui32From) &&
        SequentialArithmetic::lessThan (ui32, ui32LatestMsgRcvdPerSender)) {
        return true;
    }
    return false;
}

//==============================================================================
//  DiscreteHistory
//==============================================================================
DiscreteHistory::DiscreteHistory (uint32 ui32From, uint32 ui32To, int64 i64TimeOut)
    : History (i64TimeOut)
{
    _ui32From = ui32From;
    _ui32To = ui32To;
}

DiscreteHistory::~DiscreteHistory()
{
}

bool DiscreteHistory::isInHistory (Message *pMsg, uint32)
{
    MessageInfo *pMI = pMsg->getMessageInfo();
    uint32 ui32 = pMI->getMsgSeqId();
    if (SequentialArithmetic::greaterThanOrEqual (ui32, _ui32From) &&
        SequentialArithmetic::lessThanOrEqual(ui32, _ui32To)) {
        return true;
    }
    return false;
}

//==============================================================================
//  TimeHistory
//==============================================================================
TimeHistory::TimeHistory (int64 i64Since, int64 i64Until, int64 i64TimeOut)
    : History (i64TimeOut)
{
    _i64Since = i64Since;
    _i64Until = i64Until;
}

TimeHistory::~TimeHistory()
{
}

bool TimeHistory::isInHistory (Message *pMsg, uint32)
{
    // TODO: implement this
    return false;
}
