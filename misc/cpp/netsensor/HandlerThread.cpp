/*
* HandlerThread.cpp
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

#include "HandlerThread.h"
#include "PacketStructures.h"
#include "NetSensorPacket.h"
#include "NetSensorPacketQueue.h"
#include "HTCfg.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
using namespace NOMADSUtil;
using namespace IHMC_NETSENSOR_NET_UTILS;
namespace IHMC_NETSENSOR
{
HandlerThread::HandlerThread (void)
{
}

HandlerThread::~HandlerThread (void)
{
    _pQueue     = nullptr;
    _pRttQueue  = nullptr;
    _pTrt       = nullptr;
    _pTopt      = nullptr;
    _pIcmpTable = nullptr;
    _pTopCache  = nullptr;
    _pTcpRTable = nullptr;
}

bool HandlerThread::init (HTCfg * pHTC) {
    
    if (pHTC->pQueue == nullptr) {
        checkAndLogMsg ("HandlerThread::HandlerThread", Logger::L_SevereError, "pQueue is null?\n");
        return false;
    }

    if (pHTC->pRttQueue == nullptr) {
        checkAndLogMsg ("HandlerThread::HandlerThread", Logger::L_SevereError, "_pRttQueue is null?\n");
        return false;
    }

    if (pHTC->pTrT == nullptr) {
        checkAndLogMsg ("HandlerThread::HandlerThread", Logger::L_SevereError, "_pTrt is null?\n");
        return false;
    }

    if (pHTC->pTopT == nullptr) {
        checkAndLogMsg ("HandlerThread::HandlerThread", Logger::L_SevereError, "pTopT is null?\n");
        return false;
    }

    if (pHTC->pIcmpTable == nullptr) {
        checkAndLogMsg ("HandlerThread::HandlerThread", Logger::L_SevereError, "pIcmpTable is null?\n");
        return false;
    }

    if (pHTC->pTRIT == nullptr) {
        checkAndLogMsg ("HandlerThread::HandlerThread", Logger::L_SevereError, "pTRIT is null?\n");
        return false;
    }

    _pQueue     = pHTC->pQueue;
    _pRttQueue  = pHTC->pRttQueue;
    _pTrt       = pHTC->pTrT;
    _pTopt      = pHTC->pTopT;
    _pIcmpTable = pHTC->pIcmpTable;
    _pTcpRTable = pHTC->pTRIT;
    _pTopCache  = pHTC->pTopCache;

    _bStoreExternalTopology = pHTC->storeExternals;
    _bCalculateTCPRTT       = pHTC->calculateTCPRTT;
    return true;
}

void HandlerThread::run (void)
{
    started();
    auto pszMethodName = "HandlerThread::run";
    //checkAndLogMsg(mName, Logger::L_Info, "Handler Thread started\n");

    int received;
    NetSensorPacket p;
    while (!terminationRequested()) {   
        if (!isRunning()) {
            continue;
        }

        _pQueue->dequeue (p);
        if (p.received <= 0) {
            continue;
        }

        int64 i64start      = getTimeInMilliseconds();
        TrafficManager (p);
        int64 i64TrafficEnd = getTimeInMilliseconds();
        TopologyManager (p);
        int64 i64TopologyEnd= getTimeInMilliseconds();

        if ((i64TrafficEnd - i64start) > C_D_TRAFFIC_HANDLING_TIME) {
            auto pszMessage = "TrafficManager took %lldms\n";
            checkAndLogMsg (pszMethodName, Logger::L_Warning, pszMessage, (i64TrafficEnd - i64start));
        }
        if ((i64TopologyEnd - i64TrafficEnd) > C_D_TRAFFIC_HANDLING_TIME) {
            auto pszMessage = "TopologyManager took %lldms\n";
            checkAndLogMsg (pszMethodName, Logger::L_Warning, pszMessage, (i64TopologyEnd - i64start));
        }

        // Check packet type
        uint8 ui8PType = getPacketType (p);
        if (ui8PType == IP_PROTO_ICMP) {
            IcmpManager (p);
        }
        else if (ui8PType == IP_PROTO_TCP) {
            if (_bCalculateTCPRTT) {
                TCPRTTManager (p);
            }
        }   
    }
    terminating();
}

void HandlerThread::addInterfaceInfo (InterfaceInfo * pII)
{
    auto pszMethodName = "HandlerThread::addInterfaceInfo";
    if (pII == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "pII is null?");
    }
    else {
        if (pII->topologyDetectionMechanism == TopologyDetectionMechanism::TDM_NETPROXY) {
            _emacNPInternalMAC = pII->bIIface ? pII->emacInterfaceMAC : pII->emacInterfaceMAC;
        }
    }
    _iit.put (pII);
}

void HandlerThread::startDiagnostic(void)
{
    printf ("Handler Thread Tests: \n");
    printf ("1) Is running?");
    if (isRunning()) {
        printf ("Yes\n");
    }
    else {
        printf ("No\n");
    }

    printf ("Mutex Tests: \n");
    printf ("1) PQM");
    (_pQueue->mutexTest()) ? printf (" - Passed\n") : printf (" - Failed\n");

    printf ("2) RTT");
    (_pRttQueue->mutexTest()) ? printf (" - Passed\n") : printf (" - Failed\n");

    printf ("3) Traffic Table");
    (_pTrt->mutexTest()) ? printf (" - Passed\n") : printf (" - Failed\n");

    printf ("4) Topology Table");
    (_pTopt->mutexTest()) ? printf (" - Passed\n") : printf (" - Failed\n");

    printf ("5) ICMP Table");
    (_pIcmpTable->mutexTest()) ? printf (" - Passed\n") : printf (" - Failed\n");
}

TrafficElement::Classification HandlerThread::getClassification (
    const char * ifaceName, 
    const uint32 sa, 
    const uint32 ui32DestAddr,
    const NOMADSUtil::EtherMACAddr smac, 
    const NOMADSUtil::EtherMACAddr dmac)
{
    auto pszMethodName = "HandlerThread::getClassification";
    if (ifaceName == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "interfaceName is null?");
    }

    auto tmpClassification = TrafficElement::OBS;
    InterfaceInfo ii;
    int rc = _iit.fillElementWithCopy (ii, ifaceName);

    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "No pII for %s?\n", ifaceName);
        return TrafficElement::OBS;
    }

    if (ii.bIIface) {
        tmpClassification = TrafficElement::OBS;
    }
    else {
        if (smac == ii.emacInterfaceMAC) {
            tmpClassification = TrafficElement::SNT;
        }
        else if (dmac == ii.emacInterfaceMAC || (isInMulticastRange (ui32DestAddr))) {
            tmpClassification = TrafficElement::RCV;
        }
    }
    
    if (C_CLASSIFICATION_DEBUG_MODE) {
        printClassification (sa, ui32DestAddr, smac, dmac, tmpClassification);
    }

    return tmpClassification;
}

bool HandlerThread::isPacketFromInternalSource (
    const NOMADSUtil::EtherMACAddr srcMAC,
    const uint32 ui32SAddr,
    const InterfaceInfo * pII)
{
    auto pszMethodName = "HandlerThread::isPacketFromInternalSource";

    if (pII == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "pII is null?");
        return false;
    }
    else {
        uint32 tmpAddr = pII->ui32IpAddr;
        uint32 tmpNetmask = pII->ui32Netmask;
        TopologyDetectionMechanism TDM = pII->topologyDetectionMechanism;

        switch (TDM)
        {
        case TopologyDetectionMechanism::TDM_NETPROXY:
            return isSourceInternalNPHelper (srcMAC, ui32SAddr, pII);

        case TopologyDetectionMechanism::TDM_NETMASK:
            return isSourceInternalNMHelper (srcMAC, ui32SAddr, pII);

        default:
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "No TDM specified for interface?");
            return false;
        }
    }
}

bool HandlerThread::isSourceInternalNMHelper (
    const NOMADSUtil::EtherMACAddr srcMAC, 
    const uint32 ui32SAddr,
    const InterfaceInfo * pII)
{
    auto pszMethodName = "HandlerThread::isSourceInternalNMHelper";
    if (pII == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "pII is null?");
    }

    // don't count as internals 0 and the subnet
    if ((ui32SAddr == 0) || (ui32SAddr == (pII->ui32IpAddr & pII->ui32Netmask))) {
        return false;
    }

    //check if packet source IP is in subnet range
    bool isInternal = false;
    ((ui32SAddr & pII->ui32Netmask) == (pII->ui32IpAddr & pII->ui32Netmask)) ?
        (isInternal = true) : (isInternal = false);
    return isInternal;
}

bool HandlerThread::isSourceInternalNPHelper (
    const NOMADSUtil::EtherMACAddr srcMAC, 
    const uint32 ui32SAddr,
    const InterfaceInfo * pII)
{
    if (pII == nullptr) {
        checkAndLogMsg ("HandlerThread::isSourceInternalNPHelper", Logger::L_SevereError, "pII is null?");
    }

    if (pII->bIIface) {
        //need to add yourself and remove things coming from external interface
        //check if the packet come from the internal or external interfaces
        if (pII->ui32IpAddr == ui32SAddr) {
            return true;
        }
        else {
            //remove forwarded sources
            if ((srcMAC == _emacNPInternalMAC) || (srcMAC == _emacNPExternalMAC)) {
                return false;
            }
            else {
                bool bForceNetmaskFilter = true; // Force netmask check to remove weird entries
                if (bForceNetmaskFilter) {
                    return isSourceInternalNMHelper(srcMAC, ui32SAddr, pII);
                }
                else {
                    return true;
                }
            }
        }
    }
    else {
        //need to add yourself and remove things coming from internal interface
        if (pII->ui32IpAddr == ui32SAddr) {
            return true;
        }
        else {
            //remove forwarded stuff
            if ((srcMAC == _emacNPInternalMAC) || (srcMAC == _emacNPExternalMAC)) {
                return false;
            }
            else {
                //use the netmask for the external interface
                return isSourceInternalNMHelper (srcMAC, ui32SAddr, pII);
            }
        }
    }
}

void HandlerThread::populateTopologyTable (const NetSensorPacket & p)
{
    EtherFrameHeader* const pEthHeader = (EtherFrameHeader*)p.ui8Buf;
    pEthHeader->ntoh();
    IPHeader * pIPHeader = (IPHeader*) (p.ui8Buf + sizeof (EtherFrameHeader));
    uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
    uint32 ui32SrcAddr = pIPHeader->srcAddr.ui32Addr;

    InterfaceInfo ii;
    int rc = _iit.fillElementWithCopy (ii, p.sMonitoredInterface);

    NOMADSUtil::EtherMACAddr srcMAC = pEthHeader->src;
    srcMAC.ntoh();
    // I don't want to store "0.0.0.0" entries
    String sAddr = InetAddr (ui32SrcAddr).getIPAsString();
    // IPV6 gets translated into 0s, for now let's exclude the case
    if (sAddr != "0.0.0.0") {
        if (isPacketFromInternalSource (srcMAC, ui32SrcAddr, &ii)) {
            // Only check if we are using external topology
            if (_bStoreExternalTopology) {
                auto pTmpTopObject = _pTopCache->findEntriesInExternal (sAddr);

                // Remove node from external table
                if (pTmpTopObject != nullptr) {
                    ExternalTopologyCacheObject * pCachedObject;
                    while (pCachedObject = pTmpTopObject->getNext())
                    {
                        _pTopt->removeNodeFromInterfaceTable (
                            pCachedObject->sIFaceName, 
                            sAddr,
                            pCachedObject->sMACAddr);
                    }
                }
            }
            _pTopt->putInternal (p.sMonitoredInterface, sAddr,
                buildStringFromEthernetMACAddress (pEthHeader->src));
        }
        else {
            if (_bStoreExternalTopology) {
                if (!_pTopCache->isInternalEntry (sAddr)) {
                    String sMac = buildStringFromEthernetMACAddress (pEthHeader->src);
                    _pTopt->putExternal (p.sMonitoredInterface, sAddr, sMac);
                    _pTopCache->addExternalEntry (p.sMonitoredInterface, sAddr, sMac);
                }
            }
        }
    }
}

void HandlerThread::populateTrafficTables (const NetSensorPacket & p)
{
    auto pszMethodName = "HandlerThread::populateTrafficTables";

    const String sInterface    = p.sMonitoredInterface;
    const uint16 ui16PacketLen = (uint16)p.received;
    const int64 i64CurrTime    = p.int64RcvTimeStamp;

    UDPHeader  * pUDPHeader  = nullptr;
    TCPHeader  * pTCPHeader  = nullptr;
    ICMPHeader * pICMPHeader = nullptr;

    auto const pEthHeader = (EtherFrameHeader*)p.ui8Buf;
    pEthHeader->ntoh();
    NOMADSUtil::String tmpProtocol;

    if (pEthHeader->ui16EtherType != ET_IP) {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "EtherType not supported: %d\n", pEthHeader->ui16EtherType);
        return;
    }

    IPHeader * pIPHeader    = (IPHeader*)(p.ui8Buf + sizeof(EtherFrameHeader));
    uint16 ui16IPHeaderLen  = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
    uint8  uint8Protocol    = pIPHeader->ui8Proto;
    uint16 uint16SrcPort    = 0;
    uint16 uint16DestPort   = 0;
    uint32 ui32SrcAddr      = pIPHeader->srcAddr.ui32Addr;
    uint32 ui32DestAddr     = pIPHeader->destAddr.ui32Addr;

    //uint16 tcpHeadLength;
    switch (uint8Protocol)
    {
    case IP_PROTO_UDP:
        pUDPHeader = (UDPHeader*)(((uint8*)pIPHeader) + ui16IPHeaderLen);
        pUDPHeader->hton();
        uint16SrcPort   = pUDPHeader->ui16SPort;
        uint16DestPort  = pUDPHeader->ui16DPort;
        tmpProtocol = "UDP";
        break;
    case IP_PROTO_TCP:
        pTCPHeader = (TCPHeader*)(((uint8*)pIPHeader) + ui16IPHeaderLen);
        pTCPHeader->hton();
        uint16SrcPort   = pTCPHeader->ui16SPort;
        uint16DestPort  = pTCPHeader->ui16DPort;
        tmpProtocol     = "TCP";
        break;
    case IP_PROTO_ICMP:
        pICMPHeader = (ICMPHeader*)(((uint8*)pIPHeader) + ui16IPHeaderLen);
        pICMPHeader->hton();
        uint16SrcPort = 0;
        uint16DestPort = 0;
        tmpProtocol = "ICMP";
        break;
    case IP_PROTO_IGMP:
        uint16SrcPort   = 0;
        uint16DestPort  = 0;
        tmpProtocol     = "IGMP";
        break;
    default:
        checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug, "IP protocol %d unsupported, ignoring packet\n", uint8Protocol);
        return;
    }

    char tmpSP[8];
    convertIntToString (uint16SrcPort, tmpSP);
    char tmpDP[8];
    convertIntToString (uint16DestPort, tmpDP);

    MicroflowId microflowId;
    microflowId.sSA             = InetAddr (ui32SrcAddr).getIPAsString();
    microflowId.sDA             = InetAddr (ui32DestAddr).getIPAsString();
    microflowId.sSP             = tmpSP;
    microflowId.sDP             = tmpDP;
    microflowId.sProtocol       = tmpProtocol;
    microflowId.ui32Size        = ui16PacketLen;
    microflowId.i64CurrTime     = i64CurrTime;
    microflowId.classification  = getClassification (
        p.sMonitoredInterface,
        ui32SrcAddr, 
        ui32DestAddr, 
        pEthHeader->src,
        pEthHeader->dest);
    _pTrt->lockedPut (p.sMonitoredInterface, microflowId);
}

void HandlerThread::populateIcmpTable (const NetSensorPacket & p)
{
    auto pszMethodName = "HandlerThread::populateIcmpTable";

    const String sInterface = p.sMonitoredInterface;

    // Point ICMP packet to start of data of packet
    NS_ICMPPacket * pICMPPacket = new NS_ICMPPacket();

    // Point eth header to start of packet data
    pICMPPacket->pMacHeader = (NS_EtherFrameHeader *)p.ui8Buf;

    pICMPPacket->i64RcvTimeStamp = p.int64RcvTimeStamp;

    // Get the interface info
    InterfaceInfo ii;
    int rc = _iit.fillElementWithCopy (ii, sInterface);

    // Get IP Header data
    IPHeader * pIPHeader = (IPHeader*)(p.ui8Buf + sizeof (EtherFrameHeader));
    uint32 ui32SrcAddr   = pIPHeader->srcAddr.ui32Addr;
    bool isSent          = ii.ui32IpAddr == ui32SrcAddr;

    if (pICMPPacket->pMacHeader->ui16EtherType == ET_IP) {
        // Initialize packet objects
        pICMPPacket->pIpHeader = (NS_IPHeader *)(p.ui8Buf + sizeof (NS_EtherFrameHeader));
        pICMPPacket->pIcmpData = (NS_ICMP *)(p.ui8Buf + sizeof (NS_IPHeader) + sizeof (NS_EtherFrameHeader));
        _pIcmpTable->put (sInterface, pICMPPacket, isSent, p.int64RcvTimeStamp);
        delete pICMPPacket;
        pICMPPacket = nullptr;
    }
}

void HandlerThread::populateTcpRttTable (const NetSensorPacket & p)
{
    const String sInterface = p.sMonitoredInterface;

    // Get IP Header data
    IPHeader * pIPHeader    = (IPHeader*)(p.ui8Buf + sizeof(EtherFrameHeader));
    uint16 ui16IPHeaderLen  = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
    uint32 ui32SrcAddr      = pIPHeader->srcAddr.ui32Addr;
    uint32 ui32DestAddr     = pIPHeader->destAddr.ui32Addr;

    // Get TCP Header data
    TCPHeader * pTCPHeader  = (TCPHeader*)(((uint8*)pIPHeader) + ui16IPHeaderLen);
    uint16 uint16SrcPort    = pTCPHeader->ui16SPort;
    uint16 uint16DestPort   = pTCPHeader->ui16DPort;


    // Convert from int to string
    char tmpSP[8];
    convertIntToString (uint16SrcPort, tmpSP);
    char tmpDP[8];
    convertIntToString (uint16DestPort, tmpDP);


    // Get the interface info
    InterfaceInfo ii;
    int rc = _iit.fillElementWithCopy (ii, sInterface);

    bool isIncoming = ii.ui32IpAddr != ui32SrcAddr;

    TCPStreamData * pData = new TCPStreamData();
    if (isIncoming) {
        pData->sLocalIP    = InetAddr(ui32DestAddr).getIPAsString();
        pData->sRemoteIP   = InetAddr(ui32SrcAddr).getIPAsString();
        pData->sLocalPort  = tmpDP;
        pData->sRemotePort = tmpSP;
    }
    else {
        pData->sLocalIP    = InetAddr (ui32SrcAddr).getIPAsString();
        pData->sRemoteIP   = InetAddr (ui32DestAddr).getIPAsString();
        pData->sLocalPort  = tmpSP;
        pData->sRemotePort = tmpDP;
    }

    //if (pData->sLocalIP != "128.49.235.115" || pData->sRemoteIP != "128.49.235.153") {
    //    delete pData;
    //    return;
    //}

    if (pData->sLocalIP != "10.100.0.172" || pData->sRemoteIP != "10.10.10.3") {
        delete pData;
        return;
    }

    // Get tcp stream from table
    TCPStream * pStream = _pTcpRTable->getStream(*pData);
    if (pStream == nullptr) {
        pStream = new TCPStream (*pData, 5000);
        _pTcpRTable->put (sInterface, *pStream);
    }

    pStream->updateStreamRTTs (*pIPHeader, p.int64RcvTimeStamp, isIncoming);
    delete pData;
}

void HandlerThread::printClassification (
    uint32 ui32SAddr, 
    uint32 ui32DAddr,
    EtherMACAddr smac, 
    EtherMACAddr dmac,
    TrafficElement::Classification classification)
{
    printf ("TEST: mflow with sa: %s ,da: %s, sMac: %02x:%02x:%02x:%02x:%02x:%02x, dMac: %02x:%02x:%02x:%02x:%02x:%02x, classified as:",
        InetAddr (ui32SAddr).getIPAsString(), InetAddr (ui32DAddr).getIPAsString(),
        smac.ui8Byte1, smac.ui8Byte2, smac.ui8Byte3, smac.ui8Byte4, smac.ui8Byte5, smac.ui8Byte6,
        dmac.ui8Byte1, dmac.ui8Byte2, dmac.ui8Byte3, dmac.ui8Byte4, dmac.ui8Byte5, dmac.ui8Byte6);
    switch (classification)
    {
    case TrafficElement::OBS:
        printf (" OBS\n");
        break;
    case TrafficElement::RCV:
        printf (" RCV\n");
        break;
    case TrafficElement::SNT:
        printf (" SNT\n");
        break;
    }
}

void HandlerThread::IcmpManager (const NetSensorPacket & p)
{
    populateIcmpTable (p);
}

void HandlerThread::TCPRTTManager (const NetSensorPacket & p)
{
    populateTcpRttTable (p);
}

uint8 HandlerThread::getPacketType (const NetSensorPacket & p)
{
    EtherFrameHeader * const pEthHeader = (EtherFrameHeader*)p.ui8Buf;
    pEthHeader->ntoh();

    if (pEthHeader->ui16EtherType == ET_IP) {
        NS_IPHeader * pIPHeader = (NS_IPHeader*)(p.ui8Buf + sizeof (EtherFrameHeader));
        uint8 uint8Protocol = pIPHeader->ui8Proto;
        return uint8Protocol;
    }
    return 0;
}

}

