/*
 * DatagramSocket.cpp
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
 */

#include "DatagramSocket.h"

#include "Logger.h"

#include <stdlib.h>

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

#if defined (UNIX)
    DatagramSocket::DatagramSocket (bool bPktInfoEnabled)
        : _bPktInfoEnabled (bPktInfoEnabled), _perStrAddrTransmitLimits (false, true, true, false), _perIPv4AddrTransmitLimits (true)
#else
    DatagramSocket::DatagramSocket (void)
        : _bPktInfoEnabled (false), _perStrAddrTransmitLimits (false, true, true, false), _perIPv4AddrTransmitLimits (true)
#endif
{
    const char *pszRateLimit = getenv ("DATAGRAM_SOCKET_TRANSMIT_RATE_LIMIT");
    if (pszRateLimit != NULL) {
        uint32 ui32RateLimit = atoui32 (pszRateLimit);
        setTransmitRateLimit (0, ui32RateLimit);
    }
}

uint32 DatagramSocket::getTransmitRateLimit (void)
{
    return _transmitLimit.ui32RateLimit;
}

bool DatagramSocket::pktInfoEnabled (void)
{
    return _bPktInfoEnabled;
}

int DatagramSocket::setTransmitRateLimit (uint32 ui32RateLimit)
{
    if (ui32RateLimit > 0) {
        _transmitLimit.ui32RateLimit = ui32RateLimit;
        _transmitLimit.ui32RateLimitInterval = 100;     // In Milliseconds
        _transmitLimit.ui32BytesPerInterval = (_transmitLimit.ui32RateLimit * _transmitLimit.ui32RateLimitInterval) / 1000;
        checkAndLogMsg ("DatagramSocket::setTransmitRateLimit1", Logger::L_Info,
                        "set transmit rate limit to %lu bytes/sec\n",
                        _transmitLimit.ui32RateLimit);
    }
    else {
        _transmitLimit.reset();
    }
    return 0;
}

uint32 DatagramSocket::getTransmitRateLimit (const char *pszDestinationAddr)
{
    if (pszDestinationAddr == NULL) {
        return _transmitLimit.ui32RateLimit;
    }
    TransmitLimit *pLimit = NULL;
    if (InetAddr::isIPv4Addr (pszDestinationAddr)) {
        pLimit = _perIPv4AddrTransmitLimits.get (inet_addr (pszDestinationAddr));
    }
    else {
        pLimit = _perStrAddrTransmitLimits.get (pszDestinationAddr);
    }
    if (pLimit) {
        return pLimit->ui32RateLimit;
    }
    return 0;
}

int DatagramSocket::setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit)
{
    if (ui32RateLimit > 0) {
        TransmitLimit *pLimit = NULL;
        if (pszDestinationAddr == NULL) {
            pLimit = &_transmitLimit;
        }
        else {
            TransmitLimit *pLimit = NULL;
            if (InetAddr::isIPv4Addr (pszDestinationAddr)) {
                uint32 ui32DestinationAddr = inet_addr (pszDestinationAddr);
                pLimit = _perIPv4AddrTransmitLimits.get (ui32DestinationAddr);
                if (pLimit == NULL) {
                    pLimit = new TransmitLimit();
                    _perIPv4AddrTransmitLimits.put (ui32DestinationAddr, pLimit);
                }
            }
            else {
                pLimit = _perStrAddrTransmitLimits.get (pszDestinationAddr);
                if (pLimit == NULL) {
                    pLimit = new TransmitLimit();
                    _perStrAddrTransmitLimits.put (pszDestinationAddr, pLimit);
                }
            }
        }
        pLimit->ui32RateLimit = ui32RateLimit;
        pLimit->ui32RateLimitInterval = 100;     // In Milliseconds
        pLimit->ui32BytesPerInterval = (pLimit->ui32RateLimit * pLimit->ui32RateLimitInterval) / 1000;
        checkAndLogMsg ("DatagramSocket::setTransmitRateLimit2", Logger::L_Info,
                        "set transmit rate limit for destination address <%s> set to %lu bytes/sec\n",
                        pszDestinationAddr, pLimit->ui32RateLimit);
    }
    else {
        if (pszDestinationAddr == NULL) {
            _transmitLimit.reset();
        }
        else {
            if (InetAddr::isIPv4Addr (pszDestinationAddr)) {
                TransmitLimit *pLimit = _perIPv4AddrTransmitLimits.remove (inet_addr (pszDestinationAddr));
                if (pLimit != NULL) {
                    delete pLimit;
                }
            }
            else {
                TransmitLimit *pLimit = _perStrAddrTransmitLimits.remove (pszDestinationAddr);
                if (pLimit != NULL) {
                    delete pLimit;
                }
            }
        }
    }
    return 0;
}

inline DatagramSocket::TransmitLimit::TransmitLimit (void)
{
    reset();
}

inline void DatagramSocket::TransmitLimit::reset (void)
{
    ui32RateLimit = 0;
    ui32RateLimitInterval = 0;
    ui32BytesPerInterval = 0;
    i64IntervalStartTime = 0;
    ui32BytesWrittenInInterval = 0;
}

void DatagramSocket::enforceTransmitRateLimit (const char *pszAddress, uint32 ui32PacketSize)
{
    if (InetAddr::isIPv4Addr (pszAddress)) {
        enforceTransmitRateLimit (inet_addr (pszAddress), ui32PacketSize);
    }
    else {
        TransmitLimit *pLimit = NULL;
        if (_perStrAddrTransmitLimits.getCount() > 0) {
            pLimit = _perStrAddrTransmitLimits.get (pszAddress);
            if ((pLimit != NULL) && (pLimit->ui32RateLimit == 0)) {
                pLimit = NULL;
            }
        }
        else if (_transmitLimit.ui32RateLimit > 0) {
            pLimit = &_transmitLimit;
        }
        if (pLimit != NULL) {
            // There is a transmit rate limit in place
            int64 i64CurrTime = getTimeInMilliseconds();
            if ((pLimit->i64IntervalStartTime + pLimit->ui32RateLimitInterval) < i64CurrTime) {
                pLimit->i64IntervalStartTime = i64CurrTime;
                pLimit->ui32BytesWrittenInInterval = 0;
            }
            pLimit->ui32BytesWrittenInInterval += ui32PacketSize;
            if (pLimit->ui32BytesWrittenInInterval > pLimit->ui32BytesPerInterval) {
                // Just exceeded the limit by writing the packet
                // Figure out how long we need to sleep to remain within the limit
                int64 i64TimeToSleep = ((pLimit->ui32BytesWrittenInInterval * 1000) / pLimit->ui32RateLimit) - (i64CurrTime - pLimit->i64IntervalStartTime);
                sleepForMilliseconds (i64TimeToSleep);
            }
        }
    }
}

void DatagramSocket::enforceTransmitRateLimit (uint32 ui32IPv4Addr, uint32 ui32PacketSize)
{
    TransmitLimit *pLimit = NULL;
    if (_perIPv4AddrTransmitLimits.getCount() > 0) {
        pLimit = _perIPv4AddrTransmitLimits.get (ui32IPv4Addr);
        if ((pLimit != NULL) && (pLimit->ui32RateLimit == 0)) {
            pLimit = NULL;
        }
    }
    else if (_transmitLimit.ui32RateLimit > 0) {
        pLimit = &_transmitLimit;
    }
    if (pLimit != NULL) {
        // There is a transmit rate limit in place
        int64 i64CurrTime = getTimeInMilliseconds();
        if ((pLimit->i64IntervalStartTime + pLimit->ui32RateLimitInterval) < i64CurrTime) {
            pLimit->i64IntervalStartTime = i64CurrTime;
            pLimit->ui32BytesWrittenInInterval = 0;
        }
        pLimit->ui32BytesWrittenInInterval += ui32PacketSize;
        if (pLimit->ui32BytesWrittenInInterval > pLimit->ui32BytesPerInterval) {
            // Just exceeded the limit by writing the packet
            // Figure out how long we need to sleep to remain within the limit
            int64 i64TimeToSleep = ((pLimit->ui32BytesWrittenInInterval * 1000) / pLimit->ui32RateLimit) - (i64CurrTime - pLimit->i64IntervalStartTime);
            sleepForMilliseconds (i64TimeToSleep);
        }
    }
}
