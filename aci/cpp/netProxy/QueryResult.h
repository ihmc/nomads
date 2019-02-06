#ifndef INCL_QUERY_RESULT_H
#define INCL_QUERY_RESULT_H

/*
 * QueryResult.h
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
 * Class that contains the result of a query for the best connectivity
 * solution to reach a remote host or NetProxy.
 */

#include "InetAddr.h"

#include "ConfigurationParameters.h"
#include "Utilities.h"


namespace ACMNetProxy
{
    class Connection;


    struct QueryResult
    {
    public:
        QueryResult (void);
        QueryResult (uint32 ui32RemoteProxyUniqueID, const NOMADSUtil::InetAddr & rLocalProxyInterfaceAddress,
                     const NOMADSUtil::InetAddr & rRemoteProxyServerAddress, ConnectorType connectorType,
                     EncryptionType _encryptionType, Connection * const pActiveConnectionToRemoteProxy);
        QueryResult (const QueryResult & queryResult);

        bool isValid (void) const;
        const NOMADSUtil::InetAddr getBestConnectionAddressSolution (void) const;

        uint32 getRemoteProxyUniqueID (void) const;
        const NOMADSUtil::InetAddr & getLocalProxyInterfaceAddress (void) const;
        const NOMADSUtil::InetAddr & getRemoteProxyServerAddress (void) const;
        const ConnectorType getQueriedConnectorType (void) const;
        const EncryptionType getQueriedEncryptionType (void) const;
        Connection * const getActiveConnectionToRemoteProxy (void) const;

        static const QueryResult & getInvalidQueryResult (void);


    private:
        const uint32 _ui32RemoteProxyUniqueID;
        const NOMADSUtil::InetAddr _iaLocalProxyInterfaceAddress;
        const NOMADSUtil::InetAddr _iaRemoteProxyServerAddress;
        const ConnectorType _connectorType;
        const EncryptionType _encryptionType;
        Connection * const _pActiveConnectionToRemoteProxy;
    };


    inline QueryResult::QueryResult (void) :
        _ui32RemoteProxyUniqueID{0}, _iaLocalProxyInterfaceAddress{}, _iaRemoteProxyServerAddress{},
        _connectorType{CT_UNDEF}, _encryptionType{ET_UNDEF}, _pActiveConnectionToRemoteProxy{nullptr}
    { }

    inline QueryResult::QueryResult (uint32 ui32RemoteProxyUniqueID, const NOMADSUtil::InetAddr & rLocalProxyInterfaceAddress,
                                     const NOMADSUtil::InetAddr & rRemoteProxyServerAddress, ConnectorType connectorType,
                                     EncryptionType encryptionType, Connection * const pActiveConnectionToRemoteProxy) :
        _ui32RemoteProxyUniqueID{ui32RemoteProxyUniqueID}, _iaLocalProxyInterfaceAddress{rLocalProxyInterfaceAddress},
        _iaRemoteProxyServerAddress{rRemoteProxyServerAddress}, _connectorType{connectorType},
        _encryptionType{encryptionType}, _pActiveConnectionToRemoteProxy{pActiveConnectionToRemoteProxy}
    { }

    inline QueryResult::QueryResult (const QueryResult & queryResult) :
        _ui32RemoteProxyUniqueID{queryResult._ui32RemoteProxyUniqueID},
        _iaLocalProxyInterfaceAddress{queryResult._iaLocalProxyInterfaceAddress},
        _iaRemoteProxyServerAddress{queryResult._iaRemoteProxyServerAddress},
        _connectorType{queryResult._connectorType}, _encryptionType{queryResult._encryptionType},
        _pActiveConnectionToRemoteProxy{queryResult._pActiveConnectionToRemoteProxy}
    { }

    inline bool QueryResult::isValid (void) const
    {
        return _ui32RemoteProxyUniqueID != 0;
    }

    inline uint32 QueryResult::getRemoteProxyUniqueID (void) const
    {
        return _ui32RemoteProxyUniqueID;
    }

    inline const NOMADSUtil::InetAddr & QueryResult::getLocalProxyInterfaceAddress (void) const
    {
        return _iaLocalProxyInterfaceAddress;
    }

    inline const NOMADSUtil::InetAddr & QueryResult::getRemoteProxyServerAddress (void) const
    {
        return _iaRemoteProxyServerAddress;
    }

    inline const ConnectorType QueryResult::getQueriedConnectorType (void) const
    {
        return _connectorType;
    }

    inline const EncryptionType QueryResult::getQueriedEncryptionType (void) const
    {
        return _encryptionType;
    }

    inline Connection * const QueryResult::getActiveConnectionToRemoteProxy (void) const
    {
        return _pActiveConnectionToRemoteProxy;
    }

    inline const QueryResult & QueryResult::getInvalidQueryResult (void)
    {
        static const QueryResult invalidQueryResult{0, NetProxyApplicationParameters::IA_INVALID_ADDR,
                                                    NetProxyApplicationParameters::IA_INVALID_ADDR,
                                                    CT_UNDEF, ET_UNDEF, nullptr};

        return invalidQueryResult;
    }
}

#endif  // INCL_QUERY_RESULT_H
