/*
 * VoiImpl.h
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
 * Created on Febraury 15, 2017, 2:38 PM
 */

#ifndef INCL_VOI_IMPLEMENTATION_H
#define INCL_VOI_IMPLEMENTATION_H

#include "Cache.h"
#include "NodeContext.h"
#include "Score.h"

namespace IHMC_VOI
{
    class MetadataConfiguration;
    struct MetadataRankerLocalConfiguration;

    class VoiImpl
    {
        public:
            explicit VoiImpl (const char *pszSessionId, bool bInstrumented = false);
            ~VoiImpl (void);

            int init (void);

            void addMetadataForPeer (const char *pszNodeId, InformationObject *pIO);

            Score * getVoi (InformationObject *pIO, NodeContext *pNodeCtxt,
                            MetadataConfiguration *pMetadataCfg = NULL,
                            MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg = NULL) const;

            ScoreList * getVoi (InformationObjects *pIOList, NodeContext *pNodeContext,
                                MetadataConfiguration *pMetadataCfg = NULL,
                                MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg = NULL) const;

            ScoreList * getVoi (InformationObjects *pIOList, NodeContextList *pNodeCtxtList,
                                MetadataConfiguration *pMetadataCfg = NULL,
                                MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg = NULL) const;

        private:
            bool _bInstrumented;
            Cache _cache;
            MetadataConfiguration *_pMetadataCfg;
            MetadataRankerLocalConfiguration *_pMetadataRankerLocalCfg;
    };
}

#endif  /* INCL_VOI_IMPLEMENTATION_H */

