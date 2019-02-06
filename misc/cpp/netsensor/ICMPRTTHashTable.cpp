/*
* ICMPRTTHashTable.cpp
* Author: bordway@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2018 IHMC.
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
*/

#include "ICMPRTTHashTable.h"
#include "Logger.h"

//std
#include <tuple>
#include <forward_list>

using namespace NOMADSUtil;
using namespace IHMC_NETSENSOR_NET_UTILS;
#define checkAndLogMsg if (pLogger) pLogger->logMsg
namespace IHMC_NETSENSOR 
{
    ICMPRTTHashTable::ICMPRTTHashTable (uint32 ui32Resolution)
        : 
        _rttHashTable   (true, true, true, true),
        _ui32Resolution (ui32Resolution)
    {
    }

    ICMPRTTHashTable::~ICMPRTTHashTable (void)
    {
    }

    uint32 ICMPRTTHashTable::cleanTable (const uint32 ui32CleaningNumber)
    {
        uint32 ui32CleaningCounter = 0;
        std::forward_list< std::tuple<String, String>> list;
        /*String destlevelCleaner[C_MAX_CLEANING_NUMBER];
        String srcLevelCleaner[C_MAX_CLEANING_NUMBER];*/

        if ((_mutex.lock() == Mutex::RC_Ok)) {
            for (StringHashtable<StringHashtable<ICMPRTTList>>::Iterator ii = _rttHashTable.getAllElements();
                !ii.end(); ii.nextElement())
            {
                StringHashtable<ICMPRTTList> *pDestLevel = ii.getValue();
                if (pDestLevel == nullptr) {
                    continue;
                }

                for (StringHashtable<ICMPRTTList>::Iterator i = pDestLevel->getAllElements();
                    !i.end(); i.nextElement()) {
                    ICMPRTTList *pRttList = i.getValue();

                    if (pRttList != nullptr) {
                        ui32CleaningCounter += pRttList->clean(ui32CleaningNumber - ui32CleaningCounter);
                        if (pRttList->getCount() == 0) {
                            /*destlevelCleaner[ui32CleaningCounter] = i.getKey();
                            srcLevelCleaner[ui32CleaningCounter] = ii.getKey();*/
                            list.emplace_front(i.getKey(), ii.getKey());
                            ui32CleaningCounter++;
                        }
                    }
                }
            }
            for (auto iter = list.begin(); iter != list.end(); iter++) {
                StringHashtable<ICMPRTTList> *i = _rttHashTable.get (std::get<1> (*iter));   
                delete i->remove (std::get<0> (*iter));
                if (i->getCount() == 0) {
                    delete _rttHashTable.remove (std::get<1> (*iter));
                }
            }
            /*
            for (uint32 counter = 0; counter < ui32CleaningCounter; counter++)
            {
                StringHashtable<ICMPRTTList> *i = _rttHashTable.get(srcLevelCleaner[counter]);

                delete (i->remove(destlevelCleaner[counter]));
                if (i->getCount() == 0) {
                    delete _rttHashTable.remove(srcLevelCleaner[counter]);
                }

            }
            */
            _mutex.unlock();
            return ui32CleaningCounter;
        }

        return 0;
    }

    void ICMPRTTHashTable::print (void)
    {
        if (_mutex.lock() == Mutex::RC_Ok) {
            printf ("ICMP RTT\n");

            for (auto i = _rttHashTable.getAllElements(); !i.end(); i.nextElement()) {
                StringHashtable<ICMPRTTList> * pDestLevel = i.getValue();

                for (auto ii = pDestLevel->getAllElements(); !ii.end(); ii.nextElement()) {
                    ICMPRTTList *pRttList = ii.getValue();

                    if (pRttList != nullptr) {
                        printf("%15s to %15s\n----------------------\n", i.getKey(), ii.getKey());
                        pRttList->print();
                    }
                }
            }
            _mutex.unlock();
        }
    }

    void ICMPRTTHashTable::put (
        NS_ICMPPacket * pIcmpPacket, 
        bool isSent, 
        int64 timestamp)
    {
        String tmpSrcIp;
        String tmpDestIp;

        tmpSrcIp    = InetAddr (pIcmpPacket->pIpHeader->srcAddr.ui32Addr).getIPAsString();
        tmpDestIp   = InetAddr (pIcmpPacket->pIpHeader->destAddr.ui32Addr).getIPAsString();
        
        uint16 seqNum = pIcmpPacket->pIcmpData->ui16RoHWord2;

        if (!isValidTypeForRTT (pIcmpPacket->pIcmpData->ui8Type)) {
            return;
        }

        if (_mutex.lock() == Mutex::RC_Ok) {
            if (isSent) {
                if (!(pIcmpPacket->pIcmpData->ui8Type == ICMPHeader::Type::T_Echo_Request)) {
                    _mutex.unlock();
                    return;
                }

                auto pRemoteLevel = _rttHashTable.get (tmpSrcIp);
                if (pRemoteLevel == nullptr) {
                    pRemoteLevel = new StringHashtable<ICMPRTTList>();

                    ICMPRTTList * pRttList = new ICMPRTTList (_ui32Resolution);
                    
                    pRttList->addSentTime (seqNum, timestamp);
                    pRemoteLevel->put (tmpDestIp, pRttList);
                    _rttHashTable.put (tmpSrcIp, pRemoteLevel);
                }
                else {
                    ICMPRTTList * pRTTList = pRemoteLevel->get (tmpDestIp);
                    if (pRTTList == nullptr) {
                        pRTTList = new ICMPRTTList (_ui32Resolution);
                        pRemoteLevel->put (tmpDestIp, pRTTList);
                    }
                    pRTTList->addSentTime (seqNum, timestamp);
                    
                }
            }
            else {
                if (!(pIcmpPacket->pIcmpData->ui8Type == ICMPHeader::Type::T_Echo_Reply)) {
                    _mutex.unlock();
                    return;
                }

                // This is a received reply, check if we have sent one already
                StringHashtable<ICMPRTTList> * pRemoteLevel = _rttHashTable.get (tmpDestIp);
                if (pRemoteLevel == nullptr) {
                    _mutex.unlock();
                    return;
                }
                else {
                    ICMPRTTList *pRTTList = pRemoteLevel->get(tmpSrcIp);
                    if (pRTTList == nullptr) {
                        _mutex.unlock();
                        return;
                    }

                    pRTTList->addReceivedTime (seqNum, timestamp);
                    // get avg will be changed
                }
            }
            _mutex.unlock();
        }
    }

    PtrLList<measure::Measure>* ICMPRTTHashTable::createMeasures(String sSensorIP)
    {
        PtrLList<measure::Measure>* pMeasureList = new PtrLList<measure::Measure>(true);
        ProtobufWrapper protobufWrapper;
        if (_mutex.lock() == NOMADSUtil::Mutex::RC_Ok)
        {
            for (StringHashtable<StringHashtable<ICMPRTTList>>::Iterator ii = _rttHashTable.getAllElements();
                !ii.end(); ii.nextElement())
            {
                StringHashtable<ICMPRTTList> *pDestLevel = ii.getValue();
                if (pDestLevel == nullptr) {
                    continue;
                }

                for (StringHashtable<ICMPRTTList>::Iterator i = pDestLevel->getAllElements();
                    !i.end(); i.nextElement()) {
                    ICMPRTTList *pRttList = i.getValue();

                    float  avgRTT = pRttList->getAverageRTT();
                    uint32 minRTT = pRttList->getMinRTT();
                    uint32 maxRTT = pRttList->getMaxRTT();
                    uint32 mostRecentRTT = pRttList->getMostRecentRTT();

                    measure::Measure *pMeasure = protobufWrapper.getMeasureRTT(sSensorIP, ii.getKey(), i.getKey(),
                        "ICMP", "0", "0", minRTT, maxRTT, mostRecentRTT, _ui32Resolution, avgRTT);
                    pMeasureList->append(pMeasure);
                }
            }

            _mutex.unlock();
            return pMeasureList;
        }
    }

    bool ICMPRTTHashTable::isValidTypeForRTT(uint8 type)
    {
        return type == ICMPHeader::Type::T_Echo_Reply || type == ICMPHeader::Type::T_Echo_Request;
    }
}
