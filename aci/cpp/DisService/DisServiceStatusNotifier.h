/*
 * DisServiceStatusNotifier.h
 *
 *This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#ifndef INCL_DIS_SERVICE_STATUS_NOTIFIER_H
#define INCL_DIS_SERVICE_STATUS_NOTIFIER_H

#include "BufferWriter.h"
#include "UDPDatagramSocket.h"
#include "FTypes.h"
#include "InetAddr.h"
#include "StrClass.h"

#include "msgpack.hpp"

namespace IHMC_ACI
{

    class DisServiceStats;

    /*
     * The DisServiceStatusNotifier class sends out status information about
     * DisService traffic on a UDP port, which may be monitored by the
     * DisServiceStatusMonitor or some other component
     */
    class DisServiceStatusNotifier
    {
        public:
            DisServiceStatusNotifier (const char *pszNodeId, const char *pszNotifyAddr);
            ~DisServiceStatusNotifier (void);

            int init (uint16 ui16StatsPort);

            int clientConnected (uint16 ui16ClientId);
            int clientDisconnected (uint16 ui16ClientId);

            /**
             * ppszNeighbors is a NULL terminated array of \0 terminated strings
             */
            int sendNeighborList (const char **ppszNeighbors);
            int sendSummaryStats (DisServiceStats *pStats);
            int sendDetailedStats (DisServiceStats *pStats);
            int sendDuplicateTrafficStats (DisServiceStats *pStats);

        private:
            int sendPacket (void);

        private:
            msgpack::sbuffer _bufWriter;
            msgpack::packer<msgpack::sbuffer> _packer;
            NOMADSUtil::InetAddr _remoteHostAddr;
            NOMADSUtil::InetAddr _localHostAddr;
            uint16 _ui16StatsPort;
            const NOMADSUtil::String _nodeId;
            NOMADSUtil::UDPDatagramSocket _statSocket;
            bool _bStatsMultiplex;
    };
}


#endif   // #ifndef INCL_DIS_SERVICE_STATUS_NOTIFIER_H
