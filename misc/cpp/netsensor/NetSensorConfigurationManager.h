#ifndef NETSENSOR_NetSensorConfigurationManager__INCLUDED
#define NETSENSOR_NetSensorConfigurationManager__INCLUDED
/*
* NetSensorConfigurationManager.h
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
* Main configurator class for NetSensor
*/
#include "ConfigManager.h"
#include "FTypes.h"
#include "LList.h"

namespace IHMC_NETSENSOR
{
class NetSensorConfigurationManager
{
public:
    NetSensorConfigurationManager(void);
    ~NetSensorConfigurationManager(void);

    int init(void);
    int init(const char* cfgPath);

    // No monitoring interface, local host as delivery interface
    int initBase(const NOMADSUtil::String & statRecipientIP = "");
    int setNPModeDefaultConfigurations(const NOMADSUtil::String & statRecipientIP);
    int setReplayModeDefaultConfigurations(const NOMADSUtil::String & statRecipientIP,
                                           const NOMADSUtil::String & sTDM);
    void printConfigurationReport(void);

private:
    int populateConfigurations(void);
	int setDeliveryOptions(void);
	int setMonitoredInterfaces(void);
	int setTopologyOptions(void);
	int setVersion(void);
	int setRTTDetectionOptions(void);
	int setMulticastDetection(void);
    int setExternalTopologyNodeStorage(void);
//<--------------------------------------------------------------------------->

public:
	NOMADSUtil::LList<NOMADSUtil::String>* pslIpDeliveryList;
	NOMADSUtil::LList<NOMADSUtil::String>* pslMonitoredInterfacesList;

    uint32 ui32Ver;
    uint32 ui32DelMTU;
	uint32 ui32DelPort;
	uint32 ui32TDelPer;
	uint32 ui32ToDelPer;

	bool laxTopActive;
	bool arpTopActive;
	bool netmskTopActive;
	bool nproxyTopActive;
	bool icmpRTTActive;
	bool ignoreMulticast;
    bool storeExternalNodes;
    bool useProtobufCompression;
    bool calculateTCPRTT;

private:
	NOMADSUtil::ConfigManager _cfg;
};
}
#endif