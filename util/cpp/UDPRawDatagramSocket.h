/*
 * UDPRawDatagramSocket.h
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

#ifndef INCL_UDP_RAW_DATAGRAM_SOCKET_H
#define INCL_UDP_RAW_DATAGRAM_SOCKET_H

#if defined (WIN32)
    #include <mutex>
    #include <winsock2.h>
#elif defined (UNIX)
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#else
    #error Must Define WIN32 or UNIX!
#endif

#include "InetAddr.h"

namespace NOMADSUtil
{
    class UDPRawDatagramSocket
    {
        public:
            UDPRawDatagramSocket (void);
            ~UDPRawDatagramSocket (void);

            // Initialize the socket and port (port is applicable only to receive packets)
            // If port is 0, a random port is chosen
            // ui32BindAddr can be used to specify a particular address to bind to
            int init (uint16 ui16Port = 0, uint32 ui32BindAddr = INADDR_ANY);

            // Close the socket
            int close (void);

            // Get the local port
            // Returns 0 in case of error
            uint16 getLocalPort (void);

            // Get the local IP to which the socket has been bound
            InetAddr getLocalAddr (void);

            // Get the Maximux Transfer Unit
            uint16 getMTU (void);

            // Get the file descriptor for the underlying socket
            int getLocalSocket (void);

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

            int getTimeout (void);

            // Get the number of bytes available to read
            int bytesAvail (void);

            // Send a packet to the specified IP address and port
            // Returns the number of bytes sent or a negative value to indicate an error
            int sendTo (uint32 ui32SrcIPAddr, uint16 ui16SrcUDPPort, uint32 ui32DestIPAddr, uint16 ui16DestUDPPort, const void *pBuf, int iBufSize);

            // Send a packet to the specified IP address (specified in the dotted quad
            //     such as "143.88.7.1" and port)
            // Returns the number of bytes sent or a negative value to indicate an error
            int sendTo (const char *pszSrcIPAddr, uint16 ui16SrcUDPPort, const char *pszDestIPAddr, uint16 ui16DestUDPPort, const void *pBuf, int iBufSize);

            // Receive a packet
            // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
            int receive (void *pBuf, int iBufSize);

            // Receive a packet and the sender's IP address and port
            // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
            int receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr);

            // Returns the error code for the last failure
            int getLastError (void);


            // Returns an InetAddr containing the IPv4 address assigned to the interface that would be used to send datagrams to the target InetAddr, according to the machine's routing rules
            static InetAddr getLocalIPv4AddressToReachRemoteIPv4Address (const InetAddr & iaRemoteAddress);

        protected:
            int sockfd;
            InetAddr lastPacketAddr;
            #if defined (WIN32)
                static std::mutex _mtxWinsockInit;
                static bool bWinsockInitialized;
            #endif
            #if defined (UNIX)
                uint32 _ui32TimeOut;
            #endif
    };
}

#endif   // #ifndef INCL_UDP_RAW_DATAGRAM_SOCKET_H
