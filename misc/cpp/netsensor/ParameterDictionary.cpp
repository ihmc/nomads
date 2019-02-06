/*
* ParameterDictionary.cpp
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
*
* Holds types of parameter arguments that can be passed to NetSensor via
* command line
*/
#include "ParameterDictionary.h"

using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
    // Init the hashtable with constants and values of the enum
    ParameterDictionary::ParameterDictionary(void) :
        _stringToEnum (true, true, true, true)
    {
        _stringToEnum.put (C_USE_COMPRESSION,           new uint8 {C_INT_USE_COMPRESSION});
        _stringToEnum.put (C_STORE_EXTERNALS_TOPOLOGY,  new uint8 {C_INT_STORE_EXTERNALS_TOPOLOGY});
        _stringToEnum.put (C_SPECIFY_INTERFACE,         new uint8 {C_INT_SPECIFY_INTERFACE});
        _stringToEnum.put (C_SPECIFIY_CONFIG_PATH,      new uint8 {C_INT_SPECIFY_CONFIG_PATH});
        _stringToEnum.put (C_SPECIFY_NS_RECIPIENT,      new uint8 {C_INT_SPECIFY_NS_RECIPIENT});
        _stringToEnum.put (C_CALCULATE_TCP_RTT,         new uint8 {C_INT_CALCULATE_TCP_RTT});
        _stringToEnum.put (C_SHOW_HELP,                 new uint8 {C_INT_SHOW_HELP});
        _stringToEnum.put (C_REPLAY_MODE,               new uint8 {C_INT_REPLAY_MODE});
        _stringToEnum.put (C_FORCED_ADRR,               new uint8 {C_INT_FORCED_ADRR});
        _stringToEnum.put (C_FORCED_NMSK,               new uint8 {C_INT_FORCED_NMSK});
        _stringToEnum.put (C_FORCED_INTERFACES,         new uint8 {C_INT_FORCED_INTERFACES});
        _stringToEnum.put (C_ENABLE_IW,                 new uint8 {C_INT_ENABLE_IW});
    }

    // Delete entries in the hashtable
    ParameterDictionary::~ParameterDictionary()
    {
        _stringToEnum.removeAll();
    }

    // Return the enum value of the parameter
    uint8 ParameterDictionary::getIntParam (String sParam)
    {
        const uint8* param = _stringToEnum.get (sParam);
        if (param == nullptr)
            return C_INT_PARAM_UNKNOWN;
        else
            return *param;
    }

    // Print how each parameter configures NetSensor
    void ParameterDictionary::printMeanings (void)
    {
        printf ("Flags:\n");
        printf ("\t%s\t\t\t- Show possible flags\n", C_SHOW_HELP);
        printf ("\t%s\t\t\t- Use compression for protobuf messages\n", C_USE_COMPRESSION);
        printf ("\t%s\t\t\t- Store external topology\n", C_STORE_EXTERNALS_TOPOLOGY);
        printf ("\t%s\t\t\t- Calculate TCP RTT\n", C_CALCULATE_TCP_RTT);
        printf ("\t%s [interfaceName1 interfaceName2 ...]\t- Specify interfaces to listen to\n", C_SPECIFY_INTERFACE);
        printf ("\t%s [configPath]\t- Specify a path for the config file\n", C_SPECIFIY_CONFIG_PATH);
        printf ("\t%s [ipAddress]\t\t- Specify a recipient for NetSensor\n", C_SPECIFY_NS_RECIPIENT);
        printf ("\t%s [configPath]\t- Set REPLAY MODE and specify a path for the REPLAY MODE config file\n", C_REPLAY_MODE);
        printf ("\t%s [msNumber]\t- Set period of delivery time in ms\n", C_DELIVERY_TIME);
        printf ("\t%s [ipAddress]\t- Force sensor Addr\n", C_FORCED_ADRR);
        printf ("\t%s [ipNetmask]\t- Force sensor netmask\n", C_FORCED_NMSK);
        printf ("\t%s [IName1:IAddr1:INetMsk1 IName2:IAddr2:INetMsk2 ...]\t- Force interface addr and netmasks\n", C_FORCED_INTERFACES);
        printf ("\t%s [IName]\t- Enabling netlink lib information gathering\n", C_ENABLE_IW);
        printf ("\n");
    }
}