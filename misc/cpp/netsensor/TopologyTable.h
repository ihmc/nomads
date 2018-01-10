#ifndef NETSENSOR_TopologyTable__INCLUDED
#define NETSENSOR_TopologyTable__INCLUDED
/*
* TopologyTable.h
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
*
* Topology table that will contain all the topology info indexed by
* network interface name
*
*/

#include"StrClass.h"
#include"Mutex.h"
#include"StringHashtable.h"
#include"TopologyInfo.h"
#include"NLFLib.h"
namespace IHMC_NETSENSOR
{
    class TopologyTable
    {
    public:
        void cleanTable(uint32 ui32CleaningNumber);
        void fillNetworkInfoProto(const char* pcInterfaceName,
            TopologyInfo *tmpTI);
        void fillInternalTopologyProtoObject(const char* pcInterfaceName, netsensor::Topology *pT);
        void fillExternalTopologyProtoObject(const char* pcInterfaceName, netsensor::Topology *pT);

        void removeNodeFromInterfaceTable(const char* pcInterfaceName, const char* pIPAddr,
            const char* pMacAddr);

        bool mutexTest(void);

        bool putInternal(const char* interfaceName, const char* sIPAddr,
            const char* sMacAddr);
        bool putExternal(const char* pInterfaceName, const char* pSIPAddr,
            const char* pSMacAddr);
        void printContent(void);

        TopologyTable(void);
        //<--------------------------------------------------------------------------->
    private:
        NOMADSUtil::StringHashtable<TopologyInfo> _internTopologyInfoTablesContainer;
        NOMADSUtil::StringHashtable<TopologyInfo> _externTopologyInfoTablesContainer;
        NOMADSUtil::Mutex _pTMutex;
    };

    inline void TopologyTable::fillNetworkInfoProto(const char* pcInterfaceName,
        TopologyInfo *tmpTI)
    {

    }

    inline bool TopologyTable::mutexTest(void)
    {
        if (_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok) {
            _pTMutex.unlock();
            return true;
        }
        return false;
    }

    inline TopologyTable::TopologyTable()
    {
        _internTopologyInfoTablesContainer.configure(true, true, true, true);
        _externTopologyInfoTablesContainer.configure(true, true, true, true);
    }

    inline bool TopologyTable::putInternal(const char* interfaceName,
        const char* sIPAddr, const char* sMacAddr)
    {
        if (_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok) {
            //printf("Test: %s, %s, %s\n", interfaceName, sIPAddr, sMacAddr);     
            TopologyInfo* pTI;
            TopologyEntry *pTE;

            if ((pTI = _internTopologyInfoTablesContainer.get(interfaceName)) == nullptr) {
                pTI = new TopologyInfo();
                _internTopologyInfoTablesContainer.put(interfaceName, pTI);
            }
            pTI->addNodes(sIPAddr, sMacAddr);
            _pTMutex.unlock();
            return true;
        }
        return false;
    }

    inline bool TopologyTable::putExternal(const char* pInterfaceName,
        const char* pSIPAddr, const char* pSMacAddr)
    {
        if (_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)
        {
            TopologyInfo *pTI;

            if ((pTI = _externTopologyInfoTablesContainer.get(pInterfaceName)) == nullptr)
            {
                pTI = new TopologyInfo();
                _externTopologyInfoTablesContainer.put(pInterfaceName, pTI);
            }
            pTI->addNodes(pSIPAddr, pSMacAddr);

            _pTMutex.unlock();
            return true;
        }
        return false;
    }

    inline void TopologyTable::printContent(void)
    {
        if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
            printf("\nINTERNAL NODES:\n");
            printf("---------------\n");
            for (NOMADSUtil::StringHashtable<TopologyInfo>::Iterator
                i = _internTopologyInfoTablesContainer.getAllElements();
                !i.end(); i.nextElement())
            {
                printf("Interface: %s\n\n", i.getKey());
                TopologyInfo *pTI = i.getValue();
                pTI->printContent();
            }

            if (_externTopologyInfoTablesContainer.getCount() > 0)
            {
                printf("\nEXTERNAL NODES:\n");
                printf("---------------\n");
                for (NOMADSUtil::StringHashtable<TopologyInfo>::Iterator
                    i = _externTopologyInfoTablesContainer.getAllElements();
                    !i.end(); i.nextElement())
                {
                    printf("Interface: %s\n\n", i.getKey());
                    TopologyInfo *pTI = i.getValue();
                    pTI->printContent();
                }
            }

            _pTMutex.unlock();
        }
    }

    inline void TopologyTable::cleanTable(uint32 ui32CleaningNumber)
    {
        uint32 ui32cleaningCounter = 0;
        if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
            // Clean internal nodes
            for (NOMADSUtil::StringHashtable<TopologyInfo>::Iterator i =
                _internTopologyInfoTablesContainer.getAllElements();
                !i.end() && (ui32cleaningCounter < ui32CleaningNumber);
                i.nextElement()) {
                ui32cleaningCounter = ui32cleaningCounter +
                    i.getValue()->cleanTable(ui32CleaningNumber - ui32cleaningCounter);

                ui32cleaningCounter = ui32cleaningCounter + i.getValue()->cleanTable(ui32CleaningNumber - ui32cleaningCounter);
            }

            // Clean external nodes
            for (NOMADSUtil::StringHashtable<TopologyInfo>::Iterator i =
                _externTopologyInfoTablesContainer.getAllElements();
                !i.end() && ui32cleaningCounter < ui32CleaningNumber;
                i.nextElement())
            {
                ui32cleaningCounter = ui32cleaningCounter +
                    i.getValue()->cleanTable(ui32CleaningNumber - ui32cleaningCounter);

                ui32cleaningCounter = ui32cleaningCounter + i.getValue()->cleanTable(ui32CleaningNumber - ui32cleaningCounter);
            }

            _pTMutex.unlock();
        }
    }

    inline void TopologyTable::fillInternalTopologyProtoObject(const char* pcInterfaceName,
        netsensor::Topology *pT)
    {
        TopologyInfo *tmpTI = nullptr;
        if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
            tmpTI = _internTopologyInfoTablesContainer.get(pcInterfaceName);
            if (tmpTI != nullptr) {
                //tmpTI->printContent();
                tmpTI->getProtobuf(pT);
            }
            _pTMutex.unlock();
        }
    }

    inline void TopologyTable::fillExternalTopologyProtoObject(
        const char* pcInterfaceName,
        netsensor::Topology *pT)
    {
        TopologyInfo *pTmpTI = nullptr;
        if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
            pTmpTI = _externTopologyInfoTablesContainer.get(pcInterfaceName);
            if (pTmpTI != nullptr) {
                pTmpTI->getProtobuf(pT);
            }
            _pTMutex.unlock();
        }
    }

    inline void TopologyTable::removeNodeFromInterfaceTable(const char* pcInterfaceName, const char* pIPAddr, const char* pMacAddr)
    {
        TopologyInfo *tmpTI = _externTopologyInfoTablesContainer.get(pcInterfaceName);
        tmpTI->removeNode(pMacAddr, pIPAddr);
    }
}
#endif