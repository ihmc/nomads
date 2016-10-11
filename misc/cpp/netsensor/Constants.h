/*
 * Constants.h
 *
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
 */

#ifndef INCL_NET_SENSOR_CONST_H
#define INCL_NET_SENSOR_CONST_H

namespace IHMC_MISC 
{
    const static char* defaultCfgPath = "../../../../misc/conf/netSensor.cfg";
    const static int CLEANING_PER_CICLE = 100;
    static int GWTHRESHOLD = 5;
    const static int PACKETMAXSIZE = 65535;
	//const static int PACKETMAXSIZE = 1000;

//default configuration for standalone
	//GW DETECTION
	const static bool  ARP_GW_DETECTION_MECHANISM_ENABLED = true;
	
	//ENABLED FEATURES
	const static bool TOPOLOGY_DETECTION_ENABLED = true;
	const static bool TRAFFIC_DETECTION_ENABLED = true;
	const static bool TRAFFIC_BY_MAC_DETECTION_ENABLED = false;
	const static bool SEPARATED_MULTICAST_TRAFFIC_DETECTION_ENABLED = false;
	const static bool TRAFFIC_MULTICAST_DETECTION_DISABLED = false;
	const static bool RTT_DETECTION_ENABLED = false;
	const static bool DEBUG_MODE = true;
	const static int RECEIVING_QUEUE_MAX_SIZE = 500;

	//TOPOLOGY DETECTION
	const static bool LAX_TOPOLOGY_DETECTION_ENABLED = false;
	const static bool NETMASK_TOPOLOGY_DETECTION_ENABLED = true;
	const static bool ARP_TOPOLOGY_DETECTION_ = false;
	const static bool NETPROXY_TOPOLOGY_DETECTION = false;

	const static char* NOTIFIED_IP = "127.0.0.1";
	//const static char* NOTIFIED_IP = "128.49.235.250";

	const static int NOTIFIED_PORT = 7777;

	//default configuration for builtin
//Timings
	const static int CLEAN_TIME = 3500;
	const static int CLEANING_TIME_IO = 60000;
	const static int CLEANING_TIME_IE = 4000;
	const static int CLEANING_TIME_RT = 60000;
	const static int CLEANING_TIME_MAC = 60000;
	const static int CLEANING_TIME_MC = 60000;
	const static int STAT_UPDATE_TIME = 4000;

//OTHERS
	const static int DEBUG_LEVEL = 6;
}


#endif   // #ifndef INCL_NET_SENSOR_H