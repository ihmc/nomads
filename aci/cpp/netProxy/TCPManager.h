#ifndef INCL_TCP_MANAGER_H
#define INCL_TCP_MANAGER_H

/*
 * TCPManager.h
 *
 * This file is part of the IHMC NetProxy Library/Component
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
 * The TCPManager class implements the TCP protocol to permit
 * communications between the NetProxy and the local applications.
 */

#include "FTypes.h"


namespace ACMNetProxy
{
    class PacketBufferManager;
    class Entry;
    class TCPConnTable;
    class LocalTCPTransmitterThread;
    class RemoteTCPTransmitterThread;
    class ConnectionManager;
    class ConfigurationManager;
    class StatisticsManager;
    class PacketRouter;


    class TCPManager
    {
    public:
        TCPManager (PacketBufferManager & rPBM, TCPConnTable & rTCPConnTable, ConnectionManager & rConnectionManager,
                    ConfigurationManager & rConfigurationManager, StatisticsManager & rStatisticsManager,
                    PacketRouter & rPacketRouter);
        explicit TCPManager (const TCPManager & rTCPManager) = delete;
        ~TCPManager (void);

        int handleTCPPacketFromHost (const uint8 * const pPacket, uint16 ui16PacketLen, LocalTCPTransmitterThread & rLocalTCPTransmitterThread,
                                     RemoteTCPTransmitterThread & rRemoteTCPTransmitterThread);

        int sendTCPPacketToHost (Entry * const pEntry, uint8 ui8TCPFlags, uint32 ui32SeqNum,
                                 const uint8 * const pui8Payload = nullptr, uint16 ui16PayloadLen = 0);

        int openTCPConnectionToHost (uint32 ui32RemoteProxyIP, uint32 ui32RemoteProxyUniqueID, uint16 ui16RemoteID,
                                     uint32 ui32LocalIP, uint16 ui16LocalPort, uint32 ui32RemoteIP,
                                     uint16 ui16RemotePort, uint8 ui8CompressionTypeAndLevel);
        int confirmTCPConnectionToHostOpened (uint16 ui16LocalID, uint16 ui16RemoteID, uint32 ui32RemoteProxyUniqueID,
                                              uint8 ui8CompressionTypeAndLevel);
        int sendTCPDataToHost (uint16 ui16LocalID, uint16 ui16RemoteID, const uint8 * const pui8CompData, uint16 ui16CompDataLen,
                               uint8 ui8Flags, LocalTCPTransmitterThread & rLocalTCPTransmitterThread);
        int closeTCPConnectionToHost (uint16 ui16LocalID, uint16 ui16RemoteID, RemoteTCPTransmitterThread & rRemoteTCPTransmitterThread);
        int resetTCPConnectionToHost (uint16 ui16LocalID, uint16 ui16RemoteID);

        static int flushAndSendCloseConnectionRequest (Entry * const pEntry);
        static int sendRemoteResetRequestIfNeeded (Entry * const pEntry);


    private:
        PacketBufferManager & _rPBM;

        TCPConnTable & _rTCPConnTable;
        ConnectionManager & _rConnectionManager;
        ConfigurationManager & _rConfigurationManager;
        PacketRouter & _rPacketRouter;
        StatisticsManager & _rStatisticsManager;
    };


    inline TCPManager::TCPManager (PacketBufferManager & rPBM, TCPConnTable & rTCPConnTable, ConnectionManager & rConnectionManager,
                                   ConfigurationManager & rConfigurationManager, StatisticsManager & rStatisticsManager,
                                   PacketRouter & rPacketRouter) :
        _rPBM{rPBM}, _rTCPConnTable{rTCPConnTable}, _rConnectionManager{rConnectionManager},
        _rConfigurationManager{rConfigurationManager}, _rPacketRouter{rPacketRouter},
        _rStatisticsManager{rStatisticsManager}
    { }

    inline TCPManager::~TCPManager (void) { }

}

#endif      //#ifndef INCL_TCP_MANAGER_H
