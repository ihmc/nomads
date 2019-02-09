/*
 * MetaDataRankerConfigurationImpl.h
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

#ifndef INCL_METADATA_RANKER_LOCAL_CONFIGURATION_IMPL_H
#define INCL_METADATA_RANKER_LOCAL_CONFIGURATION_IMPL_H


#include "MetadataRankerLocalConfiguration.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class LocalNodeContext;

    class MetadataRankerLocalConfigurationImpl : public IHMC_VOI::MetadataRankerLocalConfiguration
    {
        public:
            MetadataRankerLocalConfigurationImpl (const char *pszNodeId, LocalNodeContext *pLocalNodeCtxt);
            ~MetadataRankerLocalConfigurationImpl (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);

            bool getLimitToLocalMatchmakingOnly (void);

        private:
            LocalNodeContext *_pLocalNodeCtxt;
    };
}

#endif  /* INCL_METADATA_RANKER_LOCAL_CONFIGURATION_IMPL_H */
