/*
 * MulticastUDPDatagramSocket.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 */

#ifndef INCL_MULTICAST_UDP_DATAGRAM_SOCKET_H
#define INCL_MULTICAST_UDP_DATAGRAM_SOCKET_H

#include "UDPDatagramSocket.h"

#include "DArray.h"
#include "ManageableThread.h"

namespace NOMADSUtil
{
    class MulticastRelayReceiver;

    class MulticastUDPDatagramSocket : public UDPDatagramSocket
    {
        public:
            #if defined UNIX
                MulticastUDPDatagramSocket (bool bPktInfoEnabled = false);
            #else
                MulticastUDPDatagramSocket (void);
            #endif
            virtual ~MulticastUDPDatagramSocket (void);

            SocketType getType (void);

            // Join a Multicast group
            // NOTE: ui32ListenAddr has to be the same address the socket is bound to
            int joinGroup (uint32 ui32MulticastGroup, uint32 ui32ListenAddr = INADDR_ANY);

            // Leave a Multicast group
            // NOTE: ui32ListenAddr has to be the same address the socket is bound to
            int leaveGroup (uint32 ui32MulticastGroup, uint32 ui32ListenAddr = INADDR_ANY);

            // Override from UDPDatagramSocket - to support relaying
            int sendTo (uint32 ui32IPAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints = NULL);

            // Set the Multicast TTL (default=1)
            int setTTL (uint8 ui8TTL);

            // Get the Multicast TTL
            uint8 getTTL (void);

            // Disable/Enable local loopback of multicast datagrams.
            int setLoopbackMode (bool bLoopbackMode);

            // Get the setting for local loopback of multicast datagrams
            bool getLoopbackMode (void);

            // Get the Maximux Transfer Unit
            uint16 getMTU (void);

            int addUnicastRelayAddress (InetAddr relayAddr);
            int addMulticastRelayAddress (InetAddr relayAddr);

        private:
            uint8 _ui8TTL;
            bool _bLoopbackMode;
            DArray<InetAddr*> _ucastRelayDestinations;
            DArray<InetAddr*> _mcastRelayDestinations;
    };

    inline DatagramSocket::SocketType MulticastUDPDatagramSocket::getType (void)
    {
        return DatagramSocket::ST_MulticastUDP;
    }

}

#endif // INCL_MULTICAST_UDP_DATAGRAM_SOCKET_H
