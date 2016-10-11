/**
 * Services.cpp
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on February 29, 2016, 11:45 PM
 */

#include "DSProServices.h"

#include "CommAdaptorManager.h"
#include "DSProImpl.h"
#include "Topology.h"

using namespace IHMC_ACI;

// ------------------------------------------------------------------------
// TopologySvc
// ------------------------------------------------------------------------

TopologySvc::TopologySvc (DSProImpl *pDSPro)
    : _pDSPro (pDSPro)
{
}

TopologySvc::~TopologySvc (void)
{
}

Topology * TopologySvc::getTopology (void)
{
    return _pDSPro->getTopology();
}

// ------------------------------------------------------------------------
// MessagingSvc
// ------------------------------------------------------------------------

MessagingSvc::MessagingSvc (DSProImpl *pDSPro)
    : _pDSPro (pDSPro)
{
}

MessagingSvc::~MessagingSvc (void)
{
}

int MessagingSvc::addRequestedMessageToUserRequests (const char *pszId, const char *pszQueryId)
{
    return _pDSPro->addRequestedMessageToUserRequests (pszId, pszQueryId);
}

int MessagingSvc::sendAsynchronousRequestMessage (const char *pszId)
{
    return _pDSPro->sendAsynchronousRequestMessage (pszId);
}

int MessagingSvc::removeAsynchronousRequestMessage (const char *pszId)
{
    return _pDSPro->removeAsynchronousRequestMessage (pszId);
}

int MessagingSvc::sendSearchMessage (SearchProperties &searchProp, Targets **ppTargets)
{
    return _pDSPro->getCommAdaptorManager()->sendSearchMessage (searchProp, ppTargets);
}

int MessagingSvc::sendSearchReplyMessage (const char *pszQueryId, const char **ppszMatchingMsgIds,
    const char *pszTarget, const char *pszMatchingNode,
    Targets **ppTargets)
{
    return _pDSPro->getCommAdaptorManager()->sendSearchReplyMessage (pszQueryId, ppszMatchingMsgIds,
                                                                     pszTarget, pszMatchingNode, ppTargets);
}

int MessagingSvc::sendSearchReplyMessage (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen,
    const char *pszTarget, const char *pszMatchingNode,
    Targets **ppTargets)
{
    return _pDSPro->getCommAdaptorManager()->sendSearchReplyMessage (pszQueryId, pReply,
                                                                     ui16ReplyLen, pszTarget,
                                                                     pszMatchingNode, ppTargets);
}

// ------------------------------------------------------------------------
// ApplicationNotificationSvc

// ------------------------------------------------------------------------
ApplicationNotificationSvc::ApplicationNotificationSvc (DSProImpl *pDSPro)
    : _pDSPro (pDSPro)
{
}

ApplicationNotificationSvc::~ApplicationNotificationSvc (void)
{
}

void ApplicationNotificationSvc::asynchronouslyNotifyMatchingMetadata (const char *pszQueryId, const char **ppszMsgIds)
{
    return _pDSPro->asynchronouslyNotifyMatchingMetadata (pszQueryId, ppszMsgIds);
}

void ApplicationNotificationSvc::asynchronouslyNotifyMatchingSearch (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen)
{
    return _pDSPro->asynchronouslyNotifyMatchingSearch (pszQueryId, pReply, ui16ReplyLen);
}

