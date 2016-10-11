/* 
 * ServingRequestProbability.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 * Created on April 15, 2015, 6:13 PM
 */

#include "ServingRequestProbability.h"

#include "DisServiceDefs.h"

#include "ConfigManager.h"
#include "Logger.h"

#include "StrClass.h"

#include <stdlib.h>

namespace IHMC_ACI
{
    double getNeighborDependentServingRequestProbability (unsigned int uiNumberOfNeighbors)
    {
        switch (uiNumberOfNeighbors) {
            // Makes sure the probability that at least one neighbor replies is >= 0.99
            case  0: return 1.0;
            case  1: return 1.0;
            /*case  2: return 0.88;
            case  3: return 0.76;
            case  4: return 0.66;
            case  5: return 0.57;
            case  6: return 0.51;
            case  7: return 0.46;
            case  8: return 0.41;
            case  9: return 0.38;
            case 10: return 0.35;
            case 11: return 0.32;
            case 12: return 0.30;*/
            // default: return (1 / sqrt ((double)uiNumberOfNeighbors));
            default: return (1.0 / ((double) uiNumberOfNeighbors));
        }
    };
}

using namespace IHMC_ACI;
using namespace NOMADSUtil;

ServingRequestProbability::ServingRequestProbability (void)
    : _mode (NEIGHBOR_DEPENDENT_PROB),
      _fMissingFragReqReplyFixedProb (1.0f)
{
}

ServingRequestProbability::~ServingRequestProbability (void)
{
}

int ServingRequestProbability::init (ConfigManager *pCfgMgr)
{
    const char *pszMethodName = "ServingRequestProbability::init";
    const char *pszPropertyName = "aci.disService.fragReqReply.probability";
    String log ("Missing Fragment Request Reply Mode: ");
    if (pCfgMgr->hasValue (pszPropertyName)) {
        _fMissingFragReqReplyFixedProb = static_cast<double>(pCfgMgr->getValueAsInt (pszPropertyName)) / (100.0);
        _mode = FIXED_REPLY_PROB;
        log += "FIXED_REPLY_PROB ";
        checkAndLogMsg (pszMethodName, Logger::L_Info, "%s and the probability was set to %f.\n",
                        log.c_str(), _fMissingFragReqReplyFixedProb * 100);
    }
    else {
        _mode = NEIGHBOR_DEPENDENT_PROB;
        log += "NEIGHBOR_DEPENDENT_PROB";
        checkAndLogMsg (pszMethodName, Logger::L_Info, "%s.\n", log.c_str());
    }
    return 0;
}

bool ServingRequestProbability::serveRequest (unsigned int uiNumberOfActiveNeighbors)
{
    static const uint64 max = RAND_MAX;
    return (rand()/((double)(max +1))) < getProbability (uiNumberOfActiveNeighbors);
}

float ServingRequestProbability::getProbability (unsigned int uiNumberOfActiveNeighbors)
{
    switch (_mode) {
        case NEIGHBOR_DEPENDENT_PROB:
            return getNeighborDependentServingRequestProbability (uiNumberOfActiveNeighbors);

        default:
            return _fMissingFragReqReplyFixedProb;
    }
}

