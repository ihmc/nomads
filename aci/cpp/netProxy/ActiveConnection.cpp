/*
 * ActiveConnection.cpp
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
 */

#include "ActiveConnection.h"
#include "Connection.h"
#include "UDPConnector.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    Connection * const ActiveConnection::setNewActiveConnection (Connection * const pActiveConnection)
    {
        if (!pActiveConnection) {
            return nullptr;
        }

        Connection * const pOldConnection = getActiveConnection (pActiveConnection->getConnectorType(), pActiveConnection->getEncryptionType());
        if (pOldConnection == pActiveConnection) {
            return nullptr;
        }

        const ConnectorType ct = pActiveConnection->getConnectorType();
        const EncryptionType et = pActiveConnection->getEncryptionType();
        if ((ct == CT_UDPSOCKET) || (ct == CT_UNDEF)) {
            return nullptr;
        }
        _connectionTable[ct][et - 1] = pActiveConnection;

        return pOldConnection;
    }

    Connection * const ActiveConnection::removeActiveConnection (const Connection * const pActiveConnection)
    {
        if (!pActiveConnection || (pActiveConnection->getConnectorType() == CT_UNDEF) ||
            (pActiveConnection->getConnectorType() == CT_UDPSOCKET)) {
            return nullptr;
        }

        if (getActiveConnection (pActiveConnection->getConnectorType(), pActiveConnection->getEncryptionType()) == pActiveConnection) {
            return removeActiveConnectionByType (pActiveConnection->getConnectorType(), pActiveConnection->getEncryptionType());
        }

        return nullptr;
    }

}
