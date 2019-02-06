#ifndef NETSENSOR_NetSensorStatus__INCLUDED
#define NETSENSOR_NetSensorStatus__INCLUDED
/*
* NetsensorStatus.h
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
* Holds status variables of netsensor
*/
#include "FTypes.h"
#include "LList.h"
namespace IHMC_NETSENSOR
{
class NetsensorStatus
{
public:
    bool bUseProtobufCompression;
    bool bExternalNodesDetection;
    bool bCalculateTCPrtt;
    bool bAutomaticInterfaceDetection;
    uint32 ui32DeliveryPeriodMs;
    NOMADSUtil::LList<NOMADSUtil::String> interfacesList;
    NOMADSUtil::String sRecipient;
};
}




#endif