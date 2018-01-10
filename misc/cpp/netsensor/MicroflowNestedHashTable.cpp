/*
* MicroflowNestedHashTable.h
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
* This Class impelemnts a five levels hash map that will contain
* stats identified by source ip, dest ip, protocol, source port, dest port
*/

#include "MicroflowNestedHashTable.h"
using namespace netsensor;
using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
uint32 MicroflowNestedHashTable::cleanTable(uint32 ui32CleaningNumber)
{
    String k1[C_MAX_CLEANING_NUMBER];
    String k2[C_MAX_CLEANING_NUMBER];
    String k3[C_MAX_CLEANING_NUMBER]; 
    String k4[C_MAX_CLEANING_NUMBER];
    String k5[C_MAX_CLEANING_NUMBER];

    bool bKeepCleaning = true;

    uint32 cleaningCounter = 0;

    if ((_pTMutex.lock() == Mutex::RC_Ok)) {    
        int64 currentTime = getTimeInMilliseconds();

        for (levelFive::Iterator v = _microflowNestedHashTable.
                getAllElements(); (!v.end() && bKeepCleaning); v.nextElement()) {
            levelFour *l4 = v.getValue();

            for (levelFour::Iterator iv = l4->getAllElements(); 
                (!iv.end() && bKeepCleaning); iv.nextElement()) {
                levelThree *l3 = iv.getValue();

                for (levelThree::Iterator iii = l3->getAllElements(); 
                    (!iii.end() && bKeepCleaning); iii.nextElement()) {
                    levelTwo *l2 = iii.getValue();

                    for (levelTwo::Iterator ii = l2->getAllElements(); 
                        (!ii.end() && bKeepCleaning); ii.nextElement()) {
                        levelOne *l1 = ii.getValue();

                        for (levelOne::Iterator i = l1->getAllElements(); 
                            (!i.end() && bKeepCleaning); i.nextElement()) {
                            TrafficElement* tel = i.getValue();
                            tel->tiaPackets.expireOldEntries();
                            tel->tiaTraffic.expireOldEntries();

                            int64 expiredT = tel->i64TimeOfLastChange + 
                                C_ENTRY_TIME_VALIDITY;

                            if (currentTime > expiredT || 
                                ((tel->tiaTraffic.getAverage() == 0))) {
                                k1[cleaningCounter] = i.getKey();
                                k2[cleaningCounter] = ii.getKey();
                                k3[cleaningCounter] = iii.getKey();
                                k4[cleaningCounter] = iv.getKey();
                                k5[cleaningCounter] = v.getKey();
                                cleaningCounter++;
                                (cleaningCounter < ui32CleaningNumber) ? 
                                    bKeepCleaning = true : 
                                    bKeepCleaning = false;
                            }
                        }
                    }
                }
            }
        }

        for (uint32 counter = 0; counter < cleaningCounter; counter++) {
            levelFour*  iv = _microflowNestedHashTable.get(k5[counter]);
            levelThree* iii = iv->get(k4[counter]);
            levelTwo*   ii = iii->get(k3[counter]);
            levelOne*   i = ii->get(k2[counter]);

            delete i->remove(k1[counter]);
            if (i->getCount() == 0) {
                delete ii->remove(k2[counter]);
                if (ii->getCount() == 0) {
                    delete iii->remove(k3[counter]);
                    if (iii->getCount() == 0) {
                        delete iv->remove(k4[counter]);
                        if (iv->getCount() == 0) {
                            delete _microflowNestedHashTable.remove(k5[counter]);
                        }
                    }
                }
            }
        }
        _pTMutex.unlock();

        return cleaningCounter;                      
    }
}

void MicroflowNestedHashTable::fillTrafficProto(TrafficByInterface *pT)
{
    if ((_pTMutex.lock() == Mutex::RC_Ok)) {
        for (levelFive::Iterator v = _microflowNestedHashTable.
                getAllElements(); !v.end(); v.nextElement()) {

            uint32 ui32SrcAddr = InetAddr(v.getKey()).getIPAddress();

            ui32SrcAddr = convertIpToHostOrder(ui32SrcAddr);

            levelFour *l4 = v.getValue();
            for (levelFour::Iterator iv = l4->getAllElements(); !iv.end();
                    iv.nextElement()) {
                uint32 ui32DstAddr = InetAddr(iv.getKey()).getIPAddress();

                ui32DstAddr = convertIpToHostOrder(ui32DstAddr);

                bool bAddIt = false;
                Microflow *pMw = new Microflow();

                //Microflow *pMw = pT->add_microflows();
                
                pMw->set_ipsrc(ui32SrcAddr);
                pMw->set_ipdst(ui32DstAddr);
                levelThree *l3 = iv.getValue();

                for (levelThree::Iterator iii = l3->getAllElements();
                        !iii.end(); iii.nextElement()) {
                    levelTwo *l2 = iii.getValue();

                    for (levelTwo::Iterator ii = l2->getAllElements();
                            !ii.end(); ii.nextElement()) {
                        levelOne *l1 = ii.getValue();

                        for (levelOne::Iterator i = l1->getAllElements();
                                !i.end(); i.nextElement()) {
                            TrafficElement* tel = i.getValue();
                            Stat *st = pMw->add_stats();
                            st->set_stattype(TRAFFIC_AVERAGE);
                            st->set_protocol(iii.getKey());
                            char*end;
                            uint32 ui32SPort = strtol(ii.getKey(), &end, 10);
                            uint32 ui32dPort = strtol(i.getKey(), &end, 10);
                            st->set_srcport(ui32SPort);
                            st->set_dstport(ui32dPort);
                            TrafficElement *te = i.getValue();

                            if (te->tiaTraffic.getAverage() > 0) {
                                Average *avg = st->add_averages();
                                bAddIt = true;
                                switch (te->classification)
                                {
                                case TrafficElement::SNT:
                                    avg->set_sent(te->tiaTraffic.getAverage());
                                    break;
                                case TrafficElement::RCV:
                                    avg->set_received(te->tiaTraffic.getAverage());
                                    break;
                                case TrafficElement::OBS:
                                    avg->set_observed(te->tiaTraffic.getAverage());
                                    break;
                                }
                                avg->set_resolution(te->resolution);
                            }
                        }
                    }
                }

                if (bAddIt) {
                    Microflow *pNewM = pT->add_microflows();
                    pNewM->CopyFrom(*pMw);
                }
                delete pMw;
            }
        }
        _pTMutex.unlock();
    }
}

TrafficElement* MicroflowNestedHashTable::get(const char* sSA, const char* sDA, const char* sProtocol,
                                              const char* sSP, const char* sDP)
{
    levelFour *l4;
    levelThree *l3;
    levelTwo *l2;
    levelOne *l1;

    if ((_pTMutex.lock() == Mutex::RC_Ok)) {
        l4 = _microflowNestedHashTable.get(sSA);
        if (l4 != NULL) {
            l3 = l4->get(sDA);
            if (l3 != NULL) {
                l2 = l3->get(sProtocol);
                if (l2 != NULL) {
                    l1 = l2->get(sSP);
                    if (l1 != NULL) {
                        _pTMutex.unlock();
                        return l1->get(sDP);
                    }
                }
            }
        }       
        _pTMutex.unlock();
    }
    return NULL;
}

void MicroflowNestedHashTable::put(const char* sSA, const char* sDA, const char* sProtocol,
                                   const char* sSP, const char* sDP, TrafficElement *t)
{
    levelFour   *l4;
    levelThree  *l3;
    levelTwo    *l2;
    levelOne    *l1;

    if ((_pTMutex.lock() == Mutex::RC_Ok)) {
        l4 = _microflowNestedHashTable.get(sSA);
        if (l4 == NULL) {
            l4 = new levelFour(false, true, true, true);
            l3 = new levelThree(false, true, true, true);
            l2 = new levelTwo(false, true, true, true);
            l1 = new levelOne(false, true, true, true);
            _microflowNestedHashTable.put(sSA, l4);
            l4->put(sDA, l3);
            l3->put(sProtocol, l2);
            l2->put(sSP, l1);
            l1->put(sDP, t);
        }
        else {
            l3 = l4->get(sDA);
            if (l3 == NULL) {
                l3 = new levelThree(false, true, true, true);
                l2 = new levelTwo(false, true, true, true);
                l1 = new levelOne(false, true, true, true);

                l4->put(sDA, l3);
                l3->put(sProtocol, l2);
                l2->put(sSP, l1);
                l1->put(sDP, t);
            }
            else {
                l2 = l3->get(sProtocol);
                if (l2 == NULL) {
                    l2 = new levelTwo(false, true, true, true);
                    l1 = new levelOne(false, true, true, true);

                    l3->put(sProtocol, l2);
                    l2->put(sSP, l1);
                    l1->put(sDP, t);
                }
                else {
                    l1 = l2->get(sSP);
                    if (l1 == NULL) {
                        l1 = new levelOne(false, true, true, true);

                        l2->put(sSP, l1);
                        l1->put(sDP, t);
                    }
                    else {
                        l1->remove(sDP);
                        l1->put(sDP, t);
                    }

                }
            }
        }               
        _pTMutex.unlock();
    }
}

void MicroflowNestedHashTable::print()
{
    if ((_pTMutex.lock() == Mutex::RC_Ok)) {

        for (levelFive::Iterator v = _microflowNestedHashTable.
             getAllElements(); !v.end(); v.nextElement()) {
            levelFour *l4 = v.getValue();

            for (levelFour::Iterator iv = l4->getAllElements(); !iv.end(); 
                 iv.nextElement()) {
                levelThree *l3 = iv.getValue();

                for (levelThree::Iterator iii = l3->getAllElements(); 
                     !iii.end(); iii.nextElement()) {
                    levelTwo *l2 = iii.getValue();

                    for (levelTwo::Iterator ii = l2->getAllElements(); 
                         !ii.end(); ii.nextElement()) {
                        levelOne *l1 = ii.getValue();

                        for (levelOne::Iterator i = l1->getAllElements(); 
                             !i.end(); i.nextElement()) {
                            TrafficElement* tel = i.getValue();

                            printf("%s : %s : %s : %s : %s : %f, last update: %llu\n",
                                   v.getKey(), iv.getKey(), iii.getKey(),
                                   ii.getKey(), i.getKey(),
                                   tel->tiaTraffic.getAverage(), tel->i64TimeOfLastChange);
                        }
                    }
                }
            }
        }       
        _pTMutex.unlock();
    }
}

}