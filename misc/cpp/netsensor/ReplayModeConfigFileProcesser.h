#ifndef NETSENSOR_ReplayModeConfigFileProcesser__INCLUDED
#define NETSENSOR_ReplayModeConfigFileProcesser__INCLUDED
/*
* ReplayModeConfigFileProcessor.h
* Author: amorelli@ihmc.us
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
* NetSensor class for parsing REPLAY MODE configuration files in JSON format
*/

#include "FTypes.h"
#include "StrClass.h"
#include "Json.h"

#include "NetSensor.h"

namespace IHMC_NETSENSOR
{
    class ReplayModeConfigFileProcessor
    {
    public:
        ReplayModeConfigFileProcessor(NetSensor *pNetSensor,
                                      const NOMADSUtil::String & sConfigFile);
        ~ReplayModeConfigFileProcessor(void);

        NOMADSUtil::String getTDM(void);
        NOMADSUtil::String getStatsRecipientIP(void);
        NOMADSUtil::LList<NOMADSUtil::String> * getInterfaceNames(void);
        NOMADSUtil::LList<NetSensor::InterfaceInfoOpt> * getInterfaceInfoList(void);
        NOMADSUtil::LList<uint32> * getRemoteNpList(void);

    private:
        int processConfigFile(void);
        int consolidatePcapFile(NOMADSUtil::String & sPcapFilePath) const;

        bool _bConfigFileProcessed;
        bool _bError;

        NOMADSUtil::String _sConfigFile;

        NOMADSUtil::String _sTDM;
        NOMADSUtil::String _sStatsRecipientIP;
        NOMADSUtil::LList<NOMADSUtil::String> _lInterfaceNames;
        NOMADSUtil::LList<NetSensor::InterfaceInfoOpt> _llIIO;
        NOMADSUtil::LList<uint32> _llRemoteNP;
    };


    inline ReplayModeConfigFileProcessor::ReplayModeConfigFileProcessor(
        NetSensor *pNetSensor, const NOMADSUtil::String & sConfigFile) :
        _bConfigFileProcessed(false), _bError(false), _sConfigFile(sConfigFile),
        _sTDM(""), _sStatsRecipientIP(""), _lInterfaceNames(),
        _llIIO(), _llRemoteNP() { }

    inline ReplayModeConfigFileProcessor::~ReplayModeConfigFileProcessor(void) { }

}

#endif