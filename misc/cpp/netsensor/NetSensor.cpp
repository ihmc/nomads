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
#include <list>

using namespace NOMADSUtil;
using namespace netsensor;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_NETSENSOR
{
    NetSensor::NetSensor (const Mode mode) :
        _mode (mode),
        _pMonitorThreadsMap (true, true, true, true)
    {
        _bCleaningTimerElapsed  = false;
        _bSendingTimerElapsed   = false;
        _pRttPQ                 = new NetSensorPacketQueue();
        _pPQ                    = new NetSensorPacketQueue();
        _pNCM                   = new NetSensorConfigurationManager();
        _pTrafficTable			= new TrafficTable (1000);
        _pTopologyTable			= new TopologyTable();
        _pIcmpTable				= new ICMPInterfaceTable (5000);
        _pTopCache				= new TopologyCache();
        _pTcpRTable				= new TCPRTTInterfaceTable (5000);
        _pHThread				= new HandlerThread ();
        _pIWDumpManager			= new IWDumpManager();

        HTCfg htc;
        htc.pQueue          = _pPQ;
        htc.pRttQueue       = _pRttPQ;
        htc.pTrT            = _pTrafficTable;
        htc.pTopT           = _pTopologyTable;
        htc.pIcmpTable      = _pIcmpTable;
        htc.pTRIT           = _pTcpRTable;
        htc.pTopCache       = _pTopCache;
        htc.storeExternals  = _pNCM->storeExternalNodes;
        htc.calculateTCPRTT = _pNCM->calculateTCPRTT;

        _pHThread->init (&htc);

        _netSensorState				= NetSensorState::CHECK_STATUS;
        _cleaningTimer				= 500;
        _sendingTimer				= 500;
        _ui32msStatsDeliveryTime	= 900;
        _lastSendTime				= getTimeInMilliseconds();

        printAbout();
    }

    NetSensor::~NetSensor (void)
    {
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
        delete _pIWDumpManager;
    }

    void NetSensor::run (void)
    {
        auto pszMethodName = "NetSensor::run";
        started();
        
        checkAndLogMsg (pszMethodName, Logger::L_Info, "NetSensor started\n");
        while (!terminationRequested()) { 
			netSensorStateMachine(); 
		}
        handleThreadsTermination();
        terminating();
    }

    int NetSensor::addMonitoringInterface (const char* pszInterfaceName)
    {
        auto pszMethodName = "NetSensor::addMonitoringInterface";
        String sInterface = pszInterfaceName;
        if (!_pNCM->pslMonitoredInterfacesList->search (sInterface)) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, 
				"Adding %s to list of monitored interfaces\n", sInterface.c_str());
            _pNCM->pslMonitoredInterfacesList->add (sInterface);
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "%s was already in the list\n", sInterface.c_str());
        }
        return 0;
    }

    int NetSensor::configureAndStartMonitoringInterfaces (
        NetSensorPacketQueue * pPQ,
        NetSensorPacketQueue * pRttPQ)
    {
        auto pszMethodName = "NetSensor::configureAndStartMonitoringInterfaces";
        int rc = 0;

        if (_mode == Mode::EM_REPLAY) {
            InterfaceInfoOpt iio;
            _lInterfaceInfoOpt.resetGet();
            while (_lInterfaceInfoOpt.getNext (iio)) {
                if ((rc = setMonitoringInterface (iio, pPQ, pRttPQ)) < 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "setMonitoringInterface failed, rc: %d\n", rc);
                    return -1;
                }
            }
            return 0;
        }

        String sMonitoredInterface;
        _pNCM->pslMonitoredInterfacesList->resetGet();
        uint16 ui16InterfaceCount = 0;
        while (_pNCM->pslMonitoredInterfacesList->getNext (sMonitoredInterface)) {
			InterfaceOptions tmpOpt;
            if (_pNCM->useForcedInterfaces) {
                LList<String> tokens;
                tokenizeStr (sMonitoredInterface, ':', &tokens);
                if (tokens.getCount() < 3) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError, 
						"forced token %s is not in the form <interfaceName:InterfaceAddr:InterfaceNetmask> \n", 
                        sMonitoredInterface.c_str());     
                    exit(-1);
                }

                String interfaceName, forcedAddr, forcedNetmask;
                tokens.getNext (interfaceName);
                tokens.getNext (forcedAddr);
                tokens.getNext (forcedNetmask);

				
                tmpOpt.sMonitoredInterface      = interfaceName;
                tmpOpt.ui32ForcedInterfaceAddr  = InetAddr (forcedAddr).getIPAddress();
                tmpOpt.ui32ForcedNetmask        = InetAddr (forcedNetmask).getIPAddress();
                _pNCM->pslMonitoredInterfacesList->replace (sMonitoredInterface, interfaceName);
            }
            else {
                tmpOpt.sMonitoredInterface      = sMonitoredInterface;
                tmpOpt.ui32ForcedInterfaceAddr  = _pNCM->ui32ForcedInterfaceAddr;
                tmpOpt.ui32ForcedNetmask        = _pNCM->ui32ForcedNetmask;
            }

            tmpOpt.pPQ      = pPQ;
            tmpOpt.pRttPQ   = pRttPQ;

            if (_mode == Mode::EM_NETPROXY) {
                tmpOpt.bIsInternal = _lnpi.iiInternal.sInterfaceName == sMonitoredInterface;
                tmpOpt.mode = _mode;
            }

            if ((rc = setMonitoringInterface (tmpOpt)) < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, 
                    "setMonitoringInterface failed for interface %s; rc: %d\n", 
					sMonitoredInterface.c_str(), rc);
                _pNCM->pslMonitoredInterfacesList->remove (sMonitoredInterface);
                continue;
            }
            ++ui16InterfaceCount;
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info,
            "Successfully set up Interface Monitors for %hu interfaces\n", ui16InterfaceCount);
        return 0;
    }

    int NetSensor::configureNetSensor (void)
    {
        auto pszMethodName = "NetSensor::configureNetSensor";
        int rc = 0;
        if ((rc = configureAndStartMonitoringInterfaces (_pPQ, _pRttPQ)) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                "configureAndStartMonitoringInterfaces failed, rc: %d\n", rc);
            return -1;
        }
        configureAndStartHandlerThread();
        configureAndStartDeliveryInterfaces();
        return 0;
    }

    InterfaceMonitor* NetSensor::createNewMonitor (
        String sInterfaceName,
        NetSensorPacketQueue * pPQ,
        NetSensorPacketQueue * pRttPQ,
        uint32 ui32ForcedInterfaceAddr,
        uint32 ui32ForcedNetmask)
    {
        auto pszMethodName = "NetSensor::createNewMonitor";
        int rc = 0;

        auto pInterfaceMonitor = new InterfaceMonitor();
        if ((rc = pInterfaceMonitor->initLive (
            sInterfaceName, 
            false, 
            false, 
            ui32ForcedInterfaceAddr,
            ui32ForcedNetmask)) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Monitoring interface init failed, rc: %d\n", rc);
            delete pInterfaceMonitor;
            pInterfaceMonitor = nullptr;
        }
        else {
            pInterfaceMonitor->pQueue = pPQ;
            pInterfaceMonitor->pRttQueue = pRttPQ;
            //keep track of the monitor
            _pMonitorThreadsMap.put (sInterfaceName, pInterfaceMonitor);
            pInterfaceMonitor->start();
        }
        return pInterfaceMonitor;
    }

    InterfaceMonitor * NetSensor::createNewMonitor (
        InterfaceInfoOpt iio,
        NetSensorPacketQueue * pPQ,
        NetSensorPacketQueue * pRttPQ)
    {
        auto pszMethodName = "NetSensor::createNewMonitor";

        int rc = 0;
        auto pInterfaceMonitor = new InterfaceMonitor();
        if ((rc = pInterfaceMonitor->initReplay(
            iio.pcIname,
            iio.sPcapFilePath,
            iio.ipAddr,
            iio.netmask,
            iio.gwAddr,
            iio.emac)) < 0) {
            checkAndLogMsg(pszMethodName, Logger::L_SevereError,
                "Monitoring interface init failed, rc: %d\n", rc);
            delete pInterfaceMonitor;
        }
        else {
            pInterfaceMonitor->pQueue = pPQ;
            pInterfaceMonitor->pRttQueue = pRttPQ;

            //keep track of the monitor
            _pMonitorThreadsMap.put(iio.pcIname, pInterfaceMonitor);
            pInterfaceMonitor->start();
        }
        return pInterfaceMonitor;
    }

    const NetSensor::InterfaceInfoOpt NetSensor::InterfaceInfoOpt::fromJSON (const JsonObject * const jObj)
    {
        InterfaceInfoOpt iio{};
        if (jObj == nullptr) { return InterfaceInfoOpt{}; }

        if (jObj->hasObject("name")) {
            if (jObj->getString("name", iio.pcIname) < 0) { return InterfaceInfoOpt{}; }
            if (iio.pcIname == "") { return InterfaceInfoOpt{}; }
        }
        if (jObj->hasObject("emac")) {
            String sEMAC("");
            if (jObj->getString("emac", sEMAC) < 0) { return InterfaceInfoOpt{}; }
            iio.emac = NetUtils::getEtherMACAddrFromString(sEMAC);
            if (iio.emac == EtherMACAddr{ 0 }) { return InterfaceInfoOpt{}; }
        }
        if (jObj->hasObject("ip")) {
            String sIP("");
            if (jObj->getString("ip", sIP) < 0) {
                return InterfaceInfoOpt{};
            }
            iio.ipAddr = InetAddr(sIP).getIPAddress();
            if (iio.ipAddr == 0) { return InterfaceInfoOpt{}; }
        }
        if (jObj->hasObject("netmask")) {
            String sNetMask("");
            if (jObj->getString("netmask", sNetMask) < 0) { return InterfaceInfoOpt{}; }
            iio.netmask = InetAddr(sNetMask).getIPAddress();
            if (iio.netmask == 0) { return InterfaceInfoOpt{}; }
        }
        if (jObj->hasObject("gw")) {
            String sGW("");
            if (jObj->getString("gw", sGW) < 0) {
                return InterfaceInfoOpt{};
            }
            iio.gwAddr = InetAddr(sGW).getIPAddress();
            if (iio.gwAddr == 0) { return InterfaceInfoOpt{}; }
        }
        if (jObj->hasObject("internal")) {
            bool bInternal = false;
            if (jObj->getBoolean("internal", bInternal) < 0) { return InterfaceInfoOpt{}; }
            iio.bIsInternal = bInternal;
        }
        if (jObj->hasObject("tdm")) {
            String sTDM("");
            if (jObj->getString("tdm", sTDM) < 0) { return InterfaceInfoOpt{}; }
            iio.sTDM = sTDM;
            if (iio.sTDM == "") { return InterfaceInfoOpt{}; }
        }
        if (jObj->hasObject("pcap_file_path")) {
            String sPcapFilePath("");
            if (jObj->getString("pcap_file_path", sPcapFilePath) < 0) { return InterfaceInfoOpt{}; }
            iio.sPcapFilePath = sPcapFilePath;
            if (iio.sPcapFilePath == "") { return InterfaceInfoOpt{}; }
        }
        return iio;
    }
    
    UserInput NetSensor::getCommandFromUserInput (String sUserInput) 
    {
        auto pszMethodName = "NetSensor::getCommandFromUserInput";
        if ((sUserInput == "1") || (sUserInput == "close")) {
            return UserInput::CLOSE;
        }
        else if ((sUserInput == "2") || (sUserInput == "cfg")) {
            return UserInput::PRINT_CONFIG;
        }
        else if ((sUserInput == "3") || (sUserInput == "printTraffic")) {
            return UserInput::PRINT_TRAFFIC_TABLE_CONTENT;
        }
        else  if ((sUserInput == "4") || (sUserInput == "printTopology")) {
            return UserInput::PRINT_TOPOLOGY_TABLE_CONTENT;
        }
        else if ((sUserInput == "5") || (sUserInput == "printICMPInfo")) {
            return UserInput::PRINT_ICMP_TABLE_CONTENT;
        }
        else if ((sUserInput == "6") || (sUserInput == "printTCPRTT")) {
            return UserInput::PRINT_TCP_RTT_TABLE_CONTENT;
        }
        else if ((sUserInput == "7") || (sUserInput == "printICMPRTT")) {
            return UserInput::PRINT_ICMP_RTT_TABLE_CONTENT;
        }
        else if ((sUserInput == "8") || (sUserInput == "diag")) {
            return UserInput::LAUNCH_DIAGNOSTIC;
        }
        else if ((sUserInput == "9") || (sUserInput == "help")) {
            return UserInput::PRINT_HELP;
        }
        else if ((sUserInput == "10") || (sUserInput == "printIWDumps")) {
            return UserInput::PRINT_IW_DUMPS;
        }
        else if ((sUserInput == "0") || (sUserInput == "about")) {
            return UserInput::PRINT_ABOUT;
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "Command unrecognized\n");
            return UserInput::PRINT_HELP;
        }
    }

    void NetSensor::handleNetproxyInfo (const google::protobuf::Timestamp& ts)
    {
		if (!_pNCM->nproxyTopActive) {
			return;
		}
        NetSensorContainer nsc;
		nsc.set_allocated_timestamp (new google::protobuf::Timestamp(ts));
		nsc.set_datatype (NETPROXY);
		auto pNetproxyInfo	  = new NetProxyInfo();
		auto pInternalNetInfo = new NetworkInfo();
        _pMonitorThreadsMap.get (_lnpi.iiInternal.sInterfaceName)
			->fillNetworkInfo (
				_lnpi.iiInternal.sInterfaceName, 
				pInternalNetInfo);
        nsc.set_allocated_netproxyinfo (pNetproxyInfo);
        pNetproxyInfo->set_allocated_internal (pInternalNetInfo);
        //pNSC->PrintDebugString();
        _pms.sendNetproxyInfo (&nsc);
    }

    void NetSensor::handleTopology (const google::protobuf::Timestamp & ts)
    {
        auto pszMethodName = "NetSensor::handleTopology";
        String sMonitoredInterface;
        _pNCM->pslMonitoredInterfacesList->resetGet();
        while (_pNCM->pslMonitoredInterfacesList->getNext (sMonitoredInterface)) {
            int64 startTime = getTimeInMilliseconds();
            NetSensorContainer nsc;

            auto pNewTimestamp = new google::protobuf::Timestamp();
            pNewTimestamp->CopyFrom(ts);
            nsc.set_allocated_timestamp(pNewTimestamp);

            nsc.set_datatype(TOPOLOGY);
            auto pTopology = nsc.add_topologies();
            auto pNetworkInfo = new NetworkInfo();

            _pMonitorThreadsMap.get (sMonitoredInterface)->fillNetworkInfo (sMonitoredInterface, pNetworkInfo);
            pTopology->set_allocated_networkinfo (pNetworkInfo);
            _pTopologyTable->fillInternalTopologyProtoObject(sMonitoredInterface, pTopology);
            int nscSize = nsc.ByteSize();
            if (_pNCM->storeExternalNodes) {
                // Create external protobuf topology info
                auto pExternTop = nsc.add_topologies();
                _pMonitorThreadsMap.get(sMonitoredInterface)->fillNetworkInfo(
                    sMonitoredInterface,
                    pExternTop->mutable_networkinfo());
                pExternTop->mutable_networkinfo()->set_networkname (C_EXTERNAL_TOPOLOGY_UNKNOWN);
                pExternTop->mutable_networkinfo()->set_networknetmask (C_EXTERNAL_TOPOLOGY_UNKNOWN);
                _pTopologyTable->fillExternalTopologyProtoObject (sMonitoredInterface, pExternTop);
                nscSize = nsc.ByteSize();
            }

            int64 endTime = getTimeInMilliseconds();
            if ((endTime - startTime) > C_D_TOPOLOGY_PROTO_TIME) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "handleTopology took %lldms\n", (endTime - startTime));
            }
            //nsc.PrintDebugString();
            _pms.sendTopology(&nsc);
        }
    }

    void NetSensor::handleTraffic (const google::protobuf::Timestamp & ts)
    {
        auto pszMethodName = "NetSensor::handleTraffic";
        String sMonitoredInterface;
        _pNCM->pslMonitoredInterfacesList->resetGet();
        while (_pNCM->pslMonitoredInterfacesList->getNext (sMonitoredInterface))
        {
            int64 startTime = getTimeInMilliseconds();
            NetSensorContainer nsc;

            auto pNewTimestamp = new google::protobuf::Timestamp();
            pNewTimestamp->CopyFrom(ts);
            nsc.set_allocated_timestamp (pNewTimestamp);

            nsc.set_datatype (TRAFFIC);
            uint32 ui32Address = _pMonitorThreadsMap.get(sMonitoredInterface)->_ui32IPAddr;
            String interfaceAddr = InetAddr(ui32Address).getIPAsString();

            auto pTrafficByInterface = nsc.add_trafficbyinterfaces();

            _pTrafficTable->fillTrafficProtoObject (
                sMonitoredInterface,
                interfaceAddr.c_str(),
                pTrafficByInterface);

            int64 endTime = getTimeInMilliseconds();
            if ((endTime - startTime) > C_D_TRAFFIC_PROTO_TIME) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "handleTraffic took %lldms\n", (endTime - startTime));
            }
            //nsc.PrintDebugString();
            _pms.sendTraffic (&nsc);
        }
    }

    void NetSensor::handleIcmp (const google::protobuf::Timestamp & ts)
    {
        auto pszMethodName = "NetSensor::handleIcmp";
        String sMonitoredInterface;

        _pNCM->pslMonitoredInterfacesList->resetGet();
        while (_pNCM->pslMonitoredInterfacesList->getNext(sMonitoredInterface))
        {
            int64 startTime = getTimeInMilliseconds();
            NetSensorContainer nsc;

            auto pNewTimestamp = new google::protobuf::Timestamp();
            pNewTimestamp->CopyFrom(ts);
            nsc.set_allocated_timestamp(pNewTimestamp);

            nsc.set_datatype(ICMP);
            auto pIcmpPacketsByInterface = nsc.add_icmpinfo();
            pIcmpPacketsByInterface->set_monitoringinterface
            (sMonitoredInterface); // Not sure if this is necessary/set later
            _pIcmpTable->fillICMPProtoObject(
                sMonitoredInterface,
                pIcmpPacketsByInterface);
            int64 endTime = getTimeInMilliseconds();
            if ((endTime - startTime) > C_D_TRAFFIC_PROTO_TIME)
            {
                checkAndLogMsg(pszMethodName, Logger::L_Warning,
                    "handleIcmp took %lldms\n", (endTime - startTime));
            }
            _pms.sendICMP(&nsc);
        }
    }

    void NetSensor::handleTCPRTT (const google::protobuf::Timestamp & ts)
    {
        auto pszMethodName = "NetSensor::handleTCPRTT";
        int64 startTime = getTimeInMilliseconds();

        String sMonitoredInterface;
        _pNCM->pslMonitoredInterfacesList->resetGet();

        while (_pNCM->pslMonitoredInterfacesList->getNext(sMonitoredInterface))
        {
            PtrLList<TCPStream> *pStreamsOnInterface = _pTcpRTable->getTCPStreamsOnInterface(sMonitoredInterface);
            if (pStreamsOnInterface == nullptr) {
                continue;
            }
            pStreamsOnInterface->resetGet();
            TCPStream *pStream;
            InterfaceInfo ii;
            _iit.fillElementWithCopy (ii, sMonitoredInterface);
            String sSensorIP = InetAddr(ii.ui32IpAddr).getIPAsString();

            while ((pStream = pStreamsOnInterface->getNext()) != nullptr) {
                PtrLList<measure::Measure> *pMeasureList = pStream->createMeasures(sSensorIP);

                measure::Measure *pNextMeasure;
                pMeasureList->resetGet();

                while ((pNextMeasure = pMeasureList->getNext()) != nullptr)
                {
                    google::protobuf::Timestamp *pNewTs = new google::protobuf::Timestamp();
                    pNewTs->CopyFrom(ts);

                    _protoWrapper.setMeasureTimestamp(pNextMeasure, pNewTs);
                    _pms.sendRTTMeasure(pNextMeasure);
                }
                pMeasureList->removeAll(true);
                delete pMeasureList;
            }
        }

        int64 endTime = getTimeInMilliseconds();
        if ((endTime - startTime) > C_D_TRAFFIC_PROTO_TIME)
        {
            checkAndLogMsg(pszMethodName, Logger::L_Warning,
                "handleTCPRTT took %lldms\n", (endTime - startTime));
        }
    }

    void NetSensor::handleIcmpRTT (const google::protobuf::Timestamp & ts)
    {
        auto pszMethodName = "NetSensor::handleIcmpRTT";
        int64 startTime = getTimeInMilliseconds();

        String sMonitoredInterface;
        _pNCM->pslMonitoredInterfacesList->resetGet();
        while (_pNCM->pslMonitoredInterfacesList->getNext (sMonitoredInterface)) {
            InterfaceInfo ii;
            _iit.fillElementWithCopy (ii, sMonitoredInterface);
            String sSensorIP = InetAddr (ii.ui32IpAddr).getIPAsString();

            ICMPRTTHashTable * pHashtable = _pIcmpTable->getRTTTableForInterface (sMonitoredInterface);
            if (pHashtable == nullptr) {
                continue;
            }

            PtrLList<measure::Measure> *pMeasureList = pHashtable->createMeasures(sSensorIP);
            measure::Measure *pNextMeasure;
            pMeasureList->resetGet();
            while ((pNextMeasure = pMeasureList->getNext()) != nullptr)
            {
                google::protobuf::Timestamp *pNewTs = new google::protobuf::Timestamp();
                pNewTs->CopyFrom(ts);

                _protoWrapper.setMeasureTimestamp(pNextMeasure, pNewTs);
                _pms.sendRTTMeasure(pNextMeasure);
            }

            pMeasureList->removeAll(true);
            delete pMeasureList;
        }

        int64 endTime = getTimeInMilliseconds();
        if ((endTime - startTime) > C_D_TRAFFIC_PROTO_TIME)
        {
            checkAndLogMsg(pszMethodName, Logger::L_Warning,
                "handleIcmpRTT took %lldms\n", (endTime - startTime));
        }
    }

    void NetSensor::handleIW (const google::protobuf::Timestamp & ts) 
    {
        auto pszMethodName = "NetSensor::handleIW";
        auto pDumpList = _pIWDumpManager->getDumps();
        if (pDumpList == nullptr) {
            return;
        }
        if (pDumpList->size() == 0) {
            delete pDumpList;    
            return;
        }     
        
        InterfaceInfo ii;

        _iit.fillElementWithCopy (ii, _pIWDumpManager->getIname());
        String sSensorIP = InetAddr (ii.ui32IpAddr).getIPAsString();
        for (auto iter = pDumpList->begin(); iter != pDumpList->end(); iter++) {
            auto pMeasure = _protoWrapper.getMeasureIW (sSensorIP, *iter);
            google::protobuf::Timestamp *pNewTs = new google::protobuf::Timestamp();
            pNewTs->CopyFrom(ts);
            _protoWrapper.setMeasureTimestamp(pMeasure, pNewTs);
            _pms.sendIWMeasure (pMeasure);
            delete pMeasure;
        }
        delete pDumpList;
    }

    void NetSensor::handleThreadsTermination (void)
    {
        printf("Request Threads termination: \n");
        printf("    Handler Thread...\n");
        _pHThread->requestTerminationAndWait();
        for (auto i = _pMonitorThreadsMap.getAllElements(); !i.end(); i.nextElement()) {
            printf("    Interface Monitor Thread for %s...\n", i.getKey());
            i.getValue()->requestTerminationAndWait();
        }
        printf("All Threads terminated\n");
    }

    void NetSensor::handleUserInput (const UserInput command)
    {
        switch (command)
        {
        case UserInput::CLOSE:
            _netSensorState = NetSensorState::USER_REQUESTED_TERMINATION;
            break;

        case UserInput::PRINT_TRAFFIC_TABLE_CONTENT:
            _pTrafficTable->printContent();
            printHelp();
            break;

        case UserInput::PRINT_TOPOLOGY_TABLE_CONTENT:
            if (_pNCM->netmskTopActive || _pNCM->nproxyTopActive) {
                _pTopologyTable->printContent();
            }
            else {
                printf ("NetSensor::TCP RTT calculation not enabled! Enable with '-trtt'\n");
            }
            printHelp();
            break;

        case UserInput::PRINT_ICMP_TABLE_CONTENT:
            _pIcmpTable->printContent();
            printHelp();
            break;

        case UserInput::PRINT_TCP_RTT_TABLE_CONTENT:
            _pTcpRTable->printContent();
            printHelp();
            break;

        case UserInput::PRINT_ICMP_RTT_TABLE_CONTENT:
            _pIcmpTable->printRTTContent();
            printHelp();
            break;

        case UserInput::PRINT_HELP:
            printHelp();
            break;

        case UserInput::LAUNCH_DIAGNOSTIC:
            printf("\n\nStarting diagnostic service:\n\n");
            _pHThread->startDiagnostic();
            //_pMonitorThreadsMap
            //_pms
            printHelp();
            break;

        case UserInput::PRINT_ABOUT:
            printAbout();
            printHelp();
            break;

        case UserInput::PRINT_CONFIG:
            printConfiguration();
            printHelp();
            break;
            
        case UserInput::PRINT_IW_DUMPS:
            printIWDumps();
            break;
        }
        
    }

    void NetSensor::initWithDefaultInterface (void)
    {
        _pNCM->init();
    }

    int NetSensor::init (int argc, char* argv[])
    {
        CommandLineConfigs cLC;
        CommandLineParser cLParser (&cLC);
        cLParser.parseArgs (argc, const_cast<const char**> (argv));

        if (cLParser.hasHelp()) {
            cLParser.printParamMeanings();
            return -1;
        }
        else if (cLParser.hasError()) {
            cLParser.printError();
            return -2;
        }

        initWithValues (&cLC);
        printf ("\n");
        printConfig();

        return 0;
    }

    void NetSensor::initWithValues (CommandLineConfigs * pCLC)
    {
		auto pszMethodName = "NetSensor::initWithValues";
        // Init NetSensor based on command line configs
        _mode = pCLC->em;

        if (pCLC->bUseCompression)      { 
            initWithCompression(); 
        }

        if (pCLC->bUseExternalTopology) { 
            initWithExternalTopology(); 
        }

        if (pCLC->bCalculateTCPRTT)     { 
            initWithTCPRTTCalculation(); 
        }

        if (pCLC->bEnableIW) {
            initWithIW (pCLC->sIWIface);
        }

        if (pCLC->bHasDifferentDeliveryTime) {
            _ui32msStatsDeliveryTime    = pCLC->ui32msStatsDeliveryTime;
            _pNCM->ui32ToDelPer         = _ui32msStatsDeliveryTime;
            _pNCM->ui32TDelPer          = _ui32msStatsDeliveryTime;        
        }
        
        if (pCLC->bHasForcedAddr) {
            _pNCM->ui32ForcedInterfaceAddr = pCLC->ui32ForcedInterfaceAddr;    
        }
        
        if (pCLC->bHasForcedNetmask) {
            _pNCM->ui32ForcedNetmask = pCLC->ui32ForcedNetmask;
        }

        if (_mode == Mode::EM_REPLAY) {
            // initReplayModeWithConfigFile (pCLC->sReplayConfigPath);
			checkAndLogMsg(pszMethodName, Logger::L_SevereError, "Replay mode is disabled\n");
			exit (-1);
        }
        else {
            if (!pCLC->bHasInterfaces) {
                InterfaceDetector id;
                id.init();
                char * pNextInterface = nullptr;
                while (id.getNext (&pNextInterface)) { 
                    initWithInterface (pNextInterface); 
                }
            }
            else {
                String interfaceName;
                pCLC->interfaceNameList.resetGet();
                while (pCLC->interfaceNameList.getNext (interfaceName)) {                                 
                    initWithInterface (interfaceName);           
                }
                _pNCM->useForcedInterfaces = pCLC->bHasForcedInterfaces;
            }
            initWithRecipient (pCLC->sRecipient);
        }

        initWithCfgFile (pCLC->sConfigPath);
    }

    void NetSensor::initWithIW (const NOMADSUtil::String& sIWIface)
    {
        const char* pszMethodName = "initWithIW";
        if (_pIWDumpManager->init (sIWIface.c_str()) < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "IW init failed IWIfaceName null\n");    
        }
        _pIWDumpManager->start();
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "IW thread started\n");
    }

	/*
    void NetSensor::initReplayModeWithConfigFile(const NOMADSUtil::String& sReplayModeConfigPath)
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
        initReplayMode(rMCFG.getStatsRecipientIP(), rMCFG.getTDM());

        auto lInterfaceNames = rMCFG.getInterfaceNames();
        lInterfaceNames->resetGet();
        while (lInterfaceNames->getNext(sInterfaceName)) {
            addMonitoringInterface(sInterfaceName);
        }
    }
	*/

    int NetSensor::initAsComponent (const String& statDestinationIP)
    {
        auto pszMethodName = "NetSensor::initAsComponent";
        switch (_mode)
        {
        case Mode::EM_NETPROXY:
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Init as Netproxy Component\n");
            initNetproxyMode (statDestinationIP);
            break;
        default:
            initWithDefaultInterface();
            break;
        }
        return 0;
    }

    int NetSensor::initNetproxyMode (const String & statRecipientIP)
    {
        int rc = 0;
        _pNCM->setNPModeDefaultConfigurations (statRecipientIP);
        _pNCM->printConfigurationReport();
        printNetworkInfoContent();
        if ((rc = configureNetSensor()) < 0) {
            return -1;
        }
        return 0;
    }

    void NetSensor::netSensorStateMachine (void)
    {
        auto pszMethodName = "NetSensor::netSensorStateMachine";
        bool bprintTraffic = false;
        bool bprintTopology = false;

        switch (_netSensorState)
        {
        case NetSensorState::UPDATE_TIMERS:
            updateNetSensorTimers();
            break;

        case NetSensorState::CHECK_STATUS:
            break;

        case NetSensorState::CLEAN_TABLES:
            _pTrafficTable->cleanTable	(C_MAX_CLEANING_NUMBER);
            _pTopologyTable->cleanTable (C_MAX_CLEANING_NUMBER);
            _pIcmpTable->cleanTable		(C_MAX_CLEANING_NUMBER);
            _pTopCache->cleanTables		(C_MAX_CLEANING_NUMBER);
            _pTcpRTable->cleanTable		(C_MAX_CLEANING_NUMBER);
            break;

        case NetSensorState::SEND_STATS:
            sendStats();
            break;

        case NetSensorState::PRINT_TIME:
            if (bprintTraffic) { _pTrafficTable->printContent(); }
            if (bprintTopology) { _pTopologyTable->printContent(); }
            break;

        case NetSensorState::REST:
            sleepForMilliseconds (C_SLEEP_TIME);
            break;

        case NetSensorState::CHECK_FOR_USER_INPUT:
            UserInput command;
            if ((_pUserInputMutex.lock() == Mutex::RC_Ok)) {
                while (!_userInputQueue.isEmpty()) {
                    _userInputQueue.dequeue (command);
                    handleUserInput (command);
                }
                _pUserInputMutex.unlock();
            }
            break;

        case NetSensorState::TERMINATE:
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Termination requested\n");
            requestTermination();
            break;
        }
        nextStatus();
    }

    void NetSensor::nextStatus(void)
    {
        switch (_netSensorState)
        {
        case NetSensorState::UPDATE_TIMERS:
            _netSensorNextState = NetSensorState::CHECK_STATUS;
            break;

        case NetSensorState::CHECK_STATUS:
            if (_bCleaningTimerElapsed) {
                _netSensorNextState = NetSensorState::CLEAN_TABLES;
            }
            else if (_bSendingTimerElapsed) {
                _netSensorNextState = NetSensorState::SEND_STATS;
            }
            else {
                _netSensorNextState = NetSensorState::REST;
            }
            break;

        case NetSensorState::CLEAN_TABLES:
            _netSensorNextState = NetSensorState::CHECK_STATUS;
            _bCleaningTimerElapsed = false;
            break;

        case NetSensorState::REST:
            _netSensorNextState = NetSensorState::CHECK_FOR_USER_INPUT;
            break;

        case NetSensorState::SEND_STATS:
            _netSensorNextState = NetSensorState::CHECK_STATUS;
            _bSendingTimerElapsed = false;
            break;

        case NetSensorState::USER_REQUESTED_TERMINATION:
            _netSensorNextState = NetSensorState::TERMINATE;
            break;

        case NetSensorState::CHECK_FOR_USER_INPUT:
            _netSensorNextState = NetSensorState::UPDATE_TIMERS;
            break;
        }
        _netSensorState = _netSensorNextState;
    }

    void NetSensor::passUserInput (const UserInput command)
    {
        if ((_pUserInputMutex.lock() == Mutex::RC_Ok)) {
            _userInputQueue.enqueue(command);
            _pUserInputMutex.unlock();
        }
    }

    int NetSensor::printMac (EtherMACAddr mac)
    {
        auto pszMethodName = "NetSensor::printMac";
        checkAndLogMsg(pszMethodName, Logger::L_Info, "\tMac: %2x:%2x:%2x:%2x:%2x:%2x\n",
            mac.ui8Byte1,
            mac.ui8Byte2,
            mac.ui8Byte3,
            mac.ui8Byte4,
            mac.ui8Byte5,
            mac.ui8Byte6);
        return 0;
    }

    int NetSensor::printNetworkInfoContent()
    {
        auto pszMN = "NetSensor::printNetworkInfoContent()";
        checkAndLogMsg (pszMN, Logger::L_Info, "Internal Interface Info:\n");
        checkAndLogMsg (pszMN, Logger::L_Info, "\tIs internal set to: %d\n", _lnpi.iiInternal.bIIface);
        checkAndLogMsg (pszMN, Logger::L_Info, "\tName set to: %s\n", _lnpi.iiInternal.sInterfaceName.c_str());
        checkAndLogMsg (pszMN, Logger::L_Info, "\tIP set to: %s\n", InetAddr (_lnpi.iiInternal.ui32IpAddr).getIPAsString());
        checkAndLogMsg (pszMN, Logger::L_Info, "\tNetmask set to: %s\n", InetAddr (_lnpi.iiInternal.ui32Netmask).getIPAsString());
        checkAndLogMsg (pszMN, Logger::L_Info, "\tTDM set to: %d\n", _lnpi.iiInternal.topologyDetectionMechanism);
        printMac	   (_lnpi.iiInternal.emacInterfaceMAC);
        return 0;
    }

    void NetSensor::printAbout(void)
    {
        printf ("     _   _      _   _____  \n");
        printf ("    | \\ | |    | | /  ___| \n");
        printf ("    |  \\| | ___| |_\\ `--.  ___ _ __  ___  ___  _ __ \n");
        printf ("    | . ` |/ _ \\ __|`--. \\/ _ \\ '_ \\/ __|/ _ \\| '__|\n");
        printf ("    | |\\  |  __/ |_/\\__/ /  __/ | | \\__ \\ (_) | |   \n");
        printf ("    \\_| \\_/\\___|\\__\\____/ \\___|_| |_|___/\\___/|_|   \n");

        printf ("* Contact: rfronteddu@ihmc.us - NOMADS\n");
        printf ("* This file is part of the IHMC NetSensor Library/Component\n");
        printf ("* Copyright (c) 2010-2017 IHMC.\n");
        printf ("* This program is free software; you can redistribute it and/or\n");
        printf ("* modify it under the terms of the GNU General Public License\n");
        printf ("* version 3 (GPLv3) as published by the Free Software Foundation.\n");
        printf ("* U.S. Government agencies and organizations may redistribute\n");
        printf ("* and/or modify this program under terms equivalent to\n");
        printf ("* Government Purpose Rights as defined by DFARS\n");
        printf ("* 252.227-7014(a)(12) (February 2014).\n");
        printf ("* Alternative licenses that allow for use within commercial products may be\n");
        printf ("* available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.\n");
        printf ("* Relsease 2/26/2018.\n\n\n");
    }

    void NetSensor::printConfiguration (void)
    {
        //I will have to add a mutex for the cfg object..
        _pNCM->printConfigurationReport();
    }

    void NetSensor::printHelp (void)
    {
        printf("\n ________________________________________________________________________\n");
        printf("|             NetSensor Help Menu - Type command ID and press enter       | \n");
        printf("|_________________________________________________________________________|\n");
        printf(".  Commands:     ID                 DESCRIPTION:                          .\n");
        printf("|            1. close         <-- Terminate Netsensor                     |\n");
        printf(".            2. cfg           <-- Print configurations                    .\n");
        printf("|            3. printTraffic  <-- Print traffic table content             |\n");
        printf(".            4. printTopology <-- Print topology table content            .\n");
        printf("|            5. printICMPInfo <-- Print ICMP table content                |\n");
        printf(".            6. printTCPRTT   <-- Print TCP RTT info                      .\n");
        printf("|            7. printIcmpRTT  <-- Print ICMP RTT info                     |\n");
        printf(".            8. diag          <-- Launch diagnostic                       .\n");
        printf("|            9. help          <-- Print help menu                         |\n");
        printf("|            10. printIWDumps <-- Print IWDumps                           |\n");
        printf(".            0. about         <-- Software Info                           .\n");
        printf("  ________________________________________________________________________\n");
    }

    void NetSensor::sendStats (void)
    {
        auto pszMethodName = "NetSensor::sendStats";
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Last stats delivery: %d\n",
            getTimeInMilliseconds() - _lastSendTime);
        _lastSendTime = getTimeInMilliseconds();

        google::protobuf::Timestamp ts;
        setProtobufTimestamp (ts);

        handleTopology      (ts);
        handleTraffic       (ts);
        handleNetproxyInfo  (ts);
        handleIcmp          (ts);
        handleTCPRTT        (ts);
        handleIcmpRTT       (ts);
        handleIW (ts);
    }

    int NetSensor::setInterfaceInfo (const InterfaceInfoOpt& IIOpt)
    {
		auto pszMethodName = "NetSensor::setInterfaceInfo";
		/*
		boolean replayCond = (_mode == Mode::EM_REPLAY) &&
			(getTDMFromString (IIOpt.sTDM) == TopologyDetectionMechanism::TDM_NETPROXY);
		if (_mode == Mode::EM_NETPROXY || replayCond) {
		*/

		if (_mode == Mode::EM_NETPROXY) {
			if (!IIOpt.bIsInternal) {
				checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Extrnal Interface Info DEPRECATED");
				exit (-1);
			}

			auto pInterfaceInfo				    = &_lnpi.iiInternal;
            pInterfaceInfo->bIIface				= IIOpt.bIsInternal;
            pInterfaceInfo->emacInterfaceMAC	= IIOpt.emac;
            pInterfaceInfo->sInterfaceName		= IIOpt.pcIname;
            pInterfaceInfo->ui32IpAddr			= IIOpt.ipAddr;
            pInterfaceInfo->ui32Netmask			= IIOpt.netmask;
            pInterfaceInfo->topologyDetectionMechanism = getTDMFromString (IIOpt.sTDM);
        }
        _lInterfaceInfoOpt.add (IIOpt);
        return 0;
    }

    int NetSensor::setMonitoringInterface (InterfaceOptions opt)
    {
        auto pszMethodName = "NetSensor::setMonitoringInterface";

        auto pInterfaceMonitor = createNewMonitor (
			opt.sMonitoredInterface, 
			opt.pPQ, 
			opt.pRttPQ, 
			opt.ui32ForcedInterfaceAddr, 
			opt.ui32ForcedNetmask);

        if (pInterfaceMonitor == nullptr) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, 
				"Creation of Monitoring interface (%s) failed\n", opt.sMonitoredInterface.c_str());
            return -1;
        }

        auto pInterfaceMonitorCpy = pInterfaceMonitor->getInterfaceInfoCopy();
        if (_mode == Mode::EM_NETPROXY) {
            pInterfaceMonitorCpy->bIIface = opt.bIsInternal;
			if (opt.bIsInternal) {
				pInterfaceMonitorCpy->topologyDetectionMechanism = _lnpi.iiInternal.topologyDetectionMechanism;
			}
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "Embedding Mode not selected\n");
        }
        _pHThread->addInterfaceInfo (pInterfaceMonitorCpy);
        auto pNSInterfaceMonitorCpy = pInterfaceMonitor->getInterfaceInfoCopy();
        _iit.put (pNSInterfaceMonitorCpy);
        return 0;
    }

    int NetSensor::setMonitoringInterface (
        InterfaceInfoOpt iio,
        NetSensorPacketQueue* pPQ,
        NetSensorPacketQueue* pRttPQ)
    {
        auto pszMethodName = "setMonitoringInterface";
        auto pInterfaceMonitor = createNewMonitor (iio, pPQ, pRttPQ);
        if (pInterfaceMonitor == nullptr) {
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, "Creation of Monitoring interface (%s) failed\n", iio.pcIname.c_str());
            return -1;
        }

        auto pInterfaceMonitorCpy = pInterfaceMonitor->getInterfaceInfoCopy();
        if (_tdm == TopologyDetectionMechanism::TDM_NETPROXY) {
            assert (_mode == Mode::EM_REPLAY);
            pInterfaceMonitorCpy->bIIface = iio.bIsInternal;
            pInterfaceMonitorCpy->topologyDetectionMechanism = getTDMFromString(iio.sTDM);
        }
        else { 
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "Embedding Mode not selected\n"); 
        }
        _pHThread->addInterfaceInfo (pInterfaceMonitorCpy);

        auto pNSInterfaceMonitorCpy = pInterfaceMonitor->getInterfaceInfoCopy();
        _iit.put (pNSInterfaceMonitorCpy);
        return 0;
    }

    void NetSensor::updateNetSensorTimers (void)
    {
        _cleaningTimer = _cleaningTimer - C_SLEEP_TIME;
        if (!_bCleaningTimerElapsed) {
            if (_cleaningTimer <= 0) {
                _cleaningTimer = C_CLEANING_TIME;
                _bCleaningTimerElapsed = true;          
            }
        }

        if (!_bSendingTimerElapsed) {
            _sendingTimer = _sendingTimer - C_SLEEP_TIME;
            if (_sendingTimer <= 0) {
                _sendingTimer = _ui32msStatsDeliveryTime;
                _bSendingTimerElapsed = true;        
            }
        }
    }

    bool NetSensor::InterfaceInfoOpt::operator== (const NetSensor::InterfaceInfoOpt & rhs) const
    {
        return
            (bIsInternal == rhs.bIsInternal) &&
            (emac == rhs.emac) &&
            (ipAddr == rhs.ipAddr) &&
            (netmask == rhs.netmask) &&
            (pcIname == rhs.pcIname) &&
            (sPcapFilePath == rhs.sPcapFilePath);
    }

    int NetSensor::addRecipient(const char * pcRAddr, uint32 ui32RPort)
    {
        _pms.addRecipient(pcRAddr, ui32RPort);
        return 0;
    }

    int NetSensor::configureAndStartDeliveryInterfaces(void)
    {
        NOMADSUtil::String ip;
        _pNCM->pslIpDeliveryList->resetGet();
        while (_pNCM->pslIpDeliveryList->getNext(ip)) {
            addRecipient(ip, _pNCM->ui32DelPort);
        }
        return 0;
    }

    int NetSensor::configureAndStartHandlerThread(void)
    {
        _pHThread->start();
        return 0;
    }

    int NetSensor::configureWithoutEmbeddedMode(void)
    {
        int rc = 0;
        if ((rc = configureNetSensor()) < 0) {
            return -1;
        }
        return 0;
    }

    void NetSensor::initWithInterface (const NOMADSUtil::String & sInterfaceName)
    {
        if (sInterfaceName != "") {
            addMonitoringInterface (sInterfaceName);
        }
        else {
            initWithDefaultInterface();
        }
    }

    void NetSensor::initWithCfgFile(const NOMADSUtil::String & sConfigFilePath)
    {
        if (sConfigFilePath != "") {
            _pNCM->init(sConfigFilePath);
        }
    }

    void NetSensor::initWithRecipient(const NOMADSUtil::String & sRecipient)
    {
        if (sRecipient == "") {
            _pNCM->initBase(DEFAULT_DELIVERY_IP);
        }
        else {
            _pNCM->initBase(sRecipient);
        }
    }

    // Sets the use of external node storage in the topology to true
    void NetSensor::initWithExternalTopology(void)
    {
        _pNCM->storeExternalNodes = true;
        _pHThread->changeExternalNodeStorage();
    }

    void NetSensor::initWithCompression(void)
    {
        _pNCM->useProtobufCompression = true;
        _pms.configureToUseCompression();
    }

    void NetSensor::initWithTCPRTTCalculation(void)
    {
        _pNCM->calculateTCPRTT = true;
        _pHThread->changeTCPRTTCalculation();
    }

    int NetSensor::initReplayMode (
        const NOMADSUtil::String & sRecipient,
        const NOMADSUtil::String & sTDM)
    {
        return _pNCM->setReplayModeDefaultConfigurations(sRecipient, sTDM);
    }

    void NetSensor::printConfig(void)
    {
        _pNCM->printConfigurationReport();
    }
    void NetSensor::printIWDumps()
    {
        _pIWDumpManager->printDumps();
    }
}
