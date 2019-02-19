/*
* TopologyCache.cpp
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
* Two-tabled class that contains all internal and external topology entries
* for quick retrieval.
*
*/

#include "TopologyCache.h"
#include <forward_list>
#include <tuple>

using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
    TopologyCache::TopologyCache(void)
        : _externalsTable(true, true, true, true)
    {
    }

    TopologyCache::~TopologyCache(void)
    {
        for (StringHashtable<ExternalTopList>::Iterator i =
            _externalsTable.getAllElements(); !i.end(); i.nextElement())
        {
            ExternalTopList *tempList = i.getValue();
            ExternalTopologyCacheObject *tempNode;
            while ((tempNode = tempList->getNext()) != nullptr)
            {
                delete tempNode;
                tempNode = nullptr;
            }
        }
        _externalsTable.removeAll();
        _internalsTable.removeAll();
    }

    PtrLList<ExternalTopologyCacheObject> *TopologyCache::findEntriesInExternal(const char *pszIPAddr)
    {
        if (_pMutex.lock() == NOMADSUtil::Mutex::RC_Ok)
        {
            // Find and remove the entries from the external table, will return a NULL if not found
            ExternalTopList* pTmpList = _externalsTable.get(pszIPAddr);
            _pMutex.unlock();
            return pTmpList;
        }
		return nullptr;
    }

    bool TopologyCache::isInternalEntry(const char *pszIPAddr)
    {
        if (_pMutex.lock() == NOMADSUtil::Mutex::RC_Ok)
        {
            InternalTopologyCacheObject *pTmpObj = new InternalTopologyCacheObject();
            pTmpObj->sIPAddr = pszIPAddr;
            bool isEntry = _internalsTable.search(pTmpObj);
            delete pTmpObj;

            _pMutex.unlock();
            return isEntry;
        }
		return false;
    }

    void TopologyCache::addInternalEntry(const char *pszIPAddr)
    {
        if (_pMutex.lock() == NOMADSUtil::Mutex::RC_Ok)
        {
            InternalTopologyCacheObject *pTmpObj = new InternalTopologyCacheObject();
            pTmpObj->sIPAddr = pszIPAddr;
            pTmpObj->creationTime = getTimeInMilliseconds();
            if (!_internalsTable.search(pTmpObj))
                _internalsTable.append(pTmpObj);
            else {
                delete pTmpObj;
            }

            _pMutex.unlock();
        }
    }

    // Add an external IP address and the interface name
    void TopologyCache::addExternalEntry(const char *pIFaceName, const char *pIPAddr, const char *pMacAddr)
    {
        if (_pMutex.lock() == NOMADSUtil::Mutex::RC_Ok)
        {
            ExternalTopList* tempList = _externalsTable.get(pIPAddr);
            if (tempList == nullptr)
            {
                // No entries exist, create new one
                ExternalTopologyCacheObject *tempTCO = new ExternalTopologyCacheObject();
                tempList = new ExternalTopList();
                tempTCO->sIFaceName = pIFaceName;
                tempTCO->sMACAddr    = pMacAddr;
                tempTCO->creationTime = getTimeInMilliseconds();
                tempList->append(tempTCO);
                _externalsTable.put(pIPAddr, tempList);
            }
            else
            {
                // Add a new external object to the list of existing objects
                ExternalTopologyCacheObject *tempTCO = new ExternalTopologyCacheObject();
                tempTCO->sIFaceName = pIFaceName;
                tempTCO->sMACAddr    = pMacAddr;
                tempTCO->creationTime = getTimeInMilliseconds();
                if (!tempList->search(tempTCO))
                    tempList->append(tempTCO);
                else
                    // Delete this object as we don't need it
                    delete tempTCO;
            }
            _pMutex.unlock();
        }
    }

    void TopologyCache::cleanTables(uint32 ui32MaxCleaningNumber)
    {
        uint32 ui32cleaningCounter = 0;
        if (_pMutex.lock() == NOMADSUtil::Mutex::RC_Ok)
        {
            cleanExternals(ui32MaxCleaningNumber);
            cleanInternals(ui32MaxCleaningNumber);

            _pMutex.unlock();
        }
    }

    // Clear TopologyCacheObjects
    void TopologyCache::cleanExternals(uint32 ui32MaxCleaningNumber)
    {
        uint32 ui32CleaningCounter = 0;
        int64 currentTime = getTimeInMilliseconds();
        std::forward_list <std::tuple <ExternalTopologyCacheObject *, String>> list;
        //String copyStrings[C_MAX_CLEANING_NUMBER];
        //ExternalTopologyCacheObject *copyExterns[C_MAX_CLEANING_NUMBER];

        for (StringHashtable<ExternalTopList>::Iterator i =
            _externalsTable.getAllElements(); !i.end(); i.nextElement())
        {
            ExternalTopList *tempList = i.getValue();
            ExternalTopologyCacheObject *tempNode;
            while((tempNode = tempList->getNext()) != nullptr)
            {
                int64 expireTime = tempNode->creationTime + C_ENTRY_TIME_VALIDITY;

                if (expireTime > currentTime)
                {
                    //copyExterns[ui32CleaningCounter] = tempNode;
                    //copyStrings[ui32CleaningCounter] = i.getKey();
                    list.emplace_front (tempNode, i.getKey());
                    ui32CleaningCounter++;
                }
            }
        }
        for (auto iter = list.begin(); iter != list.end(); iter++) {
            ExternalTopList *tempList = _externalsTable.get (std::get<1> (*iter));

            uint32 listCounter = tempList->getCount();
            delete tempList->remove (std::get<0> (*iter));
            listCounter = tempList->getCount();
            if (tempList->getCount() == 0){
                delete _externalsTable.remove (std::get<1> (*iter));
            }  
        }
        // Remove entries
        /*for (int counter = 0; counter < ui32CleaningCounter; counter++)
        {
            ExternalTopList *tempList = _externalsTable.get(copyStrings[counter]);

            uint32 listCounter = tempList->getCount();
            delete tempList->remove(copyExterns[counter]);
            listCounter = tempList->getCount();
            if (tempList->getCount() == 0){
                delete _externalsTable.remove(copyStrings[counter]);
            }
        }*/
    }

    void TopologyCache::cleanInternals(uint32 ui32MaxCleaningNumber)
    {
        //uint32 ui32CleaningCounter = 0;
        int64 currentTime = getTimeInMilliseconds();
        //InternalTopologyCacheObject *copyVal[C_MAX_CLEANING_NUMBER];
        std::forward_list <InternalTopologyCacheObject*> list;

        InternalTopologyCacheObject *tempNode;
        while ((tempNode = _internalsTable.getNext()) != nullptr)
        {
            int64 expireTime = tempNode->creationTime + C_ENTRY_TIME_VALIDITY;

            if (expireTime > currentTime)
            {
                list.push_front (tempNode);
                //copyVal[ui32CleaningCounter] = tempNode;
                //ui32CleaningCounter++;
            }
        }

        // Remove entries
        for (auto iter = list.begin(); iter != list.end(); iter++) {
            delete _internalsTable.remove(*iter);
        }
        /*for (int counter = 0; counter < ui32CleaningCounter; counter++)
        {
            delete _internalsTable.remove(copyVal[counter]);
        }*/
    }
}