/**
 * DSProProxyUnmarshaller.h
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

#ifndef INCL_DSPRO_PROXY_UNMARSHALLER_H
#define INCL_DSPRO_PROXY_UNMARSHALLER_H

#include "StrClass.h"
#include "SimpleCommHelper2.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
    class Stub;
}

namespace IHMC_VOI
{
    class NodePath;
}

namespace IHMC_ACI
{
    class DSPro;
    class DSProProxy;

    class DSProProxyUnmarshaller
    {
        public:
            // Commands
            static const NOMADSUtil::String SERVICE;
            static const NOMADSUtil::String VERSION;

            static const NOMADSUtil::String ADD_CUSTUMS_POLICIES_AS_XML;
            static const NOMADSUtil::String ADD_MESSAGE;
            static const NOMADSUtil::String ADD_MESSAGE_AV_LIST;
            static const NOMADSUtil::String ADD_CHUNK;
            static const NOMADSUtil::String ADD_CHUNK_AV_LIST;
            static const NOMADSUtil::String ADD_ADDITIONAL_CHUNK;
            static const NOMADSUtil::String ADD_CHUNKED_MESSAGE;
            static const NOMADSUtil::String ADD_CHUNKED_MESSAGE_AV_LIST;
            static const NOMADSUtil::String ADD_USER_ID;
            static const NOMADSUtil::String CANCEL;
            static const NOMADSUtil::String CANCEL_BY_OBJECT_INSTANCE_ID;
            static const NOMADSUtil::String CHANGE_ENCRYPTION_KEY;
            static const NOMADSUtil::String CHUNK_AND_ADD_MESSAGE;
            static const NOMADSUtil::String CHUNK_AND_ADD_MESSAGE_AV_LIST;
            static const NOMADSUtil::String DISSEMINATE;
            static const NOMADSUtil::String DISSEMINATE_METADATA;
            static const NOMADSUtil::String GET_DATA;
            static const NOMADSUtil::String GET_NODE_CONTEXT;
            static const NOMADSUtil::String GET_NODE_ID;
            static const NOMADSUtil::String GET_PEER_MSG_COUNTS;
            static const NOMADSUtil::String GET_SESSION_ID;
            static const NOMADSUtil::String PING_METHOD;
            static const NOMADSUtil::String SET_CURRENT_PATH;
            static const NOMADSUtil::String REGISTER_PATH;
            static const NOMADSUtil::String ADD_AREA_OF_INTEREST;
            static const NOMADSUtil::String RESET_TRANSMISSION_COUNTERS;
            static const NOMADSUtil::String SEARCH;
            static const NOMADSUtil::String SEARCH_REPLY;
            static const NOMADSUtil::String SET_MISSION_ID;
            static const NOMADSUtil::String SET_DEFAULT_RANGE_OF_INFLUENCE;
            static const NOMADSUtil::String SET_SELECTIVITY;
            static const NOMADSUtil::String SET_RANGE_OF_INFLUENCE;
            static const NOMADSUtil::String SET_ROLE;
            static const NOMADSUtil::String SET_TEAM_ID;
            static const NOMADSUtil::String SET_NODE_TYPE;
            static const NOMADSUtil::String SET_CURRENT_POSITION;
            static const NOMADSUtil::String SET_DEFAULT_USEFUL_DISTANCE;
            static const NOMADSUtil::String SET_USEFUL_DISTANCE;
            static const NOMADSUtil::String SET_RANKING_WEIGHTS;
            static const NOMADSUtil::String SUBSCRIBE;

            // Listener Registration
            static const NOMADSUtil::String REGISTER_DSPRO_LISTENER_METHOD;
            static const NOMADSUtil::String REGISTER_MATCHMAKING_LISTENER_METHOD;
            static const NOMADSUtil::String REGISTER_SEARCH_LISTENER_METHOD;

            // Chunking Plugin Registration
            static const NOMADSUtil::String REGISTER_CHUNK_FRAGMENTER;
            static const NOMADSUtil::String REGISTER_CHUNK_REASSEMBLER;

            // Callbacks
            static const NOMADSUtil::String METADATA_ARRIVED;
            static const NOMADSUtil::String DATA_ARRIVED;
            static const NOMADSUtil::String DATA_AVAILABLE;
            static const NOMADSUtil::String SEARCH_ARRIVED;
            static const NOMADSUtil::String SEARCH_REPLY_ARRIVED;
            static const NOMADSUtil::String VOLATILE_SEARCH_REPLY_ARRIVED;

            static bool methodInvoked (uint16 ui16ClientId, const NOMADSUtil::String & methodName,
                                       void * pDSPro, NOMADSUtil::SimpleCommHelper2 * pCommHelper,
                                       NOMADSUtil::SimpleCommHelper2::Error & error);

            static bool methodArrived (uint16 ui16ClientId, const NOMADSUtil::String & methodName,
                                       NOMADSUtil::Stub * pDSProProxy, NOMADSUtil::SimpleCommHelper2 * pCommHelper);

            static IHMC_VOI::NodePath * readNodePath (NOMADSUtil::Reader * pReader);
            static int write (IHMC_VOI::NodePath * pPath, NOMADSUtil::Writer * pWriter);
    };
}

#endif  /* INCL_DSPRO_PROXY_UNMARSHALLER_H */

