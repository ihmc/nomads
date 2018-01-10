/*
* Launcher.cpp
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

#include "CommandLineParser.h"
#include "CommandLineConfigs.h"
#include "Logger.h"
#include "NetSensor.h"
#include "NLFLib.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
using namespace NOMADSUtil;
using namespace IHMC_NETSENSOR;

IHMC_NETSENSOR::NETSENSOR_USER_INPUT::UserInput handleUserInput(NOMADSUtil::String sUserInput)
{
    if ((sUserInput == "1") || (sUserInput == "close")) {
        return IHMC_NETSENSOR::NETSENSOR_USER_INPUT::CLOSE;
    }
    else if ((sUserInput == "2") || (sUserInput == "cfg")) {
        return IHMC_NETSENSOR::NETSENSOR_USER_INPUT::PRINT_CONFIG;
    }
    else if ((sUserInput == "3") || (sUserInput == "printTraffic")) {
        return IHMC_NETSENSOR::NETSENSOR_USER_INPUT::PRINT_TRAFFIC_TABLE_CONTENT;
    }
    else  if ((sUserInput == "4") || (sUserInput == "printTopology")) {
        return IHMC_NETSENSOR::NETSENSOR_USER_INPUT::PRINT_TOPOLOGY_TABLE_CONTENT;
    }
    else if ((sUserInput == "5") || (sUserInput == "printICMPInfo")) {
        return IHMC_NETSENSOR::NETSENSOR_USER_INPUT::PRINT_ICMP_TABLE_CONTENT;
    }
    else if ((sUserInput == "6") || (sUserInput == "printTCPRTT")) {
        return IHMC_NETSENSOR::NETSENSOR_USER_INPUT::PRINT_TCP_RTT_TABLE_CONTENT;
    }
    else if ((sUserInput == "7") || (sUserInput == "diag")) {
        return IHMC_NETSENSOR::NETSENSOR_USER_INPUT::LAUNCH_DIAGNOSTIC;
    }
    else if ((sUserInput == "8") || (sUserInput == "help")) {
        return IHMC_NETSENSOR::NETSENSOR_USER_INPUT::PRINT_HELP;
    }
    else if ((sUserInput == "9") || (sUserInput == "about")) {
        return IHMC_NETSENSOR::NETSENSOR_USER_INPUT::PRINT_ABOUT;
    }
    else {
        printf("Command unrecognized\n");
        return IHMC_NETSENSOR::NETSENSOR_USER_INPUT::PRINT_HELP;
    }
}

int main(int argc, char *argv[])
{
    NOMADSUtil::pLogger = new NOMADSUtil::Logger();
    NOMADSUtil::pLogger->initLogFile("netsensor.log", false);
    NOMADSUtil::pLogger->enableScreenOutput();
    NOMADSUtil::pLogger->enableFileOutput();
    NOMADSUtil::pLogger->setDebugLevel(NOMADSUtil::Logger::L_Info);

    IHMC_NETSENSOR::NetSensor nts;
    if (nts.init(argc, argv)) {
        exit(-1);
    }


    if (nts.configureWithoutEmbeddedMode() < 0) {
        exit(-1);
    };

    nts.start();

    nts.passUserInput(IHMC_NETSENSOR::NETSENSOR_USER_INPUT::PRINT_HELP);
    IHMC_NETSENSOR::NETSENSOR_USER_INPUT::UserInput userInput;

    bool keepGoing = true;
    NOMADSUtil::String sUserInput;
    bool terminationRequested = false;
    while (keepGoing) {
        char tmpBuff[20];
        scanf("%s", tmpBuff);
        sUserInput = tmpBuff;
        userInput = handleUserInput(sUserInput);

        terminationRequested = (userInput == IHMC_NETSENSOR::NETSENSOR_USER_INPUT::CLOSE);
        nts.passUserInput(userInput);
        if (terminationRequested) {
            while (!nts.hasTerminated()) {}
            keepGoing = false;
        }
    }
    return 0;
}