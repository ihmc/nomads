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
#include "EmbeddingMode.h"

namespace IHMC_NETSENSOR
{
    class CommandLineConfigs
    {
    public:
        CommandLineConfigs(void);
        ~CommandLineConfigs(void);

        bool bUseCompression;
        bool bUseExternalTopology;
        bool bCalculateTCPRTT;
        bool bHasInterfaces;
        EmbeddingMode em;
        NOMADSUtil::LList<NOMADSUtil::String> interfaceNameList;
        NOMADSUtil::String sConfigPath;
        NOMADSUtil::String sReplayConfigPath;
        NOMADSUtil::String sRecipient;
    };


    inline CommandLineConfigs::CommandLineConfigs(void):
        bUseCompression(false),
        bUseExternalTopology(false),
        bHasInterfaces(false),
        bCalculateTCPRTT(false),
        em(EmbeddingMode::EM_NONE),
        sConfigPath(""),
        sReplayConfigPath(""),
        sRecipient("")
    {
    }

    inline CommandLineConfigs::~CommandLineConfigs(void)
    {
    }
}

#endif