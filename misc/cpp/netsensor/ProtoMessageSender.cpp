/*
* ProtoMessageSender.cpp
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
#include"ProtoMessageSender.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;
using namespace netsensor;

namespace IHMC_NETSENSOR
{
int ProtoMessageSender::addRecipient(const char* pcAddr, const uint32 ui32Port)
{
    NetSensorRecipient *newNSR = new NetSensorRecipient(pcAddr, ui32Port);
    if(_pPMSMutex.lock() == NOMADSUtil::Mutex::RC_Ok){
        _recipientList.append(newNSR);
        _pPMSMutex.unlock();
    }
    return 0;
}

void ProtoMessageSender::initMicroflow(Microflow *to,
    const Microflow *from)
{
    to->set_ipdst(from->ipdst());
    to->set_ipsrc(from->ipsrc());
    to->set_latency(from->latency());
}

void ProtoMessageSender::initContainer(NetSensorContainer *pNSC, const google::protobuf::Timestamp *ts, DataType dt)
{
    pNSC->set_datatype(dt);
    google::protobuf::Timestamp *t = new google::protobuf::Timestamp();
    t->CopyFrom(*ts);
    pNSC->set_allocated_timestamp(t);
}

void ProtoMessageSender::initTopologyEl(Topology *to, const Topology *from)
{
    NetworkInfo *newNI = new NetworkInfo();
    newNI->set_networkname(from->networkinfo().networkname());
    newNI->set_networknetmask(from->networkinfo().networknetmask());
    newNI->set_interfaceip(from->networkinfo().interfaceip());
    to->set_allocated_networkinfo(newNI);
}

void ProtoMessageSender::initTrafficEl(TrafficByInterface *to,
    const TrafficByInterface *from)
{
    to->set_monitoringinterface(from->monitoringinterface());
}

void ProtoMessageSender::initStat(Stat *to, const Stat *from)
{
    to->set_dstport (from->dstport());
    to->set_protocol(from->protocol());
    to->set_srcport (from->srcport());
    to->set_stattype(from->stattype());
}

ProtoMessageSender::ProtoMessageSender(void)
{
    _ui32Mtu = C_PACKET_MAX_SIZE;
    _ui32MaxPacketSize = _ui32Mtu - 100;
    _bUseCompression = false;
}

ProtoMessageSender::~ProtoMessageSender(void)
{
    _recipientList.removeAll(true);
}

void ProtoMessageSender::sendNetproxyInfo(NetSensorContainer *nsc)
{
    static const char* meName = "NetSensor - ProtoMessageSender::sendNetproxyInfo";
    uint32 ui32size;
    if ((ui32size = nsc->ByteSize()) < _ui32Mtu) {
        char * pcBuffer = new char[ui32size];
        if (!pcBuffer) {
            checkAndLogMsg(meName, Logger::L_SevereError,
                           "Memory allocation of %uB failed!\n", ui32size);
            return;
        }

        checkAndLogMsg("ProtoMessageSender::sendNetproxyInfo",
            Logger::L_HighDetailDebug, "About to send: %uB\n\n", ui32size);
        nsc->SerializeToArray(pcBuffer, ui32size);
        sendSerializedData(pcBuffer, ui32size);
        delete pcBuffer;
    }
}

int ProtoMessageSender::sendSerializedData(
    const char* serializedData,
    const uint32 ui32Size)
{
    int rc;
    NetSensorRecipient *nr;
    if (_pPMSMutex.lock() == NOMADSUtil::Mutex::RC_Ok) {
        _recipientList.resetGet();

        ZlibCompressionInterface zli;
        BufferWriter bw;

        // Compress the data
        if (_bUseCompression)
        {
            zli.writeCompressedBuffer(serializedData, ui32Size, &bw);
        }


        while (nr = _recipientList.getNext())
        {
            if (ui32Size < _ui32Mtu) {
                if (_bUseCompression)
                {
                    nr->pNotifierSocket->sendTo(
                        nr->notifyAddr->getIPAddress(),
                        nr->notifyAddr->getPort(),
                        bw.getBuffer(),
                        bw.getBufferLength());
                }
                else if ((rc = nr->pNotifierSocket->sendTo(
                        nr->notifyAddr->getIPAddress(), 
                        nr->notifyAddr->getPort(), 
                        serializedData, ui32Size)) < 0) {
                    checkAndLogMsg("ProtoMessageSender::sendSerializedData",
                        Logger::L_MildError, "UDP send; rc = %d\n", rc);
                    _pPMSMutex.unlock();
                    return rc;
                }
            }
            else {
                checkAndLogMsg("ProtoMessageSender::sendSerializedData",
                    Logger::L_Warning,
                    "Message too large - it is %u, out of %u\n",
                    ui32Size, _ui32Mtu);
                _pPMSMutex.unlock();
                return 0;
            }
        }
        _pPMSMutex.unlock();
    }
    return ui32Size;
}

void ProtoMessageSender::sendTopology(NetSensorContainer *nsc)
{
    static const char* meName = "NetSensor - ProtoMessageSender::sendTopology";
    uint32 ui32size = 0;
    if ((ui32size = nsc->ByteSize()) < _ui32Mtu) {
        sendTopologyFragment(nsc);
    }
    else {
        checkAndLogMsg(meName, Logger::L_MediumDetailDebug, "Spliting packet(%uB)\n", ui32size);
        splitAndSendTopologyPacket(nsc);
    }
}

void ProtoMessageSender::sendTopologyFragment(NetSensorContainer *newC)
{
    static const char* meName = "ProtoMessageSender::sendTopologyFragment";
    uint32 ui32Size = newC->ByteSize();
    if (ui32Size > 0) {
        char * pcBuffer = new char[ui32Size];
        if (!pcBuffer) {
            checkAndLogMsg(meName, Logger::L_SevereError,
                           "Memory allocation of %uB failed!\n", ui32Size);
            return;
        }

        newC->SerializeToArray(pcBuffer, ui32Size);
        checkAndLogMsg(meName, Logger::L_MediumDetailDebug, "Sending: %uB\n", ui32Size);
        sendSerializedData(pcBuffer, ui32Size);
        delete[] pcBuffer;
    }
}

int ProtoMessageSender::splitAndSendTopologyPacket(NetSensorContainer *oldCont)
{
    for (int topIndex = 0; topIndex < oldCont->topologies_size(); topIndex++) {
        NetSensorContainer newCont;
        initContainer(&newCont, &oldCont->timestamp(), TOPOLOGY);

        const Topology oldTop = oldCont->topologies(topIndex);
        Topology *newTopology = newCont.add_topologies();
        initTopologyEl(newTopology, &oldTop);

        //Internals
        for (int index = 0; index < 2; index++) {
            int elNum = 0;

            (index == 0) ? elNum = oldTop.internals_size() :
                elNum = oldTop.localgws_size();

            for (int hostIndex = 0; hostIndex < elNum; hostIndex++) {
                Host oldHost;
                (index == 0) ? oldHost = oldTop.internals(hostIndex) :
                    oldHost = oldTop.localgws(hostIndex);;
                if ((newCont.ByteSize() + oldHost.ByteSize()) < _ui32MaxPacketSize) {
                    (index == 0) ?
                        newTopology->add_internals()->CopyFrom(oldHost) :
                        newTopology->add_localgws()->CopyFrom(oldHost);
                }
                else {
                    sendTopologyFragment(&newCont);
                    if (index == 0) {
                        newTopology->clear_internals();
                        newTopology->add_internals()->CopyFrom(oldHost);
                    }
                    else {
                        newTopology->clear_localgws();
                        newTopology->add_localgws()->CopyFrom(oldHost);
                    }
                }
            }

            if (newCont.ByteSize() > 0) {
                if ((newTopology->internals_size() > 0) ||
                    (newTopology->localgws_size() > 0)) {
                    sendTopologyFragment(&newCont);
                    newTopology->clear_internals();
                }
            }
        }
    }
    return 0;
}

void ProtoMessageSender::sendTrafficFragment(NetSensorContainer *newC)
{
    //printf("\n\n %s\n", newC->ShortDebugString().c_str());

    //newC->PrintDebugString();
    //std::fstream myfile;
    //myfile.open("test.txt", std::ios::app);
    //myfile << newC->ShortDebugString() << "\n";
    //myfile.close();

    static const char * meName = "ProtoMessageSender::sendTrafficFragment";
    uint32 ui32Size = newC->ByteSize();
    if (ui32Size > 0) {
        char * pcBuffer = new char[ui32Size];
        if (!pcBuffer) {
            checkAndLogMsg(meName, Logger::L_SevereError,
                           "Memory allocation of %uB failed!\n", ui32Size);
            return;
        }

        //printf("%d\n", ui32Size);
        newC->SerializeToArray(pcBuffer, ui32Size);
        checkAndLogMsg(meName, Logger::L_MediumDetailDebug, "Sending: %uB\n", ui32Size);
        sendSerializedData(pcBuffer, ui32Size);
        delete[] pcBuffer;
    }
}

int ProtoMessageSender::splitAndSendTrafficPacket(NetSensorContainer *oldCont)
{
    //DEBUG - Print content pre splitting
    //std::ofstream first;
    //first.open("pre-splitting.txt");
    //first.clear();
    //first << oldContainer->DebugString();

    for (int trIndex = 0; trIndex < oldCont->trafficbyinterfaces_size(); trIndex++) {

        NetSensorContainer newContainer;
        initContainer(&newContainer, &oldCont->timestamp(), TRAFFIC);

        TrafficByInterface oldTr = oldCont->trafficbyinterfaces(trIndex);
        TrafficByInterface *newTraffic = newContainer.add_trafficbyinterfaces();
        initTrafficEl(newTraffic, &oldTr);

    //DEBUG - Print Fragments Initialization
        //std::ofstream fragment;
        //fragment.open("fragment.txt");
        //fragment.clear();

        for (int mflwIndex = 0; mflwIndex < oldTr.microflows_size(); mflwIndex++) {
            Microflow oldFlw;
            oldFlw = oldTr.microflows(mflwIndex);

            if ((newContainer.ByteSize() + oldFlw.ByteSize()) < _ui32MaxPacketSize) {
                newTraffic->add_microflows()->CopyFrom(oldFlw);
            }
            else {

                Microflow *newFlow = newTraffic->add_microflows();
                initMicroflow(newFlow, &oldFlw);

                for (int statIndex = 0; statIndex < oldFlw.stats_size(); statIndex++) {
                    Stat oldStat = oldFlw.stats(statIndex);

                    if ((newContainer.ByteSize() + oldStat.ByteSize()) < _ui32MaxPacketSize) {
                        newFlow->add_stats()->CopyFrom(oldStat);
                    }
                    else {
                        Stat *newStat = newFlow->add_stats();
                        initStat(newStat, &oldStat);

                        for (int averageIndex = 0; averageIndex < oldStat.averages_size(); averageIndex++) {
                            Average oldAvg = oldStat.averages(averageIndex);

                            if ((newContainer.ByteSize() + oldAvg.ByteSize()) < _ui32MaxPacketSize) {
                                newStat->add_averages()->CopyFrom(oldAvg);
                            }
                            else {
                                //fragment << newContainer.DebugString();
                                sendTrafficFragment(&newContainer);
                                newTraffic->clear_microflows();
                                newFlow = newTraffic->add_microflows();
                                initMicroflow(newFlow, &oldFlw);
                                newStat = newFlow->add_stats();
                                newStat->add_averages()->CopyFrom(oldAvg);
                            }
                        }
                    }
                }
            }
        }
        //if (newContainer.ByteSize() > 0);
        //fragment << newContainer.DebugString();
        //sendTrafficFragment(&newContainer);
    }
    return 0;
}

int ProtoMessageSender::splitAndSendTrafficPacket2(NetSensorContainer *oldCont)
{
    for (int interfaceIndex = 0; interfaceIndex < oldCont->trafficbyinterfaces_size(); interfaceIndex++) {

        NetSensorContainer newC;
        initContainer(&newC, &oldCont->timestamp(), TRAFFIC);
        TrafficByInterface oldTr = oldCont->trafficbyinterfaces(interfaceIndex);
        TrafficByInterface *pNTr = newC.add_trafficbyinterfaces();
        initTrafficEl(pNTr, &oldTr);

        for (int flwIndex = 0; flwIndex < oldTr.microflows_size(); flwIndex++) {
            Microflow oldFlw;
            oldFlw = oldTr.microflows(flwIndex);

            if ((newC.ByteSize() + oldFlw.ByteSize()) < _ui32MaxPacketSize) {
                pNTr->add_microflows()->CopyFrom(oldFlw);
            }
            else {
                sendTrafficFragment(&newC);
                pNTr->clear_microflows();

                Microflow *newFlow;
                newFlow = pNTr->add_microflows();
                initMicroflow(newFlow, &oldFlw);

                for (int statIndex = 0; statIndex < oldFlw.stats_size(); statIndex++) {
                    Stat oldStat = oldFlw.stats(statIndex);

                    if(newC.ByteSize() + oldStat.ByteSize() < _ui32MaxPacketSize) {
                        if (newFlow == NULL) {
                            newFlow = pNTr->add_microflows();
                            initMicroflow(newFlow, &oldFlw);
                        }
                        newFlow->add_stats()->CopyFrom(oldStat);
                    }
                    else {
                        sendTrafficFragment(&newC);
                        pNTr->clear_microflows();
                    }
                }
            }
        }
        if (pNTr->microflows_size() > 0) {
            sendTrafficFragment(&newC);
        }
    }
    return 0;
}

void ProtoMessageSender::sendTraffic(NetSensorContainer *nsc)
{
    const char* meName = "ProtoMessageSender::sendTraffic";
    uint32 ui32size;
    if ((ui32size = nsc->ByteSize()) < _ui32Mtu) {
        sendTrafficFragment(nsc);
    }
    else {
        checkAndLogMsg(
            meName,
            Logger::L_MediumDetailDebug,
            "Splitting packet(%uB)\n", ui32size);
        splitAndSendTrafficPacket2(nsc);
    }
}

void ProtoMessageSender::sendICMP(NetSensorContainer *pNsc)
{
    const char* meName = "ProtoMessageSender::sendTraffic";
    uint32 size;

    if (pNsc == NULL)
    {
        checkAndLogMsg(meName, Logger::L_SevereError, "NetSensorContainer param is null");
    }

    if ((size = pNsc->ByteSize()) < _ui32Mtu)
    {
        sendICMPFragment(pNsc);
    }
    else {
        checkAndLogMsg(meName, Logger::L_MediumDetailDebug, "Splitting packet(%dB)\n", size);
        splitAndSendICMPPacket(pNsc);
    }
}

void ProtoMessageSender::sendICMPFragment(NetSensorContainer *pNewC)
{
    const char* meName = "ProtoMessageSender::sendTrafficFragment";
    if (pNewC == NULL)
    {
        checkAndLogMsg(meName, Logger::L_SevereError, "NetSensorContainer param is null");
        return;
    }
    uint32 ui32Size = pNewC->ByteSize();
    if (ui32Size > 0) {
        char * pBuffer = new char[ui32Size];
        pNewC->SerializeToArray(pBuffer, ui32Size);
        checkAndLogMsg(meName, Logger::L_MediumDetailDebug, "Sending: %dB\n", ui32Size);
        sendSerializedData(pBuffer, ui32Size);
        delete[] pBuffer;
    }
}

int ProtoMessageSender::splitAndSendICMPPacket(NetSensorContainer *pOldCont)
{
    const char *meName = "HandlerThread::splitAndSendICMPPacket";
    if (pOldCont == NULL)
    {
        checkAndLogMsg(meName, Logger::L_SevereError, "NetSensorContainer param is null");
    }
    // Loop through the interfaces that have containers

    for (int interfaceIndex = 0; interfaceIndex < pOldCont->icmpinfo_size(); interfaceIndex++) {
        NetSensorContainer newNSC;
        initContainer(&newNSC, &pOldCont->timestamp(), ICMP);
        ICMPPacketsByInterface oldIPBI = pOldCont->icmpinfo(interfaceIndex);
        ICMPPacketsByInterface *pNIPBI = newNSC.add_icmpinfo();

        pNIPBI->set_monitoringinterface(oldIPBI.monitoringinterface());

        // Loop through the containers

        for (int contIndex = 0; contIndex < oldIPBI.icmpcontainers_size(); contIndex++) {
            ProtoICMPInfoContainer oldContainer;
            oldContainer = oldIPBI.icmpcontainers(contIndex);

            int netSensorCont = newNSC.ByteSize();
            int icmpContainer = oldContainer.ByteSize();

            // If container won't be full, add one in
            if ((newNSC.ByteSize() + oldContainer.ByteSize()) < _ui32MaxPacketSize) {
                pNIPBI->add_icmpcontainers()->CopyFrom(oldContainer);
            }
            else {
                // If the container will be full, send the fragment
                // If the container can't be split by extra addresses, send and start over

                if (oldContainer.icmppayload().extraaddresses_size() == 0)
                {
                    sendICMPFragment(&newNSC);
                    pNIPBI->clear_icmpcontainers(); // Clear the containers of this interface
                    // Because there's no extra addresses, the size of the container will never
                    // be larger than the allowed size
                    netSensorCont = newNSC.ByteSize();
                    icmpContainer = oldContainer.ByteSize();
                    pNIPBI->add_icmpcontainers()->CopyFrom(oldContainer);
                }
                else
                {
                    // Split up the container into multiple with extra addresses

                    sendICMPFragment(&newNSC);
                    pNIPBI->clear_icmpcontainers(); // Clear the containers of this interface
                    netSensorCont = newNSC.ByteSize();
                    icmpContainer = oldContainer.ByteSize();
                    // Create an empty container with no data in it so
                    // The addresses can get split up
                    ProtoICMPInfoContainer *pNIIC = pNIPBI->add_icmpcontainers();
                    ProtoData oldData   = oldContainer.icmppayload();
                    ProtoData *pNewData = new ProtoData();

                    pNewData->CopyFrom(oldData);
                    pNIIC->set_allocated_icmppayload(pNewData);
                    pNewData->clear_extraaddresses();

                    initICMPContainer(pNIIC, &oldContainer);  // Copy data from old to new

                    for (int extraAddrCounter = 0; extraAddrCounter < oldData.extraaddresses_size(); extraAddrCounter++) {
                        ProtoExtraAddresses oldAddress = oldData.extraaddresses(extraAddrCounter);

                        if ((newNSC.ByteSize() + oldAddress.ByteSize()) < _ui32MaxPacketSize) {
                            pNewData->add_extraaddresses()->CopyFrom(oldAddress);
                        }
                        else {
                            netSensorCont = newNSC.ByteSize();
                            icmpContainer = oldContainer.ByteSize();


                            // Send the container and clear
                            sendICMPFragment(&newNSC);
                            pNIPBI->clear_icmpcontainers();

                            // Start with fresh data
                            pNIIC = pNIPBI->add_icmpcontainers();
                            initICMPContainer(pNIIC, &oldContainer);

                            pNewData = new ProtoData();
                            pNewData->CopyFrom(oldData);
                            pNIIC->set_allocated_icmppayload(pNewData);


                            pNewData->clear_extraaddresses();
                            pNewData->add_extraaddresses()->CopyFrom(oldAddress);

                            netSensorCont = newNSC.ByteSize();
                            icmpContainer = oldContainer.ByteSize();
                        }
                    }
                }
            }
        }

        if (pNIPBI->icmpcontainers_size() > 0) {
            sendICMPFragment(&newNSC);
        }
    }
    return 0;
}

// Copy the old container to the new one
void ProtoMessageSender::initICMPContainer(ProtoICMPInfoContainer *to, const ProtoICMPInfoContainer *from)
{
    google::protobuf::Timestamp *newTS = new google::protobuf::Timestamp();
    newTS->CopyFrom(from->timestamp());
    to->set_sourcemac   (from->sourcemac());
    to->set_destmac     (from->destmac());
    to->set_sourceipaddr(from->sourceipaddr());
    to->set_destipaddr  (from->destipaddr());
    to->set_type        (from->type());
    to->set_code        (from->code());
    to->set_count       (from->count());
    to->set_allocated_timestamp(newTS);
}

void ProtoMessageSender::initRequiredProtoICMPData(ProtoData *to, const ProtoData *from)
{
    ProtoIpHeader     ipHead;
    ProtoDatagramInfo dGI;
    ProtoIdentification id;
    ProtoICMPTime       ICMPTime;
    if (from->has_dgram())
    {
        dGI = from->dgram();
        to->set_allocated_dgram(&dGI);
    }

    if (from->has_ipheader())
    {
        ipHead = from->ipheader();
        to->set_allocated_ipheader(&ipHead);
    }

    if (from->has_id())
    {
        id = from->id();
        to->set_allocated_id(&id);
    }

    if (from->has_icmptimestamp())
    {
        ICMPTime = from->icmptimestamp();
        to->set_allocated_icmptimestamp(&ICMPTime);
    }
}

void ProtoMessageSender::configureToUseCompression()
{
    _bUseCompression = true;
}

}
