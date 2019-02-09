/*
 * Voi.cpp
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

#include "Voi.h"

#include "VoiImpl.h"

using namespace IHMC_VOI;
using namespace NOMADSUtil;

namespace IHMC_VOI
{
    NOMADSUtil::Logger *pNetLog;
}

namespace VOI
{
    MetadataConfiguration * set (MetadataConfiguration *pMetadataCfg, MetadataConfiguration *pDef)
    {
        return (pMetadataCfg == NULL ? pDef : pMetadataCfg);
    }

    MetadataRankerLocalConfiguration * set (MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg, MetadataRankerLocalConfiguration *pDef)
    {
        return (pMetadataRankerLocalCfg == NULL ? pDef : pMetadataRankerLocalCfg);
    }
}

using namespace VOI;

Voi::Voi (const char *pszSessionId, bool bInstrumented)
    : _pImpl (new VoiImpl (pszSessionId, bInstrumented))
{
}

Voi::~Voi (void)
{
    delete _pImpl;
}

int Voi::init (void)
{
    return _pImpl->init();
}

void Voi::addMetadataForPeer (const char *pszNodeId, MetadataInterface *pMetadata, const void *pData, uint32 ui32DataLen)
{
    if (pszNodeId == NULL) {
        return;
    }
    ImmutableInformationObject io (pMetadata, pData, ui32DataLen);
    _pImpl->addMetadataForPeer (pszNodeId, &io);
}

Score * Voi::getVoi (MetadataInterface *pMetadata, NodeContext *pNodeCtxt,
                     MetadataConfiguration *pMetadataCfg,
                     MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg) const
{
    ImmutableInformationObject io (pMetadata);
    return _pImpl->getVoi (&io, pNodeCtxt, pMetadataCfg, pMetadataRankerLocalCfg);
}

ScoreList * Voi::getVoi (MetadataList *pMetadataList, NodeContext *pNodeContext,
                         MetadataConfiguration *pMetadataCfg,
                         MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg) const
{
    InformationObjects ios;
    for (MetadataInterface *pMetadata = pMetadataList->getFirst(); pMetadata != NULL; pMetadata = pMetadataList->getNext()) {
        ImmutableInformationObject *pIO = new ImmutableInformationObject (pMetadata);
        ios.append (pIO);
    }
    ScoreList *pScores = _pImpl->getVoi (&ios, pNodeContext, pMetadataCfg, pMetadataRankerLocalCfg);
    deallocateAllPtrLListElements<InformationObject> (&ios);
    return pScores;
}

ScoreList * Voi::getVoi (MetadataList *pMetadataList, NodeContextList *pNodeCtxtList,
                         MetadataConfiguration *pMetadataCfg,
                         MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg) const
{
    InformationObjects ios;
    for (MetadataInterface *pMetadata = pMetadataList->getFirst(); pMetadata != NULL; pMetadata = pMetadataList->getNext()) {
        ImmutableInformationObject *pIO = new ImmutableInformationObject (pMetadata);
        ios.append (pIO);
    }
    ScoreList *pScores = _pImpl->getVoi (&ios, pNodeCtxtList, pMetadataCfg, pMetadataRankerLocalCfg);
    deallocateAllPtrLListElements<InformationObject> (&ios);
    return pScores;
}

