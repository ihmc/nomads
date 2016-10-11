/*
 * MetadataGenerator.cpp
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
 */

#include "MetadataGenerator.h"

#include "Defs.h"
#include "DSProImpl.h"
#include "InformationStore.h"
#include "MessageIdGenerator.h"
#include "MetadataInterface.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

MetadataGenerator::MetadataGenerator (DSProImpl *pDSPro, InformationStore *pInfoStore)
{
    _pDSPro = pDSPro;
    _pInfoStore = pInfoStore;
}

MetadataGenerator::~MetadataGenerator()
{
}

char * MetadataGenerator::addPreviousMessageValue (const char *pszBaseMetadataMsgId,
                                                   PreviousMessageIds &prevMsgIds, RankByTargetMap &ranksByTarget)
{
    const char *pszMethodName = "MetadataGenerator::addPreviousMessageValue";
    if ((pszBaseMetadataMsgId == NULL)) {
        return NULL;
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "previous message pushed is %s\n",
                    prevMsgIds.isEmpty() ? "null" : (const char *) prevMsgIds);

    MetadataInterface *pMetadata = _pInfoStore->getMetadata (pszBaseMetadataMsgId);
    if (pMetadata == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "metadata %s not found\n" , pszBaseMetadataMsgId);
        return NULL;
    }

    bool bAddPreviosMsgId = true;
    String setPrevMsgId;
    if ((pMetadata->getFieldValue (MetadataInterface::PREV_MSG_ID, setPrevMsgId) < 0) || (setPrevMsgId.length() <= 0)) {
        if (prevMsgIds.isEmpty()) {
            bAddPreviosMsgId = false;
        }
    }

    bool bAddTargetsByRank = true;
    String sRanksByTarget;
    if ((pMetadata->getFieldValue (MetadataInterface::RANKS_BY_TARGET_NODE_ID, sRanksByTarget) < 0) || (sRanksByTarget.length() <= 0)) {
        if (ranksByTarget.isEmpty()) {
            bAddTargetsByRank = false;
        }
    }

    if (!bAddPreviosMsgId && !bAddTargetsByRank) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "%s for metadata %s not set. "
                        "No need to generate new metadata\n", MetadataInterface::PREV_MSG_ID,
                        pszBaseMetadataMsgId);
        // no need to generate a new previous message id
        delete pMetadata;
    }

    // addPreviousMessageValueInternal modifies pMetadata - no problem here, since
    // pMetadata is not used anymore, in fact, it is even deallocated
    char *pszNewMetadataID = addPreviousMessageValueInternal (pMetadata, pszBaseMetadataMsgId,
                                                              prevMsgIds, ranksByTarget);
    delete pMetadata;

    return pszNewMetadataID;
}

char * MetadataGenerator::addPreviousMessageValueInternal (MetadataInterface *pBaseMetadata,
                                                           const char *pszBaseMetadataMsgId,
                                                           PreviousMessageIds &prevMsgIds,
                                                           RankByTargetMap &ranksByTarget)
{
    if (pBaseMetadata == NULL) {
        checkAndLogMsg ("MetadataGenerator::addPreviousMessageValueInternal",
                        Logger::L_Warning, "original metadata %s is NULL.\n" ,
                        pszBaseMetadataMsgId);
        return NULL;
    }

    if (prevMsgIds.isEmpty()) {
        pBaseMetadata->resetFieldValue (MetadataInterface::PREV_MSG_ID);
    }
    else {
        // setFieldValue() makes a copy of prevMsgIds
        pBaseMetadata->setFieldValue (MetadataInterface::PREV_MSG_ID, prevMsgIds);
    }

    pBaseMetadata->resetFieldValue (MetadataInterface::RANKS_BY_TARGET_NODE_ID);
    String sRanksByTarget (ranksByTarget.toString());
    pBaseMetadata->setFieldValue (MetadataInterface::RANKS_BY_TARGET_NODE_ID, sRanksByTarget);

    pBaseMetadata->resetFieldValue (MetadataInterface::MESSAGE_ID);

    const String sSubGrpName (MessageIdGenerator::extractSubgroupFromMsgId (pszBaseMetadataMsgId));
    if (sSubGrpName.length() <= 0) {
        return NULL;
    }

    char *ppszId = NULL;
    // addAnnotation modifies pBaseMetadata - no problem here, pBaseMetadata is
    // a disposable copy 
    if (_pDSPro->addAnnotationNoPrestage (sSubGrpName,
                                          NULL,  // no object id for DSPro-generated metadata
                                          NULL,  // no instance id for DSPro-generated metadata
                                          pBaseMetadata,
                                          NULL,  // no need to set the referred object id, it
                                                 // it was set by the cloaning function
                                          0, &ppszId) < 0) {
        return NULL;
    }
    return ppszId;
}

