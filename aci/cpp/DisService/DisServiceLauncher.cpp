/*
 * DisServiceLauncher.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#include "DisseminationService.h"
#include "DisseminationServiceProxyServer.h"
#include "DisServiceCommandProcessor.h"

#include "ConfigManager.h"
#include "FileUtils.h"
#include "Logger.h"
#include "Mutex.h"
#include "NLFLib.h"

#include <signal.h>
#include <sys/stat.h>

#ifdef UNIX
    #include "SigFaultHandler.h"
#endif

using namespace IHMC_ACI;
using namespace NOMADSUtil;

void sigIntHandler (int sig);

bool _terminated;
Mutex _m;

int main (int argc, char *argv[])
{
    _terminated = false;
    const String execName (argv[0]);
    #ifdef UNIX
        SigFaultHandler handler (execName);
    #endif
    if (signal (SIGINT, sigIntHandler) == SIG_ERR) {
        printf ("Error handling SIGINT\n");
        exit (-1);
    }
    if (argc > 3) {
        printf ("Usage: ./DisServiceProxy <configFile>\n");
        exit (-2);
    }

    // Parse command line arguments
    String confFile;
    bool bShell = true;
    for (uint8 i = 1; i < argc; i++) {
        String opt (argv[i]);
        if (opt == "--no-shell") {
            bShell = false;
        }
        else {
            confFile = argv[i];
        }
    }

    // Read Configuration
    uint16 ui16Port = DIS_SVC_PROXY_SERVER_PORT_NUMBER;    
    ConfigManager *pCfgMgr = NULL;
    if (confFile.length() > 0) {
        if (!FileUtils::fileExists (confFile)) {
            printf ("The file at location: %s does not exist.\n", confFile.c_str());
            exit (-4);
        }
        pCfgMgr = new ConfigManager();
        pCfgMgr->init();
        int rc;
        if ((rc = pCfgMgr->readConfigFile (confFile)) != 0) {
            printf ("Error in config file %s reading. Returned code %d.\n", confFile.c_str(), rc);
            exit (-3);
        }
        Logger::configure (pCfgMgr);
        if (pCfgMgr->hasValue ("aci.disservice.proxy.port")) {
            ui16Port = (uint16) pCfgMgr->getValueAsInt("aci.disservice.proxy.port");
        }
        if (bShell && pCfgMgr->hasValue ("aci.disservice.shell")) {
            // The shell command line option superseds the config file property
            bShell = pCfgMgr->getValueAsBool ("aci.disservice.shell");
        }
    }

    if (pCfgMgr == NULL) {
        // Default Logger conf
        if (!pLogger) {
            pLogger = new Logger();
            pLogger->enableScreenOutput();
            pLogger->initLogFile("disseminationservice.log",false);
            pLogger->enableFileOutput();
            pLogger->setDebugLevel(Logger::L_LowDetailDebug);
        }
    }

    if (pLogger != NULL) {
        pLogger->displayAbsoluteTime();
    }

    //Initializing and starting DisService
    DisseminationService * _pDisService = (pCfgMgr ? new DisseminationService (pCfgMgr) : new DisseminationService());
    int rc;
    if (0 != (rc = _pDisService->init())) {
    	printf ("\nDisseminationService is failed to initialize\n");
        return -5;
    }
    _pDisService->start();

    // Initializing and starting ProxyServer
    DisseminationServiceProxyServer dsProxyServer;

    dsProxyServer.init (_pDisService, ui16Port);
    dsProxyServer.start();
    printf ("\nDisseminationProxyServer is running on port %d\n", ui16Port);

    // Invoking the command processor
    pLogger->disableScreenOutput();
    if (bShell) {
        DisServiceCommandProcessor cmdProc (_pDisService);
        cmdProc.setPrompt ("DisService");
        cmdProc.enableNetworkAccess (5555);
        cmdProc.run();
    }
    else {
        while (!_terminated) {
            sleepForMilliseconds (3000);
        }
    }

    printf ("\nDisService is terminating...\n");
    _pDisService->requestTerminationAndWait();
    delete _pDisService;
    _pDisService = NULL;
    delete pLogger;
    pLogger = NULL;
    delete pCfgMgr;
    pCfgMgr = NULL;

    return 0;
}

void sigIntHandler (int sig)
{
    _m.lock();
    _terminated = true;
    _m.unlock();
    exit(0);
}
