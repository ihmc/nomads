#ifndef INCL_AUTO_CONNECTION_ENTRY_H
#define INCL_AUTO_CONNECTION_ENTRY_H

/*
 * AutoConnectionEntry.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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

#include <bitset>
#include <array>
#include <mutex>
#include <algorithm>

#include "FTypes.h"
#include "InetAddr.h"
#include "net/NetworkHeaders.h"

#include "ConfigurationParameters.h"
#include "Utilities.h"


namespace ACMNetProxy
{
    class Connection;
    class ConnectionManager;
    class TCPConnTable;
    class TCPManager;
    class PacketRouter;
    class StatisticsManager;

    class AutoConnectionEntry
    {
    public:
        AutoConnectionEntry (void);
        AutoConnectionEntry (const uint32 ui32RemoteProxyID, const NOMADSUtil::InetAddr & iaLocalInterfaceIPv4Address,
                             const NOMADSUtil::InetAddr & iaRemoteInterfaceIPv4Address, const ConnectorType connectorType,
                             const int32 i32AutoReconnectTimeInMillis = NetworkConfigurationSettings::DEFAULT_AUTO_RECONNECT_TIME);
        AutoConnectionEntry (const AutoConnectionEntry & rhs);

        AutoConnectionEntry & operator= (const AutoConnectionEntry & rhs) = delete;

        void lock (void) const;
        void unlock (void) const;

        int synchronize (EncryptionType encryptionType, ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable,
                         TCPManager & rTCPManager, PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager);

        void updateEncryptionDescriptor (ConnectionManager & rConnectionManager);
        void resetSynchronization (EncryptionType encryptionType);
        void setInvalid (EncryptionType encryptionType);

        const uint32 getRemoteNetProxyID (void) const;
        ConnectorType getConnectorType (void) const;
        unsigned char getConnectionEncryptionDescriptor (void) const;
        const NOMADSUtil::InetAddr & getLocalinterfaceInetAddress (void) const;
        const NOMADSUtil::InetAddr & getRemoteProxyInetAddress (void) const;
        bool areConnectivitySolutionsAvailableWithEncryption (EncryptionType encryptionType, ConnectionManager & rConnectionManager) const;
        int32 getAutoReconnectTimeInMillis (void) const;
        int64 getLastConnectionAttemptTime (EncryptionType encryptionType) const;
        bool isSynchronized (EncryptionType encryptionType) const;
        bool isValid (EncryptionType encryptionType) const;
        bool isAnyValid (void) const;

        void setConnectorType (ConnectorType connectorType);
        void setAutoReconnectTime (uint32 ui32AutoReconnectTimeInMillis);
        void updateLastConnectionAttemptTime (EncryptionType encryptionType, int64 i64CurrentTime);


    private:
        friend class ConnectionManager;
        friend class AutoConnectionManager;

        void updateRemoteProxyID (uint32 ui32NewRemoteProxyID);
        void setAsSynchronized (EncryptionType encryptionType);
        void resetAutoConnectionEntry (Connection * const pConnectionToReset);

        uint32 _ui32RemoteProxyID;
        NOMADSUtil::InetAddr _iaLocalInterfaceIPv4Address;
        NOMADSUtil::InetAddr _iaRemoteInterfaceIPv4Address;
        ConnectorType _connectorType;
        unsigned char _ucAutoConnectionEncryptionDescriptor;
        int32 _i32AutoReconnectTimeInMillis;

        std::array<int64, ET_SIZE> _ai64LastConnectionAttemptTime;
        std::bitset<ET_SIZE> _bsSynchronized;                       // _bSynchronized[ET] == true means that an InitializeConnection ProxyMessage has been sent to the remote NetProxy for that particular ET
        std::bitset<ET_SIZE> _bsValid;

        mutable std::mutex _mtx;
    };


    inline AutoConnectionEntry::AutoConnectionEntry (void) :
        _ui32RemoteProxyID{0}, _iaLocalInterfaceIPv4Address{}, _iaRemoteInterfaceIPv4Address{}, _connectorType{CT_UNDEF},
        _ucAutoConnectionEncryptionDescriptor{ET_UNDEF}, _i32AutoReconnectTimeInMillis{NetworkConfigurationSettings::DEFAULT_AUTO_RECONNECT_TIME},
        _ai64LastConnectionAttemptTime{0LL, }
    {
        _bsValid.set();
    }

    inline AutoConnectionEntry::AutoConnectionEntry (const uint32 ui32RemoteProxyID, const NOMADSUtil::InetAddr & iaLocalInterfaceIPv4Address,
                                                     const NOMADSUtil::InetAddr & iaRemoteInterfaceIPv4Address, const ConnectorType connectorType,
                                                     const int32 i32AutoReconnectTimeInMillis) :
        _ui32RemoteProxyID{ui32RemoteProxyID}, _iaLocalInterfaceIPv4Address{iaLocalInterfaceIPv4Address},
        _iaRemoteInterfaceIPv4Address{iaRemoteInterfaceIPv4Address}, _connectorType{connectorType},
        _ucAutoConnectionEncryptionDescriptor{ET_UNDEF}, _i32AutoReconnectTimeInMillis{i32AutoReconnectTimeInMillis},
        _ai64LastConnectionAttemptTime{0LL, }
    {
        _bsValid.set();
    }

    inline AutoConnectionEntry::AutoConnectionEntry (const AutoConnectionEntry & rhs) :
        _ui32RemoteProxyID{rhs._ui32RemoteProxyID}, _iaLocalInterfaceIPv4Address{rhs._iaLocalInterfaceIPv4Address},
        _iaRemoteInterfaceIPv4Address{rhs._iaRemoteInterfaceIPv4Address}, _connectorType{rhs._connectorType},
        _ucAutoConnectionEncryptionDescriptor{rhs._ucAutoConnectionEncryptionDescriptor},
        _i32AutoReconnectTimeInMillis{rhs._i32AutoReconnectTimeInMillis},
        _ai64LastConnectionAttemptTime{rhs._ai64LastConnectionAttemptTime},
        _bsSynchronized{rhs._bsSynchronized}, _bsValid{rhs._bsValid}
    { }

    inline void AutoConnectionEntry::lock (void) const
    {
        _mtx.lock();
    }

    inline void AutoConnectionEntry::unlock (void) const
    {
        _mtx.unlock();
    }

    inline void AutoConnectionEntry::setAsSynchronized (EncryptionType encryptionType)
    {
        if (encryptionType == ET_UNDEF) {
            return;
        }

        _bsSynchronized.set (static_cast<unsigned int> (encryptionType) - 1);
    }

    // Passing in ET_UNDEF as an EncryptionType will reset the synchronization flag for all Encryption Types
    inline void AutoConnectionEntry::resetSynchronization (EncryptionType encryptionType)
    {
        if (encryptionType == ET_UNDEF) {
            _bsSynchronized.reset();
            return;
        }

        _bsSynchronized.reset (static_cast<unsigned int> (encryptionType) - 1);
    }

    // Passing in ET_UNDEF as an EncryptionType will set the validity flag to false for all Encryption Types
    inline void AutoConnectionEntry::setInvalid (EncryptionType encryptionType)
    {
        if (encryptionType == ET_UNDEF) {
            _bsValid.reset();
            return;
        }

        _bsValid.reset (static_cast<unsigned int> (encryptionType) - 1);
    }

    inline const uint32 AutoConnectionEntry::getRemoteNetProxyID (void) const
    {
        return _ui32RemoteProxyID;
    }

    inline ConnectorType AutoConnectionEntry::getConnectorType (void) const
    {
        return _connectorType;
    }

    inline unsigned char AutoConnectionEntry::getConnectionEncryptionDescriptor (void) const
    {
        return _ucAutoConnectionEncryptionDescriptor;
    }

    inline const NOMADSUtil::InetAddr & AutoConnectionEntry::getLocalinterfaceInetAddress (void) const
    {
        return _iaLocalInterfaceIPv4Address;
    }

    inline const NOMADSUtil::InetAddr & AutoConnectionEntry::getRemoteProxyInetAddress (void) const
    {
        return _iaRemoteInterfaceIPv4Address;
    }

    inline int32 AutoConnectionEntry::getAutoReconnectTimeInMillis (void) const
    {
        return _i32AutoReconnectTimeInMillis;
    }

    inline int64 AutoConnectionEntry::getLastConnectionAttemptTime (EncryptionType encryptionType) const
    {
        if (encryptionType == ET_UNDEF) {
            return 0;
        }

        return _ai64LastConnectionAttemptTime[static_cast<unsigned int> (encryptionType) - 1];
    }

    inline bool AutoConnectionEntry::isSynchronized (EncryptionType encryptionType) const
    {
        if (encryptionType == ET_UNDEF) {
            return false;
        }

        return _bsSynchronized[static_cast<unsigned int> (encryptionType) - 1];
    }

    inline bool AutoConnectionEntry::isValid (EncryptionType encryptionType) const
    {
        if (encryptionType == ET_UNDEF) {
            return false;
        }

        return _bsValid[static_cast<unsigned int> (encryptionType) - 1];
    }

    inline bool AutoConnectionEntry::isAnyValid (void) const
    {
        return _bsValid.any();
    }

    inline void AutoConnectionEntry::setConnectorType (ConnectorType connectorType)
    {
        _connectorType = connectorType;
    }

    inline void AutoConnectionEntry::setAutoReconnectTime (uint32 ui32AutoReconnectTimeInMillis)
    {
        _i32AutoReconnectTimeInMillis = ui32AutoReconnectTimeInMillis;
    }

    inline void AutoConnectionEntry::updateLastConnectionAttemptTime (EncryptionType encryptionType, int64 i64CurrentTime)
    {
        _ai64LastConnectionAttemptTime[static_cast<unsigned int> (encryptionType) - 1] = i64CurrentTime;
    }

    inline void AutoConnectionEntry::updateRemoteProxyID (uint32 ui32NewRemoteProxyID)
    {
        _ui32RemoteProxyID = ui32NewRemoteProxyID;
    }

}

#endif //INCL_AUTO_CONNECTION_ENTRY_H
