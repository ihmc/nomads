#ifndef INCL_MOCKET_CONNECTOR_H
#define INCL_MOCKET_CONNECTOR_H

/*
 * MocketConnector.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
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
 * Handles incoming and outgoing Mockets connections with remote NetProxies.
 */

#include "FIFOBuffer.h"
#include "UInt64Hashtable.h"
#include "InetAddr.h"
#include "Thread.h"
#include "ManageableThread.h"
#include "Mutex.h"
#include "ConditionVariable.h"
#include "Mocket.h"
#include "ServerMocket.h"

#include "Connector.h"
#include "TCPConnTable.h"
#include "ConfigurationParameters.h"


namespace ACMNetProxy
{
    class PacketRouter;
    class NetProxyConfigManager;

    class MocketConnector : public NOMADSUtil::ManageableThread, public virtual Connector
    {
    public:
        MocketConnector (void);
        virtual ~MocketConnector (void);

        virtual int init (uint16 ui16MocketPort);
        virtual void terminateExecution (void);
        void run (void);

        static bool peerUnreachableWarning (void *pCallbackArg, unsigned long ulTimeSinceLastContact);


    protected:
        virtual bool isEnqueueingAllowed (void) const;


    private:
        ServerMocket *_pServerMocket;
    };


    inline bool MocketConnector::isEnqueueingAllowed (void) const
    {
        return true;
    }
}

#endif   // #ifndef INCL_MOCKET_CONNECTOR_H
