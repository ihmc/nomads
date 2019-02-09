/*
 * C45Utils.h
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
 * Created on February 7, 2017, 12:43 AM
 */

#ifndef INCL_C45_UTILS_H
#define	INCL_C45_UTILS_H

namespace IHMC_C45
{
    class C45AVList;
    class Prediction;
}

namespace IHMC_VOI
{
    class MetadataConfiguration;
    class MetadataInterface;
    class NodeContext;
}

namespace C45Utils
{
    IHMC_C45::C45AVList * getMetadataAsDataset (IHMC_VOI::MetadataInterface *pMetadata,
                                                IHMC_VOI::MetadataConfiguration *pMetadataCfg);
    IHMC_C45::C45AVList * getMetadataAsC45List (IHMC_VOI::MetadataInterface *pMetadata,
                                                IHMC_VOI::MetadataConfiguration *pMetadataCfg);
    IHMC_C45::Prediction * getPrediction (IHMC_VOI::MetadataInterface *pMetadata,
                                          IHMC_VOI::NodeContext *pNodeCtxt,
                                          IHMC_VOI::MetadataConfiguration *pMetadataCfg);
}

#endif  /* INCL_C45_UTILS_H */
