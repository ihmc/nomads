/*
 * MessageHeaders.cpp
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

#include "MessageHeaders.h"

#include "Defs.h"

#include "BufferReader.h"
#include "BufferWriter.h"
#include "ConfigManager.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

using namespace IHMC_ACI;

const char * const MessageHeaders::DATA            = "Data";
const char * const MessageHeaders::METADATA        = "Metadata";
const char * const MessageHeaders::CHUNKED_DATA    = "ChunkedData";
const char * const MessageHeaders::CONTEXTUPDATE   = "ContextUpdate";
const char * const MessageHeaders::CONTEXTVERSION  = "ContextVersion";
const char * const MessageHeaders::POSITION        = "Position";
const char * const MessageHeaders::SEARCH          = "Search";
const char * const MessageHeaders::SEARCH_REPLY    = "SearchReply";
const char * const MessageHeaders::V_SEARCH_REPLY  = "volatileSearchReply";
const char * const MessageHeaders::TOPOLOGYREPLY   = "TopologyReply";
const char * const MessageHeaders::TOPOLOGYREQUEST = "TopologyRequest";
const char * const MessageHeaders::UPDATES         = "Updates";
const char * const MessageHeaders::VERSIONS        = "Versions";
const char * const MessageHeaders::WAYPOINT        = "Waypoint";
const char * const MessageHeaders::WHOLE           = "Whole";
const char * const MessageHeaders::MESSAGE_REQUEST = "MessageRequest";
const char * const MessageHeaders::CHUNK_REQUEST   = "ChunkRequest";
const char * const MessageHeaders::UNKNOWN         = "Unknown";

void * MessageHeaders::addDSProHeader (const void *pBuf, uint32 ui32Len, bool bIsMetadata, uint32 &ui32NewLen)
{
    BufferWriter bw ((ui32Len + 1U), 1024U);
    const uint8 ui8Header = bIsMetadata ? Metadata : Data;
    if (bw.write8 (&ui8Header) != 0) {
        return nullptr;
    }
    if (bw.writeBytes (pBuf, ui32Len) != 0) {
        return nullptr;
    }

    ui32NewLen = bw.getBufferLength();
    return bw.relinquishBuffer();
}

void * MessageHeaders::removeDSProHeader (const void *pBuf, uint32 ui32Len,
                                          uint32 &ui32NewLen, MsgType &type)
{
    uint8 ui8Header = 0;
    BufferReader br (pBuf, ui32Len);
    if (br.read8 (&ui8Header) != 0) {
        return nullptr;
    }
    switch (ui8Header) {
        case Data:
        case Metadata:
            type = (MsgType) ui8Header;
            break;

        default:
            checkAndLogMsg ("MessageHeaders::removeDSProHeader", Logger::L_Warning,
                            "message of unknown type %u\n", ui8Header);
            return nullptr;
    }

    ui32NewLen = ui32Len-1;
    void *pNewBuf = calloc (ui32NewLen, 1);
    if (pNewBuf == nullptr) {
        checkAndLogMsg ("MessageHeaders::removeDSProHeader", memoryExhausted);
        ui32NewLen = 0;
        return nullptr;
    }
    if (br.readBytes (pNewBuf, ui32NewLen) != 0) {
        checkAndLogMsg ("MessageHeaders::removeDSProHeader", Logger::L_Warning,
                        "error reading buffer\n", ui8Header);
        free (pNewBuf);
        pNewBuf = nullptr;
        ui32NewLen = 0;
    }

    return pNewBuf;
}

int MessageHeaders::ui8ToMsgType (uint8 ui8MsgType, MsgType &type)
{
    switch (ui8MsgType) {
        case Data:
            type = Data;
            break;

        case Metadata:
            type = Metadata;
            break;

        case ChunkedData:
            type = ChunkedData;
            break;

        case Position:
            type = Position;
            break;

        case Search:
            type = Search;
            break;

        case SearchReply:
            type = SearchReply;
            break;

        case VolSearchReply:
            type = VolSearchReply;
            break;

        case TopoReq:
            type = TopoReq;
            break;

        case TopoReply:
            type = TopoReply;
            break;

        case CtxtUpdates_V1:
            type = CtxtUpdates_V1;
            break;

        case CtxtUpdates_V2:
            type = CtxtUpdates_V2;
            break;

        case CtxtVersions_V1:
            type = CtxtVersions_V1;
            break;

        case CtxtVersions_V2:
            type = CtxtVersions_V2;
            break;

        case WayPoint:
            type = WayPoint;
            break;

        case CtxtWhole_V1:
            type = CtxtWhole_V1;
            break;

        case MessageRequest:
            type = MessageRequest;
            break;

        case ChunkRequest:
            type = ChunkRequest;
            break;

        default:
            return -1;
    }

    return 0;
}

const char * MessageHeaders::getMetadataAsString (const uint8 *puiType)
{
    if (puiType == nullptr) {
        return MessageHeaders::UNKNOWN;
    }

    switch (*puiType) {
        case Data:
            return DATA;

        case Metadata:
            return METADATA;

        case ChunkedData:
            return CHUNKED_DATA;

        case Search:
            return MessageHeaders::SEARCH;

        case SearchReply:
            return MessageHeaders::SEARCH_REPLY;

        case VolSearchReply:
            return MessageHeaders::V_SEARCH_REPLY;

        case CtxtUpdates_V1:
            return MessageHeaders::UPDATES;

        case CtxtUpdates_V2:
            return MessageHeaders::UPDATES;

        case CtxtVersions_V1:
            return MessageHeaders::VERSIONS;

        case CtxtVersions_V2:
            return MessageHeaders::VERSIONS;

        case WayPoint:
            return MessageHeaders::WAYPOINT;

        case CtxtWhole_V1:
            return MessageHeaders::WHOLE;

        case MessageRequest:
            return MessageHeaders::MESSAGE_REQUEST;

        case ChunkRequest:
            return MessageHeaders::CHUNK_REQUEST;

        default:
            return MessageHeaders::UNKNOWN;
    }
}

