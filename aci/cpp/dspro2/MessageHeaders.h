/*
 * MessageHeaders.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#ifndef INLC_MESSAGE_HEADERS_H
#define INLC_MESSAGE_HEADERS_H

#include "FTypes.h"

namespace IHMC_ACI
{
    class MessageHeaders
    {
        public:
            enum MsgType {
                Data            = 0x00,
                Metadata        = 0x01,
                ChunkedData     = 0x02,

                Position        = 0x03,
                Search          = 0x04,
                VolSearchReply  = 0x05,
                SearchReply     = 0x06,

                // Topology
                TopoReq         = 0x07,
                TopoReply       = 0x08,

                // Node Context
                CtxtUpdates_V1  = 0x09,
                CtxtUpdates_V2  = 0x0A,
                CtxtVersions_V1 = 0x0B,
                CtxtVersions_V2 = 0x0C,
                WayPoint        = 0x0D,
                CtxtWhole_V1    = 0x0E,

                // Requests
                MessageRequest  = 0x0F,
                ChunkRequest    = 0x10
            };

            static const char * const DATA;
            static const char * const METADATA;
            static const char * const CHUNKED_DATA;
            static const char * const CONTEXTUPDATE;
            static const char * const CONTEXTVERSION;
            static const char * const POSITION;
            static const char * const SEARCH;
            static const char * const SEARCH_REPLY;
            static const char * const V_SEARCH_REPLY;
            static const char * const TOPOLOGYREPLY;
            static const char * const TOPOLOGYREQUEST;
            static const char * const UNKNOWN;
            static const char * const UPDATES;
            static const char * const VERSIONS;
            static const char * const WAYPOINT;
            static const char * const WHOLE;
            static const char * const MESSAGE_REQUEST;
            static const char * const CHUNK_REQUEST;

            /**
             * Adds a 1-byte header to tell apart data and metadata (because
             * they are both published as DisService's data messages).
             *
             * NOTE: the caller should deallocate the returned buffer.
             */
            static void * addDSProHeader (const void *pBuf, uint32 ui32Len,
                                          bool bIsMetadata, uint32 &ui32NewLen);

            /**
             * Removes the 1-byte header (which is returned in type) and returns
             * the "data" part of the buffer.
             *
             * NOTE: the caller should deallocate the returned buffer.
             */
            static void * removeDSProHeader (const void *pBuf, uint32 ui32Len,
                                             uint32 &ui32NewLen, MsgType &type);

            static int ui8ToMsgType (uint8 ui8MsgType, MsgType &type);

            static const char * getMetadataAsString (const uint8 *puiType);
    };

}

#endif // INLC_MESSAGE_HEADERS_H


