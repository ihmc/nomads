/*
* NetSensor.cpp
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

#include "NetSensor.h"
#include "ReplayModeConfigFileProcesser.h"
#include "NetUtils.h"
#include "Json.h"

using namespace NOMADSUtil;
using namespace netsensor;
using namespace IHMC_NETSENSOR::NETSENSOR_STATE;
using namespace IHMC_NETSENSOR::NETSENSOR_USER_INPUT;

#define checkAndLogMsg if (pLogger) pLogger->logMsg


namespace IHMC_NETSENSOR
{

const NetSensor::InterfaceInfoOpt NetSensor::InterfaceInfoOpt::fromJSON(const JsonObject * const jObj)
{
    InterfaceInfoOpt iio{};
    if (jObj == nullptr) {
        return InterfaceInfoOpt{};
    }

    if (jObj->hasObject("name")) {
        if (jObj->getString("name", iio.pcIname) < 0) {
            return InterfaceInfoOpt{};
        }
        if (iio.pcIname == "") {
            return InterfaceInfoOpt{};
        }
    }
    if (jObj->hasObject("emac")) {
        String sEMAC("");
        if (jObj->getString("emac", sEMAC) < 0) {
            return InterfaceInfoOpt{};
        }
        iio.emac = NetUtils::getEtherMACAddrFromString(sEMAC);
        if (iio.emac == EtherMACAddr{ 0 }) {
            return InterfaceInfoOpt{};
        }
    }
    if (jObj->hasObject("ip")) {
        String sIP("");
        if (jObj->getString("ip", sIP) < 0) {
            return InterfaceInfoOpt{};
        }
        iio.ipAddr = InetAddr(sIP).getIPAddress();
        if (iio.ipAddr == 0) {
            return InterfaceInfoOpt{};
        }
    }
    if (jObj->hasObject("netmask")) {
        String sNetMask("");
        if (jObj->getString("netmask", sNetMask) < 0) {
            return InterfaceInfoOpt{};
        }
        iio.netmask = InetAddr(sNetMask).getIPAddress();
        if (iio.netmask == 0) {
            return InterfaceInfoOpt{};
        }
    }
    if (jObj->hasObject("gw")) {
        String sGW("");
        if (jObj->getString("gw", sGW) < 0) {
            return InterfaceInfoOpt{};
        }
        iio.gwAddr = InetAddr(sGW).getIPAddress();
        if (iio.gwAddr == 0) {
            return InterfaceInfoOpt{};
        }
    }
    if (jObj->hasObject("internal")) {
        bool bInternal = false;
        if (jObj->getBoolean("internal", bInternal) < 0) {
            return InterfaceInfoOpt{};
        }
        iio.bIsInternal = bInternal;
    }
    if (jObj->hasObject("tdm")) {
        String sTDM("");
        if (jObj->getString("tdm", sTDM) < 0) {
            return InterfaceInfoOpt{};
        }
        iio.sTDM = sTDM;
        if (iio.sTDM == "") {
            return InterfaceInfoOpt{};
        }
    }
    if (jObj->hasObject("pcap_file_path")) {
        String sPcapFilePath("");
        if (jObj->getString("pcap_file_path", sPcapFilePath) < 0) {
            return InterfaceInfoOpt{};
        }
        iio.sPcapFilePath = sPcapFilePath;
        if (iio.sPcapFilePath == "") {
            return InterfaceInfoOpt{};
        }
    }

    return iio;
}

NetSensor::NetSensor(const EmbeddingMode em) :
    _embeddingMode(em), _pMonitorThreadsMap(true, true, true, true)
{
    _bCleaningTimerElapsed  = false;
    _bSendingTimerElapsed   = false;
    _pRttPQ                 = new NetSensorPacketQueue();
    _pPQ                    = new NetSensorPacketQueue();
    _pNCM                   = new NetSensorConfigurationManager();
    _pTrafficTable          = new TrafficTable(5000);
    _pTopologyTable         = new TopologyTable();
    _pIcmpTable             = new ICMPInterfaceTable(5000);
    _pTopCache              = new TopologyCache();
    _pTcpRTable             = new TCPRTTInterfaceTable(5000);
    _pHThread = new HandlerThread(_pPQ, _pRttPQ, _pTrafficTable,
                                  _pTopologyTable, _pIcmpTable,
                                  _pTopCache, _pTcpRTable,
                                  _pNCM->storeExternalNodes,
                                  _pNCM->calculateTCPRTT);

    _netSensorState         = CHECK_STATUS;
    _cleaningTimer          = 500;
    _sendingTimer           = 500;
    lastSendTime            = getTimeInMilliseconds();

    printAbout();
}

NetSensor::~NetSensor(void)
{
    printf("NetSensor Destructor called\n");
    _pMonitorThreadsMap.removeAll();
    google::protobuf::ShutdownProtobufLibrary();

    delete _pNCM;
    delete _pRttPQ;
    delete _pPQ;
    delete _pTrafficTable;
    delete _pTopologyTable;
    delete _pIcmpTable;
    delete _pHThread;
    delete _pTopCache;
    delete _pTcpRTable;
}

int NetSensor::addMonitoringInterface(const char *pcInterfaceName)
{
    static const char *methodName = "NetSensor::addMonitoringInterface";
    String sInterface = pcInterfaceName;

    if (!_pNCM->pslMonitoredInterfacesList->search(sInterface)) {
        checkAndLogMsg(methodName, Logger::L_Info,
            "Adding %s to list of monitored interfaces\n", sInterface.c_str());
        _pNCM->pslMonitoredInterfacesList->add(sInterface);
    }
    else {
        checkAndLogMsg(methodName, Logger::L_Warning,
            "%s was already in the list\n", sInterface.c_str());
    }
    return 0;
}

int NetSensor::configureAndStartMonitoringInterfaces(NetSensorPacketQueue *pPQ,
                                                     NetSensorPacketQueue *pRttPQ)
{
    static const char *methodName = "configureAndStartMonitoringInterfaces";
    int rc = 0;

    if (_embeddingMode == EmbeddingMode::EM_REPLAY) {
        InterfaceInfoOpt iio;
        _lInterfaceInfoOpt.resetGet();
        while (_lInterfaceInfoOpt.getNext(iio)) {
            if ((rc = setMonitoringInterface(iio, pPQ, pRttPQ)) < 0) {
                checkAndLogMsg(methodName, Logger::L_SevereError,
                               "setMonitoringInterface failed, rc: %d\n", rc);
                return -1;
            }
        }

        return 0;
    }

    String sMonitoredInterface;
    _pNCM->pslMonitoredInterfacesList->resetGet();
    uint16 ui16InterfaceCount = 0;
    while (_pNCM->pslMonitoredInterfacesList->getNext(sMonitoredInterface))
    {
        MonitoringInterfaceOpt tmpOpt;
        tmpOpt.sMonitoredInterface = sMonitoredInterface;
        tmpOpt.pPQ = pPQ;
        tmpOpt.pRttPQ = pRttPQ;

        if (_embeddingMode == EmbeddingMode::EM_NETPROXY) {
            tmpOpt.bIsInternal = _lnpi.iiInternal.sInterfaceName == sMonitoredInterface;
            tmpOpt.em = _embeddingMode;
        }
        if ((rc = setMonitoringInterface(tmpOpt)) < 0) {
            checkAndLogMsg(methodName, Logger::L_MildError,
                           "setMonitoringInterface failed for interface %s; rc: %d\n",
                           sMonitoredInterface.c_str(), rc);
            _pNCM->pslMonitoredInterfacesList->remove(sMonitoredInterface);
            continue;
        }
        ++ui16InterfaceCount;
    }
    checkAndLogMsg(methodName, Logger::L_Info, "Successfully set up "
                   "Interface Monitors for %hu interfaces\n",
                   ui16InterfaceCount);

    return 0;
}

int NetSensor::configureNetSensor(void)
{
    static const char *methodName = "configureNetSensor";
    int rc = 0;
    if ((rc = configureAndStartMonitoringInterfaces(_pPQ, _pRttPQ)) < 0) {
        checkAndLogMsg(methodName, Logger::L_SevereError,
            "configureAndStartMonitoringInterfaces failed, rc: %d\n", rc);
        return -1;
    }
    configureAndStartHandlerThread();
    configureAndStartDeliveryInterfaces();
    return 0;
}

InterfaceMonitor * NetSensor::createNewMonitor(String sInterfaceName,
                                               NetSensorPacketQueue *pPQ,
                                               NetSensorPacketQueue *pRttPQ)
{
    static const char *methodName = "createNewMonitor";
    int rc = 0;

    InterfaceMonitor *_pIM = new InterfaceMonitor();
    if ((rc = _pIM->initLive(sInterfaceName)) < 0) {
        checkAndLogMsg(methodName, Logger::L_SevereError,
            "Monitoring interface init failed, rc: %d\n", rc);
        delete _pIM;
        _pIM = nullptr;
    }
    else {
        _pIM->pQueue = pPQ;
        _pIM->pRttQueue = pRttPQ;

        //keep track of the monitor
        _pMonitorThreadsMap.put(sInterfaceName, _pIM);
        _pIM->start();
    }
    return _pIM;
}

InterfaceMonitor * NetSensor::createNewMonitor(InterfaceInfoOpt iio,
                                               NetSensorPacketQueue *pPQ,
                                               NetSensorPacketQueue *pRttPQ)
{
    static const char *methodName = "createNewMonitor";

    int rc = 0;
    InterfaceMonitor *_pIM = new InterfaceMonitor();
    if ((rc = _pIM->initReplay(iio.pcIname, iio.sPcapFilePath, iio.ipAddr,
                               iio.netmask, iio.gwAddr, iio.emac)) < 0) {
        checkAndLogMsg(methodName, Logger::L_SevereError,
                       "Monitoring interface init failed, rc: %d\n", rc);
        delete _pIM;
        _pIM = nullptr;
    }
    else {
        _pIM->pQueue = pPQ;
        _pIM->pRttQueue = pRttPQ;

        //keep track of the monitor
        _pMonitorThreadsMap.put(iio.pcIname, _pIM);
        _pIM->start();
    }

    return _pIM;
}

void NetSensor::handleNetproxyInfo(const google::protobuf::Timestamp & ts)
{
    if (_pNCM->nproxyTopActive) {
        NetSensorContainer nsc;

        google::protobuf::Timestamp *timestamp = new google::protobuf::Timestamp();
        timestamp->CopyFrom(ts);
        nsc.set_allocated_timestamp(timestamp);

        nsc.set_datatype(NETPROXY);

        NetProxyInfo *npi = new NetProxyInfo();
        NetworkInfo *internalNetInfo = new NetworkInfo();
        NetworkInfo *externalNetInfo = new NetworkInfo();

        _pMonitorThreadsMap.get(_lnpi.iiInternal.sInterfaceName)
            ->fillNetworkInfo(_lnpi.iiInternal.sInterfaceName, internalNetInfo);

        _pMonitorThreadsMap.get(
            _lnpi.iiExternal.sInterfaceName)
            ->fillNetworkInfo(_lnpi.iiExternal.sInterfaceName, externalNetInfo);

        NOMADSUtil::String sAddr;
        _lnpi.remoteNetproxyAddressesList.resetGet();
        while (_lnpi.remoteNetproxyAddressesList.getNext(sAddr)) {
            uint32 networkOrderIp = convertIpToHostOrder(NOMADSUtil::InetAddr(sAddr).getIPAddress());
            npi->add_remotenetproxyips(networkOrderIp);
        }

        nsc.set_allocated_netproxyinfo(npi);
        npi->set_allocated_internal(internalNetInfo);
        npi->set_allocated_external(externalNetInfo);

        //pNSC->PrintDebugString();

        _pms.sendNetproxyInfo(&nsc);
    }
}

void NetSensor::handleTopology(const google::protobuf::Timestamp & ts)
{
    String sMonitoredInterface;
    _pNCM->pslMonitoredInterfacesList->resetGet();
    while (_pNCM->pslMonitoredInterfacesList->getNext(sMonitoredInterface))
    {
        int64 startTime = getTimeInMilliseconds();
        NetSensorContainer nsc;

        google::protobuf::Timestamp *newTs = new google::protobuf::Timestamp();
        newTs->CopyFrom(ts);
        nsc.set_allocated_timestamp(newTs);

        nsc.set_datatype(TOPOLOGY);
        Topology    *pT = nsc.add_topologies();
        NetworkInfo *pNI = new NetworkInfo();

        _pMonitorThreadsMap.get(sMonitoredInterface)->fillNetworkInfo(sMonitoredInterface, pNI);
        pT->set_allocated_networkinfo(pNI);

        _pTopologyTable->fillInternalTopologyProtoObject(sMonitoredInterface, pT);

        int nscSize = nsc.ByteSize();


        if (_pNCM->storeExternalNodes)
        {
            // Create external protobuf topology info
            Topology    *pExternTop     = nsc.add_topologies();
            _pMonitorThreadsMap.get(sMonitoredInterface)->
                fillNetworkInfo(sMonitoredInterface,
                    pExternTop->mutable_networkinfo());
            pExternTop->mutable_networkinfo()->
                set_networkname(C_EXTERNAL_TOPOLOGY_UNKNOWN);
            pExternTop->mutable_networkinfo()->
                set_networknetmask(C_EXTERNAL_TOPOLOGY_UNKNOWN);
            _pTopologyTable->
                fillExternalTopologyProtoObject(sMonitoredInterface, pExternTop);

            nscSize = nsc.ByteSize();
        }

        int64 endTime = getTimeInMilliseconds();
        if ((endTime - startTime) > C_D_TOPOLOGY_PROTO_TIME) {
            checkAndLogMsg("handleTopology", Logger::L_Warning,
                "handleTopology took %lldms\n", (endTime - startTime));
        }

        //nsc.PrintDebugString();
        _pms.sendTopology(&nsc);
    }
}

void NetSensor::handleTraffic(const google::protobuf::Timestamp & ts)
{
    String sMonitoredInterface;
    _pNCM->pslMonitoredInterfacesList->resetGet();
    while (_pNCM->pslMonitoredInterfacesList->getNext(sMonitoredInterface))
    {
        int64 startTime = getTimeInMilliseconds();
        NetSensorContainer nsc;

        google::protobuf::Timestamp *newTs = new google::protobuf::Timestamp();
        newTs->CopyFrom(ts);
        nsc.set_allocated_timestamp(newTs);

        nsc.set_datatype(TRAFFIC);
        String interfaceAddr = InetAddr(_pMonitorThreadsMap.get(sMonitoredInterface)->_ui32IPAddr).getIPAsString();
        TrafficByInterface *pT = nsc.add_trafficbyinterfaces();

        _pTrafficTable->fillTrafficProtoObject(sMonitoredInterface, interfaceAddr.c_str(), pT);
        int64 endTime = getTimeInMilliseconds();
        if ((endTime - startTime) > C_D_TRAFFIC_PROTO_TIME) {
            checkAndLogMsg("handleTraffic", Logger::L_Warning,
                "handleTraffic took %lldms\n", (endTime - startTime));
        }
        //nsc.PrintDebugString();
        _pms.sendTraffic(&nsc);
    }
}

void NetSensor::handleIcmp(const google::protobuf::Timestamp & ts)
{
    String sMonitoredInterface;

    _pNCM->pslMonitoredInterfacesList->resetGet();
    while (_pNCM->pslMonitoredInterfacesList->getNext(sMonitoredInterface))
    {
        int64 startTime = getTimeInMilliseconds();
        NetSensorContainer nsc;

        google::protobuf::Timestamp *newTs = new google::protobuf::Timestamp();
        newTs->CopyFrom(ts);
        nsc.set_allocated_timestamp(newTs);

        nsc.set_datatype(ICMP);
        ICMPPacketsByInterface *pIpbi = nsc.add_icmpinfo();
        pIpbi->set_monitoringinterface(sMonitoredInterface); // Not sure if this is necessary/set later
        _pIcmpTable->fillICMPProtoObject(sMonitoredInterface, pIpbi);
        int64 endTime = getTimeInMilliseconds();
        if ((endTime - startTime) > C_D_TRAFFIC_PROTO_TIME)
        {
            checkAndLogMsg("handleIcmp", Logger::L_Warning,
                "handleIcmp took %lldms\n", (endTime - startTime));
        }
        _pms.sendICMP(&nsc);
    }
}

void NetSensor::handleThreadsTermination(void)
{
    printf("Request Threads termination: \n");
    printf("    Handler Thread...\n");
    _pHThread->requestTerminationAndWait();
    for (NOMADSUtil::StringHashtable<InterfaceMonitor>::Iterator i = _pMonitorThreadsMap.getAllElements(); !i.end(); i.nextElement()) {
        printf("    Interface Monitor Thread for %s...\n", i.getKey());
        i.getValue()->requestTerminationAndWait();
    }
    printf("All Threads terminated\n");
}

void NetSensor::handleUserInput(const UserInput command)
{
    switch (command)
    {
    case CLOSE:
        _netSensorState = USER_REQUESTED_TERMINATION;
        break;

    case PRINT_TRAFFIC_TABLE_CONTENT:
        _pTrafficTable->printContent();
        printHelp();
        break;

    case PRINT_TOPOLOGY_TABLE_CONTENT:
        if (_pNCM->netmskTopActive || _pNCM->nproxyTopActive) {
            _pTopologyTable->printContent();
        }
        else{
            printf("NetSensor::TCP RTT calculation not enabled! Enable with '-trtt'\n");
        }
        printHelp();
        break;

    case PRINT_ICMP_TABLE_CONTENT:
        _pIcmpTable->printContent();
        printHelp();
        break;

    case PRINT_TCP_RTT_TABLE_CONTENT:
        _pTcpRTable->printContent();
        printHelp();
        break;

    case PRINT_HELP:
        printHelp();
        break;

    case LAUNCH_DIAGNOSTIC:
        printf("\n\nStarting diagnostic service:\n\n");
        _pHThread->startDiagnostic();
        //_pMonitorThreadsMap
        //_pms
        printHelp();
        break;

    case PRINT_ABOUT:
        printAbout();
        printHelp();
        break;

    case PRINT_CONFIG:
        printConfiguration();
        printHelp();
        break;
    }
}

void NetSensor::initWithDefaultInterface(void)
{
    _pNCM->init();
}

int NetSensor::init(int argc, char *argv[])
{
    CommandLineConfigs cLC;
    CommandLineParser cLParser(&cLC);
    cLParser.parseArgs(argc, const_cast<const char**> (argv));

    if (cLParser.hasHelp()) {
        cLParser.printParamMeanings();
        return -1;
    }
    else if (cLParser.hasError()) {
        cLParser.printError();
        return -2;
    }

    initWithValues(cLC.bUseCompression,
                   cLC.bUseExternalTopology,
                   cLC.bHasInterfaces,
                   cLC.bCalculateTCPRTT,
                   cLC.em, cLC.sConfigPath,
                   cLC.sReplayConfigPath,
                   cLC.interfaceNameList,
                   cLC.sRecipient);
    printf("\n");
    printConfig();

    return 0;
}

void NetSensor::initWithValues(const bool bCompression, const bool bExternalTopology,
                               const bool bHasInterfaces, const bool bCalculateTCPRTT,
                               const EmbeddingMode em, const NOMADSUtil::String sConfigPath,
                               const NOMADSUtil::String sReplayModeConfigPath,
                               NOMADSUtil::LList<NOMADSUtil::String> sInterfaceList,
                               const NOMADSUtil::String sRecipient)
{
    // Init NetSensor based on command line configs
    _embeddingMode = em;

    if (bCompression) {
        initWithCompression();
    }
    if (bExternalTopology) {
        initWithExternalTopology();
    }
    if (bCalculateTCPRTT) {
        initWithTCPRTTCalculation();
    }

    if (_embeddingMode == EmbeddingMode::EM_REPLAY) {
        initReplayModeWithConfigFile(sReplayModeConfigPath);
    }
    else {
        if (!bHasInterfaces) {
            InterfaceDetector id;
            id.init();
            char *pNextInterface = nullptr;
            while (id.getNext(&pNextInterface)) {
                initWithInterface(pNextInterface);
            }
        }
        else {
            String firstInterface;
            sInterfaceList.resetGet();
            while (sInterfaceList.getNext(firstInterface)) {
                initWithInterface(firstInterface);
            }
        }
        initWithRecipient(sRecipient);
    }

    initWithCfgFile(sConfigPath);
}

void NetSensor::initReplayModeWithConfigFile(const NOMADSUtil::String & sReplayModeConfigPath)
{
    int rc = 0;
    String sInterfaceName;
    ReplayModeConfigFileProcessor rMCFG(this, sReplayModeConfigPath);

    auto lInterfaceInfo = rMCFG.getInterfaceInfoList();
    if ((lInterfaceInfo == nullptr) || (lInterfaceInfo->getCount() == 0)) {
        checkAndLogMsg("initReplayModeWithConfigFile", Logger::L_Warning,
                       "error detected while trying to retrieve interface info"
                       "from the REPLAY MODE config file; rc = %d\n", rc);
    }
    NetSensor::InterfaceInfoOpt iio;
    lInterfaceInfo->resetGet();
    while (lInterfaceInfo->getNext(iio)) {
        if (setInterfaceInfo(iio) != 0) {
            checkAndLogMsg("initReplayModeWithConfigFile", Logger::L_Warning,
                           "error detected while trying to add new "
                           "interface information; rc = %d\n", rc);
        }
    }

    auto lRemoteNP = rMCFG.getRemoteNpList();
    if ((lRemoteNP == nullptr) || (lRemoteNP->getCount() == 0)) {
        checkAndLogMsg("initReplayModeWithConfigFile", Logger::L_Warning,
                       "error detected while trying to retrieve the list of remote "
                       "NPes from the REPLAY MODE config file; rc = %d\n", rc);
    }
    setRemoteNpList(*lRemoteNP);

    _tdm = getTDMFromString(rMCFG.getTDM());
    initReplayMode (rMCFG.getStatsRecipientIP(), rMCFG.getTDM());

    auto lInterfaceNames = rMCFG.getInterfaceNames();
    lInterfaceNames->resetGet();
    while (lInterfaceNames->getNext(sInterfaceName)) {
        addMonitoringInterface(sInterfaceName);
    }
}

int NetSensor::initAsComponent(const NOMADSUtil::String & statDestinationIP)
{
    static const char *methodName = "NetSensor::initAsComponent";
    switch (_embeddingMode)
    {
    case EmbeddingMode::EM_NETPROXY:
        checkAndLogMsg(methodName, Logger::L_Info, "Init as Netproxy Component\n");
        initNetproxyMode(statDestinationIP);
        break;
    default:
        // init with default parameters
        initWithDefaultInterface();
        break;
    }
    return 0;
}

int NetSensor::initNetproxyMode(const NOMADSUtil::String & statRecipientIP)
{
    int rc = 0;

    _pNCM->setNPModeDefaultConfigurations(statRecipientIP);
    _pNCM->printConfigurationReport();
    printNetworkInfoContent();

    if ((rc = configureNetSensor()) < 0) {
        return -1;
    }

    return 0;
}

void NetSensor::netSensorStateMachine(void)
{
    static const char* methodName = "NetSensor::netSensorStateMachine";
    bool bprintTraffic = false;
    bool bprintTopology = false;

    switch (_netSensorState)
    {
    case UPDATE_TIMERS:
        updateNetSensorTimers();
        break;

    case CHECK_STATUS:
        break;

    case CLEAN_TABLES:
        _pTrafficTable->cleanTable(C_MAX_CLEANING_NUMBER);
        _pTopologyTable->cleanTable(C_MAX_CLEANING_NUMBER);
        _pIcmpTable->cleanTable(C_MAX_CLEANING_NUMBER);
        _pTopCache->cleanTables(C_MAX_CLEANING_NUMBER);
        _pTcpRTable->cleanTable(C_MAX_CLEANING_NUMBER);
        break;

    case SEND_STATS:
        //
        sendStats();
        break;

    case PRINT_TIME:
        if (bprintTraffic) {
            _pTrafficTable->printContent();
        }
        if (bprintTopology) {
            _pTopologyTable->printContent();
        }

        break;

    case REST:
        sleepForMilliseconds(C_SLEEP_TIME);
        break;

    case CHECK_FOR_USER_INPUT:
        UserInput command;
        if ((_pUserInputMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
            while (!_userInputQueue.isEmpty()) {
                _userInputQueue.dequeue(command);
                handleUserInput(command);
            }
            _pUserInputMutex.unlock();
        }
        break;

    case TERMINATE:
        checkAndLogMsg(methodName, Logger::L_Info, "Termination requested\n");
        requestTermination();
        break;
    }
    nextStatus();
}

void NetSensor::nextStatus(void)
{
    switch (_netSensorState)
    {
    case UPDATE_TIMERS:
        _netSensorNextState = CHECK_STATUS;
        break;

    case CHECK_STATUS:
        if (_bCleaningTimerElapsed) {
			_netSensorNextState = CLEAN_TABLES;
		}
        else if (_bSendingTimerElapsed) {
			_netSensorNextState = SEND_STATS;
		}
        else {
			_netSensorNextState = REST;
		}
        break;

    case CLEAN_TABLES:
        _netSensorNextState     = CHECK_STATUS;
        _bCleaningTimerElapsed  = false;
        break;

    case REST:
        _netSensorNextState     = CHECK_FOR_USER_INPUT;
        break;

    case SEND_STATS:
        _netSensorNextState     = CHECK_STATUS;
        _bSendingTimerElapsed   = false;
        break;

     case USER_REQUESTED_TERMINATION:
         _netSensorNextState = TERMINATE;
         break;

     case CHECK_FOR_USER_INPUT:
         _netSensorNextState = UPDATE_TIMERS;
         break;
    }


    _netSensorState = _netSensorNextState;
}

void NetSensor::passUserInput(const UserInput command)
{
    if ((_pUserInputMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
        _userInputQueue.enqueue(command);
        _pUserInputMutex.unlock();
    }
}

int NetSensor::printMac(EtherMACAddr mac)
{
    const char* meName = "printMac";
    checkAndLogMsg(meName, Logger::L_Info, "\tMac: %2x:%2x:%2x:%2x:%2x:%2x\n",
        mac.ui8Byte1, mac.ui8Byte2, mac.ui8Byte3,
        mac.ui8Byte4, mac.ui8Byte5, mac.ui8Byte6);
    return 0;
}

int NetSensor::printNetworkInfoContent()
{
    const char* meName = "NetSensor::printNetworkInfoContent()";

    checkAndLogMsg(meName, Logger::L_Info, "Remote Proxy List Content:\n");
    _lnpi.remoteNetproxyAddressesList.resetGet();
    String tmpAddr;
    while (_lnpi.remoteNetproxyAddressesList.getNext(tmpAddr)) {
        checkAndLogMsg(meName, Logger::L_Info, "\t%s\n", tmpAddr.c_str());
    }

    checkAndLogMsg(meName, Logger::L_Info, "Internal Interface Info:\n");
    checkAndLogMsg(meName, Logger::L_Info, "\tIs internal set to: %d\n", _lnpi.iiInternal.bIIface);
    checkAndLogMsg(meName, Logger::L_Info, "\tName set to: %s\n", _lnpi.iiInternal.sInterfaceName.c_str());
    checkAndLogMsg(meName, Logger::L_Info, "\tIP set to: %s\n", InetAddr(_lnpi.iiInternal.ui32IpAddr).getIPAsString());
    checkAndLogMsg(meName, Logger::L_Info, "\tNetmask set to: %s\n", InetAddr(_lnpi.iiInternal.ui32Netmask).getIPAsString());
    checkAndLogMsg(meName, Logger::L_Info, "\tTDM set to: %d\n", _lnpi.iiInternal.tdm);
    printMac(_lnpi.iiInternal.emacInterfaceMAC);

    checkAndLogMsg(meName, Logger::L_Info, "External Interface Info:\n");
    checkAndLogMsg(meName, Logger::L_Info, "\tIs Internal set to: %d\n", _lnpi.iiExternal.bIIface);
    checkAndLogMsg(meName, Logger::L_Info, "\tName set to: %s\n", _lnpi.iiExternal.sInterfaceName.c_str());
    checkAndLogMsg(meName, Logger::L_Info, "\tIp set to: %s\n", InetAddr(_lnpi.iiExternal.ui32IpAddr).getIPAsString());
    checkAndLogMsg(meName, Logger::L_Info, "\tNetmask set to: %s\n", InetAddr(_lnpi.iiExternal.ui32Netmask).getIPAsString());
    checkAndLogMsg(meName, Logger::L_Info, "\tTDM set to: %d\n", _lnpi.iiExternal.tdm);
    printMac(_lnpi.iiExternal.emacInterfaceMAC);
    return 0;
}

void NetSensor::printAbout(void)
{
    printf("     _   _      _   _____  \n");
    printf("    | \\ | |    | | /  ___| \n");
    printf("    |  \\| | ___| |_\\ `--.  ___ _ __  ___  ___  _ __ \n");
    printf("    | . ` |/ _ \\ __|`--. \\/ _ \\ '_ \\/ __|/ _ \\| '__|\n");
    printf("    | |\\  |  __/ |_/\\__/ /  __/ | | \\__ \\ (_) | |   \n");
    printf("    \\_| \\_/\\___|\\__\\____/ \\___|_| |_|___/\\___/|_|   \n");

    printf("* Contact: rfronteddu@ihmc.us - NOMADS\n");
    printf("* This file is part of the IHMC NetSensor Library/Component\n");
    printf("* Copyright (c) 2010-2017 IHMC.\n");
    printf("* This program is free software; you can redistribute it and/or\n");
    printf("* modify it under the terms of the GNU General Public License\n");
    printf("* version 3 (GPLv3) as published by the Free Software Foundation.\n");
    printf("* U.S. Government agencies and organizations may redistribute\n");
    printf("* and/or modify this program under terms equivalent to\n");
    printf("* Government Purpose Rights as defined by DFARS\n");
    printf("* 252.227-7014(a)(12) (February 2014).\n");
    printf("* Alternative licenses that allow for use within commercial products may be\n");
    printf("* available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.\n\n\n");
}

void NetSensor::printConfiguration(void)
{
    //I will have to add a mutex for the cfg object..
    _pNCM->printConfigurationReport();
}

void NetSensor::printHelp(void)
{
    printf("\n ________________________________________________________________________\n");
    printf("|             NetSensor Help Menu - Type command ID and press enter       | \n");
    printf("|_________________________________________________________________________|\n");
    printf(".  Commands:     ID                 DESCRIPTION:                          .\n");
    printf("|            1. close        <-- Terminate Netsensor                      |\n");
    printf(".            2. cfg          <-- Print configurations                     .\n");
    printf("|            3. printTraffic <-- Print traffic table content              |\n");
    printf(".            4. printTopology<-- Print topology table content             .\n");
    printf("|            5. printICMPInfo<-- Print ICMP table content                 |\n");
    printf(".            6. printTCPRTT  <-- Print TCP RTT Info                       .\n");
    printf("|            7. diag         <-- Launch diagnostic                        |\n");
    printf(".            8. help         <-- Print help menu                          .\n");
    printf("|            9. about        <-- Software Info                            |\n");
    printf("  ________________________________________________________________________\n");
}

void NetSensor::run(void)
{
    const char* methodName = "NetSensor::run";
	started();
	checkAndLogMsg(methodName, Logger::L_Info, "NetSensor started\n");
	while (!terminationRequested()) {
		netSensorStateMachine();
    }
    handleThreadsTermination();
    terminating();
}

void NetSensor::sendStats(void)
{
    const char *methodName = "sendStats";
    checkAndLogMsg(methodName, Logger::L_HighDetailDebug,
        "Last stats delivery: %d\n", getTimeInMilliseconds() - lastSendTime);
    lastSendTime = getTimeInMilliseconds();

    google::protobuf::Timestamp ts;
    setProtobufTimestamp(ts);

    handleTopology      (ts);
    handleTraffic       (ts);
    handleNetproxyInfo  (ts);
    handleIcmp          (ts);
}

int NetSensor::setInterfaceInfo(const InterfaceInfoOpt & IIOpt)
{
    if ((_embeddingMode == EmbeddingMode::EM_NETPROXY) ||
        ((_embeddingMode == EmbeddingMode::EM_REPLAY) &&
        (getTDMFromString(IIOpt.sTDM) == TopologyDetectionMechanism::TDM_NETPROXY))) {
        InterfaceInfo *pii = (IIOpt.bIsInternal) ?
            &_lnpi.iiInternal : &_lnpi.iiExternal;

        pii->bIIface            = IIOpt.bIsInternal;
        pii->emacInterfaceMAC   = IIOpt.emac;
        pii->sInterfaceName     = IIOpt.pcIname;
        pii->ui32IpAddr         = IIOpt.ipAddr;
        pii->ui32Netmask        = IIOpt.netmask;
        pii->tdm                = getTDMFromString(IIOpt.sTDM);
    }

    _lInterfaceInfoOpt.add(IIOpt);

    return 0;
}

int NetSensor::setMonitoringInterface(MonitoringInterfaceOpt opt)
{
    static const char *methodName = "setMonitoringInterface";
    InterfaceMonitor *pIM = createNewMonitor(opt.sMonitoredInterface, opt.pPQ, opt.pRttPQ);
    if (pIM == nullptr) {
        checkAndLogMsg(methodName, Logger::L_SevereError,
                       "Creation of Monitoring interface (%s) failed\n",
                       opt.sMonitoredInterface.c_str());
        return -1;
    }

    InterfaceInfo *pIICopy = pIM->getInterfaceInfoCopy();
    if (_embeddingMode == EmbeddingMode::EM_NETPROXY) {
        pIICopy->bIIface = opt.bIsInternal;
        pIICopy->tdm = opt.bIsInternal ? _lnpi.iiInternal.tdm : _lnpi.iiExternal.tdm;
    }
    else {
        checkAndLogMsg(methodName, Logger::L_Warning,
            "Embedding Mode not selected\n");
    }
    _pHThread->addInterfaceInfo(pIICopy);
    pIICopy = nullptr;

    return 0;
}

int NetSensor::setMonitoringInterface(InterfaceInfoOpt iio, NetSensorPacketQueue *pPQ,
                                      NetSensorPacketQueue *pRttPQ)
{
    static const char *methodName = "setMonitoringInterface";
    InterfaceMonitor *pIM = createNewMonitor(iio, pPQ, pRttPQ);
    if (pIM == nullptr) {
        checkAndLogMsg(methodName, Logger::L_SevereError,
                       "Creation of Monitoring interface (%s) failed\n",
                       iio.pcIname.c_str());
        return -1;
    }

    InterfaceInfo *pIICopy = pIM->getInterfaceInfoCopy();
    if (_tdm == TopologyDetectionMechanism::TDM_NETPROXY) {
        assert (_embeddingMode == EmbeddingMode::EM_REPLAY);
        pIICopy->bIIface = iio.bIsInternal;
        pIICopy->tdm = getTDMFromString(iio.sTDM);
    }
    else {
        checkAndLogMsg(methodName, Logger::L_Warning,
                       "Embedding Mode not selected\n");
    }
    _pHThread->addInterfaceInfo(pIICopy);
    pIICopy = nullptr;

    return 0;
}

int NetSensor::setRemoteNpList(LList<uint32> & remoteNpList)
{
    static const char *methodName = "NetSensor::setRemoteNpList";

    uint32 tmpAddr;
    remoteNpList.resetGet();
    for (int index = 0; index < remoteNpList.getCount(); index++) {
        remoteNpList.getNext(tmpAddr);
        checkAndLogMsg(methodName, Logger::L_Info,
            "Adding %s to list of remote netproxies\n",
            InetAddr(tmpAddr).getIPAsString());

        _lnpi.remoteNetproxyAddressesList.add(InetAddr(tmpAddr).getIPAsString());
    }

    return 0;
}

int NetSensor::setRemoteNpList(LList<uint32> && remoteNpList)
{
    static const char *methodName = "NetSensor::setRemoteNpList";

    uint32 tmpAddr;
    remoteNpList.resetGet();
    for (int index = 0; index < remoteNpList.getCount(); index++) {
        remoteNpList.getNext(tmpAddr);
        checkAndLogMsg(methodName, Logger::L_Info,
                       "Adding %s to list of remote netproxies\n",
                       InetAddr(tmpAddr).getIPAsString());

        _lnpi.remoteNetproxyAddressesList.add(InetAddr(tmpAddr).getIPAsString());
    }

    return 0;
}

void NetSensor::updateNetSensorTimers(void)
{
    if (!_bCleaningTimerElapsed) {
        if (_cleaningTimer <= 0) {
            _cleaningTimer = C_CLEANING_TIME;
            _bCleaningTimerElapsed = true;
        }
        else {
            _cleaningTimer = _cleaningTimer - C_SLEEP_TIME;
        }
    }

    if (!_bSendingTimerElapsed) {
        if (_sendingTimer <= 0) {
            _sendingTimer = C_SENDING_TIME;
            _bSendingTimerElapsed = true;
        }
        else {
            _sendingTimer = _sendingTimer - C_SLEEP_TIME;
        }
    }
}
}
