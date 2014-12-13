#ifndef INCL_AUTO_CONNECTION_ENTRY_H
#define INCL_AUTO_CONNECTION_ENTRY_H

/*
 * AutoConnectionEntry.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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
 * Class that manages automatic connections to remote NetProxies.
 * It stores parameters read from the configuration files.
 */

#include "FTypes.h"
#include "net/NetworkHeaders.h"
#include "StrClass.h"

#include "ProxyMessages.h"
#include "ProtocolSetting.h"
#include "ConfigurationParameters.h"
#include "ConnectivitySolutions.h"


namespace NOMADSUtil
{
    class InetAddr;
}

namespace ACMNetProxy
{
    class Connection;
    class ConnectivitySolutions;

    class AutoConnectionEntry
    {
        public:
            AutoConnectionEntry (void);
            AutoConnectionEntry (const uint32 ui32RemoteProxyID, const ConnectorType connectorType,
                              const uint32 ui32AutoReconnectTimeInMillis = NetworkConfigurationSettings::DEFAULT_AUTO_RECONNECT_TIME);

            const AutoConnectionEntry & operator= (const AutoConnectionEntry &rhs);

            int synchronize (void);
            void synchronized (void);
            void resetSynch (void);

            bool isSynchronized (void) const;
            const uint32 getRemoteProxyID (void) const;
            ConnectorType getConnectorType (void) const;
            const NOMADSUtil::InetAddr * const getRemoteProxyInetAddress (void) const;
            uint32 getAutoReconnectTimeInMillis (void) const;
            uint64 getLastConnectionAttemptTime (void) const;

            void setConnectorType (ConnectorType connectorType);
            void setAutoReconnectTime (uint32 ui32AutoReconnectTimeInMillis);
            void updateLastConnectionAttemptTime (int64 i64CurrentTime);

        private:
            //explicit AutoConnectionEntry (const AutoConnectionEntry &rAutoConnectionEntry);

            uint32 _ui32RemoteProxyID;
            ConnectorType _connectorType;
            const ConnectivitySolutions * _pConnectivitySolutions;
            uint32 _ui32AutoReconnectTimeInMillis;
            uint64 _ui64LastConnectionAttemptTime;
            bool _bSynchronized;

            static ConnectionManager * const P_CONNECTION_MANAGER;
    };


    inline AutoConnectionEntry::AutoConnectionEntry (void) :
        _ui32RemoteProxyID (0), _connectorType (CT_UNDEF), _pConnectivitySolutions (NULL), _ui32AutoReconnectTimeInMillis (NetworkConfigurationSettings::DEFAULT_AUTO_RECONNECT_TIME),
        _ui64LastConnectionAttemptTime (0), _bSynchronized (false) { }

    inline void AutoConnectionEntry::synchronized (void)
    {
        _bSynchronized = true;
    }

    inline void AutoConnectionEntry::resetSynch (void)
    {
        _bSynchronized = false;
    }

    inline bool AutoConnectionEntry::isSynchronized (void) const
    {
        return _bSynchronized;
    }

    inline const uint32 AutoConnectionEntry::getRemoteProxyID (void) const
    {
        return _ui32RemoteProxyID;
    }

    inline ConnectorType AutoConnectionEntry::getConnectorType (void) const
    {
        return _connectorType;
    }

    inline const NOMADSUtil::InetAddr * const AutoConnectionEntry::getRemoteProxyInetAddress (void) const
    {
        return _pConnectivitySolutions ? _pConnectivitySolutions->getBestConnectionSolutionForConnectorType (_connectorType).getBestConnectionSolution() : NULL;
    }

    inline uint32 AutoConnectionEntry::getAutoReconnectTimeInMillis (void) const
    {
        return _ui32AutoReconnectTimeInMillis;
    }

    inline uint64 AutoConnectionEntry::getLastConnectionAttemptTime (void) const
    {
        return _ui64LastConnectionAttemptTime;
    }

    inline void AutoConnectionEntry::setConnectorType (ConnectorType connectorType)
    {
        _connectorType = connectorType;
    }

    inline void AutoConnectionEntry::setAutoReconnectTime (uint32 ui32AutoReconnectTimeInMillis)
    {
        _ui32AutoReconnectTimeInMillis = ui32AutoReconnectTimeInMillis;
    }

    inline void AutoConnectionEntry::updateLastConnectionAttemptTime (int64 i64CurrentTime)
    {
        _ui64LastConnectionAttemptTime = i64CurrentTime;
    }

}

#endif //INCL_AUTO_CONNECTION_ENTRY_H
