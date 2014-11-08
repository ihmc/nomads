#ifndef INCL_TCP_MANAGER_H
#define INCL_TCP_MANAGER_H

/*
 * TCPManager.h
 * 
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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
 * The TCPManager class implements the TCP protocol to permit
 * communications between the NetProxy and the local applications.
 */

#include "FTypes.h"


namespace ACMNetProxy
{
    class NetworkInterface;
    class Entry;
    class TCPConnTable;
    class ConnectionManager;
    class NetProxyConfigManager;
    class PacketRouter;

    class TCPManager
    {
        public:
            static TCPManager * const getTCPManager (void);
            ~TCPManager (void);

            static int handleTCPPacketFromHost (const uint8 *pPacket, uint16 ui16PacketLen);
            static int sendTCPPacketToHost (Entry * const pEntry, uint8 ui8TCPFlags, uint32 ui32SeqNum, const uint8 * const pui8Payload = NULL, uint16 ui16PayloadLen = 0);

            static int openTCPConnectionToHost (uint32 ui32RemoteProxyIP, uint32 ui32RemoteProxyUniqueID, uint16 ui16RemoteID, uint32 ui32LocalIP, uint16 ui16LocalPort,
                                                uint32 ui32RemoteIP, uint16 ui16RemotePort, uint8 ui8CompressionTypeAndLevel);
            static int tcpConnectionToHostOpened (uint16 ui16LocalID, uint16 ui16RemoteID, uint32 ui32RemoteProxyUniqueID, uint8 ui8CompressionTypeAndLevel);
            static int sendTCPDataToHost (uint16 ui16LocalID, uint16 ui16RemoteID, const uint8 * const pui8CompData, uint16 ui16CompDataLen, uint8 ui8Flags);
            static int closeTCPConnectionToHost (uint16 ui16LocalID, uint16 ui16RemoteID);
            static int resetTCPConnectionToHost (uint16 ui16LocalID, uint16 ui16RemoteID);

        private:
            TCPManager (void);
            explicit TCPManager (const TCPManager& rTCPManager);

            static TCPConnTable * const _pTCPConnTable;
            static ConnectionManager * const _pConnectionManager;
            static NetProxyConfigManager * const _pConfigurationManager;
            static PacketRouter * const _pPacketRouter;
    };

    inline TCPManager::TCPManager (void) {}

    inline TCPManager::~TCPManager (void) {}

    inline TCPManager * const TCPManager::getTCPManager (void)
    {
        static TCPManager tcpManager;

        return &tcpManager;
    }

}

#endif      //#ifndef INCL_TCP_MANAGER_H
