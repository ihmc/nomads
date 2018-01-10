/*
* TCPRTTInterfaceTable.cpp
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

#include "TCPRTTInterfaceTable.h"

using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
    TCPRTTInterfaceTable::TCPRTTInterfaceTable(void) : 
        _tcpRTTInterfaceTable(true, true, true, true)

    {

    }

    TCPRTTInterfaceTable::~TCPRTTInterfaceTable(void)
    {
        _tcpRTTInterfaceTable.removeAll();
    }

    TCPRTTInterfaceTable::TCPRTTInterfaceTable(uint32 ui32Resolution) :
        _ui32msResolution(ui32Resolution),
        _tcpRTTInterfaceTable(true, true, true, true)
    {

    }

    void TCPRTTInterfaceTable::cleanTable(const uint32 ui32MaxCleaningNumber)
    {
        if (_mutex.lock() == Mutex::RC_Ok) {
            if (_tcpRTTInterfaceTable.getCount() > 0) {
                uint32 ui32CleaningCounter = 0;
                for (StringHashtable<TCPRTTTable>::Iterator i = _tcpRTTInterfaceTable.getAllElements();
                    !i.end() && ui32CleaningCounter < ui32MaxCleaningNumber; i.nextElement()) {
                    TCPRTTTable *pTRT = i.getValue();
                    if (pTRT != nullptr) {
                        ui32CleaningCounter += pTRT->cleanTables(ui32MaxCleaningNumber - ui32CleaningCounter);
                    }
                }
            }
            _mutex.unlock();
        }
    }

    bool TCPRTTInterfaceTable::mutexTest(void)
    {
        if (_mutex.lock() == Mutex::RC_Ok) {
            _mutex.unlock();
            return true;
        }

        return false;
    }

    void TCPRTTInterfaceTable::printContent(void)
    {
        if (_mutex.lock() == Mutex::RC_Ok) {
            for (StringHashtable<TCPRTTTable>::Iterator i = _tcpRTTInterfaceTable.getAllElements();
                !i.end(); i.nextElement()) {
                TCPRTTTable *pTRT = i.getValue();
                
                if (pTRT->getCount() > 0) {
                    printf("\n%s :\n------------------\n", i.getKey());
                    pTRT->print();
                }
            }

            _mutex.unlock();
        }
    }

    // Pass the tcpData to the proper TCPRTTTable
    void TCPRTTInterfaceTable::put(const char *pInterfaceName, TCPRTTData & tcpData, bool isSent)
    {
        if (pInterfaceName == nullptr) {
            printf("TCPRTTHandler:put::Null interface name\n");
        }

        if (_mutex.lock() == Mutex::RC_Ok) {
            TCPRTTTable *pTRT = _tcpRTTInterfaceTable.get(pInterfaceName);
            if (pTRT == nullptr) {
                pTRT = new TCPRTTTable();
                               
                _tcpRTTInterfaceTable.put(pInterfaceName, pTRT);
                pTRT->put(tcpData, isSent);
            }
            else {
                pTRT->put(tcpData, isSent);
            }
            _mutex.unlock();
        }
    }
}