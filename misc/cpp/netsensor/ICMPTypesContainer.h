#ifndef NETSENSOR_ICMPTypesContiner__INCLUDED
#define NETSENSOR_ICMPTypesContiner__INCLUDED
/*
* ICMPTypesContainer.cpp
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
* This class stores an instance of an ICMP packet and additional information
* that can be used by NetSensor recipients.
*/

#include"StringHashtable.h"
#include"StrClass.h"
#include"PacketStructures.h"
#include"TimeIntervalAverage.h"
#include "NLFLib.h"
#include"NetSensorUtilities.h"

namespace IHMC_NETSENSOR
{
    class ICMPTypesContainer
    {
    public:
        ICMPTypesContainer(IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket *pICMPPacket, const uint32 ui32msResolution);
        ~ICMPTypesContainer(void);

        void   incrementCount(void);
        void   updateExtraAddresses(const char *ipAddress);
        bool   isExpired(void);
        bool   hasExtraAddresses(void); // Returns whether or not there should b
        uint8  getUi8Type(void);
        uint8  getUi8Code(void);
        uint32  getCount(void);
   //<----------------------------------------------------->
        IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket *pContainerIcmpPacket;
        TimeIntervalAverage<uint32> tiaICMP;
        NOMADSUtil::StringHashtable<uint32> extraIPAddresses; // This structure contains address masks/redirect addresses as a key, and a count as the value

    private:
        uint32 _ui32Count; // # of repeated ICMP packets since last cleaning
        uint32 _maxUINT32 = (0xffffffff);
    };
}
#endif
