#ifndef INCL_QUERY_RESULT_H
#define INCL_QUERY_RESULT_H

/*
 * QueryResult.h
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
 * Class that contains the result of a query for the best connectivity
 * solution to reach a remote host or NetProxy.
 */

#include "Utilities.h"
#include "Connection.h"


namespace NOMADSUtil
{
    class InetAddr;
}

namespace ACMNetProxy
{
    struct QueryResult
    {
    public:
        QueryResult (void);
        QueryResult (const QueryResult & queryResult);
        
        QueryResult & operator= (const QueryResult & queryResult);
        
        bool isValid (void) const;
        const NOMADSUtil::InetAddr * const getBestConnectionSolution (void) const;

        uint32 getRemoteProxyUniqueID (void) const;
        const NOMADSUtil::InetAddr * const getRemoteProxyServerAddress (void) const;
        Connection * const getActiveConnectionToRemoteProxy (void) const;

        
    private:
        friend class ConnectivitySolutions;
        friend class ConnectionManager;

        QueryResult (uint32 ui32RemoteProxyUniqueID, const NOMADSUtil::InetAddr * const pRemoteProxyServerAddress, Connection * const pActiveConnectionToRemoteProxy);

        static const QueryResult & getInvalidQueryResult (void);

        uint32 _ui32RemoteProxyUniqueID;
        const NOMADSUtil::InetAddr *_pRemoteProxyServerAddress;
        Connection *_pActiveConnectionToRemoteProxy;
    };
    
    
    inline QueryResult::QueryResult (void) :
        _ui32RemoteProxyUniqueID (0), _pRemoteProxyServerAddress (NULL), _pActiveConnectionToRemoteProxy (NULL) { }

    inline QueryResult::QueryResult (const QueryResult &queryResult) :
        _ui32RemoteProxyUniqueID (queryResult._ui32RemoteProxyUniqueID), _pRemoteProxyServerAddress (queryResult._pRemoteProxyServerAddress),
        _pActiveConnectionToRemoteProxy (queryResult._pActiveConnectionToRemoteProxy) { }

    inline QueryResult & QueryResult::operator= (const QueryResult & rhs)
    {
        _ui32RemoteProxyUniqueID = rhs._ui32RemoteProxyUniqueID;
        _pRemoteProxyServerAddress = rhs._pRemoteProxyServerAddress;
        _pActiveConnectionToRemoteProxy = rhs._pActiveConnectionToRemoteProxy;

        return *this;
    }

    inline bool QueryResult::isValid (void) const
    {
        return _ui32RemoteProxyUniqueID != 0;
    }

    inline const NOMADSUtil::InetAddr * const QueryResult::getBestConnectionSolution (void) const
    {
        return _pActiveConnectionToRemoteProxy ? _pActiveConnectionToRemoteProxy->getRemoteProxyInetAddr() : _pRemoteProxyServerAddress;
    }
    
    inline uint32 QueryResult::getRemoteProxyUniqueID (void) const
    {
        return _ui32RemoteProxyUniqueID;
    }
    
    inline const NOMADSUtil::InetAddr * const QueryResult::getRemoteProxyServerAddress (void) const
    {
        return _pRemoteProxyServerAddress;
    }
    
    inline Connection * const QueryResult::getActiveConnectionToRemoteProxy (void) const
    {
        return _pActiveConnectionToRemoteProxy;
    }

    inline QueryResult::QueryResult (uint32 ui32RemoteProxyUniqueID, const NOMADSUtil::InetAddr * const pRemoteProxyServerAddress, Connection * const pActiveConnectionToRemoteProxy) :
        _ui32RemoteProxyUniqueID (ui32RemoteProxyUniqueID), _pRemoteProxyServerAddress (pRemoteProxyServerAddress), _pActiveConnectionToRemoteProxy (pActiveConnectionToRemoteProxy) { }

    inline const QueryResult & QueryResult::getInvalidQueryResult (void)
    {
        static QueryResult invalidQueryResult (0, NULL, NULL);

        return invalidQueryResult;
    }

}

#endif  // INCL_QUERY_RESULT_H