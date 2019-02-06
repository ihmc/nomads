/*
* ICMPHashTable.cpp
* Author: bordway@ihmc.us rfronteddu@ihmc.us
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
* This Class implements a four levels hash map that will contain
* stats identified by source ip, dest ip, type, and code
*/
#include"ICMPHashTable.h"
#include<exception>

//std
#include <tuple>
#include <forward_list>

#define checkAndLogMsg if (pLogger) pLogger->logMsg
using namespace NOMADSUtil;
using namespace IHMC_NETSENSOR_NET_UTILS;
using namespace netsensor;
namespace IHMC_NETSENSOR
{
    // Constructor, configure hashtable settings
    ICMPHashTable::ICMPHashTable (void)
    {
        _icmpHashTable.configure (true, true, true, true);
    }

    ICMPHashTable::~ICMPHashTable (void)
    {
    }

    void ICMPHashTable::fillICMPProto (ICMPPacketsByInterface * pIpbi)
    {
        if (_pTMutex.lock() == Mutex::RC_Ok) {
            for (auto iv = _icmpHashTable.getAllElements(); !iv.end(); iv.nextElement()) {
                String sSrcAddr = iv.getKey();
                destAddrLevel * l3 = iv.getValue();
                for (auto iii = l3->getAllElements(); !iii.end(); iii.nextElement()) {
                    String sDestAddr = iii.getKey();
                    ProtoICMPInfoContainer * pIIC = new ProtoICMPInfoContainer();

                    typeLevel *l2 = iii.getValue();
                    for (auto ii = l2->getAllElements(); !ii.end(); ii.nextElement()) {
                        codeLevel *l1 = ii.getValue();
                        for (auto i = l1->getAllElements(); !i.end(); i.nextElement()) {
                            ICMPTypesContainer * pItc = i.getValue();
                            NS_ICMPPacket * tempPacket = pItc->pContainerIcmpPacket;
                            
                            String sSrcMac  = buildStringFromEthernetMACAddress (tempPacket->pMacHeader->src);
                            String sDestMac = buildStringFromEthernetMACAddress (tempPacket->pMacHeader->dest);
                            pIIC->set_sourcemac (sSrcMac);
                            pIIC->set_destmac (sDestMac);
                            pIIC->set_sourceipaddr (sSrcAddr);
                            pIIC->set_destipaddr (sDestAddr);
                            pIIC->set_type ((uint32) tempPacket->pIcmpData->ui8Type);
                            pIIC->set_code ((uint32)tempPacket->pIcmpData->ui8Code);
                            pIIC->set_count (pItc->getCount());

                            ProtoIpHeader       * pIPHead;
                            ProtoICMPTime       * pTime;
                            ProtoIdentification * pID;
                            ProtoDatagramInfo   * pDI;

                            ProtoData * pData = new ProtoData();
                            auto pTimestamp = new google::protobuf::Timestamp;

                            setProtobufTimestamp (*pTimestamp);

                            if (!pItc->isExpired()) {
                                // Set protobuf values based on packet type
                                switch (tempPacket->pIcmpData->ui8Type)
                                {
                                case (NS_ICMP::T_Echo_Reply):
                                case (NS_ICMP::T_Echo_Request):
                                {
                                    pID = new ProtoIdentification();
                                    pID->set_identifier (tempPacket->pIcmpData->ui16RoHWord1);
                                    pID->set_sequencenumber (tempPacket->pIcmpData->ui16RoHWord2);
                                    pData->set_allocated_id (pID);
                                    break;
                                }
                                case (NS_ICMP::T_Destination_Unreachable):
                                {
                                    pDI = new ProtoDatagramInfo();
                                    pIPHead = new ProtoIpHeader();
                                    fillProtoDatagram (tempPacket, pDI);
                                    fillProtoHead (tempPacket, pIPHead);
                                    pData->set_allocated_ipheader (pIPHead);
                                    pData->set_allocated_dgram (pDI);
                                    break;
                                }
                                case (NS_ICMP::T_Source_Quench):
                                {
                                    pDI = new ProtoDatagramInfo();
                                    pIPHead = new ProtoIpHeader();
                                    fillProtoDatagram (tempPacket, pDI);
                                    fillProtoHead (tempPacket, pIPHead);
                                    pData->set_allocated_ipheader (pIPHead);
                                    pData->set_allocated_dgram (pDI);
                                    break;
                                }
                                case (NS_ICMP::T_Redirect_Message):
                                {
                                    pDI = new ProtoDatagramInfo();
                                    pIPHead = new ProtoIpHeader();
                                    fillProtoDatagram (tempPacket, pDI);
                                    fillProtoHead (tempPacket, pIPHead);
                                    pData->set_allocated_ipheader (pIPHead);
                                    pData->set_allocated_dgram (pDI);
                                    break;
                                }

                                case (NS_ICMP::T_Time_Exceeded):
                                {
                                    pDI = new ProtoDatagramInfo();
                                    pIPHead = new ProtoIpHeader();
                                    fillProtoDatagram (tempPacket, pDI);
                                    fillProtoHead (tempPacket, pIPHead);
                                    pData->set_allocated_ipheader (pIPHead);
                                    pData->set_allocated_dgram (pDI);
                                    break;
                                }
                                case (NS_ICMP::T_Parameter_Problem):
                                {
                                    pDI = new ProtoDatagramInfo();
                                    pIPHead = new ProtoIpHeader();
                                    fillProtoDatagram (tempPacket, pDI);
                                    fillProtoHead (tempPacket, pIPHead);
                                    pData->set_pointertoerror ((uint32)tempPacket->pIcmpData->ui8RoHByte1);
                                    pData->set_allocated_ipheader (pIPHead);
                                    pData->set_allocated_dgram (pDI);
                                    break;
                                }
                                case (NS_ICMP::T_Timestamp_Request):
                                case (NS_ICMP::T_Timestamp_Reply):
                                {
                                    pID = new ProtoIdentification();
                                    pTime = new ProtoICMPTime();
                                    pID->set_identifier (tempPacket->pIcmpData->ui16RoHWord1);
                                    pID->set_sequencenumber (tempPacket->pIcmpData->ui16RoHWord2);
                                    pTime->set_originatetimestamp (tempPacket->pIcmpData->originateTimestamp);
                                    pTime->set_receivetimestamp (tempPacket->pIcmpData->receiveTimestamp);
                                    pTime->set_transmittimestamp (tempPacket->pIcmpData->transmitTimestamp);
                                    pData->set_allocated_icmptimestamp (pTime);
                                    pData->set_allocated_id (pID);
                                    break;
                                }
                                case (NS_ICMP::T_Information_Request):
                                case (NS_ICMP::T_Information_Reply):
                                {
                                    pID = new ProtoIdentification();
                                    pID->set_identifier (tempPacket->pIcmpData->ui16RoHWord1);
                                    pID->set_sequencenumber (tempPacket->pIcmpData->ui16RoHWord2);
                                    pData->set_allocated_id (pID);
                                    break;
                                }
                                case (NS_ICMP::T_Address_Mask_Request):
                                case (NS_ICMP::T_Address_Mask_Reply):
                                {
                                    pID = new ProtoIdentification();
                                    pID->set_identifier (tempPacket->pIcmpData->ui16RoHWord1);
                                    pID->set_sequencenumber (tempPacket->pIcmpData->ui16RoHWord2);
                                    pData->set_allocated_id (pID);
                                    break;
                                }
                                default:
                                {
                                    auto pszMethodName = "ICMPHashTable::fillICMPProto";
                                    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Unknown ICMP type: %d\n", tempPacket->pIcmpData->ui8Type);
                                    break;
                                }
                                }

                                // If type = addr mask or redirect, fill ProtoExtraAddresses
                                if (pItc->hasExtraAddresses()) {
                                    for (auto address = pItc->extraIPAddresses.getAllElements(); !address.end(); address.nextElement())
                                    {
                                        ProtoExtraAddresses *pEA = pData->add_extraaddresses();
                                        pEA->set_timesrepeated(*address.getValue()); 
                                        pEA->set_ipaddress(address.getKey());
                                    }
                                }

                                // Set the payload and the timestamp of the container
                                pIIC->set_allocated_icmppayload (pData);
                                pIIC->set_allocated_timestamp (pTimestamp);

                                ProtoICMPInfoContainer * pNewIIC = pIpbi->add_icmpcontainers();
                                pNewIIC->CopyFrom (*pIIC);
                            }
                        }
                    }
                    delete pIIC;
                }
            }
            _pTMutex.unlock();
        }
    }

    // Set protobuf values for the IP header of the original datagram
    void ICMPHashTable::fillProtoHead (
        NS_ICMPPacket * pPacket, 
        ProtoIpHeader * pIPHead)
    {
        pIPHead->set_tos ((uint32)pPacket->pIpHeader->ui8TOS);
        pIPHead->set_length ((uint32)pPacket->pIpHeader->ui16TLen);
        pIPHead->set_protocol ((uint32)pPacket->pIpHeader->ui8Proto);
        pIPHead->set_origsourceipaddr (InetAddr (pPacket->pIpHeader->srcAddr.ui32Addr).getIPAsString());
        pIPHead->set_origdestipaddr (InetAddr (pPacket->pIpHeader->destAddr.ui32Addr).getIPAsString());
    }

    // Set protobuf values for the original datagram
    void ICMPHashTable::fillProtoDatagram (
        NS_ICMPPacket * pPacket, 
        ProtoDatagramInfo * pDI)
    {
        pDI->set_sourceport ((uint32)pPacket->pIcmpData->tcpHeader.ui16SPort);
        pDI->set_destport ((uint32)pPacket->pIcmpData->tcpHeader.ui16DPort);
    }

    // Clean the table
    uint32 ICMPHashTable::cleanTable (const uint32 ui32MaxCleaningNumber)
    {
        
        std::forward_list <std::tuple <String, String, String, String>> list;

        //String k1[C_MAX_CLEANING_NUMBER];
        //String k2[C_MAX_CLEANING_NUMBER];
        //String k3[C_MAX_CLEANING_NUMBER];
        //String k4[C_MAX_CLEANING_NUMBER];

        bool bKeepCleaning = true;
        uint32 ui32CleaningCounter = 0;

        if (_pTMutex.lock() == Mutex::RC_Ok) {
            for (auto iv = _icmpHashTable.getAllElements(); !iv.end() && bKeepCleaning; iv.nextElement()) {
                destAddrLevel *pL3 = iv.getValue();

                for (auto iii = pL3->getAllElements(); (!iii.end() && bKeepCleaning); iii.nextElement())
                {
                    typeLevel * pL2 = iii.getValue();

                    for (auto ii = pL2->getAllElements(); (!ii.end() && bKeepCleaning); ii.nextElement())
                    {
                        codeLevel * pL1 = ii.getValue();

                        for (auto i = pL1->getAllElements(); (!i.end() && bKeepCleaning); i.nextElement()) {
                            //ICMPTypesContainer * tempInfo = i.getValue();

                            if (i.getValue()->isExpired()) {
                                list.emplace_front (i.getKey(), ii.getKey(), iii.getKey(), iv.getKey());
                                /*
                                k1[ui32CleaningCounter] = i.getKey();
                                k2[ui32CleaningCounter] = ii.getKey();
                                k3[ui32CleaningCounter] = iii.getKey();
                                k4[ui32CleaningCounter] = iv.getKey();
                                */
                                
                                ui32CleaningCounter++;
                                bKeepCleaning = (ui32CleaningCounter < ui32MaxCleaningNumber);
                            }
                        }
                    }
                }
            }
            for (auto iter = list.begin(); iter != list.end(); iter++) {
                destAddrLevel   * iii = _icmpHashTable.get (std::get<3> (*iter));
                typeLevel       * ii = iii->get (std::get<2> (*iter));
                codeLevel       * i = ii->get (std::get<1> (*iter));
                delete i->remove (std::get<0> (*iter));
                if (i->getCount() == 0) {
                    delete ii->remove (std::get<1> (*iter));
                    if (ii->getCount() == 0) {
                        delete iii->remove (std::get<2> (*iter));
                        if (iii->getCount() == 0) {
                            delete _icmpHashTable.remove (std::get<3> (*iter));
                        }
                    }
                }
            }
            /*for (uint32 counter = 0; counter < ui32CleaningCounter; counter++)
            {
                destAddrLevel   * iii = _icmpHashTable.get (k4[counter]);
                typeLevel       * ii = iii->get (k3[counter]);
                codeLevel       * i = ii->get (k2[counter]);

                delete i->remove (k1[counter]);
                if (i->getCount() == 0) {
                    delete ii->remove (k2[counter]);
                    if (ii->getCount() == 0) {
                        delete iii->remove (k3[counter]);
                        if (iii->getCount() == 0) {
                            delete _icmpHashTable.remove (k4[counter]);
                        }
                    }
                }
            }*/
            _pTMutex.unlock();

            return ui32CleaningCounter;
        }
    }

    // Print values from the hashtable
    void ICMPHashTable::print(void)
    {
        if ((_pTMutex.lock() == Mutex::RC_Ok)) {
            for (auto iv = _icmpHashTable.getAllElements(); !iv.end(); iv.nextElement())
            {
                destAddrLevel * l3 = iv.getValue();

                for (auto iii = l3->getAllElements(); !iii.end(); iii.nextElement()) {
                    typeLevel * l2 = iii.getValue();

                    for (auto ii = l2->getAllElements(); !ii.end(); ii.nextElement())
                    {
                        codeLevel * l1 = ii.getValue();

                        for (auto i = l1->getAllElements(); !i.end(); i.nextElement())
                        {
                            ICMPTypesContainer * info = i.getValue();

                            printf ("Src: %15s  Dest: %15s  Type: %2s Code: %2s  Packets repeated:%d\n",
                                iv.getKey(), 
                                iii.getKey(),
                                ii.getKey(), 
                                i.getKey(),
                                info->getCount());
                        }
                    }
                }
            }
            _pTMutex.unlock();
        }
    }

    // Put the icmp type container (gives details about repeated icmp packets)
    // into the table based on information given by the pICMPInfo
    void ICMPHashTable::put (ICMPTypesContainer * pICMPTypeContainer)
    {
        if (pICMPTypeContainer == nullptr) {
            printf ("ICMPHashTable::put():pIcmpInfo null\n");
            return;
        }
        NS_ICMPPacket * pTempIcmpPacket = pICMPTypeContainer->pContainerIcmpPacket;

        destAddrLevel * pL3; // DestAddr
        typeLevel   * pL2; // Type
        codeLevel   * pL1; // Code

        String tmpSrcIp;
        String tmpDestIp;
        char tmpType[8];
        char tmpCode[8];

        tmpSrcIp = InetAddr (pTempIcmpPacket->pIpHeader->srcAddr.ui32Addr)
            .getIPAsString();
        tmpDestIp = InetAddr (pTempIcmpPacket->pIpHeader->destAddr.ui32Addr)
            .getIPAsString();
        convertIntToString (pICMPTypeContainer->getUi8Type(), tmpType);
        convertIntToString (pICMPTypeContainer->getUi8Code(), tmpCode);

        if ((_pTMutex.lock() == Mutex::RC_Ok)) {
            // Get hashtable based on source addr
            pL3 = _icmpHashTable.get (tmpSrcIp);
            if (pL3 == nullptr) {
                pL3 = new destAddrLevel (true, true, true, true);
                pL2 = new typeLevel (true, true, true, true);
                pL1 = new codeLevel (true, true, true, true);

                _icmpHashTable.put (tmpSrcIp, pL3);
                pL3->put (tmpDestIp, pL2);
                pL2->put (tmpType, pL1);
                pL1->put (tmpCode, pICMPTypeContainer);
            }
            else {
                // Get hashtable based on dest addr
                pL2 = pL3->get (tmpDestIp);
                if (pL2 == nullptr) {
                    pL2 = new typeLevel (true, true, true, true);
                    pL1 = new codeLevel (true, true, true, true);

                    pL3->put (tmpDestIp, pL2);
                    pL2->put (tmpType, pL1);
                    pL1->put (tmpCode, pICMPTypeContainer);
                }
                else {
                    // Get hashtable based on ICMP packet Type
                    pL1 = pL2->get (tmpType);
                    if (pL1 == nullptr) {
                        // Make new ICMPInfo type and put into table
                        pL1 = new codeLevel (true, true, true, true);
                        pL1->put (tmpCode, pICMPTypeContainer);
                        pL2->put (tmpType, pL1);
                    }
                }
            }
            pICMPTypeContainer->incrementCount();
            
            _pTMutex.unlock();
        }
    }

    // Get the object that contains information about repeated packets
    ICMPTypesContainer * ICMPHashTable::get (NS_ICMPPacket * pIcmpPacket)
    {
        destAddrLevel * l3;
        typeLevel * l2;
        codeLevel * l1;

        String tmpSrcIp;
        String tmpDestIp;
        char tmpType[8];
        char tmpCode[8];

        tmpSrcIp = InetAddr (pIcmpPacket->pIpHeader->srcAddr.ui32Addr).getIPAsString();
        tmpDestIp = InetAddr (pIcmpPacket->pIpHeader->destAddr.ui32Addr).getIPAsString();
        convertIntToString (pIcmpPacket->pIcmpData->ui8Type, tmpType);
        convertIntToString (pIcmpPacket->pIcmpData->ui8Code, tmpCode);

        if ((_pTMutex.lock() == Mutex::RC_Ok)) {
            l3 = _icmpHashTable.get (tmpSrcIp);
            if (l3 != nullptr) {
                l2 = l3->get (tmpDestIp);
                if (l2 != nullptr) {
                    l1 = l2->get (tmpType);
                    if (l1 != nullptr) {
                        _pTMutex.unlock();
                        return l1->get (tmpCode);
                    }
                }
            }
            _pTMutex.unlock();
        }
        return nullptr;
    }

    // Returns number of source addresses in the container
    uint8 ICMPHashTable::getCount (void) 
    {
        return (uint8)_icmpHashTable.getCount();
    }

}