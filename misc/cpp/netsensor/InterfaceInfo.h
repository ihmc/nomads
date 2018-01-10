#ifndef NETSENSOR_InterfaceInfo__INCLUDED
#define NETSENSOR_InterfaceInfo__INCLUDED
/*
* InterfaceInfo.h
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
* This class stores information about the network interface.
*
*/
#include"StrClass.h"
#include"NetworkHeaders.h"

#include "TopologyDetectionMechanism.h"

namespace IHMC_NETSENSOR
{
class InterfaceInfo
{
public:

    InterfaceInfo();
//<---------------------------------------------------------------------------->
public:
    bool bIIface;
    NOMADSUtil::String sInterfaceName;
    uint32 ui32IpAddr;
    uint32 ui32Netmask;
    NOMADSUtil::EtherMACAddr emacInterfaceMAC;
    TopologyDetectionMechanism tdm;
};

inline InterfaceInfo::InterfaceInfo()
{
    bIIface             = false;
    sInterfaceName      = "";
    ui32IpAddr          = 0;
    ui32Netmask         = 0;
    emacInterfaceMAC    = {}; //find a way to initialize this c89 style
    tdm                 = TopologyDetectionMechanism::TDM_NETMASK;
}
}
#endif
