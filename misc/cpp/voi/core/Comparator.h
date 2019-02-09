/*
 * Comparator.h
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
 * Created on Febraury 15, 2017, 2:20 PM
 */

#ifndef INCL_VOI_COMPARATOR_H
#define	INCL_VOI_COMPARATOR_H

#include "Score.h"

namespace IHMC_VOI
{
    class MetadataInterface;
    struct MetadataRankerLocalConfiguration;
    class NodeContext;

    struct Comparator
    {
        static Score::Novelty compare (const MetadataInterface *pOldMetadata, const MetadataInterface *pNewMetadata,
                                       NodeContext *pNodeCtxt, MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg);
    };
}

namespace COMPARATOR
{
    IHMC_VOI::Score::Novelty compareTracks (const IHMC_VOI::MetadataInterface *pOldMetadata,
                                            const IHMC_VOI::MetadataInterface *pNewMetadata,
                                            IHMC_VOI::NodeContext *pNodeCtxt,
                                            IHMC_VOI::MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg);

    // Return the maximum percentage increase/decrease of the update
    float compareLogstats (const char *pszOldLogstat, const char *pszNewLogstat);

    IHMC_VOI::Score::Novelty compareLogstats (const IHMC_VOI::MetadataInterface *pOldMetadata,
                                              const IHMC_VOI::MetadataInterface *pNewMetadata,
                                              IHMC_VOI::NodeContext *pNodeCtxt,
                                              IHMC_VOI::MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg);
}

#endif  /* INCL_VOI_COMPARATOR_H */

