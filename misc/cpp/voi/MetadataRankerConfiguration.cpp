/*
 * MetadataRankerConfiguration.cpp
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
 * Created on July 5, 2013, 12:39 PM
 */

#include "MetadataRankerConfiguration.h"

#include "MetaDataRanker.h"
#include "VoiDefs.h"

#include "ConfigManager.h"
#include "Json.h"
#include "Logger.h"
#include "StringStringHashtable.h"

using namespace IHMC_VOI;
using namespace NOMADSUtil;

namespace METADATA_RANKER_WEIGHTS
{
    static const char * WEIGHTS = "matchmakerWeights";
    static const char * COORDINATE_RANK = "coord";
    static const char * TIME_RANK = "time";
    static const char * EXPIRATION_RANK = "exp";
    static const char * IMPORTANCE_RANK = "imp";
    static const char * SOURCE_RELIABILITY_RANK = "srcRel";
    static const char * INFORMATION_CONTENT_RANK = "infCtnt";
    static const char * PREDICTION_RANK = "pred";
    static const char * TARGET_RANK = "targ";

    static const unsigned short ALL_WEIGHTS_LEN = 8;
    static const char * ALL_WEIGHTS[ALL_WEIGHTS_LEN] = {
        COORDINATE_RANK, TIME_RANK, EXPIRATION_RANK, IMPORTANCE_RANK,
        SOURCE_RELIABILITY_RANK, INFORMATION_CONTENT_RANK, PREDICTION_RANK,
        TARGET_RANK
    };
}

namespace METADATA_RANKER_CONF
{
    static const char * STRICT_TARGET = "strictTarget";
    static const char * CONSIDER_FUTURE_SEGMENT = "considerFutureSegments";
}

namespace METADATA_RANKER_CONFIGURATION_UTILS
{
    bool setIfDifferent (float &fOldValue, float newValue)
    {
        if (fEquals (fOldValue, newValue, 0.0001f)) {
            return false;
        }
        fOldValue = newValue;
        return true;
    }

    bool setIfDifferent (bool &oldValue, bool newValue)
    {
        if (newValue) {
            if (oldValue) {
                return false;
            }
        }
        else if (!oldValue) {
            return false;
        }
        oldValue = !oldValue;
        return true;
    }

    bool setIfDifferent (ConfigManager *pCfgMgr, const char *pszPropertyName, bool &oldValue)
    {
        if (pCfgMgr->hasValue (pszPropertyName)) {
            return setIfDifferent (oldValue, pCfgMgr->getValueAsBool (pszPropertyName));
        }
        return false;
    }

    bool setIfDifferent (ConfigManager *pCfgMgr, const char *pszPropertyName, float &fOldValue)
    {
        if (pCfgMgr->hasValue (pszPropertyName)) {
            return setIfDifferent (fOldValue, pCfgMgr->getValueAsFloat (pszPropertyName));
        }
        return false;
    }
}

using namespace METADATA_RANKER_CONFIGURATION_UTILS;

// ----------------------- MetadataRankerConfiguration -----------------------//

MetadataRankerConfiguration::MetadataRankerConfiguration (ConfigManager *pCfgMgr)
    : _fCoordRankWeight (MetaDataRanker::DEFAULT_COORD_WEIGHT),
      _fTimeRankWeight (MetaDataRanker::DEFAULT_TIME_WEIGHT),
      _fExpirationRankWeight (MetaDataRanker::DEFAULT_EXP_WEIGHT),
      _fImpRankWeight (MetaDataRanker::DEFAULT_IMP_WEIGHT),
      _fSourceReliabilityRankWeigth (MetaDataRanker::DEFAULT_SRC_REL_WEIGHT),
      _fInformationContentRankWeigth (MetaDataRanker::DEFAULT_INFO_CONTENT_WEIGHT),
      _fPredRankWeight (MetaDataRanker::DEFAULT_PRED_WEIGHT),
      _fTargetRankWeight (MetaDataRanker::DEFAULT_TARGET_WEIGHT),
      _bStrictTarget (MetaDataRanker::DEFAULT_STRICT_TARGET),
      _bConsiderFuturePathSegmentForMatchmacking (MetaDataRanker::DEFAULT_CONSIDER_FUTURE_PATH_SEGMENTS)
{
    configure (pCfgMgr);
}

MetadataRankerConfiguration::~MetadataRankerConfiguration (void)
{
}

bool MetadataRankerConfiguration::configure (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return false;
    }

    bool bChanged = setIfDifferent (pCfgMgr, MetaDataRanker::COORDINATES_RANK_PROPERTY, _fCoordRankWeight);
    bChanged |= setIfDifferent (pCfgMgr, MetaDataRanker::TIME_RANK_PROPERTY, _fTimeRankWeight);
    bChanged |= setIfDifferent (pCfgMgr, MetaDataRanker::EXPIRATION_RANK_PROPERTY, _fExpirationRankWeight);
    bChanged |= setIfDifferent (pCfgMgr, MetaDataRanker::IMPORTANCE_RANK_PROPERTY, _fImpRankWeight);
    bChanged |= setIfDifferent (pCfgMgr, MetaDataRanker::SOURCE_RELIABILITY_RANK_PROPERTY, _fSourceReliabilityRankWeigth);
    bChanged |= setIfDifferent (pCfgMgr, MetaDataRanker::INFORMATION_CONTENT_RANK_PROPERTY, _fInformationContentRankWeigth);
    bChanged |= setIfDifferent (pCfgMgr, MetaDataRanker::PREDICTION_RANK_PROPERTY, _fPredRankWeight);
    bChanged |= setIfDifferent (pCfgMgr, MetaDataRanker::TARGET_RANK_PROPERTY, _fTargetRankWeight);

    bChanged |= setIfDifferent (pCfgMgr, MetaDataRanker::TARGET_RANK_STRICT_PROPERTY, _bStrictTarget);
    bChanged |= setIfDifferent (pCfgMgr, MetaDataRanker::TIME_RANK_CONSIDER_FUTURE_SEG_PROPERTY, _bConsiderFuturePathSegmentForMatchmacking);

    logConfig();
    return bChanged;
}

bool MetadataRankerConfiguration::configure (float coordRankWeight, float timeRankWeight,
                                             float expirationRankWeight, float impRankWeight,
                                             float sourceReliabilityRankWeigth, float informationContentRankWeigth,
                                             float predRankWeight, float targetWeight, bool bStrictTarget,
                                             bool bConsiderFuturePathSegmentForMatchmacking)
{
    bool bChanged = setIfDifferent (_fCoordRankWeight, coordRankWeight);
    bChanged |= setIfDifferent (_fTimeRankWeight, timeRankWeight);
    bChanged |= setIfDifferent (_fExpirationRankWeight, expirationRankWeight);
    bChanged |= setIfDifferent (_fImpRankWeight, impRankWeight);
    bChanged |= setIfDifferent (_fSourceReliabilityRankWeigth, sourceReliabilityRankWeigth);
    bChanged |= setIfDifferent (_fInformationContentRankWeigth, informationContentRankWeigth);
    bChanged |= setIfDifferent (_fPredRankWeight, predRankWeight);
    bChanged |= setIfDifferent (_fTargetRankWeight, targetWeight);

    bChanged |= setIfDifferent (_bStrictTarget, bStrictTarget);
    bChanged |= setIfDifferent (_bConsiderFuturePathSegmentForMatchmacking, bConsiderFuturePathSegmentForMatchmacking);

    logConfig();
    return bChanged;
}

int MetadataRankerConfiguration::fromJson (const NOMADSUtil::JsonObject *pJson)
{
    if (pJson == NULL) {
        return -1;
    }

    JsonObject *pWeights = pJson->getObject (METADATA_RANKER_WEIGHTS::WEIGHTS);
    if (pWeights != NULL) {
        for (unsigned short i = 0; i < METADATA_RANKER_WEIGHTS::ALL_WEIGHTS_LEN; i++) {
            const String name (METADATA_RANKER_WEIGHTS::ALL_WEIGHTS[i]);
            if (pWeights->hasObject (name)) {
                double dWeight;
                if ((pWeights->getNumber (name, dWeight) == 0) && (name == METADATA_RANKER_WEIGHTS::COORDINATE_RANK)) {
                    _fCoordRankWeight = (float) dWeight;
                }
                else if ((pWeights->getNumber (name, dWeight) == 0) && (name == METADATA_RANKER_WEIGHTS::TIME_RANK)) {
                    _fTimeRankWeight = (float) dWeight;
                }
                else if ((pWeights->getNumber (name, dWeight) == 0) && (name == METADATA_RANKER_WEIGHTS::EXPIRATION_RANK)) {
                    _fExpirationRankWeight = (float) dWeight;
                }
                else if ((pWeights->getNumber (name, dWeight) == 0) && (name == METADATA_RANKER_WEIGHTS::IMPORTANCE_RANK)) {
                    _fImpRankWeight = (float) dWeight;
                }
                else if ((pWeights->getNumber (name, dWeight) == 0) && (name == METADATA_RANKER_WEIGHTS::SOURCE_RELIABILITY_RANK)) {
                    _fSourceReliabilityRankWeigth = (float) dWeight;
                }
                else if ((pWeights->getNumber (name, dWeight) == 0) && (name == METADATA_RANKER_WEIGHTS::INFORMATION_CONTENT_RANK)) {
                    _fInformationContentRankWeigth = (float) dWeight;
                }
                else if ((pWeights->getNumber (name, dWeight) == 0) && (name == METADATA_RANKER_WEIGHTS::PREDICTION_RANK)) {
                    _fPredRankWeight = (float) dWeight;
                }
                else if ((pWeights->getNumber (name, dWeight) == 0) && (name == METADATA_RANKER_WEIGHTS::TARGET_RANK)) {
                    _fTargetRankWeight = (float) dWeight;
                }
            }
        }
        delete pWeights;
    }

    bool bValue;
    if (pJson->getBoolean (METADATA_RANKER_CONF::STRICT_TARGET, bValue) == 0) {
        _bStrictTarget = bValue;
    }
    if (pJson->getBoolean (METADATA_RANKER_CONF::CONSIDER_FUTURE_SEGMENT, bValue) == 0) {
        _bConsiderFuturePathSegmentForMatchmacking = bValue;
    }

    return 0;
}

JsonObject * MetadataRankerConfiguration::toJson (void)
{
    JsonObject *pJson = new JsonObject();
    if (pJson == NULL) {
        return NULL;
    }
    JsonObject *pArray = new JsonObject();
    if (pArray != NULL) {
        pArray->setNumber (METADATA_RANKER_WEIGHTS::COORDINATE_RANK, _fCoordRankWeight);
        pArray->setNumber (METADATA_RANKER_WEIGHTS::TIME_RANK, _fTimeRankWeight);
        pArray->setNumber (METADATA_RANKER_WEIGHTS::EXPIRATION_RANK, _fExpirationRankWeight);
        pArray->setNumber (METADATA_RANKER_WEIGHTS::IMPORTANCE_RANK, _fImpRankWeight);
        pArray->setNumber (METADATA_RANKER_WEIGHTS::SOURCE_RELIABILITY_RANK, _fSourceReliabilityRankWeigth);
        pArray->setNumber (METADATA_RANKER_WEIGHTS::INFORMATION_CONTENT_RANK, _fInformationContentRankWeigth);
        pArray->setNumber (METADATA_RANKER_WEIGHTS::PREDICTION_RANK, _fPredRankWeight);
        pArray->setNumber (METADATA_RANKER_WEIGHTS::TARGET_RANK, _fTargetRankWeight);

        pJson->setObject (METADATA_RANKER_WEIGHTS::WEIGHTS, pArray);
	delete pArray;
    }
    pJson->setBoolean (METADATA_RANKER_CONF::STRICT_TARGET, _bStrictTarget);
    pJson->setBoolean (METADATA_RANKER_CONF::CONSIDER_FUTURE_SEGMENT, _bConsiderFuturePathSegmentForMatchmacking);
    return pJson;
}

void MetadataRankerConfiguration::logConfig (void)
{
    const char *pszMethodName = "MetadataRankerConfiguration::logConfig";
    const JsonObject *pJson = toJson();
    if (pJson != NULL) {
        const String s (pJson->toString());
        checkAndLogMsg (pszMethodName, Logger::L_Info, "%s\n", s.c_str());
	delete pJson;
    }
}


