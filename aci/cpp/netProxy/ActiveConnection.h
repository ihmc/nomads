#ifndef INCL_ACTIVE_CONNECTION_H
#define INCL_ACTIVE_CONNECTION_H

/*
 * ActiveConnection.h
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
 * Class that keeps track of all active connections
 * with a single instance of a remote NetProxy.
 */

#include "Utilities.h"
#include "Connection.h"


namespace NOMADSUtil
{
    class InetAddr;
}

namespace ACMNetProxy
{
    class ActiveConnection
    {
    public:
        ActiveConnection (void);
        ~ActiveConnection (void);

        const bool isAnyConnectorActive (void) const;
        Connection * const getActiveConnection (ConnectorType connectorType) const;
        const NOMADSUtil::InetAddr * const getActiveConnectionAddr (ConnectorType connectorType) const;

        Connection * const setNewActiveConnection (Connection * const pActiveConnection);
        Connection * const removeActiveConnection (const Connection * const pActiveConnection);
        
    private:
        Connection * const removeActiveConnectionByType (ConnectorType connectorType);
        
        Connection *_pMocketsConnection;
        Connection *_pSocketConnection;
        Connection *_pCSRConnection;
    };


    inline ActiveConnection::ActiveConnection (void) :
        _pMocketsConnection (NULL), _pSocketConnection (NULL), _pCSRConnection (NULL) { }

    inline ActiveConnection::~ActiveConnection (void)
    {
        _pMocketsConnection = NULL;
        _pSocketConnection = NULL;
        _pCSRConnection = NULL;
    }

    inline const bool ActiveConnection::isAnyConnectorActive (void) const
    {
        return (_pMocketsConnection && _pMocketsConnection->isConnected()) ||
               (_pSocketConnection && _pSocketConnection->isConnected()) ||
               (_pCSRConnection && _pCSRConnection->isConnected());
    }

}

#endif  // INCL_ACTIVE_CONNECTION_H
