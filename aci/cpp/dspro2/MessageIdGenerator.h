/*
 * MessageIdGenerator.h
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

#ifndef INCL_MESSAGE_ID_GENERATOR_H
#define INCL_MESSAGE_ID_GENERATOR_H

#include "FTypes.h"

#include "StrClass.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{
    class PropertyStoreInterface;

    class MessageIdGenerator
    {
        public:
            MessageIdGenerator (const char *pszNodeId,
                                const char *pszRootGroupName,
                                PropertyStoreInterface *pPropertyStore);
            ~MessageIdGenerator (void);

            /**
             * Given a group name, it returns a message id
             */
            char * getMsgId (const char *pszGroupName, bool bDisseminate);

            /**
             * Given a group name, it returns a chunk message id
             */
            char * chunkId (const char *pszGroupName, bool bDisseminate);

            static NOMADSUtil::String extractSubgroupFromMsgId (const char *pszMsgId);
            static NOMADSUtil::String extractSubgroupFromMsgGroup (const char *pszMsgId);

        private:
            char * getIdInternal (NOMADSUtil::String &group);

        private:
            static const char * LATEST_SEQ_ID_PROPERTY_PREFIX;

            const NOMADSUtil::String _nodeId;
            const NOMADSUtil::String _groupName;
            PropertyStoreInterface *_pPropertyStore;
            NOMADSUtil::StringHashtable<uint32> _seqIdsByGrp;
    };
}

#endif // INCL_MESSAGE_ID_GENERATOR_H
