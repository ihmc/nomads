/*
* Rank.cpp
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
*/

#include "Rank.h"

using namespace IHMC_VOI;
using namespace NOMADSUtil;

Rank::Rank (const char *pszMsgId, NodeIdSet &rhsMatchingNodes,
    RankByTargetMap &rhdRankByTarget, bool bFiltered,
    float fTotalRank, float fTimeRank, uint32 ui32RefDataSize)
    : _msgId (pszMsgId), _targetId (rhsMatchingNodes), _rankByTarget (rhdRankByTarget), _bFiltered (bFiltered),
    _fTotalRank (fTotalRank), _fTimeRank (fTimeRank), _ui32RefDataSize (ui32RefDataSize)
{
}

Rank::Rank (const Rank &rhsRank)
    : _msgId (rhsRank._msgId), _targetId (rhsRank._targetId), _rankByTarget (rhsRank._rankByTarget),
    _bFiltered (rhsRank._bFiltered), _fTotalRank (rhsRank._fTotalRank), _fTimeRank (rhsRank._fTimeRank),
    _objectInfo (rhsRank._objectInfo), _loggingInfo (rhsRank._loggingInfo)
{
    for (unsigned int i = 0; i < rhsRank._partialRanks.size (); i++) {
        addRank (_partialRanks[i]._partialRankDescription,
            _partialRanks[i]._partialRank,
            _partialRanks[i]._partialRankWeigth);
    }
}

Rank::~Rank (void)
{
}

Rank * Rank::clone (void)
{
    return new Rank (*this);
}

bool Rank::operator > (const Rank &rhsRank)
{
    return _fTotalRank > rhsRank._fTotalRank;
}

bool Rank::operator < (const Rank &rhsRank)
{
    return _fTotalRank < rhsRank._fTotalRank;
}

bool Rank::operator == (const Rank &rhsRank)
{
    return _msgId == rhsRank._msgId &&
        _targetId == rhsRank._targetId;
}

Rank & Rank::operator += (const Rank &rhsRank)
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

void Rank::addRank (const char *pszDescription, float fValue, float fWeigth)
{
    int i = _partialRanks.firstFree ();
    if (i < 0) {
        return;
    }
    _partialRanks[i]._partialRank = fValue;
    _partialRanks[i]._partialRankDescription = pszDescription;
    _partialRanks[i]._partialRankWeigth = fWeigth;
}

RankObjectInfo::RankObjectInfo (void)
    : _i64SourceTimestamp (MetadataInterface::SOURCE_TIME_STAMP_UNSET)
{
}

RankObjectInfo::RankObjectInfo (const RankObjectInfo &rhsRank)
    : _i64SourceTimestamp (rhsRank._i64SourceTimestamp),
    _objectId (rhsRank._objectId),
    _instanceId (rhsRank._instanceId),
    _mimeType (rhsRank._mimeType)
{
}

RankObjectInfo::~RankObjectInfo (void)
{
}

RankObjectInfo & RankObjectInfo::operator = (const RankObjectInfo &rhsRank)
{
    _i64SourceTimestamp = rhsRank._i64SourceTimestamp;
    _objectId = rhsRank._objectId;
    _instanceId = rhsRank._instanceId;
    _mimeType = rhsRank._mimeType;

    return *this;
}

RankLoggingInfo::RankLoggingInfo (void)
{
}

RankLoggingInfo::RankLoggingInfo (const RankLoggingInfo &rhsLoggingInfo)
    : _objectName (rhsLoggingInfo._objectName), _comment (rhsLoggingInfo._comment)
{
}

RankLoggingInfo::~RankLoggingInfo (void)
{
}

RankLoggingInfo & RankLoggingInfo::operator = (const RankLoggingInfo& rhsRank)
{
    _objectName = rhsRank._objectName;
    _comment = rhsRank._comment;

    return *this;
}

//------------------------------------------------------------------------------
// Ranks
//------------------------------------------------------------------------------

Ranks::Ranks (void)
    : PtrLList<Rank> (true /* descendingOrder */)
{
}

Ranks::Ranks (NOMADSUtil::String &targetNodeId)
    : PtrLList<Rank> (true /* descendingOrder */),
      _targetNodeId (targetNodeId)
{
}

Ranks::~Ranks (void)
{
}

