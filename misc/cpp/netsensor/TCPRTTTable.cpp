/*
* TCPRttTable.h
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
*/

#include "TCPRTTTable.h"

using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
    TCPRTTTable::TCPRTTTable(void) :
        _tcpRTTTable(true, true, true, true)
    {
    }

    TCPRTTTable::~TCPRTTTable(void)
    {
        _tcpRTTTable.removeAll();
    }

    void TCPRTTTable::put(TCPRTTData & pTCPStruct, bool isSent)
    {
        if (isSent) {
            checkSent(pTCPStruct);
            return;
        }

        checkReceived(pTCPStruct);
        return;
    }

    void TCPRTTTable::print(void)
    {
        if (_mutex.lock() == Mutex::RC_Ok)
        {
            for (SIPLevel::Iterator iv = _tcpRTTTable.getAllElements(); !iv.end();
                iv.nextElement())
            {
                DIPLevel *l3 = iv.getValue();

                for (DIPLevel::Iterator iii = l3->getAllElements();
                    !iii.end(); iii.nextElement())
                {
                    SPLevel *l2 = iii.getValue();

                    for (SPLevel::Iterator ii = l2->getAllElements();
                        !ii.end(); ii.nextElement())
                    {
                        DPLevel *l1 = ii.getValue();

                        for (DPLevel::Iterator i = l1->getAllElements();
                            !i.end(); i.nextElement())
                        {
                            TCPRTTContainer* info = i.getValue();

                            printf("Src: %15s  Dest: %15s  Src Port: %2s  "
                                "Dest Port: %2s  Avg Rtt:%f sec\n",
                                iv.getKey(), iii.getKey(),
                                ii.getKey(), i.getKey(),
                                info->getAvgRTTinSec());
                        }
                    }
                }
            }
            _mutex.unlock();
        }
    }

    // It is currently possible for a sent packet and a received packet to have
    // the same timestamp. This means the RTT can be 0
    // --It may be worthwhile to store packets based on ACK packets
    // --Also need to store timestamps if the packet has one. That would be done
    //   in the handler thread.
    void TCPRTTTable::checkSent(TCPRTTData & pTCPStruct)
    {
        DIPLevel *pDstIPLevel   = nullptr;
        SPLevel  *pSrcPortLevel = nullptr;
        DPLevel  *pDstPortLevel = nullptr;
        TCPRTTContainer *pContainer = nullptr;

        int64 currTime = pTCPStruct.i64rcvTime;

        if (_mutex.lock() == Mutex::RC_Ok) {
            // Check if the TCP data has already been tracked
            pDstIPLevel = _tcpRTTTable.get(pTCPStruct.sSourceIP);
            if (pDstIPLevel == nullptr) {
                pDstIPLevel = new DIPLevel(true, true, true, true);
                pSrcPortLevel = new SPLevel(true, true, true, true);
                pDstPortLevel = new DPLevel(true, true, true, true);
                pContainer = new TCPRTTContainer(5000);

                _tcpRTTTable.put(pTCPStruct.sSourceIP, pDstIPLevel);
                pDstIPLevel->put(pTCPStruct.sDestIP, pSrcPortLevel);
                pSrcPortLevel->put(pTCPStruct.sSourcePort, pDstPortLevel);
                pDstPortLevel->put(pTCPStruct.sDestPort, pContainer);

                pContainer->setSentTime(currTime);
            }
            else {
                pSrcPortLevel = pDstIPLevel->get(pTCPStruct.sDestIP);

                if (pSrcPortLevel == nullptr) {
                    pSrcPortLevel = new SPLevel(true, true, true, true);
                    pDstPortLevel = new DPLevel(true, true, true, true);
                    pContainer = new TCPRTTContainer(5000);

                    pDstIPLevel->put(pTCPStruct.sDestIP, pSrcPortLevel);
                    pSrcPortLevel->put(pTCPStruct.sSourcePort, pDstPortLevel);
                    pDstPortLevel->put(pTCPStruct.sDestPort, pContainer);

                    pContainer->setSentTime(currTime);
                }
                else {
                    pDstPortLevel = pSrcPortLevel->get(pTCPStruct.sSourcePort);

                    if (pDstPortLevel == nullptr) {
                        pDstPortLevel = new DPLevel(true, true, true, true);
                        pContainer = new TCPRTTContainer(5000);

                        pSrcPortLevel->put(pTCPStruct.sSourcePort, pDstPortLevel);
                        pDstPortLevel->put(pTCPStruct.sDestPort, pContainer);

                        pContainer->setSentTime(currTime);
                    }
                    else {
                        pContainer = pDstPortLevel->get(pTCPStruct.sDestPort);
                        
                        if (pContainer == nullptr) {
                            pContainer = new TCPRTTContainer(5000);

                            pDstPortLevel->put(pTCPStruct.sDestPort, pContainer);
                            pContainer->setSentTime(currTime);
                        }
                        else {
                            if (!pContainer->hasSentTime()) {
                                pContainer->setSentTime(currTime);
                            }
                        }
                    }
                }
            }
            _mutex.unlock();
        }
    }

    void TCPRTTTable::checkReceived(TCPRTTData & pTCPStruct)
    {
        DIPLevel *pDstIPLevel = nullptr;
        SPLevel  *pSrcPortLevel = nullptr;
        DPLevel  *pDstPortLevel = nullptr;
        TCPRTTContainer *pContainer = nullptr;

        int64 currTime = pTCPStruct.i64rcvTime;

        if (_mutex.lock() == Mutex::RC_Ok) {
            pDstIPLevel = _tcpRTTTable.get(pTCPStruct.sDestIP);

            if (pDstIPLevel == nullptr) {
                _mutex.unlock();
                return;
            }

            pSrcPortLevel = pDstIPLevel->get(pTCPStruct.sSourceIP);

            if (pSrcPortLevel == nullptr) {
                _mutex.unlock();
                return;
            }

            pDstPortLevel = pSrcPortLevel->get(pTCPStruct.sDestPort);

            if (pDstPortLevel == nullptr) {
                _mutex.unlock();
                return;
            }

            pContainer = pDstPortLevel->get(pTCPStruct.sSourcePort);
            
            // Set values and calculate rtt
            if (pContainer == nullptr) {
                _mutex.unlock();
                return;
            }

            // An entry has been found for this tcp connection
            // now do redundancy checks and set values if applicable
            if (!pContainer->hasReceivedTime() && pContainer->hasSentTime()) {
                pContainer->setReceiveTime(currTime);
                pContainer->calculateMostRecentRTT();
            }          
            _mutex.unlock();
        }
    }

    uint32 TCPRTTTable::cleanTables(const uint32 ui32MaxCleaningNumber)
    {
        String k1[C_MAX_CLEANING_NUMBER];
        String k2[C_MAX_CLEANING_NUMBER];
        String k3[C_MAX_CLEANING_NUMBER];
        String k4[C_MAX_CLEANING_NUMBER];

        bool bKeepCleaning = true;
        uint32 ui32CleaningCounter = 0;

        // Clean tiered hash table
        if (_mutex.lock() == Mutex::RC_Ok) {
            for (SIPLevel::Iterator iv = _tcpRTTTable.getAllElements();
                !iv.end() && bKeepCleaning; iv.nextElement()) {

                DIPLevel *pL3 = iv.getValue();

                for (DIPLevel::Iterator iii = pL3->getAllElements();
                    !iii.end() && bKeepCleaning; iii.nextElement()) {

                    SPLevel *pL2 = iii.getValue();

                    for (SPLevel::Iterator ii = pL2->getAllElements();
                        !ii.end() && bKeepCleaning; ii.nextElement()) {
                        
                        DPLevel *pL1 = ii.getValue();

                        for (DPLevel::Iterator i = pL1->getAllElements();
                            !i.end() && bKeepCleaning; i.nextElement()) {

                            TCPRTTContainer *pContainer = i.getValue();

                            // Remove the entry if the time has been too long
                            if (pContainer->tiaTCPRTT.getAverage() == 0) {
                                k1[ui32CleaningCounter] = i.getKey();
                                k2[ui32CleaningCounter] = ii.getKey();
                                k3[ui32CleaningCounter] = iii.getKey();
                                k4[ui32CleaningCounter] = iv.getKey();
                                ui32CleaningCounter++;
                                bKeepCleaning = (ui32CleaningCounter <
                                    ui32MaxCleaningNumber);
                            }
                        }
                    }
                }
            }
            for (uint32 counter = 0; counter < ui32CleaningCounter; counter++)
            {
                DIPLevel *iii = _tcpRTTTable.get(k4[counter]);
                SPLevel   *ii = iii->get(k3[counter]);
                DPLevel    *i = ii->get(k2[counter]);

                delete i->remove(k1[counter]);
                if (i->getCount() == 0) {
                    delete ii->remove(k2[counter]);
                    if (ii->getCount() == 0) {
                        delete iii->remove(k3[counter]);
                        if (iii->getCount() == 0) {
                            delete _tcpRTTTable.remove(k4[counter]);
                        }
                    }
                }
            }

            _mutex.unlock();
        }

        return ui32CleaningCounter;
    }

    // Returns number of source addresses in the container
    uint8 TCPRTTTable::getCount() {
        return (uint8)_tcpRTTTable.getCount();
    }
}