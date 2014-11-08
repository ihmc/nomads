/* 
 * NodeId.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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
 * NodeInfo stores state information of the local node.
 * RemoteNodeInfo stores state information of a remote node.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on September 18, 2013, 4:52 PM
 */

#ifndef INCL_NODE_ID_H
#define	INCL_NODE_ID_H

#include "StrClass.h"
#include "Mutex.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class PropertyStoreInterface;

    class NodeId
    {
        public:
            NodeId (PropertyStoreInterface *pPropStoreIface);
            ~NodeId (void);

            const char * generateNodeId (NOMADSUtil::ConfigManager *pCfgMgr);
            NOMADSUtil::String getNodeId (void);

        private:
            NOMADSUtil::String getSuffix (void);
            NOMADSUtil::String generateSuffix (void);
            void generateSuffixedNodeId (void);
            void generateSuffixedNodeId (const char *pszSuffix);

        private:
            static const NOMADSUtil::String NODE_SUFFIX_ATTRIBUTE;
            static const NOMADSUtil::String SESSION_KEY_SEPARATOR;

            NOMADSUtil::Mutex _m;
            PropertyStoreInterface *_pPropStore;
            NOMADSUtil::String _nodeId;            
    };
}

#endif	/* NODEID_H */

