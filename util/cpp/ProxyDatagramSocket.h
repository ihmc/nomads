/*
 * ProxyDatagramSocket.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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
 * The ProxyDatagramSocket uses the following protocol to communicate with the CSR proxy server
 * All communication occurs over a TCP channel and any encoding of multi-byte data uses big endian ordering
 * 
 * In the following protocol description, S is the CSR proxy server and C is the ProxyDatagramSocket (client)
 * 
 * Startup handshake is
 *   S2C 'w'     
 *   S2C 'a', 4, interfaceAddress32  (note 4 is byte count of interfaceAddress)
 *   S2C 'm', MTU16
 *   C2S 'B', port16
 *   S2C 'b', port16
 *
 * ProxyDatagramSocket to proxy server to send data via a NeighborCast (only for CSR), MultiCast or UniCast
 *   C2S 'D', srcAddr32, srcPort16, destAddr32, destPort16, dataPacketBytes
 *       if high order byte of destAddr32 is between 224 and 239 then MultiCast
 *       else if high order byte of destAddr32 is 255 then NeighborCast
 *       else UniCast to destAddr32
 *
 * ProxyDatagramSocket to proxy server to send data via a NeighborCast (only for CSR),
 * MultiCast or UniCast excluding one destination
 *   C2S 'X', srcAddr32, srcPort16, destAddr32, destExcludeAddr32, destPort16, dataPacketBytes
 *       if high order byte of destAddr32 is between 224 and 239 then MultiCast
 *       else if high order byte of destAddr32 is 255 then NeighborCast
 *       else error (ignore the request)
 *       
 * Proxy server to ProxyDatagramSocket when data has been received
 *   S2C 'd', srcAddr32, srcPort16, destAddr32, destPort16, dataPacketBytes
 *
 * Proxy server to ProxyDatagramSocket for flow control
 *   S2C 'f' flowControl8 where 0 indicates that the ProxyDatagramSocket should stop sending data
 *           and 1 indicates that the ProxyDatagramSocket can resume sending data. Initial status
 *           is assumed to be clear to send (i.e. 1).
 */

#ifndef INCL_PROXY_DATAGRAM_SOCKET_H
#define INCL_PROXY_DATAGRAM_SOCKET_H

#if defined (WIN32)
    #include <winsock2.h>
#elif defined (UNIX)
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#else
    #error Must Define WIN32 or UNIX!
#endif

#include "DatagramSocket.h"
#include "ManageableThread.h"
#include "Mutex.h"

namespace NOMADSUtil
{

    class CommHelper2;
    class Connector;
    class Socket;

    class ProxyDatagramSocket : public DatagramSocket
    {
        public:
            ProxyDatagramSocket (void);
            virtual ~ProxyDatagramSocket (void);

            // Initialize the socket and port (port is applicable only to receive packets)
            // If port is 0, a random port is chosen
            // ui32ListenAddr can be useed to specify a particular address to listen to
            int init (const char *pszProxyServerAddr, uint16 ui16ProxyServerPort, uint16 ui16Port = 0);

            // Close the socket
            int close (void);

            // Get the local port
            // Returns 0 in case of error
            uint16 getLocalPort (void);

            // Get the local IP to which the socket has been bound
            InetAddr getLocalAddr (void);

            // Get the file descriptor for the underlying socket
            int getLocalSocket();

            // Get the Maximux Transfer Unit
            uint16 getMTU (void);

            // Get the receive buffer size
            int getReceiveBufferSize (void);

            // Set the receive buffer size
            int setReceiveBufferSize (int iSize);

            // Get the send buffer size
            int getSendBufferSize (void);

            // Set the send buffer size
            int setSendBufferSize (int iSize);

            // Set the socket timeout (in milliseconds)
            // NOTE: On Win32 systems, the resolution seems to be around 500 ms
            int setTimeout (uint32 ui32TimeoutInMS);

            // Get the number of bytes available to read
            int bytesAvail (void);

            // Check whether it is ok to transmit
            bool clearToSend (void);

            // Send a packet to the specified IP address (specified in the dotted quad notation
            //     such as "143.88.7.1") and port
            // The optional hints parameter is used with specific radios to take advantage of unique features
            // Currently, the only supported parameter is "exclude=<address>", which is used to support the
            //     feature of multicasting or broadcasting to all nodes except the one specified
            // Returns the number of bytes sent or a negative value to indicate an error
            int sendTo (const char *pszIPAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints = NULL);

            // Send a packet to the specified IP address and port
            // See the other sendTo method for a description of the hints parameter
            // Returns the number of bytes sent or a negative value to indicate an error
            int sendTo (uint32 ui32IPAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints = NULL);

            // Receive a packet
            // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
            int receive (void *pBuf, int iBufSize);

            // Receive a packet and the sender's IP address and port
            // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
            int receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr);

            #if defined (UNIX)
                // Receive a packet, the sender's IP address and port, and the index of the incoming interface.
                // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
                int receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr, int &iIncomingIfaceIdx);
            #endif

            // Returns the error code for the last failure
            int getLastError (void);

        protected:
            int connectToProxyServer (void);

        protected:
            static const uint16 MAXIMUM_PROXY_PACKET_SIZE = 2048;
            static const uint16 DEFAULT_MTU = 1400;        // Used when MTU is unknown
            static const uint16 RECONNECT_INTERVAL = 1000; // In Milliseconds

        protected:
            friend class Connector;

        protected:
            Mutex _m;
            InetAddr _proxyServerAddr;
            Socket *_pSocketToServer;
            NOMADSUtil::CommHelper2 *_pchToServer;
            Connector *_pConnector;
            bool _bConnectedToServer;
            bool _bReadyToConnect;
            int64 _i64LastReconnectAttemptTime;
            uint32 _ui32LocalAddr;
            uint16 _ui16LocalPort;
            uint16 _ui16MTU;
            InetAddr lastPacketAddr;
            bool _bClearToSend;
            uint32 _ui32Timeout;
            #if defined (WIN32)
                static bool bWinsockInitialized;
            #endif
            uint32 _ui32SentPacketCount;
            uint32 _ui32ReceivedPacketCount;
    };

    class Connector : public ManageableThread
    {
        public:
            Connector (ProxyDatagramSocket *pPDS);
            void run (void);
        private:
            ProxyDatagramSocket *_pPDS;
    };

}

#endif   // #ifndef INCL_PROXY_DATAGRAM_SOCKET_H
