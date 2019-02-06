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
#include "UUID.h"
#include "UUIDGenerator.h"
#include "NLFLib.h"

namespace IHMC_NETSENSOR
{
    class RTTContainer
    {
    public:
        RTTContainer();
        int64 getRTT(void);
        int64 getRcvTime(void);
        int64 getSentTime(void);
        int64 getLastChangeTime(void);
        void print(void);
        void setSentTime(int64 sentTime);
        void setReceivedTime(int64 receivedTime);
        void updateChangeTime(int64 changeTime);
        bool operator==(const RTTContainer & rhs);
        void setDataSent(bool value);
        bool hasDataBeenSent(void);

        //<---------------------------------------->
    private:
        int64 _i64TimeOfLastChange;
        int64 _i64SntTime;
        int64 _i64RcvTime;
        NOMADSUtil::UUID _uuid;
        bool _bDataSent;
    };

    inline bool RTTContainer::operator==(const RTTContainer & rhs)
    {
        return _uuid == rhs._uuid;
    }

}

#endif
