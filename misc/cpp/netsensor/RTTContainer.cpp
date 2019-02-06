/*
* TCPRttContainer.cpp
* Author: bordway@ihmc.us rfronteddu@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
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
*/

#include "RTTContainer.h"
using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
    RTTContainer::RTTContainer() :
        _i64SntTime(0),
        _i64RcvTime(0),
        _i64TimeOfLastChange(0),
        _bDataSent(false)
    {
        _uuid.generate();
    }

    int64 RTTContainer::getLastChangeTime(void)
    {
        return _i64TimeOfLastChange;
    }

    void RTTContainer::print()
    {
        printf("Sent: %lld, Received: %lld, RTT: %d", _i64SntTime, _i64RcvTime, (_i64RcvTime - _i64SntTime));
    }

    int64 RTTContainer::getRcvTime(void)
    {
        return _i64RcvTime;
    }

    int64 RTTContainer::getSentTime(void)
    {
        return _i64SntTime;
    }


    int64 RTTContainer::getRTT (void)
    {
        if (_i64RcvTime == 0) {
            return getTimeInMilliseconds() - _i64SntTime;
        }

        return _i64RcvTime - _i64SntTime;
    }

    void RTTContainer::setSentTime (int64 sentTime)
    {
        _i64SntTime = sentTime;
    }

    void RTTContainer::setReceivedTime (int64 receivedTime)
    {
        _i64RcvTime = receivedTime;
    }

    void RTTContainer::updateChangeTime(int64 changeTime)
    {
        _i64TimeOfLastChange = changeTime;
    }

    void RTTContainer::setDataSent(bool value)
    {
        _bDataSent = value;
    }

    bool RTTContainer::hasDataBeenSent()
    {
        return _bDataSent;
    }
}