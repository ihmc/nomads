#ifndef NETSENSOR_I_OPT__INCLUDED
#define NETSENSOR_I_OPT__INCLUDED
/*
* InterfaceOptions.h
* Author: rfronteddu@ihmc.us
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
* Main NetSensor library Class
*/

#include "StrClass.h"
#include "Mode.h"
#include "NetSensorPacketQueue.h"

namespace IHMC_NETSENSOR
{
class InterfaceOptions
{
public:
    InterfaceOptions (void);
    ~InterfaceOptions (void) {};
    // <------------------------------------------------------------------------------------------>
public:
    NOMADSUtil::String    sMonitoredInterface;
    NetSensorPacketQueue * pPQ;
    NetSensorPacketQueue * pRttPQ;
    bool                  bIsInternal;
    Mode                  mode;
    uint32                ui32ForcedInterfaceAddr;
    uint32                ui32ForcedNetmask;
};
}

#endif
