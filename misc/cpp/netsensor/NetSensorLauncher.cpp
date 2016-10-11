/*
 * NetSensorLauncher.cpp
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

/*
* NetSensorLauncher.cpp
*
* Author:                   Roberto Fronteddu
* Year of creation:         2015/2016
* Last Revision by:
* Year of last Revision:*
* Version: 1.0
*/


#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define PROTOBUF_USE_DLLS

#include <stdio.h>
#include "Logger.h"
#include "NetSensor.h"
#include "NLFLib.h"
#include "Constants.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
using namespace NOMADSUtil;

int main (int argc, char *argv[])
{
    int rc;
    const char* cfgPath;
    
	//Initialize logger
    NOMADSUtil::pLogger = new NOMADSUtil::Logger ();
    NOMADSUtil::pLogger->initLogFile ("netsensor.log", false);
    NOMADSUtil::pLogger->enableScreenOutput();
    NOMADSUtil::pLogger->enableFileOutput();
    NOMADSUtil::pLogger->setDebugLevel (NOMADSUtil::Logger::L_Info);
    
	int test = DEFAULT;
    //int test = CLEANING_TEST;

    //User friendly name of the interface to sniff
	#if defined (WIN32)
		const char *pszInterface = "Local Area Connection"; //windows 7 ethernet
		//const char *pszInterface = "Wi-Fi"; //windows 7 ethernet
	#elif defined (UNIX)
		const char *pszInterface = "eth0"; //windows 7 ethernet
	#endif

    //Instantiate a NetSensor here and run it.
    IHMC_MISC::NetSensor theSensor;

	if (argc < 3) {
		#if defined (WIN32)
			if (0 != (rc = theSensor.init(0, pszInterface))) {
				checkAndLogMsg("main", Logger::L_SevereError, "Could not initialize NetSensor; rc = %d\n", rc);
				return -1;
			}
		#elif defined (UNIX)
				checkAndLogMsg("main", Logger::L_Warning, "Usage: NetSensorLauncher -i <interfaceName>\n");
		#endif
	}
	if (argc == 3) {
		//default mode
		if (!strcmp(argv[1], "-i")) {
			if (0 != (rc = theSensor.init(0,argv[2]))) {
				checkAndLogMsg("main", Logger::L_SevereError, "Could not initialize NetSensor; rc = %d\n", rc);
				return -1;
			}
		}
		if (!strcmp(argv[1], "-conf")) {
			if (0 != (rc = theSensor.init(1, argv[2]))) {
				checkAndLogMsg("main", Logger::L_SevereError, "Could not initialize NetSensor; rc = %d\n", rc);
				return -2;
			}
		}
	}
	theSensor.start();
	checkAndLogMsg("Main", Logger::L_Info, "Enter c to terminate\n\n");

	char* humanCommand = new char(30);
	bool waitForInput = true;
	while (!theSensor.hasTerminated()) {
		sleepForMilliseconds(1000);
		if (waitForInput) {
			scanf("%s", humanCommand);
			if (!strcmp("c", humanCommand)) {
				//termination requested
				theSensor.requestTermination();
				waitForInput = false;
				checkAndLogMsg("Main", Logger::L_Info, "Termination of NetSensor requested\n");
			}
		}
    }
    checkAndLogMsg("Main", Logger::L_Info, "Termination of NetSensor completed\n");
    return 0;
}
