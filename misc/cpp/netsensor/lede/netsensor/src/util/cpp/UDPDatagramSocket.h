/*
 * UDPDatagramSocket.h
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

#ifndef INCL_UDP_DATAGRAM_SOCKET_H
#define INCL_UDP_DATAGRAM_SOCKET_H

#if defined (WIN32)
    #include <winsock2.h>
    #include <Mswsock.h>
#elif defined (UNIX)
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#else
    #error Must Define WIN32 or UNIX!
#endif

#include "DatagramSocket.h"

namespace NOMADSUtil
{
    class UDPDatagramSocket : public DatagramSocket
    {
        public:
            #if defined UNIX
                UDPDatagramSocket (bool bPktInfoEnabled = false);
            #else
                UDPDatagramSocket (void);
            #endif
            virtual ~UDPDatagramSocket (void);

            SocketType getType (void);

            // Initialize the socket and port (port is applicable only to receive packets)
            // If port is 0, a random port is chosen
            // ui32ListenAddr can be useed to specify a particular address to listen to
            int init (uint16 ui16Port = 0, uint32 ui32ListenAddr = INADDR_ANY);

            // Close the socket
            int close (void);

            // Shuts down the reading, writing or both the sides of the socket
            int shutdown (bool bReadMode, bool bWriteMode);

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

            // Check whether it is ok to transmit
            bool clearToSend (void);

            // Send a packet to the specified IP address and port
            // Returns the number of bytes sent or a negative value to indicate an error
            int sendTo (uint32 ui32IPAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints = NULL);

            // Send a packet to the specified IP address (specified in the dotted quad
            //     such as "143.88.7.1" and port)
            // Returns the number of bytes sent or a negative value to indicate an error
            int sendTo (const char *pszIPAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints = NULL);

            // Receive a packet
            // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
            int receive (void *pBuf, int iBufSize);

            // Receive a packet and the sender's IP address and port
            // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
            int receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr);

            #if defined (WIN32) && defined (NTDDI_VERSION) && defined (NTDDI_WINXP) && (NTDDI_VERSION >= NTDDI_WINXP)
                // Receive a packet, the sender's IP address and port, and the receiver's IP address
                // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
                int receive (void *pBuf, int iBufSize, InetAddr *pLocalAddr, InetAddr *pRemoteAddr, int iFlags = 0);
            #elif defined (UNIX)
                // Receive a packet, the sender's IP address and port, and the receiver's IP address
                // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
                int receive (void *pBuf, int iBufSize, InetAddr *pLocalAddr, InetAddr *pRemoteAddr);

                // Receive a packet, the sender's IP address and port, and the index of the incoming interface.
                // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
                int receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr, int &iIncomingIfaceIdx);
            #endif

            // Returns the error code for the last failure
            int getLastError (void);

        protected:
            int sockfd;
            uint16 _ui16Port;
            uint32 _ui32ListenAddr;
            InetAddr lastPacketAddr;
            #if defined (WIN32)
                static bool bWinsockInitialized;
            #elif defined (UNIX)
                uint32 _ui32TimeOut;
            #endif

        private:
            #if defined (WIN32) && defined (NTDDI_VERSION) && defined (NTDDI_WINXP) && (NTDDI_VERSION >= NTDDI_WINXP)
                LPFN_WSARECVMSG WSARecvMsg;
                GUID WSARecvMsg_GUID = WSAID_WSARECVMSG;
                DWORD numberOfBytes;
            #elif defined (UNIX)
                char msgCtrlBuf[0x100];
            #endif
    };


    inline DatagramSocket::SocketType UDPDatagramSocket::getType (void)
    {
        return DatagramSocket::ST_UDP;
    }

    inline int UDPDatagramSocket::receive (void *pBuf, int iBufSize)
    {
        return receive (pBuf, iBufSize, &lastPacketAddr);
    }

}

#endif   // #ifndef INCL_UDP_DATAGRAM_SOCKET_H
