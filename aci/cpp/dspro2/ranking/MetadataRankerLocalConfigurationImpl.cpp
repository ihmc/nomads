/*
 * MetaDataRankerConfigurationImpl.cpp
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 *
 * Author: Giacomo Benincasa   (gbenincasa@ihmc.us)
 * Created on February 8, 2017, 3:47 PM
 */

#include "MetadataRankerLocalConfigurationImpl.h"
#include "StringTokenizer.h"

#include "Logger.h"
#include "Defs.h"
#include "ConfigManager.h"
#include "LocalNodeContext.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

MetadataRankerLocalConfigurationImpl::MetadataRankerLocalConfigurationImpl (const char *pszNodeId, LocalNodeContext *pLocalNodeCtxt)
    : MetadataRankerLocalConfiguration (pszNodeId),
      _pLocalNodeCtxt (pLocalNodeCtxt)
{
}

MetadataRankerLocalConfigurationImpl::~MetadataRankerLocalConfigurationImpl (void)
{
}

namespace MD_RANKER_LOCAL_CONF_PROPERTY
{
    static const char * NON_MATCHING_DATA_TYPES = "aci.dspro.controller.nonMatchingDataTypes";
    static const char * FILTER_MATCHMAKING_BY_PEER = "aci.dspro.controller.nonMatchingDataTypesByPeer";
    static const char * RANGE_OF_INFLUENCE_KEY_ATTRIBUTE = "aci.dspro.matchmaking.rangeOfInfluence.keyProperty";
    static const char * TRACK_NOVELTY_INSIGN_DISTANCE_FACTOR = "aci.dspro.novelty.tracks.insignificantDistancePerc";
    static const char * LOGSTAT_NOVELTY_INSIGN_CHANGE_FACTOR = "aci.dspro.novelty.logstat.insignificantChangePerc";
}

int MetadataRankerLocalConfigurationImpl::init (NOMADSUtil::ConfigManager *pCfgMgr)
{
    const char *pszMethodName = "MetadataRankerLocalConfiguration::init";
    if (pCfgMgr == nullptr) {
        return -1;
    }

    const char *pszMatchamakingFiltersByNodeId = pCfgMgr->getValue (MD_RANKER_LOCAL_CONF_PROPERTY::FILTER_MATCHMAKING_BY_PEER);
    if (pszMatchamakingFiltersByNodeId != nullptr) {
        const char OUTER_SEPARATOR = ';';
        const char INNER_SEPARATOR = ',';
        StringTokenizer outerTokenizer (pszMatchamakingFiltersByNodeId, OUTER_SEPARATOR, OUTER_SEPARATOR);
        StringTokenizer innerTokenizer;
        for (const char *pszOuterToken; (pszOuterToken = outerTokenizer.getNextToken ()) != nullptr;) {
            innerTokenizer.init (pszOuterToken, INNER_SEPARATOR, INNER_SEPARATOR);
            const char *pszKey = innerTokenizer.getNextToken ();
            String value (innerTokenizer.getNextToken ());
            if ((pszKey != nullptr) && (value.length () > 0)) {
                addFilter (pszKey, value.c_str());
            }
        }
    }
    if (pszMatchamakingFiltersByNodeId != nullptr) {
        const char OUTER_SEPARATOR = ';';
        const char INNER_SEPARATOR = ',';
        StringTokenizer outerTokenizer (pszMatchamakingFiltersByNodeId, OUTER_SEPARATOR, OUTER_SEPARATOR);
        StringTokenizer innerTokenizer;
        for (const char *pszOuterToken; (pszOuterToken = outerTokenizer.getNextToken ()) != nullptr;) {
            innerTokenizer.init (pszOuterToken, INNER_SEPARATOR, INNER_SEPARATOR);
            const char *pszKey = innerTokenizer.getNextToken ();
            String value (innerTokenizer.getNextToken ());
            if ((pszKey != nullptr) && (value.length () > 0)) {
                String searchKey (pszKey);
                if (searchKey.endsWith ("*")) {
                    searchKey = searchKey.substring (0, searchKey.length () - 1);
                    searchKey += '-';
                }
                if (hasFilterForTypeAndPeer (searchKey, value)) {
                    checkAndLogMsg (pszMethodName, Logger::L_Info, "--- adding matchmaking filter for peer %s on %s.\n",
                                    pszKey, value.c_str());
                }
            }
        }
    }

    const char *pszNonMatchingTypes = pCfgMgr->getValue (MD_RANKER_LOCAL_CONF_PROPERTY::NON_MATCHING_DATA_TYPES);
    if (pszNonMatchingTypes != nullptr) {
        StringTokenizer tokenizer (pszNonMatchingTypes, ',', ',');
        for (const char *pszToken; (pszToken = tokenizer.getNextToken ()) != nullptr;) {
            if (strlen (pszToken) > 0) {
                addFilter (pszToken);
            }
        }
    }

    if (pCfgMgr->hasValue (MD_RANKER_LOCAL_CONF_PROPERTY::RANGE_OF_INFLUENCE_KEY_ATTRIBUTE)) {
        const String val (pCfgMgr->getValue (MD_RANKER_LOCAL_CONF_PROPERTY::RANGE_OF_INFLUENCE_KEY_ATTRIBUTE));
        setRangeOfInfluenceAttributeName (val);
    }

    if (pCfgMgr->hasValue (MD_RANKER_LOCAL_CONF_PROPERTY::TRACK_NOVELTY_INSIGN_DISTANCE_FACTOR)) {
        const String val (pCfgMgr->getValue (MD_RANKER_LOCAL_CONF_PROPERTY::TRACK_NOVELTY_INSIGN_DISTANCE_FACTOR));
        if (val.length() > 0) {
            _track.setInsignificantTrackMovementFactor ((float)atof (val));
        }
    }

    if (pCfgMgr->hasValue (MD_RANKER_LOCAL_CONF_PROPERTY::LOGSTAT_NOVELTY_INSIGN_CHANGE_FACTOR)) {
        const String val (pCfgMgr->getValue (MD_RANKER_LOCAL_CONF_PROPERTY::LOGSTAT_NOVELTY_INSIGN_CHANGE_FACTOR));
        if (val.length() > 0) {
            _logStat.setInsignificantUpdatePerc ((float)atof (val));
        }
    }

    return 0;
}

bool MetadataRankerLocalConfigurationImpl::getLimitToLocalMatchmakingOnly (void)
{
    return _pLocalNodeCtxt->getLimitToLocalMatchmakingOnly();
}

