#ifndef NETSENSOR_ICMPRTTList__INCLUDED
#define NETSENSOR_ICMPRTTList__INCLUDED
/*
* ICMPRTTList.h
* Author: bordway@ihmc.us
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

#include"PacketStructures.h"
#include "RTTContainer.h"
#include "UInt32Hashtable.h"
#include "ICMPTypesContainer.h"
#include "NLFLib.h"
#include "measure.pb.h"
#include "ProtobufWrapper.h"

namespace IHMC_NETSENSOR
{
    class ICMPRTTList
    {
    public:
        ICMPRTTList (uint32 uiResolution);
        ~ICMPRTTList(void);

        uint32 clean (const uint32 ui32CleaningNumber);
        uint32 getCount(void);
        uint32 getMinRTT(void);
        uint32 getMostRecentRTT(void);
        float getAverageRTT(void);
        uint32 getMaxRTT(void); 
        void print(void);
        void addSentTime (uint32 seqNum, int64 time);
        void addReceivedTime (uint32 seqNum, int64 time);

    private:
        NOMADSUtil::UInt32Hashtable<RTTContainer> _rttTable;
        TimeIntervalAverage<uint32> _avgRttTIA;
        NOMADSUtil::Mutex _mutex;
    };
}

#endif