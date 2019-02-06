#ifndef NETSENSOR_HandlerThread__INCLUDED
#define NETSENSOR_HandlerThread__INCLUDED
/*
* HandlerThread.h
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
*
*
* The handler thread retrive packets from the packet queue
* and proceeds in populating the stats tables
*
*/
#include "HTCfg.h"
#include "ICMPInterfaceTable.h"
#include "InterfaceInfo.h"
#include "InterfacesInfoTable.h"
#include "InetAddr.h"
#include "Logger.h"
#include "ManageableThread.h"
#include "MicroflowId.h"
#include "NetworkHeaders.h"
#include "NetSensorUtilities.h"
#include "TrafficTable.h"
#include "TrafficElement.h"
#include "TopologyTable.h"
#include "TrafficElement.h"
#include "NetSensorConstants.h"
#include "TopologyCache.h"
#include "TCPRTTInterfaceTable.h"

namespace IHMC_NETSENSOR
{
    class NetSensorPacket;
    class NetSensorPacketQueue;

class HandlerThread : public NOMADSUtil::ManageableThread
{
public:
    /*
    *
    */
    void addInterfaceInfo (InterfaceInfo *pII);

    /*
    * Pointers are owned by NetSensor
    *
    */
    HandlerThread (void);

    ~HandlerThread (void);

    bool init (HTCfg * pHTC);


    void startDiagnostic(void);

    void changeExternalNodeStorage(void);

    void changeTCPRTTCalculation(void);

    void run();
private:
    TrafficElement::Classification getClassification (
        const char* interfaceName,
        const uint32 sa, const uint32 da,
        const NOMADSUtil::EtherMACAddr smac,
        const NOMADSUtil::EtherMACAddr dmac);

    bool isPacketFromInternalSource(const NOMADSUtil::EtherMACAddr srcMAC,
        const uint32 ui32SAddr, const InterfaceInfo *pII);

    bool isSourceInternalNMHelper(const NOMADSUtil::EtherMACAddr srcMAC,
        const uint32 ui32SourceAddr, const InterfaceInfo *pII);

    bool isSourceInternalNPHelper(const NOMADSUtil::EtherMACAddr srcMAC,
        const uint32 ui32SAddr, const InterfaceInfo *pII);

    void populateTrafficTables(const NetSensorPacket & p);

    void populateTopologyTable(const NetSensorPacket & p);

    void populateIcmpTable(const NetSensorPacket & p);

    void populateTcpRttTable(const NetSensorPacket & p);

    void printClassification(uint32 ui32SAddr, uint32 ui32DAddr, NOMADSUtil::EtherMACAddr smac,
        NOMADSUtil::EtherMACAddr dmac, TrafficElement::Classification tmpClassification);

    void TopologyManager(const NetSensorPacket & p);

    void TrafficManager(const NetSensorPacket & p);

    void IcmpManager(const NetSensorPacket & p);

    void TCPRTTManager(const NetSensorPacket & p);

    uint8 getPacketType(const NetSensorPacket & p);
//<---------------------------------------------------------------------------->
public:
    //NetSensor owns the pointers
    NetSensorPacketQueue * _pQueue;
    NetSensorPacketQueue * _pRttQueue;
    TrafficTable         * _pTrt;
    TopologyTable        * _pTopt;
    ICMPInterfaceTable   * _pIcmpTable;
    TCPRTTInterfaceTable * _pTcpRTable;
    TopologyCache *_pTopCache;
private:
    InterfacesInfoTable      _iit;
    NOMADSUtil::EtherMACAddr _emacNPExternalMAC;
    NOMADSUtil::EtherMACAddr _emacNPInternalMAC;

    bool _bStoreExternalTopology;
    bool _bCalculateTCPRTT;
};


inline void HandlerThread::TopologyManager(const NetSensorPacket & p)
{
    populateTopologyTable(p);
}

inline void HandlerThread::TrafficManager (const NetSensorPacket & p)
{
    populateTrafficTables (p);
}

inline void HandlerThread::changeExternalNodeStorage(void)
{
    _bStoreExternalTopology = !_bStoreExternalTopology;
}

inline void HandlerThread::changeTCPRTTCalculation(void)
{
    _bCalculateTCPRTT = !_bCalculateTCPRTT;
}

}
#endif