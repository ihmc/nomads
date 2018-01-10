#pragma once
/*
* MicroflowId.h
*
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2016 IHMC.
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
#include"StrClass.h"
#include"TrafficElement.h"
namespace IHMC_NETSENSOR
{
    class MicroflowId
    {
    public:
        MicroflowId(void);
        NOMADSUtil::String sSA;
        NOMADSUtil::String sDA;
        NOMADSUtil::String sSP;
        NOMADSUtil::String sDP;
        NOMADSUtil::String sProtocol;
        TrafficElement::Classification classification;
        uint32 ui32Size;
        int64 i64CurrTime;
    };
   inline MicroflowId::MicroflowId(void) {}
}