#ifndef NETSENSOR_TCPRTTContainer__INCLUDED
#define NETSENSOR_TCPRTTContainer__INCLUDED
/*
* TCPRttContainer.h
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

#include "TimeIntervalAverage.h"
#include "LList.h"

namespace IHMC_NETSENSOR
{

    class TCPRTTContainer
    {
    public:
        TCPRTTContainer(uint32 ui32msResolution);
        ~TCPRTTContainer(void);

        void calculateMostRecentRTT(void);
        float getAvgRTTinMS(void);
        float getAvgRTTinSec(void);
        uint16 getMaxRTT(void);
        uint16 getMinRtt(void);
        bool hasReceivedTime(void);
        bool hasSentTime(void);
        void setSentTime(int64 i64Time);
        void setReceiveTime(int64 i64Time);

    private:
        void clearTimes(void);
        void addRTT(uint16 rttVal);

        //<---------------------------------------->
    public:
        TimeIntervalAverage<uint32> tiaTCPRTT;
    private:
        int64 _i64SntTime;
        int64 _i64RcvTime;

        NOMADSUtil::LList<uint16> _rttLList;
    };

}

#endif
