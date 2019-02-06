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
#include "TimeIntervalAverage.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
using namespace NOMADSUtil;
using namespace IHMC_NETSENSOR;

int main (int argc, char * argv[])
{
    auto pszMethodName = "main";
    pLogger = new Logger();
    pLogger->initLogFile ("netsensor.log", false);
    pLogger->enableScreenOutput();
    pLogger->enableFileOutput();
    pLogger->setDebugLevel (Logger::L_Info);

    IHMC_NETSENSOR::NetSensor netsensor;
    if (netsensor.init (argc, argv) || (netsensor.configureWithoutEmbeddedMode() < 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Init failed\n");
        exit (-1); 
    }

    netsensor.start();
    netsensor.passUserInput (UserInput::PRINT_HELP);
    UserInput userInput;

    bool keepGoing = true;
    bool terminationRequested = false;
    while (keepGoing) {
        char tmpBuff[20];
        scanf ("%s", tmpBuff);
        String sUserInput = tmpBuff;
        userInput = netsensor.getCommandFromUserInput (sUserInput);
        terminationRequested = (userInput == UserInput::CLOSE);
        netsensor.passUserInput (userInput);
        if (terminationRequested) {
            while (!netsensor.hasTerminated()) {}
            keepGoing = false;
        }
    }
    return 0;
}