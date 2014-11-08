#ifndef INCL_STREAM_MOCKET_H
#define INCL_STREAM_MOCKET_H

/*
 * StreamMocket.h
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

#include "Mocket.h"

#include "FTypes.h"
#include "Mutex.h"
#include "Thread.h"


class Mocket;
class MocketReader;
class MocketWriter;

class StreamMocket : public Mocket, private NOMADSUtil::Thread
{
    public:
        StreamMocket (void);
        ~StreamMocket (void);

        // See Mocket.h
        void setIdentifier (const char *pszIdentifier);

        // See Mocket.h
        const char * getIdentifier (void);

        // See Mocket.h
        int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg);

        // See Mocket.h
        MocketStats * getStatistics (void);

        // See Mocket.h
        int bind (const char* pszBindAddr, uint16 ui16BindPort);

        // See Mocket.h
        int connect (const char *pszRemoteHost, uint16 ui16RemotePort);

        // See Mocket.h
        int connect (const char *pszRemoteHost, uint16 ui16RemotePort, int64 i64Timeout);

        // See Mocket.h
        uint32 getRemoteAddress (void);

        // See Mocket.h
        uint16 getRemotePort (void);

        // See Mocket.h
        uint32 getLocalAddress (void);

        // See Mocket.h
        uint16 getLocalPort (void);

        // See Mocket.h
        int close (void);

        // See Mocket.h
        int setConnectionLingerTime (uint32 ui32LingerTime);

        // See Mocket.h
        uint32 getConnectionLingerTime (void);

        // Send the specified data
        // Returns 0 if successful or a negative value in case of failure
        // NOTE: This method does not return the number of bytes sent!
        int send (const void *pBuf, unsigned long ulBufSize);

        // Flush any buffered data
        // Returns 1 if data there was some data buffered that was flushed,
        //     0 if there was no data buffered, and a negative value in case of failure
        int flush (void);

        // Receive data into the specified buffer
        // Tries to receive upto the number of bytes specified in ulBufSize but might return fewer
        // The timeout parameter is specified in milliseconds and 0 implies no timeout
        // Returns the number of bytes received, 0 in case of EOF, -1 in the case of timeout, or < -1
        //     in case of error
        int receive (void *pBuf, unsigned long ulBufSize, unsigned long ulTimeOut = 0);

        // Set the maximum time for buffering outgoing data before transmitting data
        // Equivalent to TCP_NDELAY socket option
        // Note that setting this to 0 generates a packet each time send() is called
        // Accuracy of this setting depends on the period of the run() loop in the transmitter - currently 100ms
        void setDataBufferingTime (uint32 ui32MilliSec);

        // Returns the current setting for the data buffering time before transmission
        // Value is in milliseconds.
        uint32 getDataBufferingTime (void);

        // Returns the number of bytes available to be read without blocking
        uint32 getBytesAvailable (void);

    public:
        static const uint16 DEFAULT_DATA_BUFFERING_TIME = 100;

    private:
        friend class StreamServerMocket;
        StreamMocket (Mocket *pMMocket);
        void run (void);

    private:
        NOMADSUtil::Mutex _m;
        NOMADSUtil::ConditionVariable _cv;
        bool _bTerminateThread;
        bool _bThreadTerminated;
        uint32 _ui32DataBufferingTime;
        int64 _i64LastTransmitTime;
        char *_pTransmitBuf;
        uint16 _ui16TransmitBufCount;
        char *_pReceiveBuf;
        uint16 _ui16ReceiveBufCount;
        uint16 _ui16ReceiveBufOffset;
        Mocket *_pMMocket;
};

inline void StreamMocket::setDataBufferingTime (uint32 ui32MilliSec)
{
    _ui32DataBufferingTime = ui32MilliSec;
}

inline uint32 StreamMocket::getDataBufferingTime (void)
{
    return _ui32DataBufferingTime;
}

#endif   // #ifndef INCL_STREAM_MOCKET_H
