#ifndef NETSENSOR_TopologyEntry__INCLUDED
#define NETSENSOR_TopologyEntry__INCLUDED
/*
* TopologyEntry.h
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
* This class contains a list of IPs that are detected in a subnet
*/
#include"StrClass.h"
#include"FTypes.h"
namespace IHMC_NETSENSOR
{
class TopologyEntry
{
//<--------------------------------------------------------------------------->
public:
    NOMADSUtil::LList<NOMADSUtil::String> ipsList;
    int64 i64TimeOfLastChange;
};
}
#endif