#include "DisseminationService.h"
#include "DisseminationServiceProxyServer.h"
#include "DisServiceCommandProcessor.h"
#include "DSPro.h"
#include "DSProCmdProcessor.h"
#include "Instrumentator.h"
#include "Topology.h"
#include "Voi.h"
#include "DSProProxyServer.h"
#include "DSProUtils.h"
#include "DSLib.h"

#include "Voi.h"

#include "CommAdaptor.h"
#include "MocketsAdaptor.h"

#include "ConfigManager.h"
#include "FileUtils.h"
#include "Logger.h"
#include "Mutex.h"
#include "NLFLib.h"
#include "StringTokenizer.h"

#include <signal.h>
#include <sys/stat.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;
using namespace IHMC_VOI;

static const char *pszLoggerRemoteHost = "10.2.35.100";
static const char *pszLocalHost = "10.101.0.27";
static String sConfigPath = "/IHMC/conf/dspro.properties";
static String sLogPath = "/IHMC/log/dspro.log";
static String sLogNetPath = "/IHMC/log/dspro-matchmaking.log";
static String sDsLogPath = "/IHMC/log/disseminationservice.log";
static String sLogTopoPath = "/IHMC/log/dspro-topology.log";
static String sNotificationLogPath = "/IHMC/log/dspro-notification.log";
static String sDbPath = "/IHMC/log/db.sqlite";
static String sMetadataExtraAttributesPath = "/IHMC/conf/metadataExtraAttributes.xml";
static String sMetadataValuesPath = "/IHMC/conf/metadataExtraValues.xml";
static String sStatsFile = "/sdcard/ihmc/log/dspro-stats.csv"; //comment to my future self or interns, this is because when you read from the ConfigManager, use /sdcardto start the path

static DSPro *pDSPro = NULL;
static DSProCmdProcessor *pDSProCmdProc = NULL;
static DSProProxyServer proxySrv (true);
static DisseminationService *pDisService = NULL;
static DisseminationServiceProxyServer dsProxyServer;
static ConfigManager *pConfigManager = NULL;
static bool TERMINATE = false;
const char *pszProxyServerInterface = NULL;
const char *pszTeamID = NULL;
const char *pszMissionID = NULL;
const char *pszRole = NULL;

Mutex netReloadMutex;

char *pszMetadataExtraAttributes = NULL;
char *pszMetadataValues = NULL;

int configure(bool isDSPro, uint16 ui16Port, const char *pszStorageDir, const char *pszVersion)
{
        String sStorageDir (pszStorageDir);
        String sConfigFile = sStorageDir + sConfigPath;
        String sLogFile = (isDSPro? sStorageDir + sLogPath : sStorageDir + sDsLogPath);
        String sLogNetFile = sStorageDir + sLogNetPath;
        String sLogTopoFile = sStorageDir + sLogTopoPath;
        String sLogNotificationFile = sStorageDir + sNotificationLogPath;
        String sDbFile = sStorageDir + sDbPath;
        String sMetadataExtraAttributesFile = sStorageDir + sMetadataExtraAttributesPath;
        String sMetadataValuesFile = sStorageDir + sMetadataValuesPath;

        if (!pLogger) {
            pLogger = new Logger();
            //pLogger->initNetworkLogging (pszLoggerRemoteHost, 1306);
            pLogger->enableScreenOutput();
     	    pLogger->initLogFile(sLogFile.c_str(), false);
            pLogger->enableFileOutput();
            pLogger->setDebugLevel(Logger::L_MediumDetailDebug);
            //pLogger->enableNetworkOutput (Logger::L_MediumDetailDebug);
            pLogger->logMsg ("main", Logger::L_Info, "Running DSPro version built on %s.\n", pszVersion); 
        }

        if (!IHMC_VOI::pNetLog && isDSPro) {
            IHMC_VOI::pNetLog = new Logger();
            //pNetLog->initNetworkLogging (pszLoggerRemoteHost, 1306);
            IHMC_VOI::pNetLog->enableScreenOutput();
            IHMC_VOI::pNetLog->initLogFile(sLogNetFile.c_str(), false);
            IHMC_VOI::pNetLog->enableFileOutput();
            IHMC_VOI::pNetLog->setDebugLevel (Logger::L_MediumDetailDebug);
            //pNetLog->enableNetworkOutput (Logger::L_MediumDetailDebug);
        }

        if (!pTopoLog && isDSPro) {
            pTopoLog = new Logger();
            //pLogger->initNetworkLogging (pszLoggerRemoteHost, 1306);
            pTopoLog->disableScreenOutput();
     	    pTopoLog->initLogFile(sLogTopoFile.c_str(), false);
            pTopoLog->enableFileOutput();
            pTopoLog->setDebugLevel(Logger::L_MediumDetailDebug);
            //pLogger->enableNetworkOutput (Logger::L_MediumDetailDebug);
        }

        if (!pCmdProcLog && isDSPro) {
            pCmdProcLog = new Logger();
            //pLogger->initNetworkLogging (pszLoggerRemoteHost, 1306);
            pCmdProcLog->enableScreenOutput();
     	    pCmdProcLog->initLogFile(sLogNotificationFile.c_str(), false);
            pCmdProcLog->enableFileOutput();
            pCmdProcLog->setDebugLevel(Logger::L_MediumDetailDebug);
            //pCmdProcLog->enableNetworkOutput (Logger::L_MediumDetailDebug);
        }

        if (false) {
            //use static configuration for DisServicePro
            pConfigManager = new ConfigManager();
            pConfigManager->init();
            pConfigManager->setValue ("aci.disService.propagation.targetFiltering", "true");
            //pConfigManager->setValue ("aci.disService.netIFs", pszLocalHost);
            pConfigManager->setValue ("aci.disService.networkMessageService.port", 6669);
            pConfigManager->setValue ("aci.disService.forwarding.mode", 1);
            pConfigManager->setValue ("aci.disService.forwarding.probability", 0);
            pConfigManager->setValue ("aci.disService.networkMessageService.delivery.async", "true");
            pConfigManager->setValue ("aci.disService.nodeConfiguration.bandwidth", 1024);
            //pConfigManager->setValue ("aci.disService.storageMode", 1);
            //pConfigManager->setValue ("aci.disService.storageFile", sDbFile.c_str());
            pConfigManager->setValue ("aci.dspro.scheduler.thread.run", "false");
            //local node context
            pConfigManager->setValue ("aci.dspro.localNodeContext.pszTeamID", "Marine Rifle Squad");
            pConfigManager->setValue ("aci.dspro.localNodeContext.pszMissionID", "Mission X");
            pConfigManager->setValue ("aci.dspro.localNodeContext.pszRole", "Automatic Rifleman");
            pConfigManager->setValue ("aci.dspro.localNodeContext.usefulDistance", 1000);
            pConfigManager->setValue ("aci.dspro.localNodeContext.classifier.type", "NON_CLASSIFYING");
            // MetadataRanker weights
            pConfigManager->setValue ("aci.dspro.metadataRanker.coordRankWeight", 1.0);
            pConfigManager->setValue ("aci.dspro.metadataRanker.timeRankWeight", 2.0);
            pConfigManager->setValue ("aci.dspro.metadataRanker.expirationRankWeight", 0.0);
            pConfigManager->setValue ("aci.dspro.metadataRanker.impRankWeight", 0.0);
            pConfigManager->setValue ("aci.dspro.metadataRanker.predRankWeight", 0.0);
            //Information Pull
            pConfigManager->setValue ("aci.dspro.informationPull.rankThreshold", 0);
            //Information Push
            pConfigManager->setValue ("aci.dspro.informationPush.rankThreshold", 6);
            //Pre-Staging
            pConfigManager->setValue ("aci.dspro.dsprorepctrl.prestaging.enabled", "false");
            //pConfigManager->setValue ("aci.dspro.instrument", "false");
        }
        else {
            if (sConfigFile != NULL) {
                if (FileUtils::fileExists (sConfigFile.c_str())) {
                    pConfigManager = new ConfigManager();
                    pConfigManager->init();
                    int rc;
                    if ((rc = pConfigManager->readConfigFile((const char *) sConfigFile.c_str())) != 0) {
                        pLogger->logMsg("DisServiceProApp::startDisService", Logger::L_MildError, "Error in config file reading.\n");
                        return -3;
                    }
                    if (pConfigManager->hasValue ("aci.disservice.proxy.interface")) {
                        pszProxyServerInterface = pConfigManager->getValue ("aci.disservice.proxy.interface");
                    }
                    if (pConfigManager->hasValue ("aci.disservice.proxy.port")) {
                        ui16Port = (uint16) pConfigManager->getValueAsInt("aci.disservice.proxy.port");
                    }
                    if (pConfigManager->hasValue ("aci.dspro.localNodeContext.teamId")) {
                        pszTeamID = pConfigManager->getValue ("aci.dspro.localNodeContext.teamId", "");
                    }
                    if (pConfigManager->hasValue ("aci.dspro.localNodeContext.missionId")) {
                        pszMissionID = pConfigManager->getValue ("aci.dspro.localNodeContext.missionId", "");
                    }
                    if (pConfigManager->hasValue ("aci.dspro.localNodeContext.role")) {
                        pszRole = pConfigManager->getValue ("aci.dspro.localNodeContext.role", "");
                    }
                }
                else {
                    pLogger->logMsg("DisServicePro2App::startDisServicePro", Logger::L_MildError, "The file at location: %s does not exist!\n", (const char *) sConfigFile.c_str());
                    return (-4);
                }
            }
        }

        //read extra attributes from file
        if (FileUtils::fileExists (sMetadataExtraAttributesFile.c_str())) {
            DSLib::readFileIntoString (sMetadataExtraAttributesFile.c_str(), &pszMetadataExtraAttributes);
        }

        if (FileUtils::fileExists (sMetadataValuesFile.c_str())) {
            DSLib::readFileIntoString (sMetadataValuesFile.c_str(), &pszMetadataValues);
        }

    return 0;
}


int startDSPro(uint16 ui16Port, const char *pszStorageDir, const char *pszVersion)
{
        configure(true, ui16Port, pszStorageDir, pszVersion);

        //Initializing and starting DSPro
        const char *pszNodeId = NULL;
        if (pConfigManager->hasValue ("aci.dspro.node.id")) {
            pszNodeId = pConfigManager->getValue ("aci.dspro.node.id");
        }
        else if (pConfigManager->hasValue ("aci.disService.nodeUUID")) {
            pszNodeId = pConfigManager->getValue ("aci.disService.nodeUUID");
        }
        //set stats file if not present
        if (!pConfigManager->hasValue ("aci.dspro.stats.file")) {
            pConfigManager->setValue ("aci.dspro.stats.file", sStatsFile.c_str());
        }

        if (pszNodeId == NULL) {
            printf ("Node ID was not set.\n");
            return 1;
        }
        pDSPro = new DSPro (pszNodeId, pszVersion);

        int rc;
        if ((rc = pDSPro->init (pConfigManager, pszMetadataExtraAttributes, pszMetadataValues)) < 0) {
            printf ("DSPro has failed to initialize. Return code: %d\n", rc);
            return 2;
        }

        // Instantiate Command Processor
        pDSProCmdProc = new DSProCmdProcessor (pDSPro);
        uint16 ui16;
        uint16 ui16ClientId = 0;
        pDSPro->registerDSProListener (ui16ClientId, pDSProCmdProc, ui16);

        //Initializing and starting ProxyServer
        if (proxySrv.init (pDSPro, pszProxyServerInterface, ui16Port) != 0) {
            printf ("DSProProxyServer has failed to initialize\n");
            return 3;
        }
        proxySrv.start();
        printf ("DSProProxyServer is running on port %d\n", ui16Port);

        // Connect to peers
        DSProUtils::addPeers (*pDSPro, *pConfigManager); 

    while (!TERMINATE) {
        sleepForMilliseconds (2000);
    }

    if (pLogger != NULL)
        pLogger->logMsg("DisServiceProApp::startDisServicePro()", Logger::L_Info, "Before RequestTerminationAndWait()\n");
    // Request Termination
        proxySrv.requestTerminationAndWait();
    if (pLogger != NULL)
        pLogger->logMsg("DisServiceProApp::startDisServicePro()", Logger::L_Info, "After RequestTerminationAndWait()\n");

    // And delete objects
    delete pDSPro;
    pDSPro = NULL;
    delete pConfigManager;
    pConfigManager = NULL;
    delete pLogger;
    pLogger = NULL;
    delete IHMC_VOI::pNetLog;
    IHMC_VOI::pNetLog = NULL;
    delete pTopoLog;
    pTopoLog = NULL;

    return 0;
}

int startDisService(uint16 ui16Port, const char *pszStorageDir, const char *pszVersion)
{
    configure(false, ui16Port, pszStorageDir, pszVersion);

    //Initializing and starting DisService
    pDisService = (pConfigManager ? new DisseminationService(pConfigManager) : new DisseminationService());
    int rc;
    netReloadMutex.lock();
    if (0 != (rc = pDisService->init())) {
        if (pLogger != NULL)
    	    pLogger->logMsg ("DisServiceProApp::startDisService", Logger::L_Info, "DisseminationService failed to initialize\n");
        netReloadMutex.unlock();
        return -5;
    }
    pDisService->start();

    //Initializing and starting ProxyServer
    dsProxyServer;
    if (0 != (rc = dsProxyServer.init (pDisService, ui16Port, pszProxyServerInterface))) {
        if (pLogger != NULL)
            pLogger->logMsg ("DisServiceProApp::startDisService", Logger::L_Warning, "DisseminationServiceProxyServer failed to initialize\n");
        netReloadMutex.unlock();
        return -6;
    }

    dsProxyServer.start();
    if (pLogger != NULL)
        pLogger->logMsg ("DisServiceProApp::startDisService", Logger::L_Info, "DisseminationServiceProxyServer is running on port %d\n", ui16Port);
    netReloadMutex.unlock();

    while (!TERMINATE) {
        sleepForMilliseconds(1000);
    }

    if (pLogger != NULL)
        pLogger->logMsg ("DisServiceProApp::startDisService", Logger::L_Info, "DisseminationProxyServer is quitting...\n");
    pDisService->requestTerminationAndWait();
    delete pDisService;
    pDisService = NULL;
    delete pConfigManager;
    pConfigManager = NULL;
    delete pLogger;
    pLogger = NULL;

    return 0;
}

int reloadTransmissionService()
{

    if (pDisService == NULL) {
        printf ("DisService is NULL.\n");
        return -1;
    }

    int rc;
    netReloadMutex.lock();
    if (pLogger != NULL)
        pLogger->logMsg("DisServiceProApp::reloadTransmissionService", Logger::L_Info, "Before reloadTransmissionService.\n");
    rc = pDisService->reloadTransmissionService();
    if (pLogger != NULL)
        pLogger->logMsg("DisServiceProApp::reloadTransmissionService", Logger::L_Info, "After reloadTransmissionService.\n");
    netReloadMutex.unlock();

    return rc;
}

int reloadCommAdaptors()
{
    if (pDSPro == NULL) {
        printf ("DSPro is NULL.\n");
        return -1;
    }

    int rc;
    netReloadMutex.lock();
    if (pLogger != NULL)
        pLogger->logMsg("DisServiceProApp::reloadCommAdaptors", Logger::L_Info, "Before reloadCommAdaptors.\n");
    rc = pDSPro->reloadCommAdaptors();
    if (pLogger != NULL)
        pLogger->logMsg("DisServiceProApp::reloadCommAdaptors", Logger::L_Info, "After reloadCommAdaptors.\n");
    netReloadMutex.unlock();

    return rc;
}


int stopDisServicePro()
{
    TERMINATE = true;
//    if (pLogger != NULL)
//        pLogger->logMsg("DisServiceProApp::stopDisServicePro()", Logger::L_Info, "Before RequestTerminationAndWait()\n");
//    // Request Termination
//    proxySrv.requestTerminationAndWait();
//    if (pLogger != NULL)
//        pLogger->logMsg("DisServiceProApp::stopDisServicePro()", Logger::L_Info, "After RequestTerminationAndWait()\n");
//
//    // And delete objects
//    delete pDSPro;
//    pDSPro = NULL;
//    delete pConfigManager;
//    pConfigManager = NULL;
//    delete pLogger;
//    pLogger = NULL;
//    delete pNetLog;
//    pNetLog = NULL;
//    delete pTopoLog;
//    pTopoLog = NULL;

    return 0;
}
