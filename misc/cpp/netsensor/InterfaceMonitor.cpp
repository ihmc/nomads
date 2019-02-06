/*
* InterfaceMonitor.cpp
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
#if defined (WIN32)
    #include <stdio.h>
    #include <winsock2.h>
    #include <Winbase.h>
    #define snprintf _snprintf
    #define PROTOBUF_USE_DLLS
#elif defined (LINUX)
    #include <stdio.h>
    //#include <time.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <linux/if_tun.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

#ifdef ANDROID
    #include <arpa/inet.h>
    #include <linux/if.h>
    #include <linux/in.h>
    #include <linux/sockios.h>
    #include <stdint.h>
#endif

#include "Logger.h"
#include "InterfaceInfo.h"
#include "topology.pb.h"

#include "InterfaceMonitor.h"
#include "NetSensorUtilities.h"
#include "PCapInterface.h"
#include "NetSensorPacket.h"
#include "NetSensorPacketQueue.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
using namespace NOMADSUtil;
using namespace netsensor;

namespace IHMC_NETSENSOR
{
int InterfaceMonitor::initLive (
    const NOMADSUtil::String & sInterfaceName,
    bool isInternal, 
    bool rttDetection,
    uint32 ui32ForcedInterfaceAddr,
    uint32 ui32ForcedNetmask)
{
    auto pszMethodName = "InterfaceMonitor::InterfaceMonitor";
    _sInterfaceName = sInterfaceName;
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Creating new LIVE IMT for: %s\n", sInterfaceName.c_str());

    int msValidity = 1;
    _pNetInterface = PCapInterface::getPCapInterface (sInterfaceName, msValidity);
    if (_pNetInterface == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "_pNetInterface for <%s> NULL\n",  
            sInterfaceName.c_str());
        return -1;
    }

    bool bForceInterfaceAddr = ui32ForcedInterfaceAddr != 0;
    if (bForceInterfaceAddr) {
        _ui32IPAddr = ui32ForcedInterfaceAddr;
        checkAndLogMsg (pszMethodName, Logger::L_Info, "forced interface addr %s for network interface <%s>\n\n",
            InetAddr (_ui32IPAddr).getIPAsString(), sInterfaceName.c_str());
    }
    else {
        _ui32IPAddr = _pNetInterface->getIPv4Addr() ? _pNetInterface->getIPv4Addr()->ui32Addr : 0;
        if (_ui32IPAddr == 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "IP address Detection of network interface <%s> failed\n",
                sInterfaceName.c_str());
            delete _pNetInterface;
            _pNetInterface = nullptr;
            return -2;
        }
    }

#ifndef ANDROID
    if ((pszExternalMACAddr = _pNetInterface->getMACAddr()) == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
			"Get for MAC for network interface <%s>\n\n failed", sInterfaceName.c_str());
        delete _pNetInterface;
        _pNetInterface = nullptr;
        return -3;
    }
    buildEthernetMACAddressFromString (_emacInterfaceMAC, pszExternalMACAddr);
#elif ANDROID
    checkAndLogMsg (pszMethodName, Logger::L_Info,
		"Trying to get MAC address for ANDROID device...\n");
    struct ifreq *ifr;
    struct ifconf ifc;
    int s, i;
    int numif;

    // find number of interfaces.
    memset(&ifc, 0, sizeof(ifc));
    ifc.ifc_ifcu.ifcu_req = NULL;
    ifc.ifc_len = 0;

    if ((s = ::socket (PF_INET, SOCK_STREAM, 0)) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not obtain socket!\n");
        return -4;
    }

    if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "ioctl SIOCGIFCONF error!\n");
        return -5;
    }

    if ((ifr = (ifreq*)malloc(ifc.ifc_len)) == NULL) {
        checkAndLogMsg (pszMethodName,
			Logger::L_SevereError, "Could not malloc ifreq!\n");
        return -6;
    }

    ifc.ifc_ifcu.ifcu_req = ifr;

    if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
        checkAndLogMsg(pszMethodName,
			Logger::L_SevereError, "ioctl SIOCGIFCONF error!\n");
        return -7;
    }

    numif = ifc.ifc_len / sizeof(struct ifreq);

    for (i = 0; i < numif; i++) {
        struct ifreq *r = &ifr[i];
        struct sockaddr_in *sin = (struct sockaddr_in *)&r->ifr_addr;
        if (strcmp(r->ifr_name, _cInterface)) {
            continue; // skip loopback interface
        }
        if (ioctl(s, SIOCGIFHWADDR, r) < 0) { //get MAC address
            checkAndLogMsg (pszMethodName,
				Logger::L_SevereError, "ioctl(SIOCGIFHWADDR) error!\n");
            return -8;
        }

        char macaddr[18];
        sprintf(macaddr, " %02X:%02X:%02X:%02X:%02X:%02X",
            (unsigned char)r->ifr_hwaddr.sa_data[0],
            (unsigned char)r->ifr_hwaddr.sa_data[1],
            (unsigned char)r->ifr_hwaddr.sa_data[2],
            (unsigned char)r->ifr_hwaddr.sa_data[3],
            (unsigned char)r->ifr_hwaddr.sa_data[4],
            (unsigned char)r->ifr_hwaddr.sa_data[5]);

        _emacInterfaceMAC.ui8Byte1 = (unsigned char)r->ifr_hwaddr.sa_data[0];
        _emacInterfaceMAC.ui8Byte2 = (unsigned char)r->ifr_hwaddr.sa_data[1];
        _emacInterfaceMAC.ui8Byte3 = (unsigned char)r->ifr_hwaddr.sa_data[2];
        _emacInterfaceMAC.ui8Byte4 = (unsigned char)r->ifr_hwaddr.sa_data[3];
        _emacInterfaceMAC.ui8Byte5 = (unsigned char)r->ifr_hwaddr.sa_data[4];
        _emacInterfaceMAC.ui8Byte6 = (unsigned char)r->ifr_hwaddr.sa_data[5];

        checkAndLogMsg (pszMethodName,
            Logger::L_Info, "MAC is %02X:%02X:%02X:%02X:%02X:%02X\n",
                _emacInterfaceMAC.ui8Byte1, _emacInterfaceMAC.ui8Byte2,
                _emacInterfaceMAC.ui8Byte3,
                _emacInterfaceMAC.ui8Byte4, _emacInterfaceMAC.ui8Byte5,
                _emacInterfaceMAC.ui8Byte6);
    }
    close(s);
    free(ifr);
#endif
    bool bForceNetmask = ui32ForcedNetmask != 0;
    if (bForceNetmask) {
        _ui32Netmask = ui32ForcedNetmask;
        checkAndLogMsg (pszMethodName, Logger::L_Info, "forced netmask %s for network interface <%s>\n\n",
            InetAddr (_ui32Netmask).getIPAsString(), sInterfaceName.c_str());
    }
    else {
        // Retrieve IPv4 Netmask
        _ui32Netmask = _pNetInterface->getNetmask() ? _pNetInterface->getNetmask()->ui32Addr : 0;
        if (!_ui32Netmask) {
            checkAndLogMsg(pszMethodName,
                Logger::L_SevereError,
                "could not determine Netmask of the network interface\n");
            delete _pNetInterface;
            _pNetInterface = nullptr;
            return -9;
        }
        else {
            checkAndLogMsg(pszMethodName, Logger::L_Info,
                "retrieved netmask %s for network interface <%s>\n\n",
                InetAddr (_ui32Netmask).getIPAsString(), sInterfaceName.c_str());
        }
    }
    if ((pIPv4IPAddr = _pNetInterface->getDefaultGateway()) == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Info,
			"getDefaultGateway(); couldn't retrieve gw IP for <%s>\n",
            sInterfaceName.c_str());
        _ui32GwIPAddr = 0;
    }
    else {
        _ui32GwIPAddr = pIPv4IPAddr->ui32Addr;
    }

    return 0;
}

int InterfaceMonitor::initReplay (
    const NOMADSUtil::String & sInterfaceName,
    const NOMADSUtil::String & sReplayFile,
    uint32 ui32IPAddr, 
    uint32 ui32Netmask,
    uint32 ui32GwIPAddr,
    NOMADSUtil::EtherMACAddr emacInterfaceMAC)
{
    static const char* meName = "InterfaceMonitor::InterfaceMonitor";
    _sInterfaceName = sInterfaceName;
    checkAndLogMsg(meName, Logger::L_Info,
                   "Creating new REPLAY IMT for interface %s from file: %s\n",
                   _sInterfaceName.c_str(), sReplayFile.c_str());

    _pNetInterface = PCapInterface::getPCapInterface(sInterfaceName, sReplayFile, ui32IPAddr,
                                                     ui32Netmask, ui32GwIPAddr, emacInterfaceMAC);
    if (_pNetInterface == NULL) {
        checkAndLogMsg(meName, Logger::L_SevereError,
                       "_pNetInterface for <%s> NULL\n", sReplayFile.c_str());
        return -1;
    }

    // Populate interface info
    _ui32IPAddr = ui32IPAddr;
    _ui32Netmask = ui32Netmask;
    _ui32GwIPAddr = ui32GwIPAddr;
    _emacInterfaceMAC = emacInterfaceMAC;

    return 0;
}

InterfaceInfo * InterfaceMonitor::getInterfaceInfoCopy()
{
    InterfaceInfo* pii = new InterfaceInfo();
    pii->emacInterfaceMAC   = _emacInterfaceMAC;
    pii->ui32IpAddr         = _ui32IPAddr;
    pii->ui32Netmask        = _ui32Netmask;
    pii->sInterfaceName     = _sInterfaceName;

    return pii;
}

void InterfaceMonitor::fillNetworkInfo(const char* pcInterfaceName,
                                       NetworkInfo *protoNI)
{
    protoNI->set_interfaceip (convertIpToHostOrder(_ui32IPAddr));
    protoNI->set_networkname (InetAddr((_ui32Netmask & _ui32IPAddr)).
                             getIPAsString());
    protoNI->set_networknetmask (NOMADSUtil::InetAddr(_ui32Netmask).
                                getIPAsString());
}

void InterfaceMonitor::run(void)
{
    started();
    static const char* meName = "InterfaceMonitor::run";
    checkAndLogMsg(meName, Logger::L_Info, "IMT for Interface %s started\n",
		_sInterfaceName.c_str());
    int received;
    NetSensorPacket packet;
    while (!terminationRequested() && !_pNetInterface->terminationRequested()) {
        if (isRunning()) {
            received = _pNetInterface->readPacket(packet.ui8Buf, sizeof(packet.ui8Buf),
                                                  &packet.int64RcvTimeStamp);
            if (received > 0) {
                /*
                if (pEthHeader->ui16EtherType == 0xDD86)
                {
                printf("IPv6!\n");
                }
                */
                packet.received = received;
                packet.sMonitoredInterface  = _sInterfaceName;
                packet.int64RcvTimeStamp    = getTimeInMilliseconds();
                enqueue(packet);
            }
        }
    }
    terminating();
    setTerminatingResultCode(0);
}

void InterfaceMonitor::enqueue(const NetSensorPacket & p)
{
    static const char* meName = "InterfaceMonitor::enqueue";
    while (!pQueue->enqueue(p))
    {
        checkAndLogMsg(meName, Logger::L_MediumDetailDebug, "Enqueue delayed\n");
        sleepForMilliseconds(70);
    }

    if (_bRttDetection) {
        while (!pRttQueue->enqueue(p)) {
            checkAndLogMsg(meName, Logger::L_Warning, "Rtt Enqueue delayed\n");
            sleepForMilliseconds(70);
        }
    }
}

}
