/*
 * ReceivedMessagesInterface.cpp
 *
 * This file is part of the IHMC DisService Library/Component
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

#include "ReceivedMessagesInterface.h"
#include "ReceivedMessages.h"

#include "DisServiceDefs.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

ReceivedMessagesInterface * ReceivedMessagesInterface::_pInstance = NULL;

ReceivedMessagesInterface::ReceivedMessagesInterface ()
{
}

ReceivedMessagesInterface::~ReceivedMessagesInterface ()
{
}

ReceivedMessagesInterface * ReceivedMessagesInterface::getReceivedMessagesInterface (const char *pszStorageFile)
{
    if (_pInstance != NULL) {
        return _pInstance;
    }

    ReceivedMessages *pInstance = new ReceivedMessages (pszStorageFile);
    int rc = pInstance->init();
    if (rc < 0) {
        checkAndLogMsg ("ReceivedMessagesInterface::getReceivedMessagesInterface", Logger::L_SevereError,
            "failed to initialize ReceivedMessages structure. Error code: %d\n", rc);
        delete pInstance;
    }
    _pInstance = pInstance;

    return _pInstance;
}
