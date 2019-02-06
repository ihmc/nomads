/*
 * ManageableDatagramSocket.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on September 2, 2015, 2:00 PM
 */

#ifndef INCL_MANAGEABLE_DATAGRAM_SOCKET_H
#define INCL_MANAGEABLE_DATAGRAM_SOCKET_H

#include "DatagramSocket.h"
#include "ManageableThread.h"
#include "Mutex.h"
#include "StringHashset.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;

    class ManageableDatagramSocket : public DatagramSocket
    {
        public:
            ~ManageableDatagramSocket (void);

            SocketType getType (void);
            int close (void);
            uint16 getLocalPort (void);
            InetAddr getLocalAddr (void);
            int getLocalSocket (void);
            uint16 getMTU (void);
            int getReceiveBufferSize (void) const;
            int setReceiveBufferSize (int iSize);
            // Get the send buffer size
            int getSendBufferSize (void) const;
            bool pktInfoEnabled (void);
            int setSendBufferSize (int iSize);
            int setTimeout (uint32 ui32TimeoutInMS);
            uint32 getTransmitRateLimit (void);
            int setTransmitRateLimit (uint32 ui32RateLimitInBps);
            uint32 getTransmitRateLimit (const char *pszDestinationAddr);
            int setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit);
            int bytesAvail (void);
            bool clearToSend (void);

            // Send a packet to the specified IPv4 address and port
            // Returns the number of bytes sent or a negative value to indicate an error
            int sendTo (uint32 ui32IPv4Addr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints = NULL);
            int sendTo (const char *pszAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints = NULL);
            int receive (void *pBuf, int iBufSize);
            int receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr);
            #if defined (UNIX)
                int receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr, int &iIncomingIfaceIdx);
            #endif

            int getLastError (void);

        private:
            friend class ManageableDatagramSocketManager;
            explicit ManageableDatagramSocket (DatagramSocket *pDGramSock);

        private:
            bool _bExclusive;
            DatagramSocket *_pDGramSock;
            Mutex _m;
            StringHashset _addresses;
    };

    class ManageableDatagramSocketManager : public ManageableThread
    {
        public:
            ManageableDatagramSocketManager (void);
            ~ManageableDatagramSocketManager (void);

            void run (void);

            ManageableDatagramSocket * getManageableDatagramSocket (DatagramSocket *pDGramSock);

        private:
            void parse (Reader *pReader);

        private:
            StringHashtable<ManageableDatagramSocket> _sockets;
    };

    struct MessageHeader
    {
        enum Offsets
        {
            RESET_LIST = 0x00,
            EXCLUSIVE_LIST = 0x01
        };

        int read (Reader *pReader);
        int write (Writer *pWriter);

        bool bReset;
        bool bExclusive;
    };

    inline
    DatagramSocket::SocketType ManageableDatagramSocket::getType (void)
    {
        return _pDGramSock->getType();
    }

    inline
    int ManageableDatagramSocket::close (void)
    {
        return _pDGramSock->close();
    }

    inline
    uint16 ManageableDatagramSocket::getLocalPort (void)
    {
        return _pDGramSock->getLocalPort();
    }

    inline
    InetAddr ManageableDatagramSocket::getLocalAddr (void)
    {
        return _pDGramSock->getLocalAddr();
    }

    inline
    int ManageableDatagramSocket::getLocalSocket (void)
    {
        return _pDGramSock->getLocalSocket();
    }

    inline
    uint16 ManageableDatagramSocket::getMTU (void)
    {
        return _pDGramSock->getMTU();
    }

    inline
    int ManageableDatagramSocket::getReceiveBufferSize (void) const
    {
        return _pDGramSock->getReceiveBufferSize();
    }

    inline
    int ManageableDatagramSocket::setReceiveBufferSize (int iSize)
    {
        return _pDGramSock->setReceiveBufferSize (iSize);
    }

    inline
    int ManageableDatagramSocket::getSendBufferSize (void) const
    {
        return _pDGramSock->getSendBufferSize();
    }

    inline
    bool ManageableDatagramSocket::pktInfoEnabled (void)
    {
        return _pDGramSock->pktInfoEnabled();
    }

    inline
    int ManageableDatagramSocket::setSendBufferSize (int iSize)
    {
        return _pDGramSock->setSendBufferSize (iSize);
    }

    inline
    int ManageableDatagramSocket::setTimeout (uint32 ui32TimeoutInMS)
    {
        return _pDGramSock->setTimeout (ui32TimeoutInMS);
    }

    inline
    uint32 ManageableDatagramSocket::getTransmitRateLimit (void)
    {
        return _pDGramSock->getTransmitRateLimit();
    }

    inline
    int ManageableDatagramSocket::setTransmitRateLimit (uint32 ui32RateLimitInBps)
    {
        return _pDGramSock->setTransmitRateLimit (ui32RateLimitInBps);
    }

    inline
    uint32 ManageableDatagramSocket::getTransmitRateLimit (const char *pszDestinationAddr)
    {
        return _pDGramSock->getTransmitRateLimit (pszDestinationAddr);
    }

    inline
    int ManageableDatagramSocket::setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit)
    {
        return _pDGramSock->setTransmitRateLimit (pszDestinationAddr, ui32RateLimit);
    }

    inline
    int ManageableDatagramSocket::bytesAvail (void)
    {
        return _pDGramSock->bytesAvail();
    }

    inline
    bool ManageableDatagramSocket::clearToSend (void)
    {
        return _pDGramSock->clearToSend();
    }

    inline
    int ManageableDatagramSocket::getLastError (void)
    {
        return _pDGramSock->getLastError();
    }
}

#endif  // INCL_MANAGEABLE_DATAGRAM_SOCKET_H

