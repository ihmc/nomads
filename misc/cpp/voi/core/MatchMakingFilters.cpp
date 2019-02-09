/*
 * MatchMakingFilters.cpp
 *
 * This file is part of the IHMC Voi Library/Component
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
 * Created on February 9, 2017, 3:50 AM
 */

#include "MatchMakingFilters.h"

#include "NodeContext.h"
#include "MetadataRankerLocalConfiguration.h"
#include "MetadataInterface.h"
#include "Rank.h"
#include "RankFactory.h"

using namespace IHMC_VOI;
using namespace NOMADSUtil;

Rank * IHMC_VOI::filterByDataType (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                                   MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
{
    if ((pMetadata == NULL) || (pNodeContext == NULL) || (pMetadataRankerLocalConf == NULL)) {
        return NULL;
    }

    String dataFormat;
    pMetadata->getFieldValue (MetadataInterface::DATA_FORMAT, dataFormat);
    if (dataFormat.length() <= 0) {
        return NULL;
    }

    const String nodeId (pNodeContext->getNodeId());
    if (pMetadataRankerLocalConf->hasFilterForTypeAndPeer (nodeId, dataFormat)) {
        Rank *pRank = RankFactory::getFilteredRank (pMetadata, nodeId, pMetadataRankerLocalConf->_bInstrumented);
        if (pRank != NULL) {
            char szComment[256];
            snprintf (szComment, 256, "Metadata of type %s is configured not to match for peer %s.\n",
                      dataFormat.c_str(), nodeId.c_str());
            pRank->_loggingInfo._comment = szComment;
            return pRank;
        }
    }

    return NULL;
}

Rank * IHMC_VOI::filterByPedigree (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                                   MetadataRankerLocalConfiguration *pMetadataRankerLocalConf)
{
    if ((pMetadata == NULL) || (pNodeContext == NULL) || (pMetadataRankerLocalConf == NULL)) {
        return NULL;
    }

    const String remotePeer (pNodeContext->getNodeId());
    String source;
    pMetadata->getFieldValue (MetadataInterface::SOURCE, source);

    if (source.length() > 0) {
        if (remotePeer == source) {
            Rank *pRank = RankFactory::getFilteredRank (pMetadata, remotePeer, pMetadataRankerLocalConf->_bInstrumented);
            if (pRank != NULL) {
                char szComment[256];
                snprintf (szComment, 256, "Node %s is the source of the metadata\n", source.c_str());
                pRank->_loggingInfo._comment = szComment;
            }
            return pRank;
        }
        if ((pMetadataRankerLocalConf != NULL) && (pMetadataRankerLocalConf->getLimitToLocalMatchmakingOnly())
            && (pMetadataRankerLocalConf->_nodeId != source)) {
            // The node is configured to pre-stage only the messages for which it is the source
            Rank *pRank = RankFactory::getFilteredRank (pMetadata, remotePeer, pMetadataRankerLocalConf->_bInstrumented);
            if (pRank != NULL) {
                char szComment[256];
                snprintf (szComment, 256, "Node configured to only replicate its own data\n");
                pRank->_loggingInfo._comment = szComment;
            }
            return pRank;
        }
    }

    String ped;
    pMetadata->getFieldValue (MetadataInterface::PEDIGREE, ped);
    if ((ped.length() > 0) && (ped != MetadataInterface::UNKNOWN)) {
        if ((pMetadataRankerLocalConf != NULL) && (pMetadataRankerLocalConf->getLimitToLocalMatchmakingOnly())) {
            // The node is configured to pre-stage only the nodes for which it is the source
            Rank *pRank = RankFactory::getFilteredRank (pMetadata, remotePeer, pMetadataRankerLocalConf->_bInstrumented);
            if (pRank != NULL) {
                char szComment[256];
                snprintf (szComment, 256, "Node configured to only replicate its own "
                    "data contains pedigree (%s)\n", ped.c_str());
                pRank->_loggingInfo._comment = szComment;
            }
            return pRank;
        }
        Pedigree pedigree (source, ped);
        if (pedigree.containsNodeID (remotePeer, true)) {
            Rank *pRank = RankFactory::getFilteredRank (pMetadata, remotePeer, pMetadataRankerLocalConf->_bInstrumented);
            if (pRank != NULL) {
                char szComment[256];
                snprintf (szComment, 256, "Node %s is the pedigree %s\n", remotePeer.c_str (), ped.c_str());
                pRank->_loggingInfo._comment = szComment;
            }
            return pRank;
        }
    }

    return NULL;
}

