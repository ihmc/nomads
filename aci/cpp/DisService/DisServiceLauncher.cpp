/*
 * DisServiceLauncher.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

using namespace IHMC_ACI;
using namespace NOMADSUtil;

void sigIntHandler (int sig);

bool _terminated;
Mutex _m;

int main (int argc, char *argv[])
{
    _m.lock();
    _terminated = false;
    _m.unlock();

    if (signal (SIGINT, sigIntHandler) == SIG_ERR) {
        printf ("Error handling SIGINT\n");
        exit (-1);
    }

    ConfigManager *pConfigManager = NULL;
    uint16 ui16Port = DIS_SVC_PROXY_SERVER_PORT_NUMBER;

    // Read Configuration
    if (argc > 2) {
        printf ("Usage: ./DisServiceProxy <configFile>\n");
        exit (-2);
    }

    if (argv[1]) {
        if (FileUtils::fileExists(argv[1])) {
            pConfigManager = new ConfigManager();
            pConfigManager->init();
            int rc;
            if ((rc = pConfigManager->readConfigFile((const char *) argv[1])) != 0) {
                printf ("Error in config file %s reading. Returned code %d.\n", (const char *) argv[1], rc);
                exit (-3);
            }
            if (pConfigManager->hasValue("aci.disservice.proxy.port")) {
                ui16Port = (uint16) pConfigManager->getValueAsInt("aci.disservice.proxy.port");
            }
            if (pConfigManager->getValueAsBool("util.logger.enabled", true)) {
                if (!pLogger) {
                    pLogger = new Logger();
                    if (pConfigManager->getValueAsBool("util.logger.out.screen.enabled", true)) {
                        pLogger->enableScreenOutput();
                    }
                    if (pConfigManager->getValueAsBool("util.logger.out.file.enabled", true)) {
                        pLogger->initLogFile (pConfigManager->getValue ("util.logger.out.file.path", "ds.log"), false);
                        pLogger->enableFileOutput();
                        pLogger->initErrorLogFile (pConfigManager->getValue ("util.logger.error.file.path", "dserror.log"), false);
                        pLogger->enableErrorLogFileOutput();
                    }
                    uint8 ui8DbgDetLevel = (uint8) pConfigManager->getValueAsInt ("util.logger.detail", Logger::L_LowDetailDebug);
                    switch (ui8DbgDetLevel) {
                        case Logger::L_SevereError:
                        case Logger::L_MildError:
                        case Logger::L_Warning:
                        case Logger::L_Info:
                        case Logger::L_NetDetailDebug:
                        case Logger::L_LowDetailDebug:
                        case Logger::L_MediumDetailDebug:
                        case Logger::L_HighDetailDebug:
                            pLogger->setDebugLevel (ui8DbgDetLevel);
                            pLogger->logMsg ("DisServiceLauncher::main", Logger::L_Info,
                                             "Setting debug level to %d\n", ui8DbgDetLevel);
                            break;
                        default:
                            pLogger->setDebugLevel(Logger::L_LowDetailDebug);
                            pLogger->logMsg("DisServiceLauncher::main",
                                            Logger::L_Info,
                                            "Invalid Logger detail debug level. Setting it to %d\n",
                                            Logger::L_LowDetailDebug);
                    }
                }
            }
        }
        else {
            printf ("The file at location: %s does not exist.\n", (const char *) argv[1]);
            exit (-4);
        }
    }

    if (pConfigManager == NULL) {
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
    DisseminationService * _pDisService = (pConfigManager ? new DisseminationService(pConfigManager) : new DisseminationService());
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
    DisServiceCommandProcessor cmdProc (_pDisService);
    cmdProc.setPrompt ("DisService");
    cmdProc.enableNetworkAccess (5555);
    cmdProc.run();

    printf ("\nDisService is terminating...\n");
    _pDisService->requestTerminationAndWait();
    delete _pDisService;
    _pDisService = NULL;
    delete pLogger;
    pLogger = NULL;
    delete pConfigManager;
    pConfigManager = NULL;

    return 0;
}

void sigIntHandler (int sig)
{
    _m.lock();
    _terminated = true;
    _m.unlock();
}
