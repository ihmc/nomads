#ifndef NETSENSOR_InterfaceMonitor__INCLUDED
#define NETSENSOR_InterfaceMonitor__INCLUDED
/*
* InterfaceMonitor.h
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
* Each InterfaceMonitor is linked to a specific network interface and it uses
* pcap to retrieve packets from the network and then proceeds in enqueuing
* the packets in the packet queue
*
*/
#include "Logger.h"
#include "FTypes.h"
#include "StrClass.h"
#include "NetworkInterface.h"
#include "InterfaceInfo.h"
#include"topology.pb.h"

#include "InterfaceCaptureMode.h"

namespace IHMC_NETSENSOR
{
    class NetSensorPacket;
    class NetSensorPacketQueue;

    class InterfaceMonitor : public NOMADSUtil::ManageableThread
    {
    public:
        InterfaceMonitor(void);
        ~InterfaceMonitor(void);
        
        int initLive (
            const NOMADSUtil::String & sInterfaceName,
            bool isInternal = false, 
            bool rttDetection = false,
            uint32 ui32ForcedInterfaceAddr = 0,
            uint32 ui32ForcedNetmask = 0);

        int initReplay(const NOMADSUtil::String & sInterfaceName,
                       const NOMADSUtil::String & sReplayFile,
                       uint32 ui32IPAddr, uint32 ui32Netmask, uint32 ui32GwIPAddr,
                       NOMADSUtil::EtherMACAddr emacInterfaceMAC);

        void fillNetworkInfo(const char* pcInterfaceName, netsensor::NetworkInfo *protoNI);
        // The client will be responsable for deleting the InterfaceInfo object returned
        InterfaceInfo * getInterfaceInfoCopy(void);

        void run(void);

        virtual void requestTermination         (void);
        virtual void requestTerminationAndWait  (void);

    private:
        void enqueue(const NetSensorPacket & packet);
    //<--------------------------------------------------------------------------->

    public:
        uint32  _ui32IPAddr;
        uint32  _ui32Netmask;
        uint32  _ui32GwIPAddr;
        bool    _bRttDetection;
        NOMADSUtil::EtherMACAddr    _emacInterfaceMAC;

        const uint8                *pszExternalMACAddr;
        const NOMADSUtil::IPv4Addr *pIPv4IPAddr;
        NetSensorPacketQueue       *pQueue;
        NetSensorPacketQueue       *pRttQueue;

    private:
        NetworkInterface           *_pNetInterface;
        NOMADSUtil::String          _sInterfaceName;
    };


    inline InterfaceMonitor::InterfaceMonitor(void) :
        _ui32IPAddr(0), _ui32Netmask(0), _ui32GwIPAddr(0), _bRttDetection(false),
        _emacInterfaceMAC{0}, pszExternalMACAddr(nullptr),
        pIPv4IPAddr(nullptr), pQueue(nullptr), pRttQueue(nullptr),
        _pNetInterface(nullptr), _sInterfaceName("") { }

    inline InterfaceMonitor::~InterfaceMonitor(void)
    {
        delete _pNetInterface;
        _pNetInterface = nullptr;
    }

    inline void InterfaceMonitor::requestTermination(void)
    {
        _pNetInterface->requestTermination();
        ManageableThread::requestTermination();
    }

    inline void InterfaceMonitor::requestTerminationAndWait(void)
    {
        _pNetInterface->requestTermination();
        ManageableThread::requestTerminationAndWait();
    }

}
#endif