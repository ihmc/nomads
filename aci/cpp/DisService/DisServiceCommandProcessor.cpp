/*
 * DisServiceCommandProcessor.cpp
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
 */

#include "DisServiceCommandProcessor.h"

#include "DataCacheInterface.h"
#include "DisseminationService.h"
#include "MessageId.h"
#include "MessageReassembler.h"
#include "PropertyStoreInterface.h"

#include "Logger.h"
#include "StringTokenizer.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DisServiceCommandProcessor::DisServiceCommandProcessor (DisseminationService *pDisService)
{
    _pDisService = pDisService;
}

DisServiceCommandProcessor::~DisServiceCommandProcessor (void)
{
    _pDisService = NULL;
}

int DisServiceCommandProcessor::processCmd (const void *pToken, char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    const char *pszCmd = st.getNextToken();
    if (pszCmd == NULL) {
        // Should not happen, but return 0 anyways
        return 0;
    }
    else if (0 == stricmp (pszCmd, "help")) {
        const char *pszHelpCmd = st.getNextToken();
        if (pszHelpCmd == NULL) {
            displayGeneralHelpMsg (pToken);
        }
        else {
            displayHelpMsgForCmd (pToken, pszHelpCmd);
        }
    }
    else if (0 == stricmp (pszCmd, "db")) {
        handleDBCmd (pToken, pszCmdLine);
    }
    else if (0 == stricmp (pszCmd, "prop")) {
        handlePropCmd (pToken, pszCmdLine);
    }
    else if (0 == stricmp (pszCmd, "hasFragment")) {
        handleHasFragmentCmd (pToken, pszCmdLine);
    }
    else if (0 == stricmp (pszCmd, "screenOutput")) {
        handleScreenOutputCmd (pToken, pszCmdLine);
    }
    else if ((0 == stricmp (pszCmd, "exit")) || (0 == stricmp (pszCmd, "quit"))) {
        return -1;
    }
    else {
        print (pToken, "unknown command - type help for a list of valid commands\n");
    }
    return 0;
}

void DisServiceCommandProcessor::displayGeneralHelpMsg (const void *pToken)
{
    print (pToken, "The following commands are available\n");
    print (pToken, "Type help <cmd> for more help on a particular command\n");
    print (pToken, "    exit - terminate program\n");
    print (pToken, "    db - various database operations\n");
    print (pToken, "    prop - operations related to properties\n");
    print (pToken, "    hasFragment - check to see whether a fragment is present\n");
    print (pToken, "    screenOutput - enable or disable screen output of log messsages\n");
    print (pToken, "    quit - terminate program\n");
}

void DisServiceCommandProcessor::displayHelpMsgForCmd (const void *pToken, const char *pszCmd)
{
    if (0 == stricmp (pszCmd, "exit")) {
        print (pToken, "usage: exit\n");
        print (pToken, "    terminates the program\n");
    }
    else if (0 == stricmp (pszCmd, "db")) {
        print (pToken, "usage: db query|modify [options...]\n");
        print (pToken, "       db query dataCache <GroupName>:<SenderNodeId>:<MessageSeqId>:<ChunkId>\n");
        print (pToken, "       db query replicationList|replicationHistory <targetNodeId>\n");
        print (pToken, "    queries or modifies the database\n");
        print (pToken, "    query dataCache queries and displays the content of the messages that match the specification\n");
        print (pToken, "    query replicationList queries and displays the messages that would be replicated to the specified target node id\n");
        print (pToken, "    query replicationHistory queries and displays the messages that were previously replicated to the specified target node id\n");
    }
    else if (0 == stricmp (pszCmd, "prop")) {
        print (pToken, "usage: prop put|get|remove [options...]\n");
        print (pToken, "       prop put <NodeID> <Attribute> <Value>\n");
        print (pToken, "       prop get <NodeID> <Attribute>\n");
        print (pToken, "       prop remove <NodeID> <Attribute>\n");
        print (pToken, "    stores, retrieves, or deletes properties from the database\n");
    }
    else if (0 == stricmp (pszCmd, "hasFragment")) {
        print (pToken, "usage: hasFragment <GroupName> <SenderNodeId> <MsgSeqNo> <ChunkId> <FragmentOffset> <FragmentLength>\n");
        print (pToken, "     displays true if the specified fragment is present in the data cache / message reassembler or false otherwise\n");
    }
    else if (0 == stricmp (pszCmd, "screenOutput")) {
        print (pToken, "usage: screenOutput true|on|enable|false|off|disable\n");
        print (pToken, "    turns screen output of log message on or off\n");
    }
    else if (0 == stricmp (pszCmd, "quit")) {
        print (pToken, "usage: quit\n");
        print (pToken, "    terminates the program\n");
    }
    else {
        print (pToken, "unknown command \"%s\" - type help for a list of valid commands\n", pszCmd);
    }
}

void DisServiceCommandProcessor::handleDBCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszSubCmd = st.getNextToken();
    if (pszSubCmd == NULL) {
        print (pToken, "invalid params - try help db\n");
        return;
    }
    if (0 == stricmp (pszSubCmd, "query")) {
        const char *pszQueryOp = st.getNextToken();
        if (pszQueryOp == NULL) {
            print (pToken, "invalid params - try help db\n");
            return;
        }
        else if (0 == stricmp (pszQueryOp, "dataCache")) {
            const char *pszMessageId = st.getNextToken();
            if (pszMessageId == NULL) {
                print (pToken, "invalid params - try help db\n");
                return;
            }
            uint32 ui32Len;
            const void *pData = _pDisService->_pDataCacheInterface->getData (pszMessageId, ui32Len);
            if (pData == NULL) {
                print (pToken, "did not find any data matching id <%s>\n", pszMessageId);
            }
            else {
                print (pToken, "found data of length %lu; data follows\n", ui32Len);
                print (pToken, "%s\n", (const char*) pData);
            }
            return;
        }
        else if (0 == stricmp (pszQueryOp, "replicationList")) {
            const char *pszTargetNodeId = st.getNextToken();
            if (pszTargetNodeId == NULL) {
                print (pToken, "invalid params - try help db\n");
                return;
            }
            PtrLList<MessageId> *pMsgIds = _pDisService->_pDataCacheInterface->getNotReplicatedMsgList (pszTargetNodeId, 0, NULL);
            if (pMsgIds != NULL) {
                pMsgIds->resetGet();
                MessageId *pMId;
                while (NULL != (pMId = pMsgIds->getNext())) {
                    print (pToken, "%s\n", pMId->getId());
                    delete pMId;
                }
                delete pMsgIds;
            }
            return;
        }
        else if (0 == stricmp (pszQueryOp, "replicationHistory")) {
            print (pToken, "db query replicationHistory not yet implemented\n");
            return;
        }
        else {
            print (pToken, "invalid db query operation - try help db\n");
            return;
        }
    }
    else if (0 == stricmp (pszSubCmd, "modify")) {
        print (pToken, "db modify sub command not yet implemented\n");
        return;
    }
    else {
        print (pToken, "invalid sub command for db - try help db\n");
    }
}

void DisServiceCommandProcessor::handlePropCmd (const void *pToken, const char *pszCmdLine)
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
        PropertyStoreInterface *pPropStore = _pDisService->_pDataCacheInterface->getStorageInterface()->getPropertyStore();
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
        PropertyStoreInterface *pPropStore = _pDisService->_pDataCacheInterface->getStorageInterface()->getPropertyStore();
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
        PropertyStoreInterface *pPropStore = _pDisService->_pDataCacheInterface->getStorageInterface()->getPropertyStore();
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

void DisServiceCommandProcessor::handleHasFragmentCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszGroupName = st.getNextToken();
    const char *pszSenderNodeId = st.getNextToken();
    const char *pszMsgSeqNo = st.getNextToken();
    const char *pszChunkId = st.getNextToken();
    const char *pszFragOffset = st.getNextToken();
    const char *pszFragLength = st.getNextToken();
    if ((pszGroupName == NULL) || (pszSenderNodeId == NULL) || (pszMsgSeqNo == NULL) ||
        (pszChunkId == NULL) || (pszFragOffset == NULL) || (pszFragLength == NULL)) {
            print (pToken, "invalid params - try help hasFragment\n");
            return;
    }
    MessageReassembler *pReassembler = _pDisService->_pMessageReassembler;
    if (pReassembler->hasFragment (pszGroupName, pszSenderNodeId, atoui32 (pszMsgSeqNo), (uint8) atoui32 (pszChunkId), atoui32 (pszFragOffset), (uint16) atoui32 (pszFragLength))) {
        print (pToken, "    fragment is present in the MessageReassembler\n");
    }
    else {
        print (pToken, "    fragment not found in the MessageReassembler\n");
    }
}

void DisServiceCommandProcessor::handleScreenOutputCmd (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszArg = st.getNextToken();
    if (pszArg == NULL) {
        print (pToken, "invalid params - try help screenOutput\n");
        return;
    }
    else if ((0 == stricmp (pszArg, "true")) || (0 == stricmp (pszArg, "on")) || (0 == stricmp (pszArg, "enable"))) {
        pLogger->enableScreenOutput();
    }
    else if ((0 == stricmp (pszArg, "false")) || (0 == stricmp (pszArg, "off")) || (0 == stricmp (pszArg, "disable"))) {
        pLogger->disableScreenOutput();
    }
    else {
        print (pToken, "invalid params - try help screenOutput\n");
    }
}
