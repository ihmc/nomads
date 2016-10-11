/* 
 * NetworkMessageServiceCallbackHandler.cpp
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2014 IHMC.
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 26, 2015, 9:39 PM
 */

#include "NetworkMessageServiceCallbackManager.h"

#include "SimpleCommHelper2.h"
#include "NetworkMessageServiceUnmarshaller.h"

using namespace NOMADSUtil;

//------------------------------------------------------------------------------
// NetworkMessageServiceCallbackHandler
//------------------------------------------------------------------------------

NetworkMessageServiceCallbackManager::NetworkMessageServiceCallbackManager (uint16 ui16ApplicationId, SimpleCommHelper2 *pCallbackCommHelper)
    : NetworkMessageServiceListener (ui16ApplicationId),
      _pCallbackCommHelper (pCallbackCommHelper)
{
}

NetworkMessageServiceCallbackManager::~NetworkMessageServiceCallbackManager (void)
{
}

int NetworkMessageServiceCallbackManager::messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress,
                                                          uint8 ui8MsgType, uint16 ui16MsgId, uint8 ui8HopCount,
                                                          uint8 ui8TTL, const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                          const void *pMsg, uint16 ui16MsgLen, int64 i64Timestamp)
{
    SimpleCommHelper2::Error error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, NetworkMessageServiceUnmarshaller::MESSAGE_ARRIVED);
    if (error != SimpleCommHelper2::None) {
        return -1;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();
    if (pWriter->writeString (pszIncomingInterface) < 0) {
        error = SimpleCommHelper2::CommError;
        return -2;
    }

    if (pWriter->writeUI32 (&ui32SourceIPAddress) < 0) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }

    if (pWriter->writeUI8 (&ui8MsgType) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }

    if (pWriter->writeUI16 (&ui16MsgId) < 0) {
        error = SimpleCommHelper2::CommError;
        return -5;
    }

    if (pWriter->writeUI8 (&ui8HopCount) < 0) {
        error = SimpleCommHelper2::CommError;
        return -6;
    }

    if (pWriter->writeUI8 (&ui8TTL) < 0) {
        error = SimpleCommHelper2::CommError;
        return -7;
    }

    if (pWriter->writeUI16 (&ui16MsgMetaDataLen) < 0) {
        error = SimpleCommHelper2::CommError;
        return -8;
    }
    if ((ui16MsgMetaDataLen > 0) && (pWriter->writeBytes (pMsgMetaData, ui16MsgMetaDataLen) < 0)) {
        error = SimpleCommHelper2::CommError;
        return -9;
    }

    if (pWriter->writeUI16 (&ui16MsgLen) < 0) {
        error = SimpleCommHelper2::CommError;
        return -10;
    }
    if ((ui16MsgLen > 0) && (pWriter->writeBytes (pMsg, ui16MsgLen) < 0)) {
        error = SimpleCommHelper2::CommError;
        return -11;
    }
    if ((ui16MsgLen > 0) && (pWriter->write64 (&i64Timestamp) < 0)) {
        error = SimpleCommHelper2::CommError;
        return -12;
    }

    return 0;
}

