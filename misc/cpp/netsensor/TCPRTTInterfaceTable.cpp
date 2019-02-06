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
#include <forward_list>


using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
    TCPRTTInterfaceTable::TCPRTTInterfaceTable(void) : 
        _tcpStreamListInterfaceTable(true, true, true, true)

    {

    }

    TCPRTTInterfaceTable::~TCPRTTInterfaceTable(void)
    {
        _tcpStreamListInterfaceTable.removeAll();
    }

    TCPRTTInterfaceTable::TCPRTTInterfaceTable(uint32 ui32Resolution) :
        _ui32msResolution(ui32Resolution),
        _tcpStreamListInterfaceTable(true, true, true, true)
    {

    }

    void TCPRTTInterfaceTable::cleanTable(const uint32 ui32MaxCleaningNumber)
    {
        int64 currentTime = getTimeInMilliseconds();
        //TCPStream* streamCleaner[C_MAX_CLEANING_NUMBER];
        std::forward_list <TCPStream *> list;
        if (_mutex.lock() == Mutex::RC_Ok) {
            if (_tcpStreamListInterfaceTable.getCount() > 0) {
                uint32 ui32CleaningCounter = 0;
                for (StringHashtable<PtrLList<TCPStream>>::Iterator i = _tcpStreamListInterfaceTable.getAllElements();
                    !i.end() && ui32CleaningCounter < ui32MaxCleaningNumber; i.nextElement()) {

                    uint32 ui32StreamCleaningCounter = 0;
                    PtrLList<TCPStream> *pStreamList = i.getValue();
                    TCPStream *pStream;
                    pStreamList->resetGet();
                    while((pStream = pStreamList->getNext()) != nullptr) {
                        if (pStream != nullptr) {
                            ui32CleaningCounter += pStream->clean(ui32MaxCleaningNumber - ui32CleaningCounter);

                            int64 expiredT = pStream->i64LastUpdateTime + C_ENTRY_TIME_VALIDITY;
                            if (pStream->getCount() == 0 && 
                                (pStream->hasClosed() || currentTime > expiredT)) {
                                //streamCleaner[ui32StreamCleaningCounter] = pStream;
                                list.push_front (pStream);
                                ui32StreamCleaningCounter++;
                            }
                        }
                    }

                    for (auto iter = list.begin(); iter != list.end(); iter++)
                    {
                        delete pStreamList->remove(*iter);
                    }
                    /*for (uint32 counter = 0; counter < ui32StreamCleaningCounter; counter++)
                    {
                        delete pStreamList->remove(streamCleaner[counter]);
                    }*/
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
            for (StringHashtable<PtrLList<TCPStream>>::Iterator i = _tcpStreamListInterfaceTable.getAllElements();
                !i.end(); i.nextElement()) {
                PtrLList<TCPStream> *pStreamList = i.getValue();
                TCPStream *pStream;
                pStreamList->resetGet();
                printf("\n%s :\n------------------\n", i.getKey());
                while ((pStream = pStreamList->getNext()) != nullptr) {
                    if (pStream->getCount() > 0) {
                        pStream->print();
                    }
                }                
            }
            _mutex.unlock();
        }
    }

    // Pass the tcpData to the proper TCPRTTTable
    void TCPRTTInterfaceTable::put(const char *pInterfaceName, TCPStream & tcpStream)
    {
        if (pInterfaceName == nullptr) {
            printf("TCPRTTHandler:put::Null interface name\n");
        }

        if (_mutex.lock() == Mutex::RC_Ok) {
            PtrLList<TCPStream> *pStreamList = _tcpStreamListInterfaceTable.get(pInterfaceName);
            if (pStreamList == nullptr) {
                PtrLList<TCPStream> *pStreamList = new PtrLList<TCPStream>();
                pStreamList->append(&tcpStream);
                _tcpStreamListInterfaceTable.put(pInterfaceName, pStreamList);
            }
            else {
                pStreamList->append(&tcpStream);
            }
            _mutex.unlock();
        }
    }

    TCPStream* TCPRTTInterfaceTable::getStream(TCPStreamData data)
    {
        if (_mutex.lock() == Mutex::RC_Ok) {
            for (StringHashtable<PtrLList<TCPStream>>::Iterator i = _tcpStreamListInterfaceTable.getAllElements();
                !i.end(); i.nextElement()) {
                PtrLList<TCPStream> *pStreamList = i.getValue();
                TCPStream *pStream;
                pStreamList->resetGet();
                while ((pStream = pStreamList->getNext()) != nullptr) {
                    if (pStream->getData() == data) {
                        _mutex.unlock();
                        return pStream;
                    }
                }
            }
            _mutex.unlock();
        }
        return nullptr;
    }
    PtrLList<TCPStream>* TCPRTTInterfaceTable::getTCPStreamsOnInterface(const char * pInterfaceName)
    {
        if (_mutex.lock() == Mutex::RC_Ok) {
            PtrLList<TCPStream> *pList = _tcpStreamListInterfaceTable.get(pInterfaceName);
            _mutex.unlock();
            return pList;
        }
        return nullptr;
    }
}