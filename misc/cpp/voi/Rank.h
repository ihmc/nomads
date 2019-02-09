/*
 * Rank.h
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
 * Created on June 28, 2013, 11:28 AM
 */

#ifndef INCL_RANK_H
#define	INCL_RANK_H

#include "MetadataInterface.h"
#include "NodeIdSet.h"
#include "RankByTargetMap.h"

#include "DArray2.h"

namespace IHMC_VOI
{
    enum RankType
    {
        BasicRank,
        InstrumentedRank
    };

    struct RankLoggingInfo
    {
        RankLoggingInfo (void);
        RankLoggingInfo (const RankLoggingInfo &rhsLoggingInfo);
        ~RankLoggingInfo (void);

        RankLoggingInfo & operator = (const RankLoggingInfo &rhsLoggingInfo);

        NOMADSUtil::String _objectName;
        NOMADSUtil::String _comment;
    };

    struct RankObjectInfo
    {
        RankObjectInfo (void);
        RankObjectInfo (const RankObjectInfo &rhsRank);
        ~RankObjectInfo (void);

        RankObjectInfo & operator = (const RankObjectInfo &rhsRank);

        int64 _i64SourceTimestamp;
        NOMADSUtil::String _objectId;
        NOMADSUtil::String _instanceId;
        NOMADSUtil::String _mimeType;
    };

    struct Rank
    {
        struct PartialRank
        {
            float _partialRank;
            float _partialRankWeigth;
            NOMADSUtil::String _partialRankDescription;
        };

        Rank (const char *pszMsgId, NodeIdSet &rhsMatchingNodes, RankByTargetMap &rhdRankByTarget,
              bool bFiltered, float fTotalRank=0.0f, float fTimeRank=0.0f,
              uint32 ui32RefDataSize = 0U);
        Rank (const Rank &rhsRank);
        ~Rank (void);

        Rank * clone (void);

        void addRank (const char *pszDescription, float fValue, float fWeigth);

        bool operator > (const Rank &rhsRank);
        bool operator < (const Rank &rhsRank);
        bool operator == (const Rank &rhsRank);
        Rank & operator += (const Rank &rhsRank);

        const NOMADSUtil::String _msgId;
        mutable NodeIdSet _targetId;
        mutable RankByTargetMap _rankByTarget;

        const bool _bFiltered;
        float _fTotalRank; // _fTotalRank is in the interval [0.0 - 10.0]. "0"
                           // indicates that the given metadata is not useful,
                           // "10" indicates that the metadata is the most useful.
        float _fTimeRank;
        uint32 _ui32RefDataSize;
        NOMADSUtil::DArray2<PartialRank> _partialRanks;

        RankObjectInfo _objectInfo;
        RankLoggingInfo _loggingInfo;
    };

    class Ranks : public NOMADSUtil::PtrLList<Rank>
    {
        public:
            Ranks (void);

            /**
             * If the targetNodeId is set, only ranks for that target can be
             * added.
             */
            explicit Ranks (NOMADSUtil::String &targetNodeId);
            ~Ranks (void);

        private:
            const NOMADSUtil::String _targetNodeId;
    };
}

#endif	/* INCL_RANK_H */

