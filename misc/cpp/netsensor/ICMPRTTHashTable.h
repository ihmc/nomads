#ifndef NETSENSOR_ICMPRTTHashTable__INCLUDED
#define NETSENSOR_ICMPRTTHashTable__INCLUDED
/*
* ICMPRTTHashTable.h
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

#include"StringHashtable.h"
#include"PacketStructures.h"
#include "RTTContainer.h"
#include "PtrLList.h"
#include "ICMPTypesContainer.h"
#include "ICMPRTTList.h"
#include "measure.pb.h"

namespace IHMC_NETSENSOR
{
    class ICMPRTTHashTable
    {
    public:
        ICMPRTTHashTable (uint32 ui32Resolution);
        ~ICMPRTTHashTable (void);

        uint32 cleanTable (const uint32 ui32CleaningNumber);
        void print (void);
        void put (
            IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket * pICMPTypeContainer, 
            bool isSent, 
            int64 timestamp);
        NOMADSUtil::PtrLList<measure::Measure> * createMeasures (NOMADSUtil::String sSensorIP);
    private:
        bool isValidTypeForRTT (uint8 type);
        uint32 _ui32Resolution;
        NOMADSUtil::StringHashtable<NOMADSUtil::StringHashtable<ICMPRTTList>> _rttHashTable;
        NOMADSUtil::Mutex _mutex;
    };
}

#endif