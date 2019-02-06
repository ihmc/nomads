/*
* ICMPRTTTable.cpp
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

#include "ICMPRTTTable.h"
#include <forward_list>

using namespace NOMADSUtil;

namespace IHMC_NETSENSOR
{
    ICMPRTTTable::ICMPRTTTable(void)
    {
        _pRttList = new LList<RTTContainer>();
    }

    void ICMPRTTTable::addReplyTime(int64 time)
    {
        _currentContainer.setReceivedTime(time);
    }

    void ICMPRTTTable::addRequestTime(int64 time)
    {
        RTTContainer rttContainer;
        _pRttList->add(rttContainer);

        rttContainer.setSentTime(time);

        _currentContainer = rttContainer;
    }

    uint8 ICMPRTTTable::getMostRecentRTT(void)
    {
      return _currentContainer.getRTT();
    }

    uint8 ICMPRTTTable::getAverageRTT(void)
    {
        if (_mutex.lock() == Mutex::RC_Ok) {
            RTTContainer currContainer;
            uint32 rttSum = 0;

            _pRttList->resetGet();
            while (_pRttList->getNext(currContainer) != 0) {
                rttSum += currContainer.getRTT();
            }

            _mutex.unlock();

            return (rttSum / _pRttList->length);
        }
    }

    uint32 ICMPRTTTable::cleanTables(const uint32 ui32MaxCleaningNumber)
    {
        //RTTContainer containersToRemove[C_MAX_CLEANING_NUMBER];
        std::forward_list <RTTContainer> list;
        bool bKeepCleaning = true;
        uint32 ui32CleaningCounter = 0;
        int64 currentTime = getTimeInMilliseconds();

        // Clean tiered hash table
        if (_mutex.lock() == Mutex::RC_Ok) {
            RTTContainer currContainer;
            _pRttList->resetGet();
            while (_pRttList->getNext(currContainer) != 0 && bKeepCleaning) {
                int64 expiredT = currContainer.getLastChangeTime() +
                    C_ENTRY_TIME_VALIDITY;

                if (currentTime > expiredT) {
                    RTTContainer copyContainer = currContainer;
                    //containersToRemove[ui32CleaningCounter] = copyContainer;
                    list.push_front (copyContainer);
                    ui32CleaningCounter++;
                    bKeepCleaning = (ui32CleaningCounter <
                        ui32MaxCleaningNumber);
                }
            }
            for (auto iter = list.begin(); iter != list.end(); iter++) {
                _pRttList->remove(*iter);
            }
            /*for (uint32 counter = 0; counter < ui32CleaningCounter; counter++) {
                _pRttList->remove(containersToRemove[counter]);
            }*/

            _mutex.unlock();
        }

        return ui32CleaningCounter;
    }

    void ICMPRTTTable::print(void)
    {
        if (_mutex.lock() == Mutex::RC_Ok)
        {
            RTTContainer currContainer;
            _pRttList->resetGet();
            while (_pRttList->getNext(currContainer) != 0) {
                printf("RTT val: %dms\n", currContainer.getRTT());
            }

            _mutex.unlock();
        }
    }
}