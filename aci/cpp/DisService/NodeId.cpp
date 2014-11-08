/* 
 * NodeId.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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
 * NodeInfo stores state information of the local node.
 * RemoteNodeInfo stores state information of a remote node.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on September 18, 2013, 4:52 PM
 */

#include "NodeId.h"

#include "DisServiceDefs.h"
#include "PropertyStoreInterface.h"

#include "Base64Transcoders.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "NetUtils.h"
#include "NLFLib.h"

#include "UUID.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const NOMADSUtil::String NodeId::NODE_SUFFIX_ATTRIBUTE = "NodeIdSuffix";
const NOMADSUtil::String NodeId::SESSION_KEY_SEPARATOR = "/";

NodeId::NodeId (PropertyStoreInterface *pPropStoreIface)
    : _pPropStore (pPropStoreIface)
{
}

NodeId::~NodeId()
{
}

const char * NodeId::generateNodeId (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return NULL;
    }
    _m.lock();

    bool bNodeIdGenerated = false;
    if (pCfgMgr->hasValue ("aci.disService.nodeUUID")) {
        _nodeId = pCfgMgr->getValue ("aci.disService.nodeUUID");
    }
    else if (strcmp ("hostname", pCfgMgr->getValue ("aci.disService.nodeUUID.auto.mode", "UUID")) == 0) {
        _nodeId = NetUtils::getLocalHostName();
    }
    else {
        NOMADSUtil::UUID uuid;
        uuid.generate();
        _nodeId = uuid.getAsString();
        bNodeIdGenerated = true;
    }

    if (!bNodeIdGenerated && pCfgMgr->getValueAsBool ("aci.disService.nodeUUID.randomize", false)) {
        generateSuffixedNodeId();
    }

    _m.unlock();
    return _nodeId.c_str();
}

String NodeId::getNodeId (void)
{
    _m.lock();
    String tmp (_nodeId);
    _m.unlock();
    return tmp;
}

void NodeId::generateSuffixedNodeId()
{
    return generateSuffixedNodeId (getSuffix());
}

void NodeId::generateSuffixedNodeId (const char *pszSuffix)
{
    String idSuffix (pszSuffix);
    if (idSuffix.c_str() != NULL) {
        _nodeId += "-";
        _nodeId += idSuffix;
        checkAndLogMsg ("NodeId::generateSuffixedNodeId", Logger::L_Info,
                        "node id changed to <%s>\n", _nodeId.c_str());
    }
}

String NodeId::getSuffix (void)
{
    // Get old suffix
    String idSuffix (_pPropStore->get (_nodeId, NODE_SUFFIX_ATTRIBUTE));
    if (idSuffix.length() > 0) {
        // return old suffix, if it exists
        return idSuffix;
    }
    // Other wise generate new one
    return generateSuffix();
}

String NodeId::generateSuffix (void)
{
    const char *pszMethodName = "NodeId::generateSuffix";

    String idSuffix;
    uint32 ui32RandomValue;
    if (getRandomBytes (&ui32RandomValue, sizeof (ui32RandomValue)) != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "getRandomBytes() failed\n");
        return idSuffix;
    }

    // Encode the random number, and strip of any trailing '=' characters (which is padding)
    char *pszEncodedValue = Base64Transcoders::encode (&ui32RandomValue, sizeof (ui32RandomValue));
    if (pszEncodedValue == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to base64 encode the randomly generated value\n");
        return idSuffix;
    }

    char *pszTemp = strchr (pszEncodedValue, '=');
    if (pszTemp != NULL) {
        *pszTemp = '\0';
    }
    /*!!*/ // Comment-out the following to force the node to randomize the UUID upon each restart
           // This change was made at TS13 to handle what appeared to be Database Corruption of the SQLite Database
           // which resulted in the sequence numbers starting over, but with the same node id
    _pPropStore->set (_nodeId, NODE_SUFFIX_ATTRIBUTE, pszEncodedValue);
    idSuffix = pszEncodedValue;
    free (pszEncodedValue);

    return idSuffix;
}

