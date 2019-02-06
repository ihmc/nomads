/*
* TCPRttTable.cpp
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

#include "TCPRTTList.h"
#include <forward_list>
#include <tuple>

using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
    TCPRTTList::TCPRTTList(uint32 ui32msResolution)
        : _avgRttTIA(ui32msResolution),
        _i64MostRecentRTT(0)
    {
    }

    uint32 TCPRTTList::cleanTables(const uint32 ui32MaxCleaningNumber)
    {
        /*uint32 ackLevelCleaner[C_MAX_CLEANING_NUMBER];
        uint32 seqLevelCleaner[C_MAX_CLEANING_NUMBER];*/
        std::forward_list <std::tuple <uint32, uint32>> list;
        
        bool bKeepCleaning = true;
        uint32 ui32CleaningCounter = 0;
        int64 currentTime = getTimeInMilliseconds();
        // Clean tiered hash table
        if (_mutex.lock() == Mutex::RC_Ok) {
            for (SeqLevel::Iterator ii = _tcpRTTTable.getAllElements(); !ii.end(); ii.nextElement())
            {
                ACKLevel *l1 = ii.getValue();

                for (ACKLevel::Iterator i = l1->getAllElements(); !i.end(); i.nextElement())
                {
                    RTTContainer *pContainer = i.getValue();

                    // Remove the entry if the time has been too long
                    int64 expiredT = pContainer->getLastChangeTime() +
                        C_ENTRY_TIME_VALIDITY;

                    if (currentTime > expiredT) {
                        /*ackLevelCleaner[ui32CleaningCounter] = i.getKey();
                        seqLevelCleaner[ui32CleaningCounter] = ii.getKey();*/
                        list.emplace_front (i.getKey(), ii.getKey());
                        ui32CleaningCounter++;
                        bKeepCleaning = (ui32CleaningCounter <
                            ui32MaxCleaningNumber);
                    }
                }
            }
            for (auto iter = list.begin(); iter != list.end(); iter++) {
                ACKLevel *i = _tcpRTTTable.get (std::get<1> (*iter));
                delete i->remove (std::get<0> (*iter));
                if (i->getCount() == 0) {
                    delete _tcpRTTTable.remove (std::get<1> (*iter));
                }
            }
            /*for (uint32 counter = 0; counter < ui32CleaningCounter; counter++)
            {
                ACKLevel *i = _tcpRTTTable.get(seqLevelCleaner[counter]);
                delete i->remove(ackLevelCleaner[counter]);
                if (i->getCount() == 0) {
                    delete _tcpRTTTable.remove(seqLevelCleaner[counter]);
                }
            }*/
            _mutex.unlock();
        }

        return ui32CleaningCounter;
    }

    uint32 TCPRTTList::getCount() 
    {
        if (_mutex.lock() == Mutex::RC_Ok)
        {
            uint32 _ui32Count = 0;
            for (SeqLevel::Iterator ii = _tcpRTTTable.getAllElements(); !ii.end();
                ii.nextElement())
            {
                ACKLevel *l1 = ii.getValue();
                for (ACKLevel::Iterator i = l1->getAllElements(); !i.end();
                    i.nextElement())
                {
                    RTTContainer *pContainer = i.getValue();
                    if (pContainer->getRcvTime() != 0) {
                        _ui32Count++;
                    }
                }
            }
            return _ui32Count;
            _mutex.unlock();
        }
    }

    bool TCPRTTList::hasAckNum(uint32 ui32AckNum)
    {
        if (_mutex.lock() == Mutex::RC_Ok) 
        {
            for (SeqLevel::Iterator ii = _tcpRTTTable.getAllElements(); !ii.end();
                ii.nextElement())
            {
                ACKLevel *l1 = ii.getValue();

                if (l1->contains(ui32AckNum)) {
                    _mutex.unlock();
                    return true;
                }
            }

            _mutex.unlock();
        }
        return false;
    }

    void TCPRTTList::print(void)
    {
        if (_mutex.lock() == Mutex::RC_Ok)
        {
            for (SeqLevel::Iterator ii = _tcpRTTTable.getAllElements(); !ii.end();
                ii.nextElement())
            {
                ACKLevel *l1 = ii.getValue();
                for (ACKLevel::Iterator i = l1->getAllElements(); !i.end();
                    i.nextElement())
                {
                    RTTContainer *info = i.getValue();

                    printf("\nSeq Num: %u  Ack Num: %u   ",
                        ii.getKey(), i.getValue());

                    info->print();
                }
            }
            _mutex.unlock();
        }
    }

    void TCPRTTList::putNewAckEntry(uint32 ui32AckNum, const int64 i64RcvTimestamp)
    {
        if (_mutex.lock() == Mutex::RC_Ok)
        {
            //printf("Received ackNum: %d\n", ui32AckNum);
            for (SeqLevel::Iterator ii = _tcpRTTTable.getAllElements(); !ii.end();
                ii.nextElement())
            {
                ACKLevel *l1 = ii.getValue();
                
                for (ACKLevel::Iterator i = l1->getAllElements(); !i.end();
                    i.nextElement())
                {
                    RTTContainer *pContainer = i.getValue();

                    // If this ack number is below the passed in ack number,
                    // check if there's a rcv time already associated with it
                    if (i.getKey() <= ui32AckNum)
                    {
                        if (pContainer->getRcvTime() == 0)
                        {
                            pContainer->setReceivedTime(i64RcvTimestamp);
                            pContainer->updateChangeTime(i64RcvTimestamp);
                            _avgRttTIA.add(pContainer->getRTT());

                           /* printf("Calculating RTT %d,", pContainer->getRTT());
                            printf(" seqNum %u,", ii.getKey());
                            printf(" ackNum %u\n", i.getKey());*/
                        }
                    }

                    if (i.getKey() == ui32AckNum)
                    {
                        _i64MostRecentRTT = pContainer->getRTT();
                    }
                }
            }
            //printf("Min: %u, Max: %u, Avg: %3f, Recent: %u\n", getMinRTT(), getMaxRTT(), getAvgRTT(), getMostRecentRTT());
            _mutex.unlock();
        }
    }
    
    void TCPRTTList::putNewSeqEntry(uint32 ui32SeqNum, uint32 ui32NextAckNum,
        const int64 i64SntTimestamp)
    {
        if (_mutex.lock() == Mutex::RC_Ok)
        {
            ACKLevel *pACKLevel = _tcpRTTTable.get(ui32SeqNum);
            if (pACKLevel != nullptr) {
                RTTContainer *pContainer = pACKLevel->get(ui32NextAckNum);
                if (pContainer != nullptr) {
                    pContainer->updateChangeTime(getTimeInMilliseconds());
                    pContainer->setSentTime(i64SntTimestamp);
                    pContainer->setReceivedTime(0);
                }

                _mutex.unlock();
                return;
            }

            RTTContainer *pContainer = new RTTContainer();
            pContainer->setSentTime(i64SntTimestamp);
            pContainer->updateChangeTime(getTimeInMilliseconds());

            pACKLevel = new ACKLevel;
            pACKLevel->put(ui32NextAckNum, pContainer);
            _tcpRTTTable.put(ui32SeqNum, pACKLevel);

            _mutex.unlock();
        }
    }

    uint32 TCPRTTList::getMinRTT()
    {
        if (_avgRttTIA.getMin() == 0xFFFFFFFF) {
            uint32 hashTableCount = _tcpRTTTable.getCount();
            uint32 tiaCount = _avgRttTIA.getNumValues();
            printf("Bad\n");
        }
        return _avgRttTIA.getMin();
    }

    float TCPRTTList::getAvgRTT()
    {
        if (_avgRttTIA.getNumValues() == 0) {
            return 0.0;
        }
        return _avgRttTIA.getSum() / _avgRttTIA.getNumValues();
    }

    uint32 TCPRTTList::getMaxRTT()
    {
        if (_mutex.lock() == Mutex::RC_Ok)
        {
            uint32 maxRTT = 0;
            for (SeqLevel::Iterator ii = _tcpRTTTable.getAllElements(); !ii.end();
                ii.nextElement())
            {
                ACKLevel *l1 = ii.getValue();

                for (ACKLevel::Iterator i = l1->getAllElements(); !i.end();
                    i.nextElement())
                {
                    RTTContainer *pContainer = i.getValue();
                    uint32 currRTT = pContainer->getRTT();

                    // If there hasn't been a receive time recorded yet, we don't want to include this metric
                    // Similar to how ping works
                    if (currRTT > maxRTT && pContainer->getRcvTime() != 0) {
                        maxRTT = currRTT;
                    }
                }
            }
            _mutex.unlock();
            return maxRTT;
        }

        return 0;
    }

    uint32 TCPRTTList::getMostRecentRTT()
    {
        return _i64MostRecentRTT;
    }
}