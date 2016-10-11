/*
 * DSProCmdProcessor.cpp
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
 * Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on April 17, 2014, 1:21 AM
 */

#include "DSProCmdProcessor.h"

#include "DataStore.h"
#include "DSPro.h"
#include "DSProImpl.h"
#include "Instrumentator.h"
#include "MessageId.h"
#include "NodePath.h"
#include "Topology.h"
#include "SQLAVList.h"

#include "PropertyStoreInterface.h"

#include "MimeUtils.h"

#include "FileWriter.h"
#include "StringTokenizer.h"
#include "Timestamp.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;
using namespace IHMC_MISC;

#define log if (pCmdProcLog != NULL) pCmdProcLog->logMsg
static const Logger::Level INFO = Logger::L_Info;

namespace IHMC_ACI
{
    Logger *pCmdProcLog;
}

namespace IHMC_ACI
{
    void storeData (const char *pszId, const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks, const char *pszMIMIType)
    {
        Timestamp tstamp;
        String filename (tstamp.toString());
        filename += '-';
        filename += pszId;
        filename += '-';
        filename += static_cast<uint32>(ui8NChunks);
        filename += '_';
        filename += static_cast<uint32>(ui8TotNChunks);
        filename += '.';
        const String extension (MimeUtils::toExtesion (MimeUtils::mimeTypeToFragmentType (pszMIMIType)));
        filename += (extension.length() > 0 ? extension : ".dat");
        const int iFileNameLen = filename.length();
        char *pszFileName = filename.r_str();
        for (int i = 0; i < iFileNameLen; i++) {
            if ((pszFileName[i] == ':') ||(pszFileName[i] == '[') || (pszFileName[i] == ']')) {
                pszFileName[i] = '-';
            }
        }
        filename = pszFileName;
        free (pszFileName);
        pszFileName = NULL;
        FileWriter fw (filename, "wb");
        if (fw.writeBytes (pBuf, ui32Len) < 0) {
            log ("[DATA ARRIVED]", INFO, "could not save data into %s.\n", filename.c_str());
        }
        else {
            log ("[DATA ARRIVED]", INFO, "data saved into %s.\n", filename.c_str());
        }
    }
}

DSProCmdProcessor::DSProCmdProcessor (DSPro *pDSPro)
    : SearchListener ("DSProCmdProcessor"),
      _pDSPro (pDSPro),
      _bStoreIncomingData (true)
{
}

DSProCmdProcessor::~DSProCmdProcessor (void)
{
    _pDSPro = NULL;
}

bool DSProCmdProcessor::pathRegistered (NodePath *pNodePath, const char *pszNodeId, const char *pszTeam, const char *pszMission)
{
    log ("[PATH REGISTERED]", Logger::L_Info, "peer %s register path %s.\n",
         pszNodeId, pNodePath->getPathID());
    return true;
}

bool DSProCmdProcessor::positionUpdated (float fLatitude, float fLongitude, float fAltitude, const char *pszNodeId)
{
    log ("[POSITION UPDATED]\n", INFO, "peer %s moved to %f %f %f.\n",
         pszNodeId, fLatitude, fLongitude, fAltitude);
    return true;
}

void DSProCmdProcessor::newPeer (const char* pszPeerNodeId)
{
    log ("[NEW NEIGHBOR]", INFO, " %s\n", pszPeerNodeId);
}

void DSProCmdProcessor::deadPeer (const char* pszPeerNodeId)
{
    log ("[DEAD NEIGHBOR]", INFO, " %s\n", pszPeerNodeId);
}

int DSProCmdProcessor::dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                                    const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                                    const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks, const char *pszQueryId)
{
    MessageId messageId (pszId);
    messageId.setChunkId (ui8NChunks);

    log ("[DATA ARRIVED]", INFO, "<%s> query id <%s>\n", messageId.getId(), pszQueryId);
    if (static_cast<const bool>(_bStoreIncomingData)) {
        storeData (pszId, pBuf, ui32Len, ui8NChunks, ui8TotNChunks, pszMimeType);
    }
    return 0;
}

int DSProCmdProcessor::metadataArrived (const char *pszId, const char *pszGroupName, const char *pszReferredDataObjectId,
                                        const char *pszReferredDataInstanceId, const char *pszXMLMetadada, const char *pszReferredDataId,
                                        const char *pszQueryId)
{
    log ("[METADATA ARRIVED]", INFO, "<%s> referring to <%s> query id <%s>:\n%s\n",
        pszId, pszReferredDataId, pszQueryId, pszXMLMetadada);
    return 0;
}

void DSProCmdProcessor::searchArrived (const char  *pszQueryId, const char *pszGroupName, const char *pszQuerier,
                                       const char *pszQueryType, const char *pszQueryQualifiers, const void *pszQuery,
                                       unsigned uiQueryLen)
{
    log ("[SEARCH ARRIVED]", INFO, "query id <%s> querier <%s> query type <%s>\n",
         pszQueryId, pszQuerier, pszQueryType);
}

void DSProCmdProcessor::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId)
{
    log ("[SEARCH REPLY ARRIVED]", INFO, "query id <%s> matching node <%s>\n",
         pszQueryId, pszMatchingNodeId);
}

void DSProCmdProcessor::volatileSearchReplyArrived (const char *pszQueryId, const void *pReply,
                                                    uint16 ui162ReplyLen, const char *pszMatchingNodeId)
{
    const String reply (static_cast<const char *>(pReply), ui162ReplyLen);
    log ("[VOLATILE SEARCH REPLY ARRIVED]", INFO, "query id <%s> matching node <%s>. Reply: <%s>\n",
         pszQueryId, pszMatchingNodeId, reply.c_str());
}

int DSProCmdProcessor::processCmd (const void *pToken, char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    const char *pszCmd = st.getNextToken();
    String cmd (pszCmd);
    cmd.trim();
    cmd.convertToLowerCase();
    if (cmd.length() <= 0) {
        // Should not happen, but return 0 anyways
        return 0;
    }
    else if (cmd == "help") {
        const char *pszHelpCmd = st.getNextToken();
        if (pszHelpCmd == NULL) {
            displayGeneralHelpMsg (pToken);
        }
        else {
            displayHelpMsgForCmd (pToken, pszHelpCmd);
        }
    }
    else if (cmd ^= "getdata") {
        handleGetDataCmd (pToken, pszCmdLine);
    }
    else if ((cmd ^= "getdsproids") || (cmd ^= "getids")) {
        handleGetSProIdsCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "getpeers") {
        handlePeersCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "adduserid") {
        handleAddUserIdCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "requestmorechunks") {
        handleRequestMoreChunks (pToken, pszCmdLine);
    }
    else if (cmd ^= "gendata") {
        handleGenDataCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "getdisdervice") {
        handleGetDisServiceCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "prop") {
        handlePropCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "screenOutput") {
        handleScreenOutputCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "setloglevel") {
		handleSetLogLevel (pToken, pszCmdLine);
	}
    else if (cmd ^= "search") {
        handleSearch (pToken, pszCmdLine);
    }
    else if (cmd ^= "searchreply") {
        handleSearchReply (pToken, pszCmdLine);
    }
    else if ((cmd ^= "volatilesearchreply") || (cmd ^= "vsearchreply")) {
        handleVolatileSearchReply (pToken, pszCmdLine);
    }
    else if ((cmd ^= "storedata") || (cmd ^= "storeincomingdata")) {
        handleStoreIncomingData (pToken, pszCmdLine);
    }
    else if ((cmd ^= "exit") || (cmd ^= "quit")) {
        return -1;
    }
    else {
        print (pToken, "unknown command - type help for a list of valid commands\n");
    }
    return 0;
}

void DSProCmdProcessor::displayGeneralHelpMsg (const void *pToken)
{
    print (pToken, "The following commands are available\n");
    print (pToken, "Type help <cmd> for more help on a particular command\n");
    print (pToken, "    exit - terminate program\n");
    print (pToken, "    db - various database operations\n");
    print (pToken, "    prop - operations related to properties\n");
    print (pToken, "    hasFragment - check to see whether a fragment is present\n");
    print (pToken, "    screenOutput - enable or disable screen output of log messsages\n");
	print (pToken, "    setLogLevel - set the minimum log level for a messafe to be logged\n");
    print (pToken, "    quit - terminate program\n");
}

void DSProCmdProcessor::displayHelpMsgForCmd (const void *pToken, const char *pszCmdLine)
{
    
}

void DSProCmdProcessor::handleGetDataCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszReferredMsgId = st.getNextToken();
    const char *pszSearchId = st.getNextToken();
    const char *pszExtension = NULL;
    String searchId;
    String mimeType;
    if (pszSearchId != NULL) {
        if (!(searchId ^= "none") && !(searchId ^= "null") && !(searchId ^= "no")) {
            searchId = pszSearchId;
        }
        pszExtension = st.getNextToken();
        if (pszExtension != NULL) {
            mimeType = MimeUtils::toMimeType (MimeUtils::toType (pszExtension));
        }
    }
    void *pData = NULL;
    uint32 ui32DataLen = 0U;
    bool bHasMoreChunks = false;
    _pDSPro->getData (pszReferredMsgId, searchId, &pData, ui32DataLen, bHasMoreChunks);
    if (static_cast<const bool>(_bStoreIncomingData)) {
        storeData (pszReferredMsgId, pData, ui32DataLen, 1, (bHasMoreChunks ? 2 : 1), mimeType);
    }
    if (pData != NULL) {
        free (pData);
    }
}

void DSProCmdProcessor::handleGetSProIdsCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszObjectId = st.getNextToken();
    if (pszObjectId == NULL) {
        print (pToken, "Could not retireve the ids: need object id and (optionaly) instance id.\n");
        return;
    }
    const char *pszInstanceId = st.getNextToken();
    char **ppszIds = _pDSPro->getDSProIds (pszObjectId, pszInstanceId);
    if ((ppszIds == NULL) || (ppszIds[0] == NULL)) {
        print (pToken, "No Id was found for objectId <%s> instanceId <%s>.\n",
               pszObjectId, pszInstanceId);
    }
    else {
        String ids;
        for (int i = 0; ppszIds[i] != NULL; i++) {
            if (ids.length() > 0) {
                ids += ", ";
            }
            ids += ppszIds[i];
        }
        print (pToken, "PSPro IDs for objectId <%s> instanceId <%s>:\n",
               pszObjectId, pszInstanceId, ids.c_str());
    }
}

void DSProCmdProcessor::handlePeersCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    char **ppszPeers = _pDSPro->getPeerList();
    print (pToken, "Peer List:\n");
    if (ppszPeers != NULL) {
        for (unsigned int i = 0; ppszPeers[i] != NULL; i++) {
            print (pToken, "%s\n", ppszPeers[i]);
            free (ppszPeers[i]);
            ppszPeers[i] = NULL;
        }
        free (ppszPeers);
        ppszPeers = NULL;
    }
}

void DSProCmdProcessor::handleAddUserIdCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszUserName = st.getNextToken();
    if (_pDSPro->addUserId (pszUserName) == 0) {
        print (pToken, "Added User Name %s.\n", pszUserName);
    }
    else {
        print (pToken, "Could not add User Name %s.\n", pszUserName);
    }
}

void DSProCmdProcessor::handleRequestMoreChunks (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszLargeObjecId = st.getNextToken();
    const char *pszMIMEType = st.getNextToken();
    DArray<uint64> dimensions (0U, 0U);
    const int64 i64Timeout = 0;
    if (pszMIMEType != NULL) {
        const char *pszPixel;
        for (unsigned int i = 0; (pszPixel = st.getNextToken()) != NULL; i++) {
            dimensions[i] = atoui64 (pszPixel);
        }
    }
    int rc;
    switch (dimensions.size()) {
        case 0:
            rc = _pDSPro->requestMoreChunks (pszLargeObjecId, NULL);
            break;

        case 2:
            rc = _pDSPro->requestCustomTimeChunk (pszLargeObjecId, pszMIMEType, (int64) dimensions[0], (int64) dimensions[1],
                                                  (uint8) 100, i64Timeout);
            break;

        case 4:
            rc = _pDSPro->requestCustomAreaChunk (pszLargeObjecId, pszMIMEType, (uint32) dimensions[0], (uint32) dimensions[1],
                                                  (uint32) dimensions[2], (uint32) dimensions[3], (uint8) 100, i64Timeout);
            break;

        default:
            print (pToken, "Could not request more chunks: argument parsing error.\n");
            rc = -1;
    }
    if (rc == 0) {
        print (pToken, "Requested more chunks for message %s.\n", pszLargeObjecId);
    }
    else {
        print (pToken, "Could not request more chunks for message %s.\n", pszLargeObjecId);
    }
}

void DSProCmdProcessor::handleGenDataCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszToken = st.getNextToken();
    uint32 ui32DataLen = (pszToken == NULL ? 1024 : atoui32 (pszToken));
    SQLAVList metadataList;
    metadataList.addPair (MetadataInterface::APPLICATION_METADATA, "phony application metadata");
    void *pData = NULL;
    char *pszId = NULL;
    if (ui32DataLen > 0) {
        pData = malloc (ui32DataLen);
    }
    static const char *pszObjectId = "rndoi";
    static int iInstanceId = 0;
    iInstanceId++;
    char chInstanceId[12];
    itoa (chInstanceId, iInstanceId);
    if ((_pDSPro->addMessage ("grp", pszObjectId, chInstanceId, &metadataList, pData, ui32DataLen, 0, &pszId) == 0) && (pszId != NULL)) {
        print (pToken, "Added message %s. ObjectId: <%s> InstanceId <%s>.\n", pszId, pszObjectId, chInstanceId);
    }
    else {
        print (pToken, "Could not add message.\n");
    }
    if (pData != NULL) {
        free (pData);
    }
}

void DSProCmdProcessor::handleGetDisServiceCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    if (_pDSPro->getDisService() == NULL) {
        print (pToken, "Started DisService Proxy Server.\n");
    }
    else {
        print (pToken, "DisService Proxy Server could not be started.\n");
    }
}

void DSProCmdProcessor::handlePropCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszSubCmd = st.getNextToken();
    if (pszSubCmd == NULL) {
        print (pToken, "invalid params - try help prop\n");
        return;
    }
    if (0 == stricmp (pszSubCmd, "put")) {
        const char *pszNodeID = st.getNextToken();
        const char *pszAttr = st.getNextToken();
        const char *pszValue = st.getNextToken();
        if ((pszNodeID == NULL) || (pszAttr == NULL) || (pszValue == NULL)) {
            print (pToken, "invalid params - try help prop\n");
            return;
        }
        PropertyStoreInterface *pPropStore = _pDSPro->_pImpl->getDataStore()->getPropertyStore();
        if (pPropStore == NULL) {
            print (pToken, "internal error\n");
            return;
        }
        if (pPropStore->set (pszNodeID, pszAttr, pszValue)) {
            print (pToken, "failed to store property\n");
            return;
        }
        print (pToken, "Ok\n");
    }
    else if (0 == stricmp (pszSubCmd, "get")) {
        const char *pszNodeID = st.getNextToken();
        const char *pszAttr = st.getNextToken();
        if ((pszNodeID == NULL) || (pszAttr == NULL)) {
            print (pToken, "invalid params - try help prop\n");
            return;
        }
        PropertyStoreInterface *pPropStore = _pDSPro->_pImpl->getDataStore()->getPropertyStore();
        if (pPropStore == NULL) {
            print (pToken, "internal error\n");
            return;
        }
        String value = pPropStore->get (pszNodeID, pszAttr);
        if (value.c_str() == NULL) {
            print (pToken, "attribute not found\n");
            return;
        }
        print (pToken, "value = <%s>\n", value.c_str());
    }
    else if (0 == stricmp (pszSubCmd, "remove")) {
        const char *pszNodeID = st.getNextToken();
        const char *pszAttr = st.getNextToken();
        if ((pszNodeID == NULL) || (pszAttr == NULL)) {
            print (pToken, "invalid params - try help prop\n");
            return;
        }
        PropertyStoreInterface *pPropStore = _pDSPro->_pImpl->getDataStore()->getPropertyStore ();
        if (pPropStore == NULL) {
            print (pToken, "internal error\n");
            return;
        }
        if (pPropStore->remove (pszNodeID, pszAttr)) {
            print (pToken, "attribute not found\n");
            return;
        }
        print (pToken, "ok\n");
    }
    else {
        print (pToken, "invalid subcommand <%s> - type help prop\n", pszSubCmd);
    }
}

void DSProCmdProcessor::handleScreenOutputCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszArg = st.getNextToken ();
    if (pszArg == NULL) {
        print (pToken, "invalid params - try help screenOutput\n");
        return;
    }
    else if ((0 == stricmp (pszArg, "true")) || (0 == stricmp (pszArg, "on")) || (0 == stricmp (pszArg, "enable"))) {
        if (pLogger != NULL) pLogger->enableScreenOutput();
        if (pNetLog != NULL) pLogger->enableScreenOutput();
        if (pTopoLog != NULL) pLogger->enableScreenOutput();
    }
    else if ((0 == stricmp (pszArg, "false")) || (0 == stricmp (pszArg, "off")) || (0 == stricmp (pszArg, "disable"))) {
        if (pLogger != NULL) pLogger->disableScreenOutput();
        if (pNetLog != NULL) pLogger->disableScreenOutput();
        if (pTopoLog != NULL) pLogger->disableScreenOutput();
    }
    else {
        print (pToken, "invalid params - try help screenOutput\n");
    }
}

void DSProCmdProcessor::handleSetLogLevel (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszArg = st.getNextToken();
    if (pszArg == NULL) {
        print (pToken, "invalid params - try help screenOutput\n");
        return;
    }
    if (pLogger != NULL) {
        if (pLogger->setDebugLevel (pszArg) == 0) {
            print (pToken, "log level set to %d.\n", pLogger->getDebugLevel());
        }
        else {
            print (pToken, "invalid log level %s.\n", pszArg);
        }
    }
}

void DSProCmdProcessor::handleSearch (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken ();  // This is the command itself - discard
    String queryType ("dspro-application-query");
    String grpName ("cmdProc");
    String qualifiers ("");
    String query = "phony";
    int64 i64Timeout = 0;
    int i = 0;
    for (const char *pszArg; (pszArg = st.getNextToken()) != NULL; i++) {
        switch (i) {
            case 0: queryType = pszArg; break;
            case 1: grpName = pszArg; break;
            case 2: qualifiers = pszArg; break;
            case 3: query = pszArg; break;
            case 4: i64Timeout = atoi64 (pszArg); break;
            default:
                print (pToken, "invalid params - try help search\n");
                return;
        }
    }
    if (i > 4) {
        print (pToken, "invalid params - try help search\n");
    }

    char *pszQueryId = NULL;
    int rc = _pDSPro->search (grpName, queryType, qualifiers, query.c_str(), query.length(), i64Timeout, &pszQueryId);
    if (0 == rc) {
        print (pToken, "Search successful. Search id: %s.\n", pszQueryId);
    }
    else {
        print (pToken, "Search failed. Returned code: %d\n", rc);
    }
}

void DSProCmdProcessor::handleSearchReply (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszQueryId = st.getNextToken();
    const char *pIDs[64];
    pIDs[0] = st.getNextToken();
    if ((pszQueryId == NULL) || (pIDs[0] == NULL)) {
        print (pToken, "invalid params - try help searchReply\n");
    }
    unsigned int i = 1;
    for (const char *pszMessageId; (i < 64) && ((pszMessageId = st.getNextToken ()) != NULL); i++) {
        pIDs[i] = pszMessageId;
    }
    pIDs[i] = NULL;
    int rc = _pDSPro->searchReply (pszQueryId, pIDs);
    if (0 == rc) {
        print (pToken, "Search reply successful.\n");
    }
    else {
        print (pToken, "Search reply failed. Returned code: %d\n", rc);
    }
}

void DSProCmdProcessor::handleVolatileSearchReply (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszQueryId = st.getNextToken();
    String reply (st.getNextToken());
    int rc = _pDSPro->volatileSearchReply (pszQueryId, reply.c_str(), reply.length());
    if (0 == rc) {
        print (pToken, "Search reply successful.\n");
    }
    else {
        print (pToken, "Search reply failed. Returned code: %d\n", rc);
    }
}

void DSProCmdProcessor::handleStoreIncomingData (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken ();  // This is the command itself - discard
    const char *pszArg = st.getNextToken();
    String token (pszArg);
    token.convertToLowerCase();
    if ((pszArg == NULL) || (token == "enable") || (token == "on") || (token == "yes") || (token == "1")) {
        _bStoreIncomingData = true;
    }
    else if ((token == "disable") || (token == "off") || (token == "no") || (token == "0")) {
        _bStoreIncomingData = false;
    }
    else {
        print (pToken, "invalid params - try help storeData\n");
    }
}

