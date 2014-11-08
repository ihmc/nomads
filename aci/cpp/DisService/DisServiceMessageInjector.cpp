/*
 * DisServiceMessageInjector.cpp
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

#include <stdio.h>

#include "DisServiceMsg.h"
#include "Message.h"
#include "MessageInfo.h"

#include "FTypes.h"
#include "MD5.h"
#include "StringTokenizer.h"
#include "net/NetworkMessageService.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

void displayHelpMsg (void);
int generateDisServiceDataMsg (NetworkMessageService *pNMS, const char *pszCmd);
DisServiceDataMsg * createDisServiceDataMsg (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, uint8 ui8ChunkId,
                                             const char *pszObjectId, const char *pszInstanceId, uint16 ui16Tag, uint16 ui16ClientId,
                                             uint8 ui8ClientType, const char *pszMimeType, uint32 ui32TotalMsgLen, uint32 ui32FragOff, uint32 ui32FragLen, uint32 ui32MetaDataLen,
                                             uint16 ui16HistoryWindow, uint8 ui8Priority, int64 i64Expiration, bool bAck, bool bMetaData,
                                             uint8 ui8TotalNumOfChunks, const char *pszTargetNodeId);

int main (int argc, char *argv[])
{
    char szBuf[1024];
    NetworkMessageService *pNMS = new NetworkMessageService();
    pNMS->init (6669, "239.0.0.239", 3);
    pNMS->start();
    while (true) {
        printf ("> ");
        fflush (stdout);
        if (NULL == fgets (szBuf, sizeof (szBuf), stdin)) {
            break;
        }
        StringTokenizer st (szBuf);
        const char *pszCmd = st.getNextToken();
        if (pszCmd == NULL) {
            continue;
        }
        else if (0 == stricmp (pszCmd, "help")) {
            displayHelpMsg();
        }
        else if ((0 == stricmp (pszCmd, "dataMsg")) || (0 == stricmp (pszCmd, "dm"))) {
            generateDisServiceDataMsg (pNMS, szBuf);
        }
        else if ((0 == stricmp (pszCmd, "exit")) || (0 == stricmp (pszCmd, "quit"))) {
            break;
        }
        else {
            printf ("unknown command - type help for a list of valid commands\n");
        }
    }

    return 0;
}

void displayHelpMsg (void)
{
    printf ("commands:\n");
    printf ("    help - Display this message\n");
    printf ("    datamsg|dm <GroupName> <SenderNodeId> <MsgSeqId> <TotalMsgLen> <ObjectId> <InstanceId> <MimeType> <FragOff> <FragLen> - Generate a DisServiceDataMsg using the specified parameters\n");
    printf ("    exit|quit - End the program\n");
}

int generateDisServiceDataMsg (NetworkMessageService *pNMS, const char *pszCmd)
{
    StringTokenizer st (pszCmd);
    st.getNextToken();  // Discard the command itself
    const char *pszGroupName = st.getNextToken();
    const char *pszSenderNodeId = st.getNextToken();
    const char *pszMsgSeqId = st.getNextToken();
    const char *pszObjectId = st.getNextToken();
    const char *pszInstanceId = st.getNextToken();
    const char *pszMimeType = st.getNextToken();
    const char *pszTotalMsgLen = st.getNextToken();
    const char *pszFragOff = st.getNextToken();
    const char *pszFragLen = st.getNextToken();
    if ((pszGroupName == NULL) || (pszSenderNodeId == NULL) || (pszMsgSeqId == NULL) || (pszTotalMsgLen == NULL) || (pszFragOff == NULL) || (pszFragLen == NULL)) {
        printf ("invalid parameters for command - type help for help\n");
        return -1;
    }
    uint32 ui32MsgSeqId = atoui32 (pszMsgSeqId);
    uint32 ui32TotalMsgLen = atoui32 (pszTotalMsgLen);
    uint32 ui32FragOff = atoui32 (pszFragOff);
    uint32 ui32FragLen = atoui32 (pszFragLen);
    printf ("Generating and pushing %s:%s:%u:%u:%u-%u\n", pszGroupName, pszSenderNodeId, ui32MsgSeqId, ui32TotalMsgLen, ui32FragOff, ui32FragOff+ui32FragLen);
    DisServiceDataMsg *pDSDM = createDisServiceDataMsg (pszGroupName, pszSenderNodeId, ui32MsgSeqId, 0, pszObjectId,
                                                        pszInstanceId, 0, 0, 0, pszMimeType, ui32TotalMsgLen, ui32FragOff,
                                                        ui32FragLen, 0, 0, 0, 0, false, false, 0, NULL);
    BufferWriter bw;
    pDSDM->write (&bw, 1500);
    uint8 ui8MsgType = pDSDM->getType();
    
    int rc = pNMS->broadcastMessage ('d', NULL, 0, 0, 0, 1, 0, &ui8MsgType, 1, bw.getBuffer(), bw.getBufferLength(), false);
    
    delete pDSDM;
    if (rc != 0) {
        return -2;
    }
    return 0;
}

DisServiceDataMsg * createDisServiceDataMsg (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, uint8 ui8ChunkId,
                                             const char *pszObjectId, const char *pszInstanceId, uint16 ui16Tag, uint16 ui16ClientId,
                                             uint8 ui8ClientType, const char *pszMimeType, uint32 ui32TotalMsgLen, uint32 ui32FragOff, uint32 ui32FragLen, uint32 ui32MetaDataLen,
                                             uint16 ui16HistoryWindow, uint8 ui8Priority, int64 i64Expiration, bool bAck, bool bMetaData, uint8 ui8TotalNumOfChunks,
                                             const char *pszTargetNodeId)
{
    uint8 *pui8Data = new uint8[ui32FragLen];
    for (uint32 ui = ui32FragOff; ui < ui32FragLen; ui++) {
        pui8Data[ui] = (uint8) ('0' + (ui % 10));
    }

    char *pszChecksum = MD5Utils::getMD5Checksum ((const void*)pui8Data, ui32TotalMsgLen);
    MessageHeader *pMH = MessageHeaderHelper::getMessageHeader (pszGroupName, pszSenderNodeId, ui32MsgSeqId, ui8ChunkId, pszObjectId,
                                                                pszInstanceId, ui16Tag, ui16ClientId, ui8ClientType, pszMimeType,
                                                                pszChecksum, ui32TotalMsgLen, ui32FragOff, ui32FragLen, ui32MetaDataLen,
                                                                ui16HistoryWindow, ui8Priority, i64Expiration, bAck, bMetaData, ui8TotalNumOfChunks);
    if (pszChecksum != NULL) {
        free (pszChecksum);
        pszChecksum = NULL;
    }

    Message *pm = new Message (pMH, pui8Data);

    DisServiceDataMsg *pDSDM = new DisServiceDataMsg (pszSenderNodeId, pm, pszTargetNodeId);
    return pDSDM;
}
