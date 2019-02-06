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

#include "CommandLineParser.h"
#include "Mode.h"

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
#include "InterfaceOptions.h"
#include "ProtobufWrapper.h"
#include "IWDumpManager.h"
//proto
#include "container.pb.h"
#include "icmpinfo.pb.h"
#include "InterfaceDetector.h"
#include "measure.pb.h"

#include <list>
namespace NOMADSUtil
{
    class JsonObject;
}

namespace IHMC_NETSENSOR
{

enum class NetSensorState
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
    
enum class UserInput
{
    CLOSE = 0,
    PRINT_CONFIG = 1,
    PRINT_TRAFFIC_TABLE_CONTENT = 2,
    PRINT_TOPOLOGY_TABLE_CONTENT = 3,
    PRINT_ICMP_TABLE_CONTENT = 4,
    PRINT_TCP_RTT_TABLE_CONTENT = 5,
    PRINT_ICMP_RTT_TABLE_CONTENT = 6,
    LAUNCH_DIAGNOSTIC = 7,
    PRINT_HELP = 8,
    PRINT_ABOUT = 9,
    PRINT_IW_DUMPS = 10
};
    
class NetSensor : public NOMADSUtil::ManageableThread
{
public:
    struct InterfaceInfoOpt
    {
        static const InterfaceInfoOpt fromJSON (const NOMADSUtil::JsonObject * const jObj);

        bool operator== (const InterfaceInfoOpt & rhs) const;

        NOMADSUtil::String       pcIname;
        NOMADSUtil::EtherMACAddr emac;
        uint32                   ipAddr;
        uint32                   netmask;
        uint32                   gwAddr;
        bool                     bIsInternal;
        NOMADSUtil::String       sTDM;
        NOMADSUtil::String       sPcapFilePath;
    };

    NetSensor (const Mode m = Mode::EM_NONE);
    
    ~NetSensor (void); // NetSensor is not intended for polymorphism



    /*
    * Call this before init as component
    */
    int addMonitoringInterface (const char * pszInterfaceName);

    int configureWithoutEmbeddedMode (void);

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
    int initAsComponent (const NOMADSUtil::String & statRecipientIp);

    /*
    * Initialize NetSensor with Default Values
    * Default interfaces are eth0 for linux and Ethernet for windows
    */
    void initWithDefaultInterface (void);

    int init (int argc, char * argv[]);

    void run(void);

    UserInput getCommandFromUserInput (NOMADSUtil::String sCommand);

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
    void passUserInput (const UserInput command);

    /*
    * This method is used to describe interface info and should be called
    * before init as component. DO THIS ONLY FOR THE INTERNAL INTERFACES! 
	* -- CURRENTLY ONLY ONE INTERNAL INTERFACE IS SUPPORTED --
    */
    int setInterfaceInfo (const InterfaceInfoOpt& iiOpt);

private:
    int addRecipient (
        const char * pcRecipientAddr, 
        uint32 ui32RecipientPort);

    int configureNetSensor (void);

    int configureAndStartMonitoringInterfaces (NetSensorPacketQueue * pPQ, NetSensorPacketQueue * pRttPQ);
    int configureAndStartHandlerThread (void);
    int configureAndStartDeliveryInterfaces (void);

    InterfaceMonitor * createNewMonitor (
        NOMADSUtil::String sInterfaceName,
        NetSensorPacketQueue * pPQ,
        NetSensorPacketQueue * pRttPQ,
        uint32 ui32ForcedInterfaceAddr = 0,
        uint32 ui32ForcedNetmask = 0);

    InterfaceMonitor * createNewMonitor (
        InterfaceInfoOpt iio,
        NetSensorPacketQueue *pPQ,
        NetSensorPacketQueue *pRttPQ);

    void handleNetproxyInfo (const google::protobuf::Timestamp & ts);
    void handleTopology (const google::protobuf::Timestamp & ts);
    void handleTraffic (const google::protobuf::Timestamp & ts);
    void handleIcmp (const google::protobuf::Timestamp & ts);
    void handleTCPRTT(const google::protobuf::Timestamp & ts);
    void handleIcmpRTT (const google::protobuf::Timestamp & ts);
    void handleIW (const google::protobuf::Timestamp & ts);
    void handleThreadsTermination(void);
    void handleUserInput (const UserInput command);

    // Functions for configuring NetSensor
    void initWithValues (CommandLineConfigs * pCLC);

    void initWithInterface (const NOMADSUtil::String & sInterfaceName);

    void initWithCfgFile (const NOMADSUtil::String & sConfigFilePath);
    void initWithRecipient (const NOMADSUtil::String & sRecipient);
    void initWithExternalTopology (void);
    void initWithCompression (void);
    void initWithTCPRTTCalculation (void);
    void initWithIW (const NOMADSUtil::String& sIWIface);

    int initNetproxyMode (const NOMADSUtil::String & statRecipientIP);
    int initReplayMode (
        const NOMADSUtil::String & statRecipientIP,
        const NOMADSUtil::String & sTDM);
    
	// void initReplayModeWithConfigFile (const NOMADSUtil::String & sReplayModeConfigPath);

    void printConfig (void);
    void printIWDumps (void);
    void nextStatus (void);
    void netSensorStateMachine (void);
    void printAbout (void);
    void printHelp (void);
    void printConfiguration (void);
    int printMac (NOMADSUtil::EtherMACAddr mac);
    int printNetworkInfoContent (void);
    void sendStats (void);
    int setMonitoringInterface (InterfaceOptions opt);
    int setMonitoringInterface (InterfaceInfoOpt iio, NetSensorPacketQueue * pPQ, NetSensorPacketQueue * pRttPQ);
    void updateNetSensorTimers (void);

private:
    NOMADSUtil::Mutex _pUserInputMutex;
    NOMADSUtil::Queue<UserInput> _userInputQueue;

    Mode _mode;
    TopologyDetectionMechanism _tdm;
    //NOMADSUtil::Queue<NOMADSUtil::String> _MonitoringInterfaceQueue;

    NetSensorState _netSensorState;
    NetSensorState _netSensorNextState;
    NetSensorConfigurationManager * _pNCM;
    NOMADSUtil::StringHashtable<InterfaceMonitor> _pMonitorThreadsMap;
    NOMADSUtil::LList<InterfaceInfoOpt> _lInterfaceInfoOpt;

    NetSensorPacketQueue * _pRttPQ;
    NetSensorPacketQueue * _pPQ;
    HandlerThread        * _pHThread;
    TopologyTable        * _pTopologyTable;
    TrafficTable         * _pTrafficTable;
    ICMPInterfaceTable   * _pIcmpTable;
    TopologyCache        * _pTopCache;
    TCPRTTInterfaceTable * _pTcpRTable;


    IWDumpManager * _pIWDumpManager;

    InterfacesInfoTable _iit;
    ProtobufWrapper _protoWrapper;

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

    int64 _lastSendTime;
    uint32 _ui32msStatsDeliveryTime;
};
}
#endif