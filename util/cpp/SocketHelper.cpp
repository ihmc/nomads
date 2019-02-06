/*
* SocketHelper.cpp
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
*
* Author: Giacomo Benincasa     (gbenincasa@ihmc.us)
*/

#include "SocketHelper.h"

#if defined (WIN32)
    #include <winsock2.h>
#elif defined (UNIX)
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#else
    #error Must Define WIN32 or UNIX!
#endif

using namespace NOMADSUtil;

int SocketHelper::getLastError()
{
    #ifdef WIN32
        return WSAGetLastError();
    #else
        // TODO: may want to implement this for UNIX systems...
        return 0;
    #endif
}

const char * SocketHelper::getLastErrorDescription()
{
    switch (getLastError()) {
    #ifdef WIN32
        case WSANOTINITIALISED:
            return "A successful WSAStartup call must occur before using this function.";

        case WSAENETDOWN:
            return "The network subsystem has failed.";

        case WSAEACCES:
            return "The requested address is a broadcast address, but the appropriate "
                "flag was not set.Call setsockopt(Windows Sockets) with the SO_BROADCAST "
                "parameter to allow the use of the broadcast address.";

        case WSAEINVAL:
            return "An unknown flag was specified, or MSG_OOB was specified for a "
                "socket with SO_OOBINLINE enabled.";

        case WSAEINTR:
            return "The socket was closed.";

        case WSAEINPROGRESS:
            return "A blocking Winsock call is in progress, or the service provider"
                " is still processing a callback function.";

        case WSAEFAULT:
            return "The buf or to parameter is not part of the user address space, "
                "or the tolen parameter is too small.";

        case WSAENETRESET:
            return "The connection has been broken due to keep - alive activity "
                "detecting a failure while the operation was in progress.";

        case WSAENOBUFS:
            return "No buffer space is available.";

        case WSAENOTCONN:
            return "The socket is not connected(connection - oriented sockets only).";

        case WSAENOTSOCK:
            return "The descriptor is not a socket.";

        case WSAEOPNOTSUPP:
            return "MSG_OOB was specified, but the socket is not stream style such "
                "as type SOCK_STREAM, out of band(OOB) data is not supported in "
                "the communication domain associated with this socket, or the socket"
                " is unidirectional and supports only receive operations.";

        case WSAESHUTDOWN:
            return "The socket has been shut down.It is not possible to call sendto"
                " on a socket after shutdown has been invoked with how set to SD_SEND or SD_BOTH.";

        case WSAEWOULDBLOCK:
            return "The socket is marked as nonblocking and the requested operation"
                " would block.";

        case WSAEMSGSIZE:
            return "The socket is message - oriented, and the message is larger than"
                " the maximum supported by the underlying transport.";

        case WSAEHOSTUNREACH:
            return "The remote host cannot be reached from this host at this time.";

        case WSAECONNABORTED:
            return "The virtual circuit was terminated due to a time - out or other"
                " failure.The application should close the socket because it is no"
                " longer usable.";

        case WSAECONNRESET:
            return "The virtual circuit was reset by the remote side executing a "
                "hard or abortive close.For UDP sockets, the remote host was unable"
                " to deliver a previously sent UDP datagram and responded with a "
                "Port Unreachable ICMP packet.The application should close the "
                "socket because it is no longer usable.";

        case WSAEADDRNOTAVAIL:
            return "The remote address is not a valid address, for example, ADDR_ANY.";

        case WSAEAFNOSUPPORT:
            return "Addresses in the specified family cannot be used with this socket.";

        case WSAEDESTADDRREQ:
            return "A destination address is required.";

        case WSAENETUNREACH:
            return "The network cannot be reached from this host at this time.";

        case WSAETIMEDOUT:
            return "The connection has been dropped because of a network failure "
                "or because the system on the other end went down without notice.";
    #else
    #endif

    default:
        return "Unknown error.";
    }
}
