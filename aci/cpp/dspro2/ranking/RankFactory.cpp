/* 
 * RankFactory.cpp
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
 * Created on July 5, 2013, 3:26 AM
 */

#include "RankFactory.h"

#include "MetaData.h"
#include "Rank.h"

using  namespace IHMC_ACI;
using  namespace NOMADSUtil;

Rank * RankFactory::getRank (const MetadataInterface *pMetadata, const char *pszTargetId,
                             bool bFiltered, float fTotalRank, float fTimeRank, bool bInstrumented)
{
    if (pMetadata == NULL) {
        return NULL;
    }
    String msgId;
    if (pMetadata->getFieldValue (MetaData::MESSAGE_ID, msgId) < 0 || msgId.length() <= 0) {
        return NULL;
    }

    NodeIdSet matchingNode (pszTargetId);
    RankByTargetMap rhdRankByTarget (pszTargetId, fTotalRank);
    Rank *pRank = new Rank (msgId, matchingNode, rhdRankByTarget, bFiltered, fTotalRank, fTimeRank);
    if (pRank != NULL) {
        String instanceId, objectId, mimeType;
        if (pMetadata->getFieldValue (MetadataInterface::REFERRED_DATA_OBJECT_ID, objectId) == 0) {
            pRank->_objectInfo._objectId = objectId;
        }
        if (pMetadata->getFieldValue (MetadataInterface::REFERRED_DATA_INSTANCE_ID, instanceId) == 0) {
            pRank->_objectInfo._instanceId = instanceId;
        }
        if (pMetadata->getFieldValue (MetadataInterface::DATA_FORMAT, mimeType) == 0) {
            pRank->_objectInfo._mimeType = mimeType;
        }

        int64 i64SourceTimestamp = 0;
        if (pMetadata->getFieldValue (MetadataInterface::SOURCE_TIME_STAMP, &i64SourceTimestamp) == 0 && i64SourceTimestamp > 0) {
            pRank->_objectInfo._i64SourceTimestamp = i64SourceTimestamp;
        }

        String dataContent;
        if (pMetadata->getFieldValue (MetadataInterface::DATA_CONTENT, dataContent) == 0 && dataContent.length() > 0) {
            pRank->_loggingInfo._objectName = dataContent;
        }
    }
    return pRank;
}

Rank * RankFactory::getFilteredRank (const MetadataInterface *pMetadata, const char *pszTargetId,
                                     bool bInstrumented)
{
    return getRank (pMetadata, pszTargetId, true, 0.0f, 0.0f, bInstrumented);
}

Rank * RankFactory::getZeroRank (const MetadataInterface *pMetadata, const char *pszTargetId,
                                 bool bInstrumented)
{
    return getRank (pMetadata, pszTargetId, false, 0.0f, 0.0f, bInstrumented);
}

