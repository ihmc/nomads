#ifndef NETSENSOR_TCPRTTInterfaceTable__INCLUDED
#define NETSENSOR_TCPRTTInterfaceTable__INCLUDED
/*
* TCPRTTInterfaceTable.h
* Author: bordway@ihmc.us rfronteddu@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2018 IHMC.
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

#include "StringHashtable.h"
#include "TCPStream.h"
#include "PtrLList.h"
#include "NLFLib.h"

namespace IHMC_NETSENSOR
{

    class TCPRTTInterfaceTable
    {
    public:
        TCPRTTInterfaceTable(void);
        TCPRTTInterfaceTable(uint32 ui32Resolution);
        ~TCPRTTInterfaceTable(void);

        void cleanTable(const uint32 ui32CleaningNumber);
        void put(const char *pInterfaceName, TCPStream & tcpStream);
        void printContent(void);
        TCPStream* getStream(TCPStreamData data);
        NOMADSUtil::PtrLList<TCPStream>* getTCPStreamsOnInterface(const char *pInterfaceName);
        bool mutexTest(void);

    private:
        NOMADSUtil::StringHashtable<NOMADSUtil::PtrLList<TCPStream>> _tcpStreamListInterfaceTable;
        NOMADSUtil::Mutex _mutex;
        uint32 _ui32msResolution;
    };
}

#endif
