/*
 * ActiveConnection.cpp
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
 */

#include "ActiveConnection.h"
#include "Connection.h"
#include "UDPConnector.h"


namespace ACMNetProxy
{
    ActiveConnection::ActiveConnection (Connection * const pConnection) :
        _connectionTable{}
    {
        if (pConnection) {
            _connectionTable[pConnection->getConnectorType()][pConnection->getEncryptionType() - 1] = pConnection;
        }
    }

    Connection * const ActiveConnection::setNewActiveConnection (Connection * const pActiveConnection)
    {
        if (!pActiveConnection) {
            return nullptr;
        }
        const ConnectorType ct = pActiveConnection->getConnectorType();
        const EncryptionType et = pActiveConnection->getEncryptionType();
        if ((ct == CT_UNDEF) || (et == ET_UNDEF)) {
            return nullptr;
        }

        auto * const pOldConnection = getActiveConnection (ct, et);
        if (pOldConnection == pActiveConnection) {
            return pActiveConnection;
        }
        _connectionTable[ct][et - 1] = pActiveConnection;

        return pOldConnection;
    }

    Connection * const ActiveConnection::removeActiveConnection (const Connection * const pActiveConnection)
    {
        if (!pActiveConnection || (pActiveConnection->getConnectorType() == CT_UNDEF)) {
            return nullptr;
        }

        if (getActiveConnection (pActiveConnection->getConnectorType(), pActiveConnection->getEncryptionType()) == pActiveConnection) {
            return removeActiveConnectionByType (pActiveConnection->getConnectorType(), pActiveConnection->getEncryptionType());
        }

        return nullptr;
    }

}
