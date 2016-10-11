/* 
 * MetadataRankerConfiguration.cpp
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
 * Created on July 5, 2013, 12:39 PM
 */

#include "MetadataRankerConfiguration.h"

#include "Defs.h"
#include "LocalNodeContext.h"
#include "MetaDataRanker.h"

#include "ConfigManager.h"
#include "Logger.h"
#include "StringStringHashtable.h"
#include "StringTokenizer.h"
#include "Writer.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;
using namespace IHMC_MISC_MIL_STD_2525;

// ----------------------- MetadataRankerConfiguration -----------------------//

MetadataRankerConfiguration::MetadataRankerConfiguration (NOMADSUtil::ConfigManager *pCfgMgr)
{
    configure (MetaDataRanker::DEFAULT_COORD_WEIGHT, MetaDataRanker::DEFAULT_TIME_WEIGHT,
               MetaDataRanker::DEFAULT_EXP_WEIGHT, MetaDataRanker::DEFAULT_IMP_WEIGHT,
               MetaDataRanker::DEFAULT_SRC_REL_WEIGHT, MetaDataRanker::DEFAULT_INFO_CONTENT_WEIGHT,
               MetaDataRanker::DEFAULT_PRED_WEIGHT, MetaDataRanker::DEFAULT_TARGET_WEIGHT,
               MetaDataRanker::DEFAULT_STRICT_TARGET, MetaDataRanker::DEFAULT_CONSIDER_FUTURE_PATH_SEGMENTS);

    configure (pCfgMgr);
}

MetadataRankerConfiguration::~MetadataRankerConfiguration (void)
{
}

int MetadataRankerConfiguration::configure (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return -1;
    }

    if (pCfgMgr->hasValue (MetaDataRanker::COORDINATES_RANK_PROPERTY)) {
        _fCoordRankWeight = pCfgMgr->getValueAsFloat (MetaDataRanker::COORDINATES_RANK_PROPERTY);
    }
    if (pCfgMgr->hasValue (MetaDataRanker::TIME_RANK_PROPERTY)) {
        _fTimeRankWeight = pCfgMgr->getValueAsFloat (MetaDataRanker::TIME_RANK_PROPERTY);
    }
    if (pCfgMgr->hasValue (MetaDataRanker::EXPIRATION_RANK_PROPERTY)) {
        _fExpirationRankWeight = pCfgMgr->getValueAsFloat (MetaDataRanker::EXPIRATION_RANK_PROPERTY);
    }
    if (pCfgMgr->hasValue (MetaDataRanker::IMPORTANCE_RANK_PROPERTY)) {
        _fImpRankWeight = pCfgMgr->getValueAsFloat (MetaDataRanker::IMPORTANCE_RANK_PROPERTY);
    }
    if (pCfgMgr->hasValue (MetaDataRanker::SOURCE_RELIABILITY_RANK_PROPERTY)) {
        _fSourceReliabilityRankWeigth = pCfgMgr->getValueAsFloat (MetaDataRanker::SOURCE_RELIABILITY_RANK_PROPERTY);
    }
    if (pCfgMgr->hasValue (MetaDataRanker::INFORMATION_CONTENT_RANK_PROPERTY)) {
        _fInformationContentRankWeigth = pCfgMgr->getValueAsFloat (MetaDataRanker::INFORMATION_CONTENT_RANK_PROPERTY);
    }
    if (pCfgMgr->hasValue (MetaDataRanker::PREDICTION_RANK_PROPERTY)) {
        _fPredRankWeight = pCfgMgr->getValueAsFloat (MetaDataRanker::PREDICTION_RANK_PROPERTY);
    }
    if (pCfgMgr->hasValue (MetaDataRanker::TARGET_RANK_PROPERTY)) {
        _fTargetRankWeight = pCfgMgr->getValueAsFloat (MetaDataRanker::TARGET_RANK_PROPERTY);
    }
    if (pCfgMgr->hasValue (MetaDataRanker::TARGET_RANK_STRICT_PROPERTY)) {
        _bStrictTarget = pCfgMgr->getValueAsBool (MetaDataRanker::TARGET_RANK_STRICT_PROPERTY);
    }
    if (pCfgMgr->hasValue (MetaDataRanker::TIME_RANK_CONSIDER_FUTURE_SEG_PROPERTY)) {
        _bConsiderFuturePathSegmentForMatchmacking = pCfgMgr->getValueAsBool (MetaDataRanker::TIME_RANK_CONSIDER_FUTURE_SEG_PROPERTY);
    }

    logConfig();

    return 0;
}

int MetadataRankerConfiguration::configure (float coordRankWeight, float timeRankWeight,
                                            float expirationRankWeight, float impRankWeight,
                                            float sourceReliabilityRankWeigth, float informationContentRankWeigth,
                                            float predRankWeight, float targetWeight, bool bStrictTarget,
                                            bool bConsiderFuturePathSegmentForMatchmacking)
{
    _fCoordRankWeight = coordRankWeight;
    _fTimeRankWeight = timeRankWeight;
    _fExpirationRankWeight = expirationRankWeight;
    _fImpRankWeight = impRankWeight;
    _fSourceReliabilityRankWeigth = sourceReliabilityRankWeigth;
    _fInformationContentRankWeigth = informationContentRankWeigth;
    _fPredRankWeight = predRankWeight;
    _fTargetRankWeight = targetWeight;

    _bStrictTarget = bStrictTarget;
    _bConsiderFuturePathSegmentForMatchmacking = bConsiderFuturePathSegmentForMatchmacking;

    logConfig();

    return 0;
}

void MetadataRankerConfiguration::display (void)
{
    const char *pszMethodName = "MetadataRankerConfiguration::display";
    checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "_coordRankWeight <%f> | _timeRankWeight <%f> | _expirationRankWeight <%f> | _impRankWeight <%f> | _fSourceReliabilityRankWeigth <%f> | "
                    "_predRankWeight <%f> | _targetRankWeight <%f> | _bStrictTarget <%s> | _bConsiderFuturePathSegmentForMatchmacking <%s>\n",
                    _fCoordRankWeight, _fTimeRankWeight, _fExpirationRankWeight, _fImpRankWeight, _fSourceReliabilityRankWeigth, 
                    _fPredRankWeight, _fTargetRankWeight, _bStrictTarget ? "true" : "false",
                    _bConsiderFuturePathSegmentForMatchmacking ? "true" : "false");
}

uint32 MetadataRankerConfiguration::getLength (void)
{
    return (4 * 8) // 4 bytes * 8 rank weights
            + 1    // _bStrictTarget
            + 1;   // _bConsiderFuturePathSegmentForMatchmacking
}

int MetadataRankerConfiguration::read (Reader *pReader, uint32 ui32MaxLen)
{
    if (pReader == NULL) {
        return -1;
    }

    if (pReader->read32 (&_fCoordRankWeight) < 0) {
        return -2;
    }
    if (pReader->read32 (&_fTimeRankWeight) < 0) {
        return -3;
    }
    if (pReader->read32 (&_fExpirationRankWeight) < 0) {
        return -4;
    }
    if (pReader->read32 (&_fImpRankWeight) < 0) {
        return -5;
    }
    if (pReader->read32 (&_fSourceReliabilityRankWeigth) < 0) {
        return -5;
    }
    if (pReader->read32 (&_fInformationContentRankWeigth) < 0) {
        return -5;
    }
    if (pReader->read32 (&_fPredRankWeight) < 0) {
        return -6;
    }
    if (pReader->read32 (&_fTargetRankWeight) < 0) {
        return -7;
    }

    uint8 ui8 = 0;
    if (pReader->read8 (&ui8) < 0) {
        return -8;
    }
    _bStrictTarget = (ui8 == 1);

    ui8 = 0;
    if (pReader->read8 (&ui8) < 0) {
        return -9;
    }
    _bConsiderFuturePathSegmentForMatchmacking = (ui8 == 1);

    return 0;
}

int MetadataRankerConfiguration::write (Writer *pWriter, uint32 ui32MaxLen)
{
    if (pWriter == NULL) {
        return -1;
    }

    if (pWriter->write32 (&_fCoordRankWeight) < 0) {
        return -2;
    }
    if (pWriter->write32 (&_fTimeRankWeight) < 0) {
        return -3;
    }
    if (pWriter->write32 (&_fExpirationRankWeight) < 0) {
        return -4;
    }
    if (pWriter->write32 (&_fImpRankWeight) < 0) {
        return -5;
    }
    if (pWriter->write32 (&_fSourceReliabilityRankWeigth) < 0) {
        return -5;
    }
    if (pWriter->write32 (&_fInformationContentRankWeigth) < 0) {
        return -5;
    }
    if (pWriter->write32 (&_fPredRankWeight) < 0) {
        return -6;
    }
    if (pWriter->write32 (&_fTargetRankWeight) < 0) {
        return -7;
    }
    uint8 ui8 = (_bStrictTarget ? 1 : 0);
    if (pWriter->write8 (&ui8) < 0) {
        return -8;
    }
    ui8 = (_bConsiderFuturePathSegmentForMatchmacking ? 1 : 0);
    if (pWriter->write8 (&ui8) < 0) {
        return -9;
    }

    return 0;
}
    
void MetadataRankerConfiguration::logConfig (void)
{
    const char *pszMethodName = "MetadataRankerConfiguration::logConfig";
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Coord Rank Weight: %f\n", _fCoordRankWeight);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Time Rank Weight: %f\n", _fTimeRankWeight);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Expiration Rank Weight: %f\n", _fExpirationRankWeight);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Importance Rank Weight: %f\n", _fImpRankWeight);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Source Reliability Rank Weight: %f\n", _fSourceReliabilityRankWeigth);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Information Content Rank Weight: %f\n", _fInformationContentRankWeigth);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Prediction Rank Weight: %f\n", _fPredRankWeight);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Target Rank Weight: %f\n", _fTargetRankWeight);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Consider Strict Target: %d\n", _bStrictTarget);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Consider Future Segments: %d\n", _bConsiderFuturePathSegmentForMatchmacking);
}

// -------------------- MetadataRankerConfiguration --------------------------//

const char * MetadataRankerLocalConfiguration::NON_MATCHING_DATA_TYPES = "aci.dspro.controller.nonMatchingDataTypes";
const char * MetadataRankerLocalConfiguration::FILTER_MATCHMAKING_BY_PEER = "aci.dspro.controller.nonMatchingDataTypesByPeer";
const char * MetadataRankerLocalConfiguration::RANGE_OF_INFLUENCE_BY_MILSTD2525_SYMBOL_CODE = "aci.dspro.controller.rangeOfInfluenceByMilSTD2525SymbolCode";

MetadataRankerLocalConfiguration::MetadataRankerLocalConfiguration (const char *pszNodeId, LocalNodeContext *pLocalNodeCtxt)
    : _bInstrumented (true),
      _nodeId (pszNodeId),
      _pLocalNodeCtxt (pLocalNodeCtxt)
{
}

MetadataRankerLocalConfiguration::~MetadataRankerLocalConfiguration()
{   
}

int MetadataRankerLocalConfiguration::init (NOMADSUtil::ConfigManager *pCfgMgr)
{
    const char *pszMethodName = "MetadataRankerLocalConfiguration::init";
    if (pCfgMgr == NULL) {
        return -1;
    }

    const char *pszMatchamakingFiltersByNodeId = pCfgMgr->getValue (FILTER_MATCHMAKING_BY_PEER);
    if (pszMatchamakingFiltersByNodeId != NULL) {
        const char OUTER_SEPARATOR = ';';
        const char INNER_SEPARATOR = ',';
        StringTokenizer outerTokenizer (pszMatchamakingFiltersByNodeId, OUTER_SEPARATOR, OUTER_SEPARATOR);
        StringTokenizer innerTokenizer;
        for (const char *pszOuterToken; (pszOuterToken = outerTokenizer.getNextToken()) != NULL;) {
            innerTokenizer.init (pszOuterToken, INNER_SEPARATOR, INNER_SEPARATOR);
            const char *pszKey = innerTokenizer.getNextToken();
            NOMADSUtil::String value (innerTokenizer.getNextToken());
            if ((pszKey != NULL) && (value.length() > 0)) {
                _dataTypesNotToMatchByNodeId.put (pszKey, value.c_str());   
            }
        }        
    }
    if (pszMatchamakingFiltersByNodeId != NULL) {
        const char OUTER_SEPARATOR = ';';
        const char INNER_SEPARATOR = ',';
        StringTokenizer outerTokenizer (pszMatchamakingFiltersByNodeId, OUTER_SEPARATOR, OUTER_SEPARATOR);
        StringTokenizer innerTokenizer;
        for (const char *pszOuterToken; (pszOuterToken = outerTokenizer.getNextToken()) != NULL;) {
            innerTokenizer.init (pszOuterToken, INNER_SEPARATOR, INNER_SEPARATOR);
            const char *pszKey = innerTokenizer.getNextToken();
            NOMADSUtil::String value (innerTokenizer.getNextToken());
            if ((pszKey != NULL) && (value.length() > 0)) {
                NOMADSUtil::String searchKey (pszKey);
                if (searchKey.endsWith ("*")) {
                    searchKey = searchKey.substring (0, searchKey.length()-1);
                    searchKey += '-';
                }
                if (_dataTypesNotToMatchByNodeId.hasKeyValue (searchKey, value)) {
                    checkAndLogMsg ("MetadataRankerLocalConfiguration::init", Logger::L_Info, "--- adding matchmaking filter for peer %s on %s.\n", pszKey, value.c_str());
                }
            }
        }        
    }

    const char *pszRangeOfInflByMilSTD = pCfgMgr->getValue (RANGE_OF_INFLUENCE_BY_MILSTD2525_SYMBOL_CODE);
    if (pszRangeOfInflByMilSTD != NULL) {
        StringStringHashtable *pRangeOfInflByMilSTD = StringStringHashtable::parseStringStringHashtable (pszRangeOfInflByMilSTD);
        if (pRangeOfInflByMilSTD != NULL) {
            StringStringHashtable::Iterator iter = pRangeOfInflByMilSTD->getAllElements();
            for (; !iter.end(); iter.nextElement()) {
                SymbolCodeTemplate milSTD2525Symbol (iter.getKey());
                addRangeOfInfluence (milSTD2525Symbol, atoui32 (iter.getValue()));
            }
        }        
    }

    const char *pszNonMatchingTypes = pCfgMgr->getValue (NON_MATCHING_DATA_TYPES);
    if (pszNonMatchingTypes != NULL) {
        StringTokenizer tokenizer (pszNonMatchingTypes, ',', ',');
        for (const char *pszToken; (pszToken = tokenizer.getNextToken()) != NULL;) {
            if (strlen (pszToken) > 0) {
                _dataTypesToFilter.put (pszToken);
            }
        }
    } 

    checkAndLogMsg (pszMethodName, Logger::L_Info, "nodeId: %s\n", _nodeId.c_str());

    return 0;
}

uint32 MetadataRankerLocalConfiguration::getRangeOfInfluence (SymbolCode &milSTD2525Symbol)
{
    SymbolCodeTemplate milSTD2525SymbolTemplate = _symbols.getBestMatchingTemplate (milSTD2525Symbol);
    String sMilSTD2525SymbolTemplate = milSTD2525SymbolTemplate.toString();
    uint32 *pUI32RangeOfInfluence = _symbolCodeToRangeOfInfluence.get (sMilSTD2525SymbolTemplate);
    if (pUI32RangeOfInfluence == NULL) {
        return 0U;
    }
    return *pUI32RangeOfInfluence;
}

bool MetadataRankerLocalConfiguration::getLimitToLocalMatchmakingOnly (void) const
{
    if (_pLocalNodeCtxt != NULL) {
        return _pLocalNodeCtxt->getLimitToLocalMatchmakingOnly();
    }
    return false;
}

void MetadataRankerLocalConfiguration::addRangeOfInfluence (IHMC_MISC_MIL_STD_2525::SymbolCodeTemplate &milSTD2525SymbolTemplate,
                                                            uint32 ui32RangeOfInfluenceInMeters)
{
    uint32 *pui32 = (uint32*) calloc (1, sizeof (uint32));
    *pui32 = ui32RangeOfInfluenceInMeters;
    String sMilSTD2525SymbolTemplate = milSTD2525SymbolTemplate.toString();
    checkAndLogMsg ("MetadataRankerLocalConfiguration::addRangeOfInfluence", Logger::L_Info,
                    "setting %s range of influence to %u\n", sMilSTD2525SymbolTemplate.c_str(),
                    ui32RangeOfInfluenceInMeters);
    uint32 *pui32Old = _symbolCodeToRangeOfInfluence.put (sMilSTD2525SymbolTemplate, pui32);
    _symbols.addTemplate (milSTD2525SymbolTemplate);
    if (pui32Old != NULL) {
        free (pui32Old);
    }
}

