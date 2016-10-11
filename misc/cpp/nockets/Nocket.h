/*
 * Nocket.h
 *
 * This file is part of the IHMC NORM Socket Library.
 * Copyright (c) 2016 IHMC.
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#ifndef INCL_NACK_ORIENTED_SOCKET_H
#define INCL_NACK_ORIENTED_SOCKET_H

#include "BufferWriter.h"
#include "ConditionVariable.h"
#include "FTypes.h"
#include "ManageableThread.h"
#include "Mutex.h"
#include "InetAddr.h"
#include "PtrQueue.h"
#include "UInt32Hashtable.h"
#include <stddef.h>

#if defined (WIN32)
    #include <winsock2.h>
#elif defined (UNIX)
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#endif

namespace NOMADSUtil
{
    class InetAddr;
}

namespace IHMC_MISC
{
    class Nocket : public NOMADSUtil::ManageableThread
    {
        public:
            Nocket (bool bReceive=true, bool bSend=true);
            ~Nocket (void);

            void run (void);

            int close (void);
            uint16 getLocalPort (void);
            NOMADSUtil::InetAddr getLocalAddr (void);
            int getLocalSocket (void);
            int getMTU (void);
            int getReceiveBufferSize (void);
            int setReceiveBufferSize (int iSize);
            int getSendBufferSize (void);
            bool pktInfoEnabled (void);
            int setSendBufferSize (int iSize);
            int setTimeout (uint32 ui32TimeoutInMS);
            int setTTL (uint8 ui8TTL);
            int init (uint16 ui16Port, uint32 ui32IPAddr);
            int init (uint16 ui16Port = 6004, const char *pszIPAddr = "214.1.2.3");

            int receive (void *pBuf, int iBufSize);
            int receive (void *pBuf, int iBufSize, NOMADSUtil::InetAddr *pRemoteAddr);

            int sendTo (uint32 ui32IPv4Addr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints = NULL);
            int setTransmissionBufferSize (unsigned int uiBufSize);

        private:
            struct SessionWrapper
            {
                const void *pHandle;
            };

            struct MessageWrapper
            {
                MessageWrapper (unsigned long ulInitialSize, const void *pSessHandle);
                ~MessageWrapper (void);
                const void *pSessionHandle;
                NOMADSUtil::BufferWriter bw;
            };

            const bool _bReceive;
            const bool _bSend;
            bool _bSent;
            const void *_pInstanceHandle;
            const void *_pSessionHandle;
            NOMADSUtil::Mutex _m;
            NOMADSUtil::Mutex _mRx;
            NOMADSUtil::Mutex _mTx;
            NOMADSUtil::ConditionVariable _cvRx;
            NOMADSUtil::ConditionVariable _cvTx;
            NOMADSUtil::InetAddr _addr;

            NOMADSUtil::PtrQueue<NOMADSUtil::BufferWriter> _incomingMsgs;
            NOMADSUtil::PtrQueue<MessageWrapper> _outgoingMsgs;
            NOMADSUtil::UInt32Hashtable<SessionWrapper> _unicastSessionsByDst;
    };

    inline Nocket::MessageWrapper::MessageWrapper (unsigned long ulInitialSize, const void *pSessHandle)
        : pSessionHandle (pSessHandle),
          bw (ulInitialSize, 1024U)
    {
    }

    inline Nocket::MessageWrapper::~MessageWrapper (void)
    {
    }
}

#endif  // close INCL_NACK_ORIENTED_SOCKET_H
