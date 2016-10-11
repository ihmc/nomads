/* 
 * Rank.h
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
 * Created on June 28, 2013, 11:28 AM
 */

#ifndef INCL_RANK_H
#define	INCL_RANK_H

#include "MetadataInterface.h"
#include "NodeIdSet.h"
#include "RankByTargetMap.h"

#include "DArray2.h"

namespace IHMC_ACI
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

        bool _bFiltered;
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

    inline Rank::Rank (const char *pszMsgId, NodeIdSet &rhsMatchingNodes,
                       RankByTargetMap &rhdRankByTarget, bool bFiltered,
                       float fTotalRank, float fTimeRank, uint32 ui32RefDataSize)
        : _msgId (pszMsgId), _targetId (rhsMatchingNodes), _rankByTarget (rhdRankByTarget), _bFiltered (bFiltered),
          _fTotalRank (fTotalRank), _fTimeRank (fTimeRank), _ui32RefDataSize (ui32RefDataSize)
    {
    }

    inline Rank::Rank (const Rank &rhsRank)
        : _msgId (rhsRank._msgId), _targetId (rhsRank._targetId), _rankByTarget (rhsRank._rankByTarget),
          _bFiltered (rhsRank._bFiltered), _fTotalRank (rhsRank._fTotalRank), _fTimeRank (rhsRank._fTimeRank),
          _objectInfo (rhsRank._objectInfo), _loggingInfo (rhsRank._loggingInfo)
    {
        for (unsigned int i = 0; i < rhsRank._partialRanks.size(); i++) {
            addRank (_partialRanks[i]._partialRankDescription,
                     _partialRanks[i]._partialRank,
                     _partialRanks[i]._partialRankWeigth);
        }
    }

    inline Rank::~Rank (void)
    {
    }

    inline Rank * Rank::clone (void)
    {
        return new Rank (*this);
    }

    inline bool Rank::operator > (const Rank &rhsRank)
    {
        return _fTotalRank > rhsRank._fTotalRank;
    }

    inline bool Rank::operator < (const Rank &rhsRank)
    {
        return _fTotalRank < rhsRank._fTotalRank;
    }

    inline bool Rank::operator == (const Rank &rhsRank)
    {
        return _msgId == rhsRank._msgId &&
               _targetId == rhsRank._targetId;
    }

    inline Rank & Rank::operator += (const Rank &rhsRank)
    {
        if (_msgId == rhsRank._msgId) {
            if (_fTotalRank < rhsRank._fTotalRank) {
                _fTotalRank = rhsRank._fTotalRank;
            }
            if (_fTimeRank< rhsRank._fTimeRank) {
                _fTimeRank = rhsRank._fTimeRank;
            }
            _targetId += rhsRank._targetId;
            _rankByTarget += rhsRank._rankByTarget;
        }
        return *this;
    }

    inline void Rank::addRank (const char *pszDescription, float fValue, float fWeigth)
    {
        int i = _partialRanks.firstFree();
        if (i < 0) {
            return;
        }
        _partialRanks[i]._partialRank = fValue;
        _partialRanks[i]._partialRankDescription = pszDescription;
        _partialRanks[i]._partialRankWeigth = fWeigth;
    }

    inline RankObjectInfo::RankObjectInfo (void)
        : _i64SourceTimestamp (MetadataInterface::SOURCE_TIME_STAMP_UNSET)
    {
    }

    inline RankObjectInfo::RankObjectInfo (const RankObjectInfo &rhsRank)
        : _i64SourceTimestamp (rhsRank._i64SourceTimestamp),
          _objectId (rhsRank._objectId),
          _instanceId (rhsRank._instanceId),
          _mimeType (rhsRank._mimeType)
    {
    }

    inline RankObjectInfo::~RankObjectInfo (void)
    {
    }

    inline RankObjectInfo & RankObjectInfo::operator = (const RankObjectInfo &rhsRank)
    {
        _i64SourceTimestamp = rhsRank._i64SourceTimestamp;
        _objectId = rhsRank._objectId;
        _instanceId = rhsRank._instanceId;
        _mimeType = rhsRank._mimeType;

        return *this;
    }

    inline RankLoggingInfo::RankLoggingInfo (void)
    {
    }

    inline RankLoggingInfo::RankLoggingInfo (const RankLoggingInfo &rhsLoggingInfo)
        : _objectName (rhsLoggingInfo._objectName), _comment (rhsLoggingInfo._comment)
    {
    }

    inline RankLoggingInfo::~RankLoggingInfo (void)
    {
    }
    
    inline RankLoggingInfo & RankLoggingInfo::operator = (const RankLoggingInfo& rhsRank)
    {
        _objectName = rhsRank._objectName;
        _comment = rhsRank._comment;

        return *this;
    }
}

#endif	/* INCL_RANK_H */

