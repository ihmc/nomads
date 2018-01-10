#ifndef NETSENSOR_InterfacesInfoTable__INCLUDED
#define NETSENSOR_InterfacesInfoTable__INCLUDED
/*
* InterfacesInfoTable.h
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
*
* This is a container for the InterfaceInfo objects
*
*/
#include"InterfaceInfo.h"
#include"StringHashtable.h"
#include"Mutex.h"
namespace IHMC_NETSENSOR
{
class InterfacesInfoTable
{
public:
    InterfaceInfo *getElementAndSetLock(const char* interfaceName);
    InterfacesInfoTable();
    /*
    * The II table owns the pointer
    */
    void put(InterfaceInfo *ii);
    void releaseLock(void);
//<--------------------------------------------------------------------------->

private:
    NOMADSUtil::StringHashtable<InterfaceInfo> _interfacesInfoTable;
    NOMADSUtil::Mutex _pTMutex;

};


inline InterfacesInfoTable::InterfacesInfoTable()
{
    _interfacesInfoTable.configure(true, true, true, true);
}

inline void InterfacesInfoTable::put(InterfaceInfo *pII)
{
    if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
        InterfaceInfo *newPII = _interfacesInfoTable.get(pII->sInterfaceName);
        if (newPII == NULL) {
            newPII = pII;
            _interfacesInfoTable.put(pII->sInterfaceName, pII);
        }
        else {
            delete _interfacesInfoTable.remove(pII->sInterfaceName);
            _interfacesInfoTable.put(pII->sInterfaceName, pII);
        }
        _pTMutex.unlock();
    }
}

inline InterfaceInfo* InterfacesInfoTable::getElementAndSetLock(
    const char* interfaceName)
{
    if ((_pTMutex.lock() == NOMADSUtil::Mutex::RC_Ok)) {
        InterfaceInfo *tmp = _interfacesInfoTable.get(interfaceName);
        if (tmp != NULL) { return tmp; }
        else {
            _pTMutex.unlock();
            return NULL;
        }
    }
    else { return NULL; }
}

inline void InterfacesInfoTable::releaseLock(void)
{
    _pTMutex.unlock();
}

}
#endif