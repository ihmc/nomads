#ifndef NETSENSOR_TopologyCache__INCLUDED
#define NETSENSOR_TopologyCache__INCLUDED
/*
* TopologyCache.h
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
* Two-tabled class that contains all internal and external topology entries
* for quick retrieval.
*
*/
#include"PtrLList.h"
#include"StringHashtable.h"
#include"StrClass.h"
#include"Mutex.h"
#include"NetSensorConstants.h"


namespace IHMC_NETSENSOR
{
    class ExternalTopologyCacheObject
    {
    public:
        NOMADSUtil::String sIFaceName;
        NOMADSUtil::String sMACAddr;
        int64 creationTime;
        //<----------------------------->
        inline bool operator == (const ExternalTopologyCacheObject &rhs) const
        {
            return ((sIFaceName == rhs.sIFaceName) && (sMACAddr == rhs.sMACAddr));
        }
        
    };

    class InternalTopologyCacheObject
    {
    public:
        NOMADSUtil::String sIPAddr;
        int64 creationTime;

        inline bool operator ==(const InternalTopologyCacheObject &rhs) const
        {
            return sIPAddr == rhs.sIPAddr;
        }
    };

    typedef NOMADSUtil::PtrLList<ExternalTopologyCacheObject> ExternalTopList;
    typedef NOMADSUtil::PtrLList<InternalTopologyCacheObject> InternalTopList;

    class TopologyCache
    {
    public:
        TopologyCache(void);
       ~TopologyCache(void);

        NOMADSUtil::PtrLList<ExternalTopologyCacheObject>* findEntriesInExternal(const char *pIPAddr);
        bool isInternalEntry(const char *pIPAddr);

        void addInternalEntry(const char *pIPAddr);
        void addExternalEntry(const char *pIFaceName, const char *pIPAddr, const char *pMacAddr);

        void cleanTables(uint32 ui32MaxCleaningNumber);

    private:
        void cleanExternals(uint32 ui32MaxCleaningNumber);
        void cleanInternals(uint32 ui32MaxCleaningNumber);
        //<------------------------------------------------------------------>
        // Hashtable with IP address as key and list of interfaces
        // that have IP in their external
        NOMADSUtil::StringHashtable<ExternalTopList> _externalsTable;
        InternalTopList _internalsTable;
        NOMADSUtil::Mutex _pMutex;
    };
}
#endif
