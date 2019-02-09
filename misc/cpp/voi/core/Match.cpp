/*
 * Match.cpp
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

#include "Match.h"

#include "MetadataRankerLocalConfiguration.h"
#include "VoiDefs.h"

#include "Logger.h"

using namespace IHMC_VOI;
using namespace NOMADSUtil;

float checkConfidenceRange (Match::FuzzyValue match, float fMatchConfidence);

Match::Match (void)
    : _match (Match::NOT_SURE),
      _fMatchConfidence (MetadataRankerLocalConfiguration::MIN_RANK)
{
}

Match::Match (float fMatchConfidence)
    : _match (Match::YES),
      _fMatchConfidence (checkConfidenceRange (_match, fMatchConfidence))
{
}

Match::Match (FuzzyValue match, float fMatchConfidence)
    : _match (match),
      _fMatchConfidence (checkConfidenceRange (_match, fMatchConfidence))
{
}

Match::~Match (void)
{
}

int Match::operator > (Match &rhsMatch) const
{
    if (_match > rhsMatch._match) {
        return 1;
    }
    else if ((_match == rhsMatch._match) && (_fMatchConfidence > rhsMatch._fMatchConfidence)) {
        return 1;
    }
    return 0;
}

float checkConfidenceRange (Match::FuzzyValue match, float fMatchConfidence)
{
    float fMatchConf;
    if (fMatchConfidence < MetadataRankerLocalConfiguration::MIN_RANK) {
        checkAndLogMsg ("Match::Match", Logger::L_SevereError, "fMatchConfidence set to a value less than MIN_RANK: %f\n", fMatchConfidence);
        assert (false);
        fMatchConf = MetadataRankerLocalConfiguration::MIN_RANK;
    }
    else if (fMatchConfidence > MetadataRankerLocalConfiguration::MAX_RANK) {
        checkAndLogMsg ("Match::Match", Logger::L_SevereError, "fMatchConfidence set to a value greater than MAX_RANK: %f\n", fMatchConfidence);
        assert (false);
        fMatchConf = MetadataRankerLocalConfiguration::MAX_RANK;
    }
    else {
        fMatchConf = fMatchConfidence;
    }

    if (match == Match::YES) {
        return fMatchConf;
    }
    if (fMatchConf > MetadataRankerLocalConfiguration::MIN_RANK) {
        checkAndLogMsg ("Match::Match", Logger::L_SevereError, "fMatchConfidence set to a value greater than MIN_RANK even though it did not match: %f\n", fMatchConf);
        assert (false);
        fMatchConf = MetadataRankerLocalConfiguration::MIN_RANK;
    }
    return fMatchConf;
}


