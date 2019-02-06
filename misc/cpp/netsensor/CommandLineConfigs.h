/*
* CommandLineConfigs.h
*
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
*
*/

#ifndef NETSENSOR_CommandLineConfigs__INCLUDED
#define NETSENSOR_CommandLineConfigs__INCLUDED

#include "StrClass.h"
#include "LList.h"
#include "Mode.h"
#include <iostream>   // std::cout
#include <string>     // std::string, std::stoi
namespace IHMC_NETSENSOR
{
class CommandLineConfigs
{
public:
    CommandLineConfigs (void);
    ~CommandLineConfigs (void);
    bool bHasDifferentDeliveryTime;
    bool bUseCompression;
    bool bUseExternalTopology;
    bool bCalculateTCPRTT;
    bool bHasInterfaces;
    bool bHasForcedInterfaces;
    bool bEnableIW;
    Mode em;
    NOMADSUtil::LList<NOMADSUtil::String> interfaceNameList;
    NOMADSUtil::String sConfigPath;
    NOMADSUtil::String sReplayConfigPath;
    NOMADSUtil::String sRecipient;
    NOMADSUtil::String sIWIface;
    uint32 ui32msStatsDeliveryTime;

    bool bHasForcedAddr;
    uint32 ui32ForcedInterfaceAddr;

    bool bHasForcedNetmask;
    uint32 ui32ForcedNetmask;

};


inline CommandLineConfigs::CommandLineConfigs (void) :
    bHasDifferentDeliveryTime (false),
    bUseCompression (false),
    bUseExternalTopology (false),
    bHasInterfaces (false),
    bCalculateTCPRTT (false),
    em (Mode::EM_NONE),
    sConfigPath (""),
    sReplayConfigPath (""),
    sRecipient (""),
    bHasForcedAddr (false),
    bHasForcedNetmask (false),
    bHasForcedInterfaces (false),
    ui32ForcedInterfaceAddr (0),
    ui32ForcedNetmask (0),
    bEnableIW (false),
    sIWIface ("")
{
}

inline CommandLineConfigs::~CommandLineConfigs(void)
{
}
}

#endif