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

#include "TCPRTTContainer.h"
using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
        
    TCPRTTContainer::TCPRTTContainer(uint32 ui32msResolution) :
        tiaTCPRTT(ui32msResolution),
        _i64SntTime(0),
        _i64RcvTime(0)
    {

    }

    TCPRTTContainer::~TCPRTTContainer(void)
    {
        _rttLList.removeAll();
    }


    void  TCPRTTContainer::calculateMostRecentRTT(void)
    {
        addRTT(_i64RcvTime - _i64SntTime);
        clearTimes();
    }

    float TCPRTTContainer::getAvgRTTinMS(void)
    {
        uint32 ui32totalRTT = 0;
        uint16 ui16CurrRTT = 0;

        _rttLList.resetGet();
        while (_rttLList.getNext(ui16CurrRTT)) {
            ui32totalRTT += ui16CurrRTT;
        }
        
        uint16 ui16RTTCount = _rttLList.getCount();

        if (ui16RTTCount == 0) {
            if (hasSentTime()) {
                float val = getTimeInMilliseconds() - _i64SntTime;
                return val;
            }   
            return 0;
        }

        return (float)ui32totalRTT / (float)ui16RTTCount;
    }

    float TCPRTTContainer::getAvgRTTinSec(void)
    {
        return getAvgRTTinMS() / 1000;
    }

    uint16 TCPRTTContainer::getMaxRTT(void)
    {
        uint16 ui16MaxRTT = 0;
        uint16 ui16CurrRTT;

        _rttLList.resetGet();
        while (_rttLList.getNext(ui16CurrRTT)) {
            if (ui16CurrRTT > ui16MaxRTT)
                ui16MaxRTT = ui16CurrRTT;
        }

        return ui16MaxRTT;
    }

    uint16 TCPRTTContainer::getMinRtt(void)
    {
        uint16 ui16MinRTT = 0xffff;
        uint16 ui16CurrRTT;
        
        _rttLList.resetGet();
        while (_rttLList.getNext(ui16CurrRTT)) {
            if (ui16CurrRTT < ui16MinRTT)
                ui16MinRTT = ui16CurrRTT;
        }

        return ui16MinRTT;
    }

    bool TCPRTTContainer::hasReceivedTime(void)
    {
        return _i64RcvTime != 0;
    }

    bool TCPRTTContainer::hasSentTime(void)
    {
        return _i64SntTime != 0;
    }

    void TCPRTTContainer::setSentTime(int64 i64Time)
    {
        _i64SntTime = i64Time;
    }

    void TCPRTTContainer::setReceiveTime(int64 i64Time)
    {
        _i64RcvTime = i64Time;
    }
         
    void TCPRTTContainer::clearTimes(void)
    {
        _i64RcvTime = 0;
        _i64SntTime = 0;
    }

    void TCPRTTContainer::addRTT(uint16 rttVal)
    {
        _rttLList.add(rttVal);
    }

}