#ifndef INCL_AUTO_CONNECTION_ENTRY_H
#define INCL_AUTO_CONNECTION_ENTRY_H

/*
 * AutoConnectionEntry.h
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

            void updateEncryptionDescriptor (void);
            int synchronize (EncryptionType encryptionType);
            void synchronized (EncryptionType encryptionType);
            void resetSynch (EncryptionType encryptionType);
            void setInvalid (EncryptionType encryptionType);

            const uint32 getRemoteProxyID (void) const;
            ConnectorType getConnectorType (void) const;
            unsigned char getConnectionEncryptionDescriptor (void) const;
            const NOMADSUtil::InetAddr * const getRemoteProxyInetAddress (EncryptionType encryptionType) const;
            uint32 getAutoReconnectTimeInMillis (void) const;
            uint64 getLastConnectionAttemptTime (EncryptionType encryptionType) const;
            bool isSynchronized (EncryptionType encryptionType) const;
            bool isValid (EncryptionType encryptionType) const;
            bool isAnyValid (void) const;

            void setConnectorType (ConnectorType connectorType);
            void setAutoReconnectTime (uint32 ui32AutoReconnectTimeInMillis);
            void updateLastConnectionAttemptTime (EncryptionType encryptionType, int64 i64CurrentTime);

        private:
            //explicit AutoConnectionEntry (const AutoConnectionEntry &rAutoConnectionEntry);
            friend Connection;

            void updateRemoteProxyID (uint32 ui32NewRemoteProxyID);

            uint32 _ui32RemoteProxyID;
            ConnectorType _connectorType;
            unsigned char _ucAutoConnectionEncryptionDescriptor;
            uint32 _ui32AutoReconnectTimeInMillis;
            uint64 _ui64LastConnectionAttemptTime[ET_SIZE];
            bool _bSynchronized[ET_SIZE];        // A synchronized autoConnection means that an InitializeConnection Proxy Message has been sent to the remote NetProxy, regardless of the encryption used
            bool _bValid[ET_SIZE];

            static ConnectionManager * const P_CONNECTION_MANAGER;
    };


    inline AutoConnectionEntry::AutoConnectionEntry (void) :
        _ui32RemoteProxyID(0), _connectorType(CT_UNDEF), _ucAutoConnectionEncryptionDescriptor(ET_UNDEF),
        _ui32AutoReconnectTimeInMillis(NetworkConfigurationSettings::DEFAULT_AUTO_RECONNECT_TIME),
        _ui64LastConnectionAttemptTime{}, _bSynchronized{false}, _bValid{true} { }

    inline void AutoConnectionEntry::synchronized (EncryptionType encryptionType)
    {
        if (encryptionType == ET_UNDEF) {
            return;
        }

        _bSynchronized[static_cast<unsigned int> (encryptionType) - 1] = true;
    }

    // Passing in ET_UNDEF as an EncryptionType will reset the synchronization flag for all Encryption Types
    inline void AutoConnectionEntry::resetSynch (EncryptionType encryptionType)
    {
        if (encryptionType == ET_UNDEF) {
            for (unsigned int i = 0; i < ET_SIZE; ++i) {
                _bSynchronized[i] = false;
            }

            return;
        }

        _bSynchronized[static_cast<unsigned int> (encryptionType) - 1] = false;
    }

    // Passing in ET_UNDEF as an EncryptionType will set the validity flag to false for all Encryption Types
    inline void AutoConnectionEntry::setInvalid (EncryptionType encryptionType)
    {
        if (encryptionType == ET_UNDEF) {
            for (unsigned int i = 0; i < ET_SIZE; ++i) {
                _bValid[i] = false;
            }

            return;
        }

        _bValid[static_cast<unsigned int> (encryptionType) - 1] = false;
    }

    inline const uint32 AutoConnectionEntry::getRemoteProxyID (void) const
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

    inline uint32 AutoConnectionEntry::getAutoReconnectTimeInMillis (void) const
    {
        return _ui32AutoReconnectTimeInMillis;
    }

    inline uint64 AutoConnectionEntry::getLastConnectionAttemptTime (EncryptionType encryptionType) const
    {
        return _ui64LastConnectionAttemptTime[encryptionType];
    }

    inline bool AutoConnectionEntry::isSynchronized (EncryptionType encryptionType) const
    {
        if (encryptionType == ET_UNDEF) {
            return false;
        }

        return _bSynchronized[static_cast<unsigned int> (encryptionType) - 1];
    }

    inline bool AutoConnectionEntry::isValid (EncryptionType encryptionType) const
    {
        if (encryptionType == ET_UNDEF) {
            return false;
        }

        return _bValid[static_cast<unsigned int> (encryptionType) - 1];
    }

    inline bool AutoConnectionEntry::isAnyValid (void) const
    {
        for (unsigned int i = 0; i < ET_SIZE; ++i) {
            if (_bValid[i]) {
                return true;
            }
        }

        return false;
    }

    inline void AutoConnectionEntry::setConnectorType (ConnectorType connectorType)
    {
        _connectorType = connectorType;
    }

    inline void AutoConnectionEntry::setAutoReconnectTime (uint32 ui32AutoReconnectTimeInMillis)
    {
        _ui32AutoReconnectTimeInMillis = ui32AutoReconnectTimeInMillis;
    }

    inline void AutoConnectionEntry::updateLastConnectionAttemptTime (EncryptionType encryptionType, int64 i64CurrentTime)
    {
        _ui64LastConnectionAttemptTime[encryptionType] = i64CurrentTime;
    }

    inline void AutoConnectionEntry::updateRemoteProxyID (uint32 ui32NewRemoteProxyID)
    {
        _ui32RemoteProxyID = ui32NewRemoteProxyID;
    }

}

#endif //INCL_AUTO_CONNECTION_ENTRY_H
