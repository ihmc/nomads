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

#include "AMTDictator.h"
#include "DataStore.h"
#include "DSProInterface.h"
#include "DSPro.h"
#include "DSProImpl.h"
#include "Instrumentator.h"
#include "MessageId.h"
#include "MetadataInterface.h"
#include "NodePath.h"
#include "Topology.h"
#include "AVList.h"

#include "PropertyStoreInterface.h"

#include "MimeUtils.h"

#include "Base64Transcoders.h"
#include "FileReader.h"
#include "FileUtils.h"
#include "FileWriter.h"
#include "NLFLib.h"
#include "StringTokenizer.h"
#include "Timestamp.h"
#include "LineOrientedReader.h"
#include "ConfigManager.h"
#include "Json.h"

#ifdef UNIX
    #include "limits.h"
#endif

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;
using namespace IHMC_MISC;

#define log if (pCmdProcLog != nullptr) pCmdProcLog->logMsg
static const Logger::Level INFO = Logger::L_Info;

namespace IHMC_ACI
{
    Logger *pCmdProcLog;
}

namespace DSPRO_CMD_PROCESSOR
{
    bool isInteger (const char *pszToken)
    {
        if (pszToken == nullptr) {
            return false;
        }
        int iLen = strlen (pszToken);
        for (int i = 0; i < iLen; i++) {
            if (!isdigit (pszToken[i])) {
                return false;
            }
        }
        return  true;
    }

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
        pszFileName = nullptr;
        FileWriter fw (filename, "wb");
        if (fw.writeBytes (pBuf, ui32Len) < 0) {
            log ("[DATA ARRIVED]", INFO, "could not save data into %s.\n", filename.c_str());
        }
        else {
            log ("[DATA ARRIVED]", INFO, "data saved into %s.\n", filename.c_str());
        }
    }

    bool atob (const char *pszToken, bool bDefault)
    {
        if (pszToken == nullptr) {
            return bDefault;
        }
        const String token (pszToken);
        return (token ^= "true") || (token == "enable") || (token == "enabled") || (token == "on") || (token == "yes") || (token == "1");
    }
}

using namespace DSPRO_CMD_PROCESSOR;

DSProCmdProcessor::DSProCmdProcessor (DSPro *pDSPro)
    : DSProCmdProcessor (pDSPro, pDSPro->_pImpl)
{
}

DSProCmdProcessor::DSProCmdProcessor (DSProInterface *pDSPro, DSProImpl *pImpl)
    : SearchListener ("DSProCmdProcessor"),
      _pDSPro (pDSPro),
      _pDSProImpl (pImpl),
      _pAMTDictator (new AMTDictator (_pDSProImpl)),
      _bStoreIncomingData (true)
{
}

DSProCmdProcessor::~DSProCmdProcessor (void)
{
    _pDSPro = nullptr;
    delete _pAMTDictator;
}

bool DSProCmdProcessor::pathRegistered (NodePath *pNodePath, const char *pszNodeId, const char *pszTeam, const char *pszMission)
{
    String nodeId (pNodePath->getPathId());
    log ("[PATH REGISTERED]", Logger::L_Info, "peer %s register path %s.\n",
         pszNodeId, nodeId.c_str());
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
                                    const void *pBuf, uint32 ui32Len, uint8 ui8ChunkIndex, uint8 ui8TotNChunks, const char *pszQueryId)
{
    MessageId messageId (pszId);
    messageId.setChunkId (ui8ChunkIndex);

    log ("[DATA ARRIVED]", INFO, "<%s> query id <%s>\n", messageId.getId(), pszQueryId);
    if (static_cast<bool>(_bStoreIncomingData)) {
        storeData (pszId, pBuf, ui32Len, ui8ChunkIndex, ui8TotNChunks, pszMimeType);
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

int  DSProCmdProcessor::dataAvailable (const char *pszId, const char *pszGroupName, const char *pszReferredDataObjectId, const char *pszReferredDataInstanceId,
                                       const char *pszReferredDataId, const char *pszMimeType, const void *pMetadata, uint32 ui32MetadataLength,
                                       const char *pszQueryId)
{
    unsigned short usLen = minimum ((uint32) USHRT_MAX, ui32MetadataLength);
    String metadata ((const char *)pMetadata, usLen);
    log ("[DATA AVAILABLE]", INFO, "<%s> referring to <%s> query id <%s>:\n%s\n",
         pszId, pszReferredDataId, pszQueryId, metadata.c_str());
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
    if (cmd == "help") {
        const char *pszHelpCmd = st.getNextToken();
        if (pszHelpCmd == nullptr) {
            displayGeneralHelpMsg (pToken);
        }
        else {
            displayHelpMsgForCmd (pToken, pszHelpCmd);
        }
    }
    else if (cmd ^= "getdata") {
        handleGetDataCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "getsessionid") {
        handleGetSessionId (pToken, pszCmdLine);
    }
    else if ((cmd ^= "getdsproids") || (cmd ^= "getids")) {
        handleGetSProIdsCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "cancelByObjId") {
        handleCancelByObjectAndInstanceId (pToken, pszCmdLine);
    }
    else if (cmd ^= "configureranks") {
        handleConfigureRanksCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "getpeers") {
        handlePeersCmd (pToken, pszCmdLine);
    }
    else if ((cmd ^= "getnodecontext") || (cmd ^= "getnodectxt")) {
        handeGetNodeContext (pToken, pszCmdLine);
    }
    else if (cmd ^= "adduserid") {
        handleAddUserIdCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "addareaofinterest") {
        handleAddAreaOfInterestCmd (pToken, pszCmdLine);
    }
    else if (cmd ^= "addcustompolicy") {
        handleAddCustomPolicy (pToken, pszCmdLine);
    }
    else if (cmd ^= "requestmorechunks") {
        handleRequestMoreChunks (pToken, pszCmdLine);
    }
    else if ((cmd ^= "addmessage") || (cmd ^= "addmsg")) {
        handleAddMessageCmd (pToken, pszCmdLine);
    }
    else if ((cmd ^= "disseminatemessage") || (cmd ^= "dissmessage") || (cmd ^= "dissmsg") || (cmd ^= "dismsg")) {
        handleDisseminatedMessageCmd (pToken, pszCmdLine);
    }
    else if ((cmd ^= "subscribe") || (cmd ^= "sub")) {
        handleSubscribe (pToken, pszCmdLine);
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
    else if (cmd ^= "setrankingweigths") {
        handleRankingWeigths (pToken, pszCmdLine);
    }
    else if (cmd ^= "setrangeofinfluence") {
        handleSetRangeOfInfluence (pToken, pszCmdLine);
    }
    else if (cmd ^= "setusefuldistance") {
        handleSetUsefulDistance (pToken, pszCmdLine);
    }
    else if (cmd ^= "setselectivity") {
        handleSetSelectivity (pToken, pszCmdLine);
    }
    else if ((cmd ^= "storedata") || (cmd ^= "storeincomingdata")) {
        handleStoreIncomingData (pToken, pszCmdLine);
    }
    else if ((cmd ^= "registerpath") || (cmd ^= "submitpath") || (cmd ^= "newpath")) {
        handleLoadPath (pToken, pszCmdLine);
    }
    else if (cmd ^= "setcurrentpath") {
        handleSetCurrentPath (pToken, pszCmdLine);
    }
    else if ((cmd ^= "setposition") || (cmd ^= "setpos") || (cmd ^= "setcurrpos")) {
        handeSetPosition (pToken, pszCmdLine);
    }
    else if (cmd ^= "setmission") {
        handleSetMission (pToken, pszCmdLine);
    }
    else if (cmd ^= "setrole") {
        handleSetRole (pToken, pszCmdLine);
    }
    else if ((cmd ^= "generatetrack") || (cmd ^= "gentrack")) {
        handleGenTrack (pToken, pszCmdLine);
    }
    else if (cmd ^= "mist") {
        handleMist (pToken, pszCmdLine);
    }
    else if (cmd ^= "setSessionId") {
        handleSetSessionId (pToken, pszCmdLine);
    }
    else if ((cmd ^= "exit") || (cmd ^= "quit")) {
        return -1;
    }
    else {
        print (pToken, "unknown command <%s> - type help for a list of valid commands\n", cmd.c_str());
    }
    return 0;
}

int DSProCmdProcessor::processCmdFile (const char *pszFileName)
{
    if (!FileUtils::fileExists (pszFileName)) {
        return -1;
    }
    FileReader fr (pszFileName, "r");
    LineOrientedReader lr (&fr, false);
    char buf[1024];
    while (lr.readLine (buf, 1024) > 0) {
        if (buf[0] != '#') {
            // If not a comment
            printf ("%s\n", buf);
            processCmd (nullptr, buf);
        }
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
    const char *pszExtension = nullptr;
    String searchId;
    String mimeType;
    if (pszSearchId != nullptr) {
        if (!(searchId ^= "none") && !(searchId ^= "null") && !(searchId ^= "no")) {
            searchId = pszSearchId;
        }
        pszExtension = st.getNextToken();
        if (pszExtension != nullptr) {
            mimeType = MimeUtils::toMimeType (MimeUtils::toType (pszExtension));
        }
    }
    void *pData = nullptr;
    uint32 ui32DataLen = 0U;
    bool bHasMoreChunks = false;
    _pDSPro->getData (pszReferredMsgId, searchId, &pData, ui32DataLen, bHasMoreChunks);
    if (static_cast<bool>(_bStoreIncomingData)) {
        storeData (pszReferredMsgId, pData, ui32DataLen, 1, (bHasMoreChunks ? 2 : 1), mimeType);
    }
    if (pData != nullptr) {
        free (pData);
    }
}

void DSProCmdProcessor::handleConfigureRanksCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    DArray2<String> tokens;
    const char *pszToken;
    for (unsigned int i = 0; (pszToken = st.getNextToken()) != nullptr; i++) {
        tokens[i] = pszToken;
    }
    if (tokens.size() > 10) {
        return;
    }
    float weights[8];
    for (unsigned int i = 0; i < 8; i++) {
        weights[i] = 0.0f;
    }
    unsigned int i = 0;
    for (; i < (unsigned int) minimum ((uint8) 8, (uint8) tokens.size()); i++) {
        if ((tokens[i] != "true") || (tokens[i] != "false")) {
            weights[i] = atof (tokens[i]);
        }
        else {
            break;
        }
    }
    bool bStrictTarget = true;
    bool bConsiderFuturePathSegmentForMatchmacking = false;
    for (unsigned j = 0; (j + i) < tokens.size(); j++) {
        bool bVal = (1 == (tokens[i+j] ^= "true"));
        switch (j)
        {
            case 0:
                bStrictTarget = bVal;
                break;
            case 1:
                bConsiderFuturePathSegmentForMatchmacking = bVal;
                break;
        }
    }
    int rc = _pDSPro->setRankingWeigths (weights[0], weights[1], weights[2], weights[3],
                                         weights[4], weights[7], weights[6], weights[7],
                                         bStrictTarget, bConsiderFuturePathSegmentForMatchmacking);
    if (rc == 0) {
        print (pToken, "weights configured\n");
    }
    else {
        print (pToken, "coldn't configure weights\n");
    }
}

void DSProCmdProcessor::handleGetSessionId (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken ();  // This is the command itself - discard
    String sessionId (_pDSPro->getSessionId());
    if (sessionId.length() > 0) {
        print (pToken, "retrieved sessionId: <%s>\n", sessionId.c_str());
    }
    else {
        print (pToken, "coldn't get sessionId\n");
    }
}

void DSProCmdProcessor::handleGetSProIdsCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszObjectId = st.getNextToken();
    if (pszObjectId == nullptr) {
        print (pToken, "Could not retireve the ids: need object id and (optionaly) instance id.\n");
        return;
    }
    const char *pszInstanceId = st.getNextToken();

    char **ppszIds = _pDSPro->getDSProIds (pszObjectId, pszInstanceId);
    if ((ppszIds == nullptr) || (ppszIds[0] == nullptr)) {
        print (pToken, "No Id was found for objectId <%s> instanceId <%s>.\n",
               pszObjectId, pszInstanceId);
    }
    else {
        String ids;
        for (int i = 0; ppszIds[i] != nullptr; i++) {
            if (ids.length() > 0) {
                ids += ", ";
            }
            ids += ppszIds[i];
        }
        print (pToken, "PSPro IDs for objectId <%s> instanceId <%s>:\n",
               pszObjectId, pszInstanceId, ids.c_str());
    }
}

void DSProCmdProcessor::handleCancelByObjectAndInstanceId (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszObjectId = st.getNextToken();
    if (pszObjectId == nullptr) {
        print (pToken, "Could not canc the ids: need object id and (optionaly) instance id.\n");
        return;
    }
    const char *pszInstanceId = st.getNextToken();

    int rc = _pDSPro->cancelByObjectAndInstanceId (pszObjectId, pszInstanceId);
    if (rc == 0) {
        print (pToken, "Deleted message objectId <%s> instanceId <%s>.\n",
               pszObjectId, (pszInstanceId == nullptr ? "" : pszInstanceId));
    }
    else {
        print (pToken, "Could not delete message objectId <%s> instanceId <%s>.\n",
               pszObjectId, (pszInstanceId == nullptr ? "" : pszInstanceId));
    }
}

void DSProCmdProcessor::handlePeersCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    char **ppszPeers = _pDSPro->getPeerList();
    print (pToken, "Peer List:\n");
    if (ppszPeers != nullptr) {
        for (unsigned int i = 0; ppszPeers[i] != nullptr; i++) {
            print (pToken, "%s\n", ppszPeers[i]);
            free (ppszPeers[i]);
            ppszPeers[i] = nullptr;
        }
        free (ppszPeers);
        ppszPeers = nullptr;
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

void DSProCmdProcessor::handleAddAreaOfInterestCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszAreaName = st.getNextToken();
    if (_pDSPro->addAreaOfInterest (pszAreaName, 60.0, 60.0, 61.0, 62.0, 0, 0x7FFFFFFFFFFFFFFF) == 0) {
        print (pToken, "Added Area of Interest %s.\n", pszAreaName);
    }
    else {
        print (pToken, "Could not add Area of Interest %s.\n", pszAreaName);
    }
}

void DSProCmdProcessor::handleAddCustomPolicy (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszXMLPolicy = st.getNextToken();
    if (pszXMLPolicy == nullptr) {
        pszXMLPolicy = "<?xml version=\"1.0\"?>"
            "<RankerPolicy>"
            "    <Type>Static</Type>"
            "    <Attribute>Affiliation</Attribute>"
            "    <Weight>5.3</Weight>"
            "    <Match>"
            "        <Value>Friendly</Value>"
            "        <Rank>5.0</Rank>"
            "    </Match>"
            "    <Match>"
            "        <Value>Hostile</Value>"
            "        <Rank>9.3</Rank>"
            "    </Match>"
            "</RankerPolicy>";
    }
    const char * policies[2] = { pszXMLPolicy, nullptr };
    if (_pDSPro->addCustumPoliciesAsXML (policies) == 0) {
        print (pToken, "Added User Name %s.\n", pszXMLPolicy);
    }
    else {
        print (pToken, "Could not add User Name %s.\n", pszXMLPolicy);
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
    if (pszMIMEType != nullptr) {
        const char *pszPixel;
        for (unsigned int i = 0; (pszPixel = st.getNextToken()) != nullptr; i++) {
            dimensions[i] = atoui64 (pszPixel);
        }
    }
    int rc;
    switch (dimensions.size()) {
        case 0:
            rc = _pDSPro->requestMoreChunks (pszLargeObjecId, nullptr);
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

void DSProCmdProcessor::handleAddMessageCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    String file (st.getNextToken());
    if ((file.length() <= 0) || !FileUtils::fileExists (file)) {
        if (file.length() <= 0) {
            print (pToken, "file not specified.\n");
        }
        else {
            print (pToken, "file %s not found.\n", file.c_str());
        }
        return;
    }
    int64 iFileSize = 0;
    void *pData = FileUtils::readFile (file, &iFileSize);
    if (pData == nullptr) {
        print (pToken, "ould not allocate buffer for file %s.\n", file.c_str());
        return;
    }

    ConfigManager cfgMgr (':');
    String metadataFile (file);
    metadataFile += ".dpmd";
    if (FileUtils::fileExists (metadataFile)) {
        cfgMgr.init (1024);
        cfgMgr.readConfigFile (metadataFile);
    }
    String objectId;
    String instanceId;
    AVList metadataList;
    StringStringHashtable *pProp = cfgMgr.getProperties();
    if (pProp != nullptr) {
        StringStringHashtable::Iterator iter = pProp->getAllElements();
        for (; !iter.end(); iter.nextElement()) {
            String key (iter.getKey());
            if (key == MetadataInterface::LATITUDE) {
                double lat = atof (iter.getValue ());
                metadataList.addPair (MetadataInterface::LEFT_UPPER_LATITUDE, lat + 0.00001);
                metadataList.addPair (MetadataInterface::RIGHT_LOWER_LATITUDE, lat - 0.00001);
            }
            if (key == MetadataInterface::LONGITUDE) {
                double lon = atof (iter.getValue ());
                metadataList.addPair (MetadataInterface::LEFT_UPPER_LATITUDE, lon - 0.00001);
                metadataList.addPair (MetadataInterface::RIGHT_LOWER_LATITUDE, lon + 0.00001);
            }
            else {
                metadataList.addPair (key, iter.getValue());
            }
        }
    }
    char *pszId = nullptr;
    file.convertToLowerCase();
    int rc;
    if (file.endsWith ("jpeg") || file.endsWith ("jpg")) {
        rc = _pDSPro->chunkAndAddMessage ("grp", objectId, instanceId, &metadataList, pData, (uint32)iFileSize, "image/jpeg", 0, &pszId);
    }
    else {
        rc = _pDSPro->addMessage ("grp", objectId, instanceId, &metadataList, pData, (uint32)iFileSize, 0, &pszId);
    }

    if (rc == 0) {
        print (pToken, "Added message %s. ObjectId: <%s> InstanceId <%s>.\n", pszId, objectId.c_str (), instanceId.c_str());
    }
    else {
        print (pToken, "Could not add message.\n");
    }
    if (pData != nullptr) {
        free (pData);
    }
}

void DSProCmdProcessor::handleDisseminatedMessageCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken ();  // This is the command itself - discard
    String file (st.getNextToken());
    if ((file.length() <= 0) || !FileUtils::fileExists (file)) {
        if (file.length() <= 0) {
            print (pToken, "file not specified.\n");
        }
        else {
            print (pToken, "file %s not found.\n", file.c_str ());
        }
        return;
    }
    int64 iFileSize = 0;
    void *pData = FileUtils::readFile (file, &iFileSize);
    if (pData == nullptr) {
        print (pToken, "ould not allocate buffer for file %s.\n", file.c_str ());
        return;
    }

    String objectId;
    String instanceId;
    AVList metadataList;
    char *pszId = nullptr;
    int rc = _pDSPro->disseminateMessage ("grp", objectId, instanceId, pData, (uint32)iFileSize, 0, &pszId);
    if (rc == 0) {
        print (pToken, "Added message %s. ObjectId: <%s> InstanceId <%s>.\n", pszId, objectId.c_str(), instanceId.c_str());
    }
    else {
        print (pToken, "Could not add message.\n");
    }
    if (pData != nullptr) {
        free (pData);
    }
}

void DSProCmdProcessor::handleSubscribe (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszGroup = st.getNextToken();

    int rc = _pDSPro->subscribe (pszGroup, 0, true, true, false);
    if (rc == 0) {
        print (pToken, "Subscribe group <%s>.\n", pszGroup);
    }
    else {
        print (pToken, "Could not subscribe group <%s>.\n", pszGroup);
    }
}

void DSProCmdProcessor::handleGenDataCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszToken = st.getNextToken();
    uint32 ui32DataLen = (pszToken == nullptr ? 1024 : atoui32 (pszToken));
    AVList metadataList;
    metadataList.addPair (MetadataInterface::APPLICATION_METADATA, "phony application metadata");
    void *pData = nullptr;
    char *pszId = nullptr;
    if (ui32DataLen > 0) {
        pData = malloc (ui32DataLen);
    }
    static const char *pszObjectId = "rndoi";
    static int iInstanceId = 0;
    iInstanceId++;
    char chInstanceId[12];
    itoa (chInstanceId, iInstanceId);
    if ((_pDSPro->addMessage ("grp", pszObjectId, chInstanceId, &metadataList, pData, ui32DataLen, 0, &pszId) == 0) && (pszId != nullptr)) {
        print (pToken, "Added message %s. ObjectId: <%s> InstanceId <%s>.\n", pszId, pszObjectId, chInstanceId);
    }
    else {
        print (pToken, "Could not add message.\n");
    }
    if (pData != nullptr) {
        free (pData);
    }
}

void DSProCmdProcessor::handleGetDisServiceCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    if (_pDSProImpl == nullptr) {
        return;
    }
    /*if (_pDSProImpl->getDisService() == nullptr) {
        print (pToken, "Started DisService Proxy Server.\n");
    }
    else {
        print (pToken, "DisService Proxy Server could not be started.\n");
    }*/
}

void DSProCmdProcessor::handlePropCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszSubCmd = st.getNextToken();
    if (pszSubCmd == nullptr) {
        print (pToken, "invalid params - try help prop\n");
        return;
    }
    if (0 == stricmp (pszSubCmd, "put")) {
        const char *pszNodeID = st.getNextToken();
        const char *pszAttr = st.getNextToken();
        const char *pszValue = st.getNextToken();
        if ((pszNodeID == nullptr) || (pszAttr == nullptr) || (pszValue == nullptr)) {
            print (pToken, "invalid params - try help prop\n");
            return;
        }
        if (_pDSProImpl == nullptr) {
            return;
        }
        PropertyStoreInterface *pPropStore = _pDSProImpl->getDataStore()->getPropertyStore();
        if (pPropStore == nullptr) {
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
        if (_pDSProImpl == nullptr) {
            return;
        }
        if ((pszNodeID == nullptr) || (pszAttr == nullptr)) {
            print (pToken, "invalid params - try help prop\n");
            return;
        }
        PropertyStoreInterface *pPropStore = _pDSProImpl->getDataStore()->getPropertyStore();
        if (pPropStore == nullptr) {
            print (pToken, "internal error\n");
            return;
        }
        String value = pPropStore->get (pszNodeID, pszAttr);
        if (value.c_str() == nullptr) {
            print (pToken, "attribute not found\n");
            return;
        }
        print (pToken, "value = <%s>\n", value.c_str());
    }
    else if (0 == stricmp (pszSubCmd, "remove")) {
        const char *pszNodeID = st.getNextToken();
        const char *pszAttr = st.getNextToken();
        if (_pDSProImpl == nullptr) {
            return;
        }
        if ((pszNodeID == nullptr) || (pszAttr == nullptr)) {
            print (pToken, "invalid params - try help prop\n");
            return;
        }
        PropertyStoreInterface *pPropStore = _pDSProImpl->getDataStore()->getPropertyStore();
        if (pPropStore == nullptr) {
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
    if (pszArg == nullptr) {
        print (pToken, "invalid params - try help screenOutput\n");
        return;
    }
    else if ((0 == stricmp (pszArg, "true")) || (0 == stricmp (pszArg, "on")) || (0 == stricmp (pszArg, "enable"))) {
        if (pLogger != nullptr) pLogger->enableScreenOutput();
        if (pNetLog != nullptr) pLogger->enableScreenOutput();
        if (pTopoLog != nullptr) pLogger->enableScreenOutput();
    }
    else if ((0 == stricmp (pszArg, "false")) || (0 == stricmp (pszArg, "off")) || (0 == stricmp (pszArg, "disable"))) {
        if (pLogger != nullptr) pLogger->disableScreenOutput();
        if (pNetLog != nullptr) pLogger->disableScreenOutput();
        if (pTopoLog != nullptr) pLogger->disableScreenOutput();
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
    if (pszArg == nullptr) {
        print (pToken, "invalid params - try help screenOutput\n");
        return;
    }
    if (pLogger != nullptr) {
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
    for (const char *pszArg; (pszArg = st.getNextToken()) != nullptr; i++) {
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

    char *pszQueryId = nullptr;
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
    if ((pszQueryId == nullptr) || (pIDs[0] == nullptr)) {
        print (pToken, "invalid params - try help searchReply\n");
    }
    unsigned int i = 1;
    for (const char *pszMessageId; (i < 64) && ((pszMessageId = st.getNextToken ()) != nullptr); i++) {
        pIDs[i] = pszMessageId;
    }
    pIDs[i] = nullptr;
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
    st.getNextToken();  // This is the command itself - discard
    const char *pszArg = st.getNextToken();
    String token (pszArg);
    token.convertToLowerCase();
    if (atob (pszArg, true)) {
        _bStoreIncomingData = true;
    }
    else if ((token == "disable") || (token == "off") || (token == "no") || (token == "0")) {
        _bStoreIncomingData = false;
    }
    else {
        print (pToken, "invalid params - try help storeData\n");
    }
}

void DSProCmdProcessor::handleLoadPath (const void *pToken, const char *pszCmdLine)
{
    static const String TEST_PATH_ID = "generated_test_path-";

    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszFileName = st.getNextToken();
    NodePath *pPath = nullptr;
    if (FileUtils::fileExists (pszFileName)) {
        pPath = new NodePath();
        FileReader reader (pszFileName, "r");
        pPath->readDPRFile (&reader);
    }
    else {
        static uint32 ui32GeneratedPaths = 0;
        String pathId (TEST_PATH_ID);
        pathId += ui32GeneratedPaths++;
        pPath = new NodePath (pathId, 1, 1.0f);
        pPath->appendWayPoint (1.0f, -1.0f, 0.0f, "loc", "note", (uint64)12);
        pPath->appendWayPoint (2.0f, -2.0f, 0.0f, "loc", "note", (uint64)14);
        pPath->appendWayPoint (3.0f, -3.0f, 0.0f, "loc", "note", (uint64)16);
    }

    const String pathId (pPath->getPathId ());
    int rc = _pDSPro->registerPath (pPath);
    if (0 == rc) {
        print (pToken, "path %s registered.\n", pathId.c_str());
    }
    else {
        print (pToken, "path registered failed. Returned code: %d\n", rc);
    }

    if (pathId.startsWith (TEST_PATH_ID) == 1) {
        _pDSPro->setCurrentPath (pathId);
    }

    // delete pPath;
}

void DSProCmdProcessor::handleSetCurrentPath (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken ();  // This is the command itself - discard
    const char *pszPathId = st.getNextToken();
    int rc = _pDSPro->setCurrentPath (pszPathId);
    if (0 == rc) {
        print (pToken, "path set as current.\n");
    }
    else {
        print (pToken, "path not set as current. Returned code: %d\n", rc);
    }
}


void DSProCmdProcessor::handleRankingWeigths (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszToken;
    float weights[8];
    bool flags[2];
    for (uint8 i = 0; (pszToken = st.getNextToken()) != nullptr; i++) {
        if (i < 8) {
            weights[i] = (float) atof (pszToken);
        }
        else {
            flags[i - 8] = atob (pszToken, true);
        }
    }
    int rc = _pDSPro->setRankingWeigths (weights[0], weights[1], weights[2], weights[3],
                                         weights[4], weights[5], weights[6], weights[7],
                                         flags[0], flags[1]);
    if (0 == rc) {
        print (pToken, "Set Ranking Weights successful.\n");
    }
    else {
        print (pToken, "Set Ranking Weights failed. Returned code: %d\n", rc);
    }
}

void DSProCmdProcessor::handleSetSelectivity (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszToken = st.getNextToken();
    if (pszToken == nullptr) {
        print (pToken, "Set Selectivity failed. Specify the value of selectivity\n");
        return;
    }
    float fSelectivity = atof (pszToken);
    int rc = _pDSPro->setSelectivity (fSelectivity);
    if (0 == rc) {
        print (pToken, "Set Selectivity successful.\n");
    }
    else {
        print (pToken, "Set Selectivity failed. Returned code: %d\n", rc);
    }
}

void DSProCmdProcessor::handleSetRangeOfInfluence (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszToken = st.getNextToken();
    if (pszToken == nullptr) {
        return;
    }
    const char *pszMilSymbol = pszToken;
    pszToken = st.getNextToken();
    if (pszToken == nullptr) {
        return;
    }
    uint32 ui32RangeOfInfluence = atoui32 (pszToken);
    int rc = _pDSPro->setRangeOfInfluence (pszMilSymbol, ui32RangeOfInfluence);
    if (0 == rc) {
        print (pToken, "Set Default Useful Distance successful.\n");
    }
    else {
        print (pToken, "Set Default Useful Distance failed. Returned code: %d\n", rc);
    }
}

void DSProCmdProcessor::handleSetUsefulDistance (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken ();  // This is the command itself - discard
    uint32 ui32UsefulDistance = 0U;
    const char *pszMIMEType = nullptr;
    const char *pszToken;
    for (uint8 i = 0; (i < 2) && ((pszToken = st.getNextToken()) != nullptr); i++) {
        if (isInteger (pszToken)) {
            // it's an unsigned integer
            ui32UsefulDistance = atoui32 (pszToken);
        }
        else if (pszMIMEType == nullptr) {
            pszMIMEType = pszToken;
        }
        else {
            // Error!
            return;
        }
    }
    int rc = pszMIMEType == nullptr ? _pDSPro->setDefaultUsefulDistance (ui32UsefulDistance) :
             _pDSPro->setUsefulDistance (pszMIMEType, ui32UsefulDistance);
    if (0 == rc) {
        print (pToken, "Set Default Useful Distance successful.\n");
    }
    else {
        print (pToken, "Set Default Useful Distance failed. Returned code: %d\n", rc);
    }
}

void DSProCmdProcessor::handleGenTrack (const void *pToken, const char *pszCmdLine)
{
    const String format ("{\n"
        "  \"track\" : {\n"
        "    \"trackType\" : \"Unit\",\n"
        "    \"environmentCategory\" : \"LND\",\n"
        "    \"threat\" : \"FRD\",\n"
        "    \"name\" : \"%s\",\n"
        "    \"milStd2525Symbol\" : \"%s\",\n"
        "    \"identifiers\" : {\n"
        "      \"uid\" : \"%s\",\n"
        "      \"nickname\" : \"%s\"\n"
        "    }\n"
        "  },\n"
        "  \"events\" : [ {\n"
        "    \"dtg\" : %lld,\n"
        "    \"location\" : {\n"
        "      \"position\" : {\n"
        "        \"latitude\" : %f,\n"
        "        \"longitude\" : %f\n"
        "      }\n"
        "    },\n"
        "    \"classification\" : \"UNKNOWN\",\n"
        "    \"source\" : \"Observed\"\n"
        "  } ]\n"
        "}");

    static String nodeId;
    if (nodeId.length() <= 0) {
        String ctxt (_pDSPro->getNodeContext (nullptr));
        JsonObject json (ctxt);
        JsonObject *pInfo = json.getObject ("nodeInfo");
        if (pInfo != nullptr) {
            pInfo->getString ("nodeId", nodeId);
            delete pInfo;
        }
    }
    static unsigned int uiInstanceId = 0;

    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    float coordinates[3] = { 0.0f, 0.0f, 0.0f };
    for (int i = 0; (i < 3); i++) {
        const char *pszToken = st.getNextToken();
        if (pszToken == nullptr) {
            break;
        }
        coordinates[i] = (float) atof (pszToken);
    }

    char track[1024];
    sprintf (track, format, nodeId.c_str(), "SD+++++", nodeId.c_str(), nodeId.c_str(),
             getTimeInMilliseconds(), coordinates[0], coordinates[1]);
    char chInstanceId[24];
    itoa (chInstanceId, uiInstanceId);
    AVList metadataList;
    metadataList.addPair (MetadataInterface::LEFT_UPPER_LATITUDE, coordinates[0]);
    metadataList.addPair (MetadataInterface::LEFT_UPPER_LONGITUDE, coordinates[1]);
    metadataList.addPair (MetadataInterface::RIGHT_LOWER_LATITUDE, coordinates[0] + 0.00001f);
    metadataList.addPair (MetadataInterface::RIGHT_LOWER_LONGITUDE, coordinates[1] - 0.00001f);
    metadataList.addPair (MetadataInterface::DATA_FORMAT, "x-dspro/x-soi-track");
    char *pszId = nullptr;
    if ((_pDSPro->addMessage ("grp", nodeId, chInstanceId, &metadataList, track, strlen (track), 0, &pszId) == 0) && (pszId != nullptr)) {
        print (pToken, "Added message %s. ObjectId: <%s> InstanceId <%s>.\n", pszId, nodeId.c_str(), chInstanceId);
    }
    else {
        print (pToken, "Could not add message.\n");
    }
    int rc = _pDSPro->setCurrentPosition (coordinates[0], coordinates[1], coordinates[2], nullptr, nullptr);
    if (0 == rc) {
        print (pToken, "Set Position successful.\n");
    }
    else {
        print (pToken, "Set Position failed. Returned code: %d\n", rc);
    }
    uiInstanceId++;
}

void DSProCmdProcessor::handleMist (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    String mistFile (st.getNextToken());
    if ((mistFile.length() <= 0) || (!FileUtils::fileExists (mistFile))) {
        print (pToken, "MIST file parsing failed. MIST file %s could not be found.\n", mistFile.c_str());
        return;
    }
    int64 i64FileSize = 0;
    void *pFileContent = FileUtils::readFile (mistFile, &i64FileSize);
    if ((pFileContent == nullptr) || (i64FileSize <= 0)) {
        print (pToken, "MIST file parsing failed. MIST file %s could not be read.\n", mistFile.c_str());
        return;
    }
    char *pszEncodedJson = Base64Transcoders::encode (pFileContent, (unsigned short)i64FileSize);
    if (pszEncodedJson == nullptr) {
        print (pToken, "MIST file parsing failed. MIST file %s could not be encoded to Base64.\n", mistFile.c_str());
        return;
    }
    static const char * MIST_MIME_TYPE = "x-dspro/x-phoenix-mist";
    static const char * DO_NOT_CHECK_NODE_ID = nullptr;
    JsonObject *pJson = AMTDictator::parseAndValidate (pszEncodedJson, strlen (pszEncodedJson), MIST_MIME_TYPE, DO_NOT_CHECK_NODE_ID);
    if (pJson == nullptr) {
        print (pToken, "MIST file %s didn't pass validation.\n", mistFile.c_str());
        return;
    }
    _pAMTDictator->messageArrived (pJson);
}

#include "SessionId.h"

void DSProCmdProcessor::handleSetSessionId (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    String sessionId (st.getNextToken());
    SessionId::getInstance()->setSessionId (sessionId, getTimeInMilliseconds());
}

void DSProCmdProcessor::handeSetPosition (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    float coordinates[3] = { 0.0f, 0.0f, 0.0f };
    for (int i = 0; i < 3; i++) {
        const char *pszToken = st.getNextToken();
        if (pszToken == nullptr) {
            break;
        }
        coordinates[i] = (float) atof (pszToken);
    }
    int rc = _pDSPro->setCurrentPosition (coordinates[0], coordinates[1], coordinates[2], nullptr, nullptr);
    if (0 == rc) {
        print (pToken, "Set Position successful.\n");
    }
    else {
        print (pToken, "Set Position failed. Returned code: %d\n", rc);
    }
}

void DSProCmdProcessor::handleSetMission (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    String mission = st.getNextToken();  // This is the command itself - discard
    int rc = _pDSPro->setMissionId (mission);
    if (0 == rc) {
        print (pToken, "Set Mission successful.\n");
    }
    else {
        print (pToken, "Set Mission failed. Returned code: %d\n", rc);
    }
}

void DSProCmdProcessor::handleSetRole (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    String role = st.getNextToken();  // This is the command itself - discard
    int rc = _pDSPro->setRole (role);
    if (0 == rc) {
        print (pToken, "Set Role successful.\n");
    }
    else {
        print (pToken, "Set Role failed. Returned code: %d\n", rc);
    }
}

void DSProCmdProcessor::handeGetNodeContext (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszToken = st.getNextToken();
    String ctxt (_pDSPro->getNodeContext (pszToken));
    print (pToken, "Node Context: %s\n", ctxt.c_str());
}
