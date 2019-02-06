#ifndef NETSENSOR_DEFAULT_VALUES__INCLUDED
#define NETSENSOR_DEFAULT_VALUES__INCLUDED
/*
* DefaultValues.h
* Author: bordway@ihmc.us rfronteddu@ihmc.us
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

namespace IHMC_NETSENSOR
{
    // Measure subjects
    const static char *SUBJECT_RTT   = "rtt";

    //RTT Measure Keys
    const static char *C_RTT_SENSOR_IP  = "sensor_ip";
    const static char *C_RTT_SRC_IP     = "src_ip";
    const static char *C_RTT_DEST_IP    = "dest_ip";
    const static char *C_RTT_PROTOCOL   = "protocol";
    const static char *C_RTT_SRC_PORT   = "src_port";
    const static char *C_RTT_DEST_PORT  = "dest_port";
    const static char *C_RTT_MIN_RTT    = "min_rtt";
    const static char *C_RTT_MAX_RTT    = "max_rtt";
    const static char *C_RTT_RECENT_RTT = "most_recent_rtt";
    const static char *C_RTT_RESOLUTION = "resolution";
    const static char *C_RTT_AVG_VALUE  = "avg_rtt";

    //IW Measure Keys
    //str
    
    const static char *SENSOR_IP = "sensor_ip";
    const static char *MAC_ADDR = "mac_address";
    const static char *DEV_NAME = "dev_name";
    const static char *TX_BITRATE = "tx_bitrate";
    const static char *P_LINK_STATE = "p_link_state";
    const static char *PEER_POWER_MODE = "peer_power_mode";
    const static char *LOCAL_POWER_MODE = "local_power_mode";
    const static char *PREAMBLE = "preamble";
    //int
    const static char *INACTIVE_TIME = "inactive_time";
    const static char *RX_BYTES = "rx_bytes";
    const static char *RX_PACKETS = "rx_packets";
    const static char *TX_BYTES = "tx_bytes";
    const static char *TX_PACKETS = "tx_packets";
    const static char *TX_RETRIES = "tx_retries";
    const static char *TX_FAILED = "tx_failed";
    const static char *SIGNAL_PWR = "signal_power";
    const static char *SIGNAL_PWR_AVG = "signal_power_avg";
    const static char *T_OFFSET = "t_offset";
    const static char *MESH_LLID = "mesh_llid";
    const static char *MESH_PLID = "mesh_plid";
    const static char *AUTHENTICATED = "authenticated";
    const static char *AUTHORIZED = "authorized";
    const static char *WMM_WME = "wmm_wme";
    const static char *MFP = "mfp";
    const static char *TDLS = "tdls";
    
}
#endif