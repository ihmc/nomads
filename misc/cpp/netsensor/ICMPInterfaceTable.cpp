/*
* ICMPInterfaceTable.cpp
* Author: bordway@ihmc.us
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
* ICMP table that will control ICMP information
*/

#include "ICMPInterfaceTable.h"
#include "Logger.h"

using namespace IHMC_NETSENSOR_NET_UTILS;
using namespace netsensor;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_NETSENSOR
{
    // Constructor, configure hashtable settings
    ICMPInterfaceTable::ICMPInterfaceTable (const uint32 timeInterval)
        : 
        _icmpInfoTableContainer (true, true, true, true),
        _icmpRTTTable (true,true,true,true) 
    {
        _ui32msResolution = timeInterval;
    }

    void ICMPInterfaceTable::fillICMPProtoObject (
        const char * pIName, 
        ICMPPacketsByInterface * pIpbi)
    {
        if (_pTMutex.lock() == Mutex::RC_Ok) {
            ICMPHashTable * pIht = _icmpInfoTableContainer.get (pIName);
            if (pIht != nullptr) {
                pIht->fillICMPProto (pIpbi); // Pass in the icmp object
            }
            _pTMutex.unlock();
        }
    }

    ICMPRTTHashTable * ICMPInterfaceTable::getRTTTableForInterface (const char * pInterfaceName)
    {
        return _icmpRTTTable.get (pInterfaceName);
    }

    void ICMPInterfaceTable::cleanTable (const uint32 ui32CleaningNumber)
    {
        if ((_pTMutex.lock() == Mutex::RC_Ok)) {
            uint32 ui32CleaningCounter = 0;

            if (_icmpInfoTableContainer.getCount() > 0) {
                for (auto i = _icmpInfoTableContainer.getAllElements(); !i.end() && (ui32CleaningCounter < ui32CleaningNumber); i.nextElement()) {
                    auto pIht = i.getValue();
                    if (pIht != nullptr) {
                        ui32CleaningCounter += pIht->cleanTable (ui32CleaningNumber - ui32CleaningCounter);
                    }
                }
            }

            for (auto i = _icmpRTTTable.getAllElements(); !i.end() && (ui32CleaningCounter < ui32CleaningNumber); i.nextElement()) {
                ICMPRTTHashTable * pIht = i.getValue();
                if (pIht != nullptr) {
                    ui32CleaningCounter += pIht->cleanTable (ui32CleaningNumber - ui32CleaningCounter);
                }
            }
            _pTMutex.unlock();
        }
    }

    // Check if mutex is available
    bool ICMPInterfaceTable::mutexTest (void)
    {
        if ((_pTMutex.lock() == Mutex::RC_Ok)) {
            _pTMutex.unlock();
            return true;
        }
        return false;
    }

    // Print content of table
    void ICMPInterfaceTable::printContent (void)
    {
        auto pszMethodName = "ICMPInterfaceTable::printContent";
        if ((_pTMutex.lock() == Mutex::RC_Ok)) {
            for (auto i = _icmpInfoTableContainer.getAllElements(); !i.end(); i.nextElement()) {
                //  Get interface table and print values
                ICMPHashTable * pIht = i.getValue();
                if (pIht->getCount() != 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_Info, "Number of items: %d\n", pIht->getCount());
                    checkAndLogMsg (pszMethodName, Logger::L_Info, "\n%s :\n------------------\n", i.getKey());
                    pIht->print();
                }
            }
            _pTMutex.unlock();
        }
    }

    void ICMPInterfaceTable::printRTTContent (void)
    {
        auto pszMethodName = "ICMPInterfaceTable::printRTTContent";
        if ((_pTMutex.lock() == Mutex::RC_Ok)) {
            for (auto i = _icmpRTTTable.getAllElements(); !i.end(); i.nextElement()) {
                //  Get interface table and print values
                ICMPRTTHashTable * pIht = i.getValue();
                checkAndLogMsg (pszMethodName, Logger::L_Info, "\n%s :\n------------------\n", i.getKey());
                pIht->print();

            }
            _pTMutex.unlock();
        }
    }

    // Put ICMP data into the table
    void ICMPInterfaceTable::put (
        const char * pInterfaceName, 
        NS_ICMPPacket * pIcmpPacket, 
        bool isSent, 
        int64 timestamp)
    {
        auto pszMethodName = "ICMPInterfaceTable::put";
        if (pIcmpPacket == nullptr) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "ICMPInterfaceTable:put:: Null icmpInfo\n");
        }

        if (pInterfaceName == nullptr) {
            checkAndLogMsg(pszMethodName, Logger::L_Info, "ICMPInterfaceTable:put:: Null interfaceName\n");
        }

        if ((_pTMutex.lock() == Mutex::RC_Ok)) {
            addToRTTTable (pInterfaceName, pIcmpPacket, isSent, timestamp);
            addToTypeTable (pInterfaceName, pIcmpPacket);
            _pTMutex.unlock();
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "ICMPTable::Unable to get mutex\n");
        }
    }

    void ICMPInterfaceTable::addToRTTTable (
        const char * pInterfaceName, 
        IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket * pIcmpPacket, 
        bool isSent, int64 timestamp)
    {
        ICMPRTTHashTable * pRTTContainer = _icmpRTTTable.get(pInterfaceName);
        if (pRTTContainer == nullptr) {
            pRTTContainer = new ICMPRTTHashTable(_ui32msResolution);
            pRTTContainer->put (pIcmpPacket, isSent, timestamp);
            _icmpRTTTable.put (pInterfaceName, pRTTContainer);
        }
        else {
            pRTTContainer->put (pIcmpPacket, isSent, timestamp);
        }
    }

    void ICMPInterfaceTable::addToTypeTable (
        const char * pInterfaceName, 
        IHMC_NETSENSOR_NET_UTILS::NS_ICMPPacket * pIcmpPacket)
    {
        // Get table based on interface name
        ICMPHashTable * pICMPInfoContainer = _icmpInfoTableContainer.get (pInterfaceName);

        if (pICMPInfoContainer == nullptr) {
            // Create new table and put icmpinfo into it
            pICMPInfoContainer = new ICMPHashTable();

            // Create new object for holding information about all repeated ICMP packets
            auto pICMPTypeContainer = new ICMPTypesContainer (pIcmpPacket, _ui32msResolution);

            // Put the table of repeated packets into the table of this interface 
            pICMPInfoContainer->put (pICMPTypeContainer);

            pICMPTypeContainer->tiaICMP.add (1);

            if (pICMPTypeContainer->hasExtraAddresses())
                pICMPTypeContainer->updateExtraAddresses (InetAddr (pIcmpPacket->pIcmpData->ui32RoH).getIPAsString());

            // Put the interface table into the table of all tables
            _icmpInfoTableContainer.put (pInterfaceName, pICMPInfoContainer);
        }
        else {
            ICMPTypesContainer * pICMPTypeContainer = pICMPInfoContainer->get (pIcmpPacket);
            if (pICMPTypeContainer == nullptr) {
                pICMPTypeContainer = new ICMPTypesContainer (pIcmpPacket, _ui32msResolution);
                pICMPInfoContainer->put (pICMPTypeContainer);
                pICMPTypeContainer->tiaICMP.add (1);
                if (pICMPTypeContainer->hasExtraAddresses())
                    pICMPTypeContainer->updateExtraAddresses (InetAddr (pIcmpPacket->pIcmpData->ui32RoH).getIPAsString());
            }
            else {
                pICMPInfoContainer->put (pICMPTypeContainer);
                pICMPTypeContainer->tiaICMP.add (1);
                if (pICMPTypeContainer->hasExtraAddresses())
                    pICMPTypeContainer->updateExtraAddresses (InetAddr (pIcmpPacket->pIcmpData->ui32RoH).getIPAsString());
            }
        }
    }
}