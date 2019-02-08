/* 
 * NMSProxyShell.h
 *
 * This file is part of the IHMC Network Message Service Library
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 *
 * Created on March 1, 2015, 7:40 PM
 */

#ifndef INCL_NMS_PROXY_SHELL_H
#define INCL_NMS_PROXY_SHELL_H

#include "CommandProcessor.h"
#include "NetworkMessageServiceListener.h"
#include "NetworkMessageServiceProxy.h"

namespace NOMADSUtil
{
    class NMSProxyShell : public CommandProcessor,
                          public NetworkMessageServiceListener
    {
        public:
            NMSProxyShell (void);
            virtual ~NMSProxyShell (void);

            int init (const char *pszServerHost, uint16 ui16ServerPort);
            int processCmd (const void *pToken, char *pszCmdLine);

            int messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, bool bUnicast,
                                const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                const void *pMsg, uint16 ui16MsgLen, int64 i64Timestamp,
                                uint64 ui64MsgCount, uint64 ui64UnicastMsgCount);

        private:
            void handleBroadcast (const void *pToken, const char *pszCmdLine);
            void handleHelp (const void *pToken, const char *pszCmdLine);
            void handleTransmit (const void *pToken, const char *pszCmdLine);
            void handleTransmitReliable (const void *pToken, const char *pszCmdLine);
            void handleSetRetransmissionTimeout (const void *pToken, const char *pszCmdLine);
            void handleSetPrimaryInterface (const void *pToken, const char *pszCmdLine);
            void handleGetMinimumMTU (const void *pToken, const char *pszCmdLine);
            void handleGetInterfaces (const void *pToken, const char *pszCmdLine);
            void handleGetPropagationMode (const void *pToken, const char *pszCmdLine);
            void handleGetDeliveryQueueSize (const void *pToken, const char *pszCmdLine);
            void handleGetReceiveRate (const void *pToken, const char *pszCmdLine);
            void handleGetTransmissionQueueSize (const void *pToken, const char *pszCmdLine);
            void handleGetRascaledTransmissionQueueSize (const void *pToken, const char *pszCmdLine);
            void hanldeGetTransmissionQueueMaxSize (const void *pToken, const char *pszCmdLine);
            void handleGetTransmitRateLimit (const void *pToken, const char *pszCmdLine);
            void handleSetTransmissionQueueMaxSize (const void *pToken, const char *pszCmdLine);
            void handleSetTransmitRateLimit (const void *pToken, const char *pszCmdLine);
            void handleGetLinkCapacity (const void *pToken, const char *pszCmdLine);
            void handleSetLinkCapacity (const void *pToken, const char *pszCmdLine);
            void handleGetNeighborQueueLength (const void *pToken, const char *pszCmdLine);
            void handleClearToSend (const void *pToken, const char *pszCmdLine);
            void handleRegisterListener (const void *pToken, const char *pszCmdLine);
            void handlePing (const void *pToken, const char *pszCmdLine);

        private:
            NetworkMessageServiceProxy _nmsProxy;
    };
}

#endif    /* INCL_NMS_PROXY_SHELL_H */

