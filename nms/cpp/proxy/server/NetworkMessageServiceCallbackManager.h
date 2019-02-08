/*
 * NetworkMessageServiceCallbackHandler.h
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
 * Created on February 26, 2015, 9:39 PM
 */

#ifndef NETWORK_MESSAGE_SERVICE_CALLBACK_MANAGER_H
#define    NETWORK_MESSAGE_SERVICE_CALLBACK_MANAGER_H

#include "Callback.h"
#include "NetworkMessageServiceListener.h"

namespace NOMADSUtil
{
    class NetworkMessageServiceCallbackManager : public NetworkMessageServiceListener
    {
        public:
            NetworkMessageServiceCallbackManager (uint16 ui16ApplicationId, SimpleCommHelper2 *pCallbackCommHelper);
            ~NetworkMessageServiceCallbackManager (void);

            int messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, bool bUnicast,
                                const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                const void *pMsg, uint16 ui16MsgLenp, int64 i64Timestamp,
                                uint64 ui64GroupMsgCount, uint64 ui64UnicastMsgCount);

        private:
            SimpleCommHelper2 *_pCallbackCommHelper;
    };
}


#endif    /* NETWORK_MESSAGE_SERVICE_CALLBACK_MANAGER_H */

