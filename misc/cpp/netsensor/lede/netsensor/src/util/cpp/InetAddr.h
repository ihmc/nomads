/*
 * InetAddr.h
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

#ifndef INCL_INET_ADDR_H
#define INCL_INET_ADDR_H
#if defined (WIN32)
    #include <winsock2.h>
#elif defined (UNIX)
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <string.h>
#else
    #error Must Define WIN32 or UNIX!
#endif

#include "EndianHelper.h"

namespace NOMADSUtil
{

    class InetAddr
    {
        public:
            InetAddr (void);

            // Constructs an instance initializing the ip address to that specified
            // NOTE: ulIPAddress must be network byte order
            InetAddr (unsigned long ulIPAddress);
            InetAddr (unsigned long ulIPAddress, unsigned short usPort);

            InetAddr (const char *pszIPAddress);
            InetAddr (const char *pszIPAddress, unsigned short usPort);

            virtual ~InetAddr (void);

            void clear (void);

            static bool isIPv4Addr (const char *pszAddr);

            // Sets the IP Address
            // NOTE: ulIPAddress must be in network byte order
            void setIPAddress (unsigned long ulIPAddress);

            void setIPAddress (const char *pszIPAddress);

            void setIPAddress (struct in_addr);

            // Sets the port
            // NOTE: usPort must be host byte order
            void setPort (unsigned short usPort);

            // Returns the IP address in network byte order
            unsigned long getIPAddress (void) const;

            // Returns the port in host byte order
            unsigned short getPort (void) const;

            /**
             * NOTE: the returned string must not be deallocated by the caller
             */
            const char * getIPAsString (void) const;

            InetAddr & operator = (const InetAddr &rhsAddr);
            bool operator == (const InetAddr &rhsAddr) const;
            bool operator != (const InetAddr &rhsAddr) const;

        public:
            static const uint16 MAX_IPADDR_STRING_LEN = 24;

        private:
            friend class UDPDatagramSocket;
            friend class UDPRawDatagramSocket;

            void updateIPAddrString (void);

            struct sockaddr_in _sa;
            char _szIPAddr [MAX_IPADDR_STRING_LEN];
    };

    inline InetAddr::InetAddr (void)
    {
        clear();
    }

    inline InetAddr::InetAddr (unsigned long ulIPAddress)
    {
        clear();
        _sa.sin_addr.s_addr = ulIPAddress;
        updateIPAddrString();
    }

    inline InetAddr::InetAddr (unsigned long ulIPAddress, unsigned short usPort)
    {
        clear();
        _sa.sin_addr.s_addr = ulIPAddress;
        setPort (usPort);
        updateIPAddrString();
    }

    inline InetAddr::InetAddr (const char *pszIPAddress)
    {
        clear();
        _sa.sin_addr.s_addr = inet_addr (pszIPAddress);
        updateIPAddrString();
    }

    inline InetAddr::InetAddr (const char *pszIPAddress, unsigned short usPort)
    {
        clear();
        _sa.sin_addr.s_addr = inet_addr (pszIPAddress);
        setPort (usPort);
        updateIPAddrString();
    }

    inline InetAddr::~InetAddr (void)
    {
        clear();
    }

    inline void InetAddr::clear (void)
    {
        _sa.sin_addr.s_addr = 0;
        _sa.sin_port = 0;
        _szIPAddr[0] = '\0';
    }

    inline void InetAddr::setIPAddress (unsigned long ulIPAddress)
    {
        _sa.sin_addr.s_addr = ulIPAddress;
        updateIPAddrString();
    }

    inline void InetAddr::setIPAddress (const char *pszIPAddress)
    {
        _sa.sin_addr.s_addr = inet_addr (pszIPAddress);
        updateIPAddrString();
    }

    inline void InetAddr::setIPAddress (struct in_addr IPAddress)
    {
        _sa.sin_addr = IPAddress;
        updateIPAddrString();
    }

    inline void InetAddr::setPort (unsigned short usPort)
    {
        _sa.sin_port = EndianHelper::htons (usPort);
    }

    inline unsigned long InetAddr::getIPAddress (void) const
    {
        return (unsigned long) (_sa.sin_addr.s_addr);
    }

    inline unsigned short InetAddr::getPort (void) const
    {
        return (unsigned short) EndianHelper::ntohs (_sa.sin_port);
    }

    inline const char * InetAddr::getIPAsString (void) const
    {
        return _szIPAddr;
    }

    inline InetAddr & InetAddr::operator = (const InetAddr &rhsAddr)
    {
        clear();
        _sa.sin_addr.s_addr = rhsAddr.getIPAddress();
        setPort (rhsAddr.getPort());
        updateIPAddrString();

        return (*this);
    }

    inline bool InetAddr::operator == (const InetAddr &rhsAddr) const
    {
        return ((getIPAddress() == rhsAddr.getIPAddress()) && (getPort() == rhsAddr.getPort()));
    }

    inline bool InetAddr::operator != (const InetAddr &rhsAddr) const
    {
        return ((getIPAddress() != rhsAddr.getIPAddress()) || (getPort() != rhsAddr.getPort()));
    }

    inline void InetAddr::updateIPAddrString (void)
    {
        #if defined (WIN32)
            const char *pszTemp = inet_ntoa (_sa.sin_addr);
            if (!pszTemp) {
                return;
            }
            strncpy (_szIPAddr, pszTemp, MAX_IPADDR_STRING_LEN);
        #else
            inet_ntop (AF_INET, &(_sa.sin_addr), _szIPAddr, MAX_IPADDR_STRING_LEN);
        #endif
        _szIPAddr[MAX_IPADDR_STRING_LEN-1] = '\0';
    }
}

#endif   // #ifndef INCL_INET_ADDR_H
