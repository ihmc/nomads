#ifndef NETSENSOR_NetSensor__INCLUDED
#define NETSENSOR_NetSensor__INCLUDED
/*
* NetSensor.h
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
*
* Main NetSensor library Class
*/

#include "EmbeddingMode.h"
#include "HandlerThread.h"
#include "ICMPInterfaceTable.h"
#include "InterfaceMonitor.h"
#include "LList.h"
#include "LocalNetproxyInfo.h"
#include "Logger.h"
#include "ManageableThread.h"
#include "NetSensorConfigurationManager.h"
#include "NetSensorConstants.h"
#include "NetSensorDefaultConfigurations.h"
#include "NetSensorPacketQueue.h"
#include "ProtoMessageSender.h"
#include "Queue.h"
#include "TrafficTable.h"
#include "UDPDatagramSocket.h"
#include "CommandLineParser.h"
//proto
#include "container.pb.h"
#include "icmpinfo.pb.h"
#include "InterfaceDetector.h"

namespace NOMADSUtil
{
    class JsonObject;
}

namespace IHMC_NETSENSOR
{
    namespace NETSENSOR_STATE
    {
        enum NetSensorState
        {
            CHECK_STATUS = 0,
            REST = 1,
            SEND_STATS = 2,
            PRINT_TIME = 3,
            CLEAN_TABLES = 4,
            UPDATE_TIMERS = 5,
            CHECK_FOR_USER_INPUT = 6,
            USER_REQUESTED_TERMINATION = 7,
            TERMINATE = 8
        };
    }

    namespace NETSENSOR_USER_INPUT
    {
        enum UserInput
        {
            CLOSE = 0,
            PRINT_CONFIG = 1,
            PRINT_TRAFFIC_TABLE_CONTENT = 2,
            PRINT_TOPOLOGY_TABLE_CONTENT = 3,
            PRINT_ICMP_TABLE_CONTENT = 4,
            PRINT_TCP_RTT_TABLE_CONTENT = 5,
            LAUNCH_DIAGNOSTIC = 6,
            PRINT_HELP = 7,
            PRINT_ABOUT = 8,
        };
    }

class NetSensor : public NOMADSUtil::ManageableThread
{

public:
    struct MonitoringInterfaceOpt
    {
        NOMADSUtil::String      sMonitoredInterface;
        NetSensorPacketQueue    *pPQ;
        NetSensorPacketQueue    *pRttPQ;
        bool                    bIsInternal;
        EmbeddingMode           em;
    };

    struct InterfaceInfoOpt
    {
        static const InterfaceInfoOpt fromJSON(const NOMADSUtil::JsonObject * const jObj);

        bool operator== (const InterfaceInfoOpt & rhs) const;

        NOMADSUtil::String          pcIname;
        NOMADSUtil::EtherMACAddr    emac;
        uint32                      ipAddr;
        uint32                      netmask;
        uint32                      gwAddr;
        bool                        bIsInternal;
        NOMADSUtil::String          sTDM;
        NOMADSUtil::String          sPcapFilePath;
    };


    NetSensor   (const EmbeddingMode em = EmbeddingMode::EM_NONE);
    ~NetSensor  (void); // NetSensor is not intended for polymorphism

    /*
    * Call this before init as component
    */
    int addMonitoringInterface(const char *pcInterfaceName);

    int configureWithoutEmbeddedMode(void);

    /*
    * Initialize NetSensor from another component
    *
    * Each mode has different instructions:
    *   NET_PROXY:
    *       This mode can be used in any gw system, what will happen is that
    *       all sources that come from the internal interface will be considered
    *       internal nodes while the external interface will use the standard
    *       netmask mechanism to detect local nodes.

    *       After you created the Netsensor Object:
    *           1) Add each monitored interface name using addMonitoringInterface.
    *           2) Add the list of remote netproxies IP using setRemoteNpList.
    *           3) Add Internal and external interface descriptions using setInterfaceInfo
    *           4) Call initAsComponent(NET_PROXY)
    *
    */
    int initAsComponent(const NOMADSUtil::String & statRecipientIp);

    /*
    * Initialize NetSensor with Default Values
    * Default interfaces are eth0 for linux and Ethernet for windows
    */

    void initWithDefaultInterface(void);

    int init(int argc, char *argv[]);

    /*
    * Pass a Command to NetSensor, possible commands are:
    *   CLOSE: to terminate netsensor.
    *   PRINT_TRAFFIC_TABLE_CONTENT: To print the content of the traffic table.
    *    PRINT_TOPOLOGY_TABLE_CONTENT: To print the content of topology table.
    *    PRINT_ICMP_TABLE_CONTENT: To print the information about icmp packets
    *    PRINT_HELP: To print summary of commands.
    *    LAUNCH_DIAGNOSTIC: To start a diagnostic of the internal
    *       behavior of netsensor.
    *    PRINT_ABOUT: To print information about copyrights and author.
    *    PRINT_CONFIG: To print list of active configurations.
    */
    void passUserInput(const NETSENSOR_USER_INPUT::UserInput command);

    void run(void);

    /*
    * This method is used to describe interface info and should be called
    * before init as component.
    */
    int setInterfaceInfo(const InterfaceInfoOpt & iiOpt);

    /*
    * This has to be called before initAsComponent(NETPROXY);
    * This function is used to set the list of remote netproxies
    * the local netproxy is supposed to be connected with
    */
    int setRemoteNpList(NOMADSUtil::LList<uint32> & remoteNpList);
    int setRemoteNpList(NOMADSUtil::LList<uint32> && remoteNpList);     // For use with rvalues

private:
    int addRecipient(const char* pcRecipientAddr,
                     uint32 ui32RecipientPort);

    int configureNetSensor(void);

    int configureAndStartMonitoringInterfaces(NetSensorPacketQueue *pPQ,
                                              NetSensorPacketQueue *pRttPQ);
    int configureAndStartHandlerThread(void);
    int configureAndStartDeliveryInterfaces(void);
    InterfaceMonitor * createNewMonitor(NOMADSUtil::String sInterfaceName,
                                        NetSensorPacketQueue *pPQ,
                                        NetSensorPacketQueue *pRttPQ);
    InterfaceMonitor * createNewMonitor(InterfaceInfoOpt iio,
                                        NetSensorPacketQueue *pPQ,
                                        NetSensorPacketQueue *pRttPQ);
    void handleNetproxyInfo(const google::protobuf::Timestamp & ts);
    void handleTopology(const google::protobuf::Timestamp & ts);
    void handleTraffic(const google::protobuf::Timestamp & ts);
    void handleIcmp(const google::protobuf::Timestamp & ts);
    void handleThreadsTermination(void);
    void handleUserInput(const NETSENSOR_USER_INPUT::UserInput command);

    // Functions for configuring NetSensor
    void initWithValues(const bool bCompression, const bool bExternalTopology,
                        const bool bHasInterfaces, const bool bCalculateTCPRTT,
                        const EmbeddingMode em, const NOMADSUtil::String sConfigPath,
                        const NOMADSUtil::String sReplayModeConfigPath,
                        NOMADSUtil::LList<NOMADSUtil::String> sInterfaceList,
                        const NOMADSUtil::String sRecipient);

    void initWithInterface(const NOMADSUtil::String & sInterfaceName);
    void initWithCfgFile(const NOMADSUtil::String & sConfigFilePath);
    void initWithRecipient(const NOMADSUtil::String & sRecipient);
    void initWithExternalTopology(void);
    void initWithCompression(void);
    void initWithTCPRTTCalculation(void);

    int initNetproxyMode(const NOMADSUtil::String & statRecipientIP);
    int initReplayMode(const NOMADSUtil::String & statRecipientIP,
                       const NOMADSUtil::String & sTDM);
    void initReplayModeWithConfigFile(const NOMADSUtil::String & sReplayModeConfigPath);

    void printConfig(void);
    void nextStatus(void);
    void netSensorStateMachine(void);
    void printAbout(void);
    void printHelp(void);
    void printConfiguration(void);
    int printMac(NOMADSUtil::EtherMACAddr mac);
    int printNetworkInfoContent();
    void sendStats(void);
    int setMonitoringInterface(MonitoringInterfaceOpt opt);
    int setMonitoringInterface(InterfaceInfoOpt iio, NetSensorPacketQueue *pPQ,
                               NetSensorPacketQueue *pRttPQ);
    void updateNetSensorTimers(void);

private:
    NOMADSUtil::Mutex _pUserInputMutex;
    NOMADSUtil::Queue<NETSENSOR_USER_INPUT::UserInput> _userInputQueue;

    EmbeddingMode _embeddingMode;
    TopologyDetectionMechanism _tdm;
    //NOMADSUtil::Queue<NOMADSUtil::String> _MonitoringInterfaceQueue;

    NETSENSOR_STATE::NetSensorState _netSensorState;
    NETSENSOR_STATE::NetSensorState _netSensorNextState;
    NetSensorConfigurationManager *_pNCM;
    NOMADSUtil::StringHashtable<InterfaceMonitor> _pMonitorThreadsMap;
    NOMADSUtil::LList<InterfaceInfoOpt> _lInterfaceInfoOpt;

    NetSensorPacketQueue *_pRttPQ;
    NetSensorPacketQueue *_pPQ;
    HandlerThread        *_pHThread;
    TopologyTable        *_pTopologyTable;
    TrafficTable         *_pTrafficTable;
    ICMPInterfaceTable   *_pIcmpTable;
    TopologyCache        *_pTopCache;
    TCPRTTInterfaceTable *_pTcpRTable;

    bool _bTimeToCleanTrafficTables;
    bool _bTimeToCleanTopologyTables;

    bool _bRttDetection;

    int64 _cleaningTimer;
    int64 _sendingTimer;

    bool _bCleaningTimerElapsed;
    bool _bSendingTimerElapsed;

    bool _bPerformCleaning;
    bool _bRest;

    ProtoMessageSender _pms;
    LocalNetproxyInfo _lnpi;

    int64 lastSendTime;
};


inline bool NetSensor::InterfaceInfoOpt::operator== (const NetSensor::InterfaceInfoOpt & rhs) const
{
    return (bIsInternal == rhs.bIsInternal) && (emac == rhs.emac) &&
        (ipAddr == rhs.ipAddr) && (netmask == rhs.netmask) &&
        (pcIname == rhs.pcIname) && (sPcapFilePath == rhs.sPcapFilePath);
}

inline int NetSensor::addRecipient(const char* pcRAddr, uint32 ui32RPort)
{
    _pms.addRecipient(pcRAddr, ui32RPort);

    return 0;
}

inline int NetSensor::configureAndStartDeliveryInterfaces(void)
{
    NOMADSUtil::String ip;
    _pNCM->pslIpDeliveryList->resetGet();
    while (_pNCM->pslIpDeliveryList->getNext(ip)) {
        addRecipient(ip, _pNCM->ui32DelPort);
    }

    return 0;
}

inline int NetSensor::configureAndStartHandlerThread(void)
{
    _pHThread->start();

    return 0;
}

inline int NetSensor::configureWithoutEmbeddedMode(void)
{
    int rc = 0;

    if ((rc = configureNetSensor()) < 0) {
        return -1;
    }

    return 0;
}

inline void NetSensor::initWithInterface(const NOMADSUtil::String & sInterfaceName)
{
    if (sInterfaceName != "") {
        _pNCM->initBase(DEFAULT_DELIVERY_IP);
        addMonitoringInterface(sInterfaceName);
    }
    else {
        initWithDefaultInterface();
    }

}

inline void NetSensor::initWithCfgFile(const NOMADSUtil::String & sConfigFilePath)
{
    if (sConfigFilePath != "") {
        _pNCM->init(sConfigFilePath);
    }
}

inline void NetSensor::initWithRecipient(const NOMADSUtil::String & sRecipient)
{
    _pNCM->initBase(sRecipient);
}

// Sets the use of external node storage in the topology to true
inline void NetSensor::initWithExternalTopology(void)
{
    _pNCM->storeExternalNodes = true;
    _pHThread->changeExternalNodeStorage();
}

inline void NetSensor::initWithCompression(void)
{
    _pNCM->useProtobufCompression = true;
    _pms.configureToUseCompression();
}

inline void NetSensor::initWithTCPRTTCalculation(void)
{
    _pNCM->calculateTCPRTT = true;
    _pHThread->changeTCPRTTCalculation();
}

inline int NetSensor::initReplayMode(const NOMADSUtil::String & sRecipient,
                                     const NOMADSUtil::String & sTDM)
{
    return _pNCM->setReplayModeDefaultConfigurations(sRecipient, sTDM);
}

inline void NetSensor::printConfig(void)
{
    _pNCM->printConfigurationReport();
}

}
#endif