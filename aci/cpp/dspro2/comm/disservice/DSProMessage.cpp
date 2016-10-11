/* 
 * DSProMessage.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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

#include "DSProMessage.h"

#include "Defs.h"
#include "DataCacheReplicationController.h"
#include "DisseminationService.h"

#include "BufferReader.h"
#include "InstrumentedReader.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//------------------------------------------------------------------------------
// DSProMessage
//------------------------------------------------------------------------------

const uint32 DSProMessage::METADATA_LENGTH = 1;

DSProMessage::DSProMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                            uint8 *pui8Type, void *pData, uint32 ui32DataLength)
    : ControllerToControllerMsg (pszSenderNodeID, pszReceiverNodeID,
                                 DisseminationService::DCReplicationCtrl,
                                 DataCacheReplicationController::DCRC_DSPro,
                                 pui8Type, DSProMessage::METADATA_LENGTH,
                                 pData, ui32DataLength)
{
}

DSProMessage::~DSProMessage()
{
}

DSProMessage * DSProMessage::getDSProMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                                              MessageHeaders::MsgType type, void *pData, uint32 ui32DataLength)
{
    if (pszSenderNodeID == NULL || pszReceiverNodeID == NULL || pData == NULL || ui32DataLength == 0) {
        return NULL;
    }

    uint8 *pui8Type = NULL;
    switch (type) {
        case MessageHeaders::TopoReply:
        case MessageHeaders::Search:
        case MessageHeaders::CtxtUpdates_V1:
        case MessageHeaders::CtxtVersions_V1:
        case MessageHeaders::WayPoint:
        case MessageHeaders::CtxtWhole_V1:
            pui8Type = (uint8*) calloc (1, sizeof (uint8));
            break;

        default:
            checkAndLogMsg ("DSProMessage::getDSProMessage", Logger::L_Warning,
                            "cant instantiate message of type %u", type);
            return NULL;
    }

    if (pui8Type == NULL) {
        checkAndLogMsg ("DSProMessage::getDSProMessage", memoryExhausted);
    }

    *pui8Type = type;

    DSProMessage *pDSProMessage = NULL; 
    switch (type) {
        case MessageHeaders::TopoReply:
            pDSProMessage = new TopologyReply (pszSenderNodeID, pszReceiverNodeID,
                                               pui8Type, pData, ui32DataLength);
            break;

        case MessageHeaders::Search:
            pDSProMessage = new SearchMessage (pszSenderNodeID, pszReceiverNodeID,
                                               pui8Type, pData, ui32DataLength);
            break;

        case MessageHeaders::CtxtUpdates_V1:
            pDSProMessage = new UpdateMessage (pszSenderNodeID, pszReceiverNodeID,
                                               pui8Type, pData, ui32DataLength);
            break;

        case MessageHeaders::CtxtVersions_V1:
            pDSProMessage = new VersionMessage (pszSenderNodeID, pszReceiverNodeID,
                                                pui8Type, pData, ui32DataLength);
            break;

        case MessageHeaders::WayPoint:
            pDSProMessage = new WayPointMessage (pszSenderNodeID, pszReceiverNodeID,
                                                 pui8Type, pData, ui32DataLength);
            break;

        case MessageHeaders::CtxtWhole_V1:
            pDSProMessage = new WholeMessage (pszSenderNodeID, pszReceiverNodeID,
                                              pui8Type, pData, ui32DataLength);
            break;

        default:
            checkAndLogMsg ("DSProMessage::getDSProMessage", Logger::L_Warning,
                            "cant instantiate message of type %u", type);
            return NULL;
    }

    if (pDSProMessage == NULL) {
        checkAndLogMsg ("DSProMessage::getDSProMessage", memoryExhausted);
    }

    return pDSProMessage;
}

char * DSProMessage::readMessagePublisher (const void *pBuf, uint32 ui32Len, uint32 &ui32BytesRead)
{
    ui32BytesRead = 0;
    if (pBuf == NULL || ui32Len == 0) {
        return NULL;
    }

    BufferReader bufr (pBuf, ui32Len);
    InstrumentedReader br (&bufr);
    uint32 ui32PublisherIdLen = 0;
    if (br.read32 (&ui32PublisherIdLen) < 0) {
        ui32BytesRead = br.getBytesRead();
        return NULL;
    }
    if (ui32PublisherIdLen == 0) {
        ui32BytesRead = br.getBytesRead();
        return NULL;
    }

    char *pszPublisherId = (char *) calloc (ui32PublisherIdLen+1, sizeof (char));
    if (br.readBytes (pszPublisherId, ui32PublisherIdLen) < 0) {
        ui32BytesRead = br.getBytesRead();
        return NULL;
    }

    ui32BytesRead = br.getBytesRead();
    return pszPublisherId;
}

//------------------------------------------------------------------------------
// TopologyReply
//------------------------------------------------------------------------------

TopologyReply::TopologyReply (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                              uint8 *pui8Type, void *pData, uint32 ui32DataLength)
    : DSProMessage (pszSenderNodeID, pszReceiverNodeID, pui8Type, pData, ui32DataLength)
{
}

TopologyReply::~TopologyReply()
{
}

//------------------------------------------------------------------------------
// Search
//------------------------------------------------------------------------------

SearchMessage::SearchMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                              uint8 *pui8Type, void *pData, uint32 ui32DataLength)
    : DSProMessage (pszSenderNodeID, pszReceiverNodeID, pui8Type, pData, ui32DataLength)
{
}

SearchMessage::~SearchMessage()
{
}

//------------------------------------------------------------------------------
// Updates
//------------------------------------------------------------------------------

UpdateMessage::UpdateMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                              uint8 *pui8Type, void *pData, uint32 ui32DataLength)
    : DSProMessage (pszSenderNodeID, pszReceiverNodeID, pui8Type, pData, ui32DataLength)
{
}

UpdateMessage::~UpdateMessage()
{
}

//------------------------------------------------------------------------------
// Versions
//------------------------------------------------------------------------------

VersionMessage::VersionMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                                uint8 *pui8Type, void *pData, uint32 ui32DataLength)
    : DSProMessage (pszSenderNodeID, pszReceiverNodeID, pui8Type, pData, ui32DataLength)
{
}

VersionMessage::~VersionMessage()
{
}

//------------------------------------------------------------------------------
// WayPoint
//------------------------------------------------------------------------------

WayPointMessage::WayPointMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                                  uint8 *pui8Type, void *pData, uint32 ui32DataLength)
    : DSProMessage (pszSenderNodeID, pszReceiverNodeID, pui8Type, pData, ui32DataLength)
{
}

WayPointMessage::~WayPointMessage()
{
}

//------------------------------------------------------------------------------
// Whole
//------------------------------------------------------------------------------

WholeMessage::WholeMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                            uint8 *pui8Type, void *pData, uint32 ui32DataLength)
    : DSProMessage (pszSenderNodeID, pszReceiverNodeID, pui8Type, pData, ui32DataLength)
{
}

WholeMessage::~WholeMessage()
{
}

