/*
* NetSensorConfigurationManager.cpp
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
*/
#include "Logger.h"
#include "LList.h"
#include "FileUtils.h"
#include "NetSensorConfigurationManager.h"
#include "NetSensorConstants.h"
#include "NetSensorUtilities.h"
#include "NetSensorDefaultConfigurations.h"

using namespace NOMADSUtil;
#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_NETSENSOR
{
NetSensorConfigurationManager::NetSensorConfigurationManager(void)
{
    pslMonitoredInterfacesList = new LList<NOMADSUtil::String>();
    pslIpDeliveryList = new LList<NOMADSUtil::String>();
    storeExternalNodes = DEFAULT_EXTERNAL_TOPOLOGY_IGNORE;
    useProtobufCompression = DEFAULT_OUTPUT_COMPRESSION_ACTIVE;
    calculateTCPRTT = DEFAULT_TCP_RTT_CALCULATION_ACTIVE;

    ui32DelPort     = DEFAULT_DELIVERY_PORT;
    ui32TDelPer     = DEFAULT_TRAFFIC_DELIVERY_PERIOD;
    ui32ToDelPer    = DEFAULT_TOPOLOGY_DELIVERY_PERIOD;
    laxTopActive    = DEFAULT_TOPOLOGY_LAX_ACTIVE;
    arpTopActive    = DEFAULT_TOPOLOGY_ARP_ACTIVE;
    netmskTopActive = DEFAULT_TOPOLOGY_NETMASK_ACTIVE;
    nproxyTopActive = DEFAULT_TOPOLOGY_NETPROXY_ACTIVE;
    icmpRTTActive   = DEFAULT_RTT_ICMP_ACTIVE;
    ignoreMulticast = DEFAULT_TRAFFIC_MULTICAST_IGNORE;

    ui32ForcedInterfaceAddr = DEFAULT_NOT_ACTIVE;
    ui32ForcedNetmask       = DEFAULT_NOT_ACTIVE;
    useForcedInterfaces     = false;
}

NetSensorConfigurationManager::~NetSensorConfigurationManager(void)
{
    delete pslIpDeliveryList;
    delete pslMonitoredInterfacesList;
}

int NetSensorConfigurationManager::init(void)
{
    pslMonitoredInterfacesList->add (DEFAULT_MONITORED_INTERFACE);
    return 0;
}

int NetSensorConfigurationManager::initBase (const NOMADSUtil::String & statRecipientIP)
{
    ui32Ver             = DEFAULT_VERSION;
    ui32DelMTU          = DEFAULT_MTU;

    if (statRecipientIP != "") {
        pslIpDeliveryList->add (statRecipientIP);
    }
    return 0;
}


int NetSensorConfigurationManager::setNPModeDefaultConfigurations (const NOMADSUtil::String & statRecipientIP)
{
    initBase(statRecipientIP);

    netmskTopActive     = false;
    nproxyTopActive     = true;

    return 0;
}


int NetSensorConfigurationManager::setReplayModeDefaultConfigurations (
    const NOMADSUtil::String & statRecipientIP,
    const NOMADSUtil::String & sTDM)
{
    initBase(statRecipientIP);

    if (sTDM ^= "netproxy") {
        netmskTopActive     = false;
        nproxyTopActive     = true;
    }
    else {
        netmskTopActive     = true;
        nproxyTopActive     = false;
    }

    return 0;
}

int NetSensorConfigurationManager::init (const char * cfgPath)
{
    if (FileUtils::fileExists (cfgPath)) {
        _cfg.init(1024);
        _cfg.readConfigFile(cfgPath, true);
        return populateConfigurations();
    }
    else {
        checkAndLogMsg(
			"NetSensorConfigurationManager::NetSensorConfigurationManager",
            Logger::L_SevereError,
			"It was not possible to find netsensor cfg file in: %s\n",
            cfgPath);
        return -1;
    }
}

int NetSensorConfigurationManager::populateConfigurations(void)
{
    int rc;
    rc = setVersion();
    if (rc != 0) {
        return -1;
    }
    setDeliveryOptions();
    if (rc != 0) {
        return -2;
    }
    setMonitoredInterfaces();
    if (rc != 0) {
        return -3;
    }
    setTopologyOptions();
    if (rc != 0) {
        return -4;
    }
    setRTTDetectionOptions();
    if (rc != 0) {
        return -5;
    }
    setMulticastDetection();
    if (rc != 0) {
        return -6;
    }

    setExternalTopologyNodeStorage();
    if (rc != 0) {
        return -7;
    }

    return 0;
}

void NetSensorConfigurationManager::printConfigurationReport(void)
{
    auto pszMethodName = "NetSensorConfigurationManager::printConfigurationReport()";
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tCfg file's version: %d\n", ui32Ver);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tDelivery Options:\n");

    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tDelivery MTU is: %d\n", ui32DelMTU);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tIPs that will receive data are:\n");
    printStringList (pslIpDeliveryList);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tDelivery Port is: %d\n", ui32DelPort);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tTraffic delivery period %dms\n", ui32TDelPer);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tTopology delivery period %dms\n", ui32ToDelPer);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tMonitoring Options:\n");

    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tMonitored interfaces are:\n");
    printStringList(pslMonitoredInterfacesList);

    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tARP topology mechanism set to: %d\n", arpTopActive);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tLax topology mechanism set to: %d\n", laxTopActive);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tNETMASK topology mechanism set to: %d\n", netmskTopActive);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tNETPROXY topology mechanism set to: %d\n", nproxyTopActive);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tICMP RTT detections set to: %d\n", icmpRTTActive);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tIGNORE Multicast set to: %d\n", ignoreMulticast);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tStore external nodes for topology set to: %d\n", storeExternalNodes);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tUse compression for protobuf stream: %d\n", useProtobufCompression);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "\tCalculate TCP RTT: %d\n", calculateTCPRTT);
    if (ui32ForcedInterfaceAddr != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "\tForced Sensor Address: %d\n", 
            InetAddr (ui32ForcedInterfaceAddr).getIPAsString());
    }
    if (ui32ForcedNetmask != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "\tForced Netmask: %d\n\n\n", 
            InetAddr (ui32ForcedNetmask).getIPAsString());
    }
}

int NetSensorConfigurationManager::setVersion (void)
{
	if (_cfg.hasValue(C_VERSION_CONF_KEY))
	{
        ui32Ver = _cfg.getValueAsInt(C_VERSION_CONF_KEY);
		return 0;
	}
	else {
		checkAndLogMsg("NetSensorConfigurationManager::setVersion",
			Logger::L_SevereError, "Key %s not found aborting\n");
		return -1;
	}
}

int NetSensorConfigurationManager::setDeliveryOptions (void)
{
	if (_cfg.hasValue(C_DELIVERY_IPS_CONF_KEY)) {
		tokenizeStr(_cfg.getValue(C_DELIVERY_IPS_CONF_KEY),' ', pslIpDeliveryList);
	}
	else {
		pslIpDeliveryList->add(DEFAULT_DELIVERY_IP);
		checkAndLogMsg("NetSensorConfigurationManager::setDeliveryOptions()"
			, Logger::L_Warning, "; Missing %s\n", C_DELIVERY_IPS_CONF_KEY);
	}

	if (_cfg.hasValue(C_DELIVERY_PORT_KEY)) {
		ui32DelPort = _cfg.getValueAsInt(C_DELIVERY_PORT_KEY);
	}
	else {
        ui32DelPort = DEFAULT_DELIVERY_PORT;
        checkAndLogMsg("NetSensorConfigurationManager::setDeliveryOptions()",
            Logger::L_Warning, "; Missing %s\n", C_DELIVERY_PORT_KEY);
	}

	if (_cfg.hasValue(C_DELIVERY_MTU_KEY)) {
		ui32DelMTU = _cfg.getValueAsInt(C_DELIVERY_MTU_KEY);
		if (ui32DelMTU <= 0) {
			checkAndLogMsg(
				"NetSensorConfigurationManager::setDeliveryOptions()",
                Logger::L_Warning, "; MTU need to be positive! %d\n",
				ui32DelMTU);
            return -1;
		}
	}
	else {
		ui32DelMTU = C_PACKET_MAX_SIZE;
        checkAndLogMsg ("NetSensorConfigurationManager::setDeliveryOptions()",
            Logger::L_Warning, "; Missing %s\n", C_DELIVERY_MTU_KEY);
	}

	if (_cfg.hasValue (C_DELIVERY_TRAFFIC_PERIOD_KEY)) {
		ui32TDelPer = _cfg.getValueAsUInt32(
			C_DELIVERY_TRAFFIC_PERIOD_KEY);
	}
	else {
        ui32TDelPer = 4000;
        checkAndLogMsg ("NetSensorConfigurationManager::setDeliveryOptions()",
            Logger::L_Warning, "; Missing %s\n",
			C_DELIVERY_TRAFFIC_PERIOD_KEY);
	}

	if (_cfg.hasValue (C_DELIVERY_TOPOLOGY_PERIOD_KEY)) {
		ui32ToDelPer = _cfg.getValueAsUInt32(
			C_DELIVERY_TOPOLOGY_PERIOD_KEY);
	}
	else {
        ui32ToDelPer = 4000;
        checkAndLogMsg("NetSensorConfigurationManager::setDeliveryOptions()",
            Logger::L_Warning, "; Missing %s\n",
			C_DELIVERY_TOPOLOGY_PERIOD_KEY);
	}
	return 0;
}

int NetSensorConfigurationManager::setMonitoredInterfaces (void)
{
	if (_cfg.hasValue(C_MONITORED_INTERFACES_CONF_KEY)) {
		tokenizeStr(_cfg.getValue(C_MONITORED_INTERFACES_CONF_KEY), ' ',
            pslMonitoredInterfacesList);
		return 0;
	}
	else {
		checkAndLogMsg(
			"NetSensorConfigurationManager::setMonitoredInterfaces()",
            Logger::L_SevereError, "; No monitored interfaces specified\n");
        return -1;
	}
}

int NetSensorConfigurationManager::setTopologyOptions (void)
{
	//LAX TOPOLOGY
	if (_cfg.hasValue(C_LAX_CONF_KEY)) {
		laxTopActive = _cfg.getValueAsBool(C_LAX_CONF_KEY);
	}
	else {
		laxTopActive = false;
		checkAndLogMsg("NetSensor::setTopologyOptions()",
			Logger::L_Warning, "; %s not present\n", C_LAX_CONF_KEY);
	}

	//ARP TOPOLOGY
	if (_cfg.hasValue(C_ARP_CONF_KEY)) {
		arpTopActive = _cfg.getValueAsBool(C_ARP_CONF_KEY);
	}
	else {
		arpTopActive = false;
        checkAndLogMsg("NetSensor::setTopologyOptions()",
            Logger::L_Warning, "; %s not present\n", C_ARP_CONF_KEY);
	}

	//NETMASK TOPOLOGY
	if (_cfg.hasValue(C_NETMASK_CONF_KEY)) {
		netmskTopActive =
			_cfg.getValueAsBool(C_NETMASK_CONF_KEY);
	}
	else {
		netmskTopActive = false;
        checkAndLogMsg("NetSensor::setTopologyOptions()",
            Logger::L_Warning, "; %s not present\n", C_NETMASK_CONF_KEY);
	}

	//NETPROXY TOPOLOGY
	if (_cfg.hasValue(C_NETPROXY_CONF_KEY)) {
		nproxyTopActive =
			_cfg.getValueAsBool(C_NETPROXY_CONF_KEY);
	}
	else {
		nproxyTopActive = true;
        checkAndLogMsg("NetSensor::setTopologyOptions()",
            Logger::L_Warning, "; %s not present\n", C_NETPROXY_CONF_KEY);
	}
	return 0;
}

int NetSensorConfigurationManager::setRTTDetectionOptions (void)
{
	//ICMP RTT
	if (_cfg.hasValue(C_ICMP_CONF_KEY)) {
		icmpRTTActive = _cfg.getValueAsBool(C_ICMP_CONF_KEY);
	}
	else {
        icmpRTTActive = false;
		checkAndLogMsg(
			"NetSensorConfigurationManager::setRTTDetectionOptions()",
			Logger::L_Warning, "; %s not present\n", C_ICMP_CONF_KEY);
	}
	return 0;
}

int NetSensorConfigurationManager::setMulticastDetection (void)
{
	if (_cfg.hasValue(C_IGNORE_MULTICAST_CONF_KEY)) {
		ignoreMulticast = !_cfg.getValueAsBool(C_IGNORE_MULTICAST_CONF_KEY);
	}
	else {
		ignoreMulticast = false;
		checkAndLogMsg("NetSensorConfigurationManager::setMulticastDetection()",
            Logger::L_Warning,
			"; %s IGNORE_MULTICAST_CONF_KEY entry not present\n",
			C_IGNORE_MULTICAST_CONF_KEY);
	}
	return 0;
}

int NetSensorConfigurationManager::setExternalTopologyNodeStorage (void)
{
    if (_cfg.hasValue(C_EXTERNAL_TOP_CONF_KEY))
    {
        storeExternalNodes = _cfg.getValueAsBool(C_EXTERNAL_TOP_CONF_KEY);
    }
    else
    {
        storeExternalNodes = false;
        checkAndLogMsg("NetSensorConfigurationManager::setExternalTopologyNodeStorage()",
            Logger::L_Warning,
            "; %s C_EXTERNAL_TOP_CONF_KEY entry not present in _cfg\n",
            C_IGNORE_MULTICAST_CONF_KEY);
    }
    return 0;
}
}
