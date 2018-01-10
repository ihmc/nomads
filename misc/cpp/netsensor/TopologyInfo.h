#ifndef NETSENSOR_TopologyInfo__INCLUDED
#define NETSENSOR_TopologyInfo__INCLUDED
/*
* TopologyInfo.h
*
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
* This is the main topology info container
*
*/


#include"LList.h"
#include"NLFLib.h"
#include"StringHashtable.h"
#include"StrClass.h"
#include"topology.pb.h"
#include"TopologyEntry.h"

namespace IHMC_NETSENSOR
{
    class TopologyInfo
    {
    public:
        TopologyInfo();

        inline void addNodes(const char* sIPAddr, const char* sMacAddr);
        inline uint32 cleanTable(uint32 ui32CleaningNumber);
        inline int getProtobuf(netsensor::Topology* ptp);
        inline void printContent();
        inline void removeNode(const char* pMacAddr, const char* pIPToRemove);
        //<--------------------------------------------------------------------------->
    private:
        NOMADSUtil::StringHashtable<TopologyEntry> _shNodes;
        NOMADSUtil::Mutex _mxTop;
    };


    inline TopologyInfo::TopologyInfo() :
        _shNodes(true, true, true, true)
    {
    }

    inline void TopologyInfo::addNodes(const char* sIPAddr,
        const char* sMacAddr)
    {
        NOMADSUtil::String tmpIP = sIPAddr;
        if (_mxTop.lock() == NOMADSUtil::Mutex::RC_Ok) {
            TopologyEntry *pTE;
            if ((pTE = _shNodes.get(sMacAddr)) == NULL) {
                pTE = new TopologyEntry();

                _shNodes.put(sMacAddr, pTE);
            }

            if (!pTE->ipsList.search(tmpIP)) {
                pTE->ipsList.add(tmpIP);
            }
            pTE->i64TimeOfLastChange = NOMADSUtil::getTimeInMilliseconds();
            _mxTop.unlock();
        }
    }

    inline uint32 TopologyInfo::cleanTable(uint32 ui32CleaningNumber)
    {
        NOMADSUtil::String internalVals[C_MAX_CLEANING_NUMBER];
        bool bKeepCleaning = true;
        uint32 cleaningCounter = 0;
        int64 currentTime = NOMADSUtil::getTimeInMilliseconds();

        if (_mxTop.lock() == NOMADSUtil::Mutex::RC_Ok) {
            // Check for cleaning internal nodes
            for (NOMADSUtil::StringHashtable<TopologyEntry>::Iterator i =
                _shNodes.getAllElements(); !i.end() && (bKeepCleaning);
                i.nextElement()) {

                int64 expTime = i.getValue()->i64TimeOfLastChange + C_ENTRY_TIME_VALIDITY;
                if (currentTime > expTime) {
                    internalVals[cleaningCounter] = i.getKey();
                    cleaningCounter++;

                    bKeepCleaning = (cleaningCounter < ui32CleaningNumber);
                }
            }
            // Clean internal nodes
            for (uint32 counter = 0; counter < cleaningCounter; counter++) {
                delete _shNodes.remove(internalVals[counter]);
            }

            _mxTop.unlock();
            return cleaningCounter;
        }
        return 0;
    }

    inline int TopologyInfo::getProtobuf(netsensor::Topology* ptp)
    {
        if (_mxTop.lock() == NOMADSUtil::Mutex::RC_Ok) {
            for (NOMADSUtil::StringHashtable<TopologyEntry>::Iterator
                i = _shNodes.getAllElements(); !i.end(); i.nextElement()) {
                TopologyEntry *pTE = i.getValue();

                if (pTE->ipsList.getCount() > 0) {
                    pTE->ipsList.resetGet();
                    NOMADSUtil::String tmpStringIP;

                    while (pTE->ipsList.getNext(tmpStringIP))
                    {
                        netsensor::Host* protoHost = ptp->add_internals();
                        protoHost->set_mac(i.getKey());

                        uint32 networkOrderIp = 
                            convertIpToHostOrder(
                                NOMADSUtil::InetAddr(tmpStringIP).getIPAddress());
                        protoHost->set_ip(networkOrderIp);                 
                    }
                }
            }
            _mxTop.unlock();
        }
        return 0;
    }

    inline void TopologyInfo::printContent()
    {
        if (_mxTop.lock() == NOMADSUtil::Mutex::RC_Ok) {
            if (_shNodes.getCount() > 0) {
                for (NOMADSUtil::StringHashtable<TopologyEntry>::Iterator ii =
                    _shNodes.getAllElements(); !ii.end(); ii.nextElement()) {

                    printf("MAC: %s ", ii.getKey());
                    printf("\nIP List: ");
                    if (ii.getValue() != NULL) {
                        int count = 0;
                        ii.getValue()->ipsList.resetGet();
                        NOMADSUtil::String el;
                        while (ii.getValue()->ipsList.getNext(el)) {
                            if (count == 0)
                                printf("%s", el.c_str());
                            else
                                printf("\n\t %s", el.c_str());

                            count++;
                        }
                        printf("\n\n");
                    }
                }
            }
            else { printf("No nodes\n"); }

            _mxTop.unlock();
        }
    }

    // Remove the node from the list
    inline void TopologyInfo::removeNode(const char* pMacAddr, const char* pIPToRemove)
    {
        TopologyEntry *tmpEntry = _shNodes.get(pMacAddr);
        tmpEntry->ipsList.remove(pIPToRemove);

        if (tmpEntry->ipsList.getCount() == 0)
        {
            delete _shNodes.remove(pMacAddr);
        }
    }
}
#endif
