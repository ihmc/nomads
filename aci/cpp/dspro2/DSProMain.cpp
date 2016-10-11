/*
 * DSProMain.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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

#include "DSPro.h"

#include "Defs.h"
#include "DSProCmdProcessor.h"
#include "DSProProxyServer.h"
#include "DSProUtils.h"
#include "DSLib.h"
#include "Instrumentator.h"
#include "Topology.h"

#include "CommAdaptor.h"

#include "ConfigManager.h"
#include "FileUtils.h"

#include "NetUtils.h"
#include "NLFLib.h"
 #ifdef UNIX
    #include "SigFaultHandler.h"
#endif
#include "StrClass.h"
#include "UUID.h"

#include <signal.h>
#include <sys/stat.h>
#include <time.h>

#ifdef UNIX
    #include <execinfo.h>
    #include <cxxabi.h>
#endif

#define usage "Usage: ./DSPro -c|--conf <configFile> [-e|--metadataExtraAttr <metadataExtraAttributesFile>] [-v|--metadataValues <metadataValuesFile>]\n"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    static NOMADSUtil::String Application_Executable_Name;
    static char BUILD_TIME[] = __DATE__ " " __TIME__;

    bool TERMINATE = false;

    struct Arguments
    {
        Arguments() {}
        ~Arguments() {}

        String pathToExecutable;
        String configFilePath;
        String metadataValuesFile;
        String metadataExtraAttributesFile;
    };

    void printUsage()
    {
        printf (usage);
        printf ("-c, --conf.\t\t\tSet the location of DS Pro property file.\n");
        printf ("-e, --metadataExtraAttr.\tSet the location of the XML file describing the extra metadata attributes to be used.\n");
        printf ("-v, --metadataValues.\t\tSet the location of the XML file describing the values that the metadata attributes can assume.\n");
        printf ("-h, --help.\t\t\tPrint this help.\n");
    }

    void printUsageAndExitWithError()
    {
        printUsage();
        exit(-1);
    }

    void getDefaultConfigFile (const char *pszHomeDir, const char *pszDefaultConfigFile, String &configFile, bool bPermissive=true)
    {
        if (configFile.length() <= 0) {
            char *pszConfigFilePath = ConfigManager::getDefaultConfigFilePath (pszHomeDir, pszDefaultConfigFile);
            if ((pszConfigFilePath == NULL) && (!bPermissive)) {
                printUsageAndExitWithError();
            }
            else if (pszConfigFilePath != NULL) {
                configFile = pszConfigFilePath;
                free (pszConfigFilePath);
                checkAndLogMsg ("DSProMain::getDefaultConfigFile", Logger::L_Info, "attempting to run "
                                "DSPro with default config file: <%s>\n", configFile.c_str());
            }
        }
    }

    int getHomeDir (const char *pszProgDir, String &homeDir)
    {
        #ifndef ANDROID
            if (pszProgDir == NULL) {
                return -1;
            }
            // Compute the Home Directory
            // Assume that the executable is in <home>\\bin (or <home/bin>)
            char szHomeDir[PATH_MAX];
            strcpy (szHomeDir, pszProgDir);
            // Strip off last directory level in path
            char *pszTemp = strrchr (szHomeDir, getPathSepChar());
            if (pszTemp == NULL) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "executable not installed in expected directory structure - could not parse directory "
                                "<%s> to compute the config file directory\n", pszProgDir);
                #if defined (WIN32)
                    printf ("\n\n");
                    system ("pause");
                #endif
                return -2;
            }
            *pszTemp = '\0';
            homeDir = szHomeDir;
        #endif
        return 0;
    }

    int getNodeId (ConfigManager *pCfgMgr, String &nodeId)
    {
        if (pCfgMgr->hasValue ("aci.dspro.node.id")) {
            nodeId = pCfgMgr->getValue ("aci.dspro.node.id");
        }
        else if (pCfgMgr->hasValue ("aci.disService.nodeUUID")) {
            // for backward compatibility
            nodeId = pCfgMgr->getValue ("aci.disService.nodeUUID");
        }
        else if (pCfgMgr->hasValue("aci.dspro.node.id.auto") &&
            (strcmp ("hostname", pCfgMgr->getValue("aci.dspro.node.id.auto")) == 0)) {
            nodeId = NetUtils::getLocalHostName();
            pCfgMgr->setValue("aci.dspro.node.id", nodeId);
        }
        else if (pCfgMgr->hasValue ("aci.disService.nodeUUID.auto.mode") &&
            (strcmp ("hostname", pCfgMgr->getValue ("aci.disService.nodeUUID.auto.mode")) == 0)) {
            // for backward compatibility
            nodeId = NetUtils::getLocalHostName();
            pCfgMgr->setValue ("aci.dspro.node.id", nodeId);
        }
        else {
            NOMADSUtil::UUID uuid;
            uuid.generate();
            const char *pszUUID = uuid.getAsString();
            if (pszUUID != NULL) {
                nodeId = pszUUID;
            }
        }

        return ((nodeId.length() > 0) ? 0 : -1);
    }

    char * getValue (int argc, char **argv, int index)
    {
        if (index <= argc) {
            return argv[index];
        }
        printUsageAndExitWithError();
        return NULL;
    }

    void initializeLogger (Logger **pLogger, const char *pszLogDir, const char *pszLogFileName,
                           const char *pszTimestamp, bool bEnableScreenOutput)
    {
        if ((pLogger == NULL) || (pszLogFileName == NULL) || (pszTimestamp == NULL)) {
            return;
        }
        if ((*pLogger) == NULL) {
            (*pLogger) = new Logger();
            (*pLogger)->setDebugLevel (Logger::L_LowDetailDebug);

            // Screen output
            if (bEnableScreenOutput) {
                (*pLogger)->enableScreenOutput();
            }

            // File output
            String filename ((pszLogDir == NULL) ? "" : pszLogDir);
            if (filename.length() > 0) {
                filename += getPathSepCharAsString();
            }
            filename += pszTimestamp;
            filename += "-";
            filename += pszLogFileName;
            (*pLogger)->initLogFile (filename.c_str(), false);
            (*pLogger)->enableFileOutput();
            printf ("Enabled logging on file %s\n", filename.c_str());

            // Network output
            (*pLogger)->initNetworkLogging ("127.0.0.1");
            (*pLogger)->enableNetworkOutput (Logger::L_LowDetailDebug);
        }
    }

    void initializeLoggers (const char *pszLogDir)
    {
        if (!FileUtils::directoryExists (pszLogDir)) {
            if (!FileUtils::createDirectory (pszLogDir)) {
                return;
            }
        }
        time_t now = time (NULL);
        struct tm *ptm = localtime (&now);
        static char timestamp[17];
        sprintf (timestamp, "%d%02d%02d-%02d_%02d_%02d",
                 (ptm->tm_year+1900), (ptm->tm_mon+1), ptm->tm_mday,
                 ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

        printf ("Running DSPro (Version built on %s).\n", BUILD_TIME);
        initializeLogger (&pLogger, pszLogDir, "dspro.log", timestamp, false);
        checkAndLogMsg ("main", Logger::L_Info, "Running DSPro version built on %s.\n", BUILD_TIME);
        initializeLogger (&pNetLog, pszLogDir, "dspro-matchmaking.log", timestamp, false);
        initializeLogger (&pTopoLog, pszLogDir, "dspro-topology.log", timestamp, false);
        initializeLogger (&pCmdProcLog, pszLogDir, "dspro-notifications.log", timestamp, false);
    }

    int initializeAndStartProxyServer (DSPro *pDSPro, ConfigManager *pCfgMgr, DSProProxyServer &proxySrv)
    {
        if (pDSPro == NULL || pCfgMgr == NULL) {
            checkAndLogMsg ("DSProMain::initializeAndStartProxyServer (1)", Logger::L_SevereError,
                            "DSProProxyServer has failed to initialize\n");
            return -1;
        }
        const char *pszProxyServerInterface = NULL;
        uint16 ui16ProxyServerPort = 56487; // DISPRO_SVC_PROXY_SERVER_PORT_NUMBER;
        if (pCfgMgr->hasValue ("aci.disservice.proxy.interface")) {
            pszProxyServerInterface = pCfgMgr->getValue ("aci.disservice.proxy.interface");
        }
        if (pCfgMgr->hasValue ("aci.disservice.proxy.port")) {
            ui16ProxyServerPort = (uint16) pCfgMgr->getValueAsInt ("aci.disservice.proxy.port", 56487);
        }

        if (proxySrv.init (pDSPro, pszProxyServerInterface, ui16ProxyServerPort) != 0) {
            checkAndLogMsg ("DSProMain::initializeAndStartProxyServer (2)", Logger::L_SevereError,
                            "DSProProxyServer has failed to initialize\n");
            return -2;
        }
        proxySrv.start();
        checkAndLogMsg ("DSProMain::initializeAndStartProxyServer", Logger::L_Info,
                        "DSProProxyServer is running on port %d\n", ui16ProxyServerPort);
        return 0;
    }

    void parseArguments (int argc, char *argv[], Arguments &arguments)
    {
        arguments.pathToExecutable = getProgHomeDir (argv[0]);

        String option;
        for (int i = 1; i < argc; i++) {
            option = argv[i];
            if (option == "-c" || option == "--conf") {
                arguments.configFilePath = getValue (argc, argv, ++i);
            }
            else if (option == "-e" || option == "--metadataExtraAttr") {
                arguments.metadataExtraAttributesFile = getValue (argc, argv, ++i);
            }
            else if (option == "-v" || option == "--metadataValues") {
                arguments.metadataValuesFile = getValue (argc, argv, ++i);
            }
            else if (option == "-h" || option == "--help") {
                printUsage();
                exit(0);
            }
            else {
                printUsageAndExitWithError();
            }
        }
    }

    void sigIntHandler (int sig)
    {
        TERMINATE = true;
        exit (0);
    }
}

int main (int argc, char *argv[])
{
    Application_Executable_Name = argv[0];
    const char *pszMethodName = "DSProMain::main";
    #ifdef UNIX
        SigFaultHandler handler (Application_Executable_Name);
    #endif
    if (signal (SIGINT, sigIntHandler) == SIG_ERR) {
        fprintf (stderr, "Error registering SIGINT handler\n");
        exit (-1);
    }

    // Parse arguments
    IHMC_ACI::Arguments arguments;
    IHMC_ACI::parseArguments (argc, argv, arguments);

    String homeDir;
    getHomeDir (arguments.pathToExecutable, homeDir);

    String logDir (homeDir);
    logDir += getPathSepCharAsString();
    logDir += "logs";
    IHMC_ACI::initializeLoggers (logDir);

    if (arguments.configFilePath.length() <= 0) {
        IHMC_ACI::getDefaultConfigFile (homeDir, "dspro.properties", arguments.configFilePath, false);
    }
    if (!FileUtils::fileExists (arguments.configFilePath.c_str())) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "The config file at "
                        "location: %s does not exist!\n", arguments.configFilePath.c_str());
    }

    ConfigManager cfgMgr;
    int rc = cfgMgr.init();
    if (rc != 0) {
        printf ("Count not initialize ConfigManager. Returned code %d\n", rc);
        exit (1);
    }
    if ((rc = cfgMgr.readConfigFile (arguments.configFilePath)) != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Error in config file "
                        "%s reading. Returned code %d\n", arguments.configFilePath.c_str(), rc);
    }

    //Initializing and starting DSPro
    String nodeId;
    if (getNodeId (&cfgMgr, nodeId) != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Node ID was not set.\n");
        return 2;
    }

    // Read extra-attributes if necessary
    char *pszMetadataExtraAttributes = NULL;
    if (arguments.metadataExtraAttributesFile.length() <= 0) {
        IHMC_ACI::getDefaultConfigFile (homeDir, "metadataExtraAttributes.xml", arguments.metadataExtraAttributesFile);
    }
    if (arguments.metadataExtraAttributesFile.length() > 0) {
        DSLib::readFileIntoString (arguments.metadataExtraAttributesFile, &pszMetadataExtraAttributes);
    }

    // Read extra-values if necessary
    char *pszMetadataValues = NULL;
    if (arguments.metadataValuesFile.length() <= 0) {
        IHMC_ACI::getDefaultConfigFile (homeDir, "metadataExtraValues.xml", arguments.metadataExtraAttributesFile);
    }
    if (arguments.metadataValuesFile.length() > 0) {
        DSLib::readFileIntoString (arguments.metadataValuesFile, &pszMetadataValues);
    }

    // Instantiate and init DSPro
    DSPro dspro (nodeId, BUILD_TIME);
    if ((rc = dspro.init (&cfgMgr, pszMetadataExtraAttributes, pszMetadataValues)) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "DSPro has failed to initialize. Return code: %d\n", rc);
        return 3;
    }

    uint16 ui16;
    uint16 ui16ClientId = 0;
    DSProCmdProcessor dsproCmdProc (&dspro);
    dspro.registerDSProListener (ui16ClientId, &dsproCmdProc, ui16);
    dspro.registerSearchListener (ui16ClientId, &dsproCmdProc, ui16);

    //Initializing and starting ProxyServer
    DSProProxyServer proxySrv;
    rc = initializeAndStartProxyServer (&dspro, &cfgMgr, proxySrv);
    if (rc != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "DSProProxyServer has failed to initialize. Return code: %d\n", rc);
        return 4;
    }

    // Connect to peers
    DSProUtils::addPeers (dspro, cfgMgr);
    dsproCmdProc.run();

    // Request Termination
    proxySrv.requestTerminationAndWait();

    // And delete objects
    delete pLogger;
    pLogger = NULL;
    delete pNetLog;
    pNetLog = NULL;
    delete pTopoLog;
    pTopoLog = NULL;

    // Terminate
    return 0;
}

