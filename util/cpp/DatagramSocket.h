/*
 * DatagramSocket.h
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

#ifndef INCL_DATAGRAM_SOCKET_H
#define INCL_DATAGRAM_SOCKET_H

#include "FTypes.h"
#include "InetAddr.h"
#include "StringHashtable.h"
#include "UInt32Hashtable.h"

#define DATAGRAM_SOCKET_INCLUDE_RATE_LIMIT_CODE

namespace NOMADSUtil
{
    class DatagramSocket
    {
        public:
            #if defined (UNIX)
                DatagramSocket (bool bPktInfoEnabled=false);
            #else
                DatagramSocket (void);
            #endif
            virtual ~DatagramSocket (void);

            enum SocketType {
                ST_Unknown = 0x00,
                ST_UDP = 0x01,
                ST_MulticastUDP = 0x02,
                ST_RawUDP = 0x03,
                ST_Proxy = 0x04
            };

            virtual SocketType getType (void) = 0;

            // Close the socket
            virtual int close (void) = 0;

            // Get the local port
            // Returns 0 in case of error
            virtual uint16 getLocalPort (void) = 0;

            // Get the local IP to which the socket has been bound
            virtual InetAddr getLocalAddr (void) = 0;

            // Get the file descriptor for the underlying socket
            virtual int getLocalSocket (void) = 0;

            // Get the Maximux Transfer Unit
            virtual uint16 getMTU (void) = 0;

            // Get the receive buffer size
            virtual int getReceiveBufferSize (void) = 0;

            // Set the receive buffer size
            virtual int setReceiveBufferSize (int iSize) = 0;

            // Get the send buffer size
            virtual int getSendBufferSize (void) = 0;

            // Returns true if the socket is configured to receive ancillary data
            bool pktInfoEnabled (void);

            // Set the send buffer size
            virtual int setSendBufferSize (int iSize) = 0;

            // Set the socket timeout (in milliseconds)
            // NOTE: On Win32 systems, the resolution seems to be around 500 ms
            virtual int setTimeout (uint32 ui32TimeoutInMS) = 0;

            // Get the transmit rate limit for this socket
            // Returns the limit in bytes per second or 0 if there is no limit set
            virtual uint32 getTransmitRateLimit (void);

            // Set the transmit rate limit for this socket expressed in Bytes per
            // second.
            // A value of 0 turns off the transmit rate limit
            // Returns 0 if successful or a negative value in case of error
            virtual int setTransmitRateLimit (uint32 ui32RateLimitInBps);

            // Get the transmit rate limit for this socket for the specified destination address
            // If the destination address is 0, it returns the global rate limit
            // Returns the limit in bytes per second or 0 if there is no limit set
            virtual uint32 getTransmitRateLimit (const char *pszDestinationAddr);

            // Set the transmit rate limit for this socket for the specified destination address
            // If the destination address is 0, the limit affects all outgoing traffic
            // A value of 0 turns off the transmit rate limit
            // Returns 0 if successful or a negative value in case of error
            virtual int setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit);

            // Get the number of bytes available to read
            virtual int bytesAvail (void) = 0;

            // Check whether it is ok to transmit
            virtual bool clearToSend (void) = 0;

            // Send a packet to the specified IPv4 address and port
            // Returns the number of bytes sent or a negative value to indicate an error
            virtual int sendTo (uint32 ui32IPv4Addr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints = NULL) = 0;

            // Send a packet to the specified address (for IPv4, this should be specified in the dotted quad
            //     such as "143.88.7.1" and port)
            // Returns the number of bytes sent or a negative value to indicate an error
            virtual int sendTo (const char *pszAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints = NULL) = 0;

            // Receive a packet
            // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
            virtual int receive (void *pBuf, int iBufSize) = 0;

            // Receive a packet and the sender's IP address and port
            // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
            virtual int receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr) = 0;

            #if defined (UNIX)
                // Receive a packet, the sender's IP address and port, and the index of the incoming interface.
                // Returns the number of bytes received, 0 in case of a timeout, or a negative value to indicate an error
                virtual int receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr, int &iIncomingIfaceIdx) = 0;
            #endif

            // Returns the error code for the last failure
            virtual int getLastError (void) = 0;

        protected:
            struct TransmitLimit
            {
                TransmitLimit (void);
                void reset (void);
                uint32 ui32RateLimitInBps;           // Specified in bytes/sec
                uint32 ui32RateLimitInterval;        // The enforcement interval in milliseconds
                uint32 ui32BytesPerInterval;         // Number of bytes that are allowed to be written in one interval;
                                                     // computed based on the _rateLimit and the _rateLimitInterval
                int64 i64IntervalStartTime;          // Time in milliseconds when interval started
                uint32 ui32BytesWrittenInInterval;   // The number of bytes written so far in this interval
            };
            TransmitLimit _transmitLimit;
            StringHashtable<TransmitLimit> _perStrAddrTransmitLimits;
            UInt32Hashtable<TransmitLimit> _perIPv4AddrTransmitLimits;

            // Called by the subclasses after transmitting a packet whose size is specified in ui32PacketSize
            // If a transmit limit is in place, this method will update counters and then check to see if
            //     the limit has been exceeded by transmitting this packet. If so, the thread is blocked (put to
            //     sleep) for the length of time necessary to comply with the rate limit in place.
            void enforceTransmitRateLimit (const char *pszAddress, uint32 ui32PacketSize);
            void enforceTransmitRateLimit (uint32 ui32IPv4Addr, uint32 ui32PacketSize);

        private:
            const bool _bPktInfoEnabled;
    };

    inline DatagramSocket::~DatagramSocket (void)
    {
    }
}

#endif   // #ifndef INCL_DATAGRAM_SOCKET_H
