#ifndef NETSENSOR_INTERFACE_INFO__INCLUDED
#define NETSENSOR_INTERFACE_INFO__INCLUDED
/*
* NetworkInfo.h
* Author: rfronteddu@ihmc.us
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
*
* This Class stores information about the local netproxy (if present)
*/
#include"StrClass.h"
#include"NetworkHeaders.h"
namespace IHMC_NETSENSOR
{
class InterfaceInfo
{   
    //<--------------------------------------------------------------------------->
    NOMADSUtil::String networkName;        //Interface Subnet address 
    NOMADSUtil::String networkNetmask;     //Interface Subnet netmask
    uint32 interfaceIp;                    //Interface IP address
    NOMADSUtil::EtherMACAddr interfaceMac; //Interface MAC address
}
}

#endif