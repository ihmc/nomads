/*
 * MatchMakingFilters.h
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
 * Created on February 9, 2017, 3:50 AM
 */

#ifndef INCL_MATCH_MAKING_FILTERS_H
#define INCL_MATCH_MAKING_FILTERS_H

namespace IHMC_VOI
{
    class MetadataInterface;
    struct MetadataRankerLocalConfiguration;
    class NodeContext;
    struct Rank;

    Rank * filterByDataType (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                             MetadataRankerLocalConfiguration *pMetadataRankerLocalConf);

    Rank * filterByPedigree (MetadataInterface *pMetadata, NodeContext *pNodeContext,
                             MetadataRankerLocalConfiguration *pMetadataRankerLocalConf);
}

#endif  /* INCL_MATCH_MAKING_FILTERS_H */

