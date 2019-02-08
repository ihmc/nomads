/*
 * MatchmakingIntrumentation.cpp
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

#include "MatchmakingIntrumentation.h"

#include "Defs.h"

#include "Rank.h"

#include "Logger.h"

#include <stdlib.h>

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

const char * MatchmakingIntrumentation::PUSH_TYPE = "Push";
const char * MatchmakingIntrumentation::PULL_TYPE = "Pull";

//==============================================================================
// MatchmakingIntrumentation
//==============================================================================

MatchmakingIntrumentation::MatchmakingIntrumentation (bool bSkipped, const char *pszLocalNodeId, Rank *pRank)
    : _bSkipped (bSkipped), _fThreshold (0.0f), _pRank (pRank), _localNodeId (pszLocalNodeId)
{
    assert (_bSkipped ==  ((pRank->_bFiltered) ||(pRank->_fTotalRank < _fThreshold)));
}

MatchmakingIntrumentation::MatchmakingIntrumentation (bool bSkipped, const char *pszLocalNodeId, float fThreshold, Rank *pRank)
    : _bSkipped (bSkipped), _fThreshold (fThreshold), _pRank (pRank), _localNodeId (pszLocalNodeId)
{
    const bool bValid = (_bSkipped ==  ((pRank->_bFiltered) ||(pRank->_fTotalRank < _fThreshold)));
    if (!bValid) {
        checkAndLogMsg ("MatchmakingIntrumentation::MatchmakingIntrumentation", Logger::L_Warning,
                        "_bSkippes set to %d while total rank is %f and the matching threshold is %f.\n",
                        _bSkipped, pRank->_fTotalRank, _fThreshold);
        assert (bValid);
    }
}

MatchmakingIntrumentation::~MatchmakingIntrumentation()
{
    if (_pRank != nullptr) {
        delete _pRank;
    }
}

void MatchmakingIntrumentation::setNextHopNodeId (const char *pszNextHopNodeId)
{
    _nextHopNodeId = pszNextHopNodeId;
}

const char * MatchmakingIntrumentation::getType (void)
{
    if (_pRank == nullptr) {
        return nullptr;
    }
    if (_localNodeId == _pRank->_targetId) {
        return PULL_TYPE;
    }
    return PUSH_TYPE;
}

bool MatchmakingIntrumentation::operator == (const MatchmakingIntrumentation &rhsMatchmakingInstrumentation)
{
    return _pRank->_msgId == rhsMatchmakingInstrumentation._pRank->_msgId &&
           _pRank->_targetId == rhsMatchmakingInstrumentation._pRank->_targetId;
}
