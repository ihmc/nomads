/*
 * DisServicePacketTool.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
#include <string.h>

#include "DisServiceMsg.h"
#include "DisServiceMsgHelper.h"
#include "Message.h"

#include "BufferReader.h"
#include "FTypes.h"
#include "MessageFactory.h"
#include "NetworkMessage.h"
#include "NLFLib.h"

int extractDataFromPacket (int argc, char *argv[]);

using namespace IHMC_ACI;
using namespace NOMADSUtil;

int main (int argc, char *argv[])
{
    if (argc < 2) {
        fprintf (stderr, "usage: %s <operation> [<arg1> <arg2> ...]\n", argv[0]);
        return -1;
    }
    if (0 == stricmp (argv[1], "extractData")) {
        return extractDataFromPacket (argc-2, &argv[2]);
    }
    else {
        fprintf (stderr, "unknown operation <%s>\n", argv[1]);
        return -2;
    }

}

int extractDataFromPacket (int argc, char *argv[])
{
    if (argc < 1) {
        fprintf (stderr, "must specify an input file for extractData operation\n");
        return -1;
    }
    FILE *fileInput = fopen (argv[0], "rb");
    if (fileInput == NULL) {
        fprintf (stderr, "could not open file <%s>\n", argv[0]);
        return -2;
    }
    int64 i64Size = getFileSize (argv[0]);
    if (i64Size < 0) {
        fprintf (stderr, "could not obtain size of file <%s>\n", argv[0]);
        return -3;
    }
    uint32 ui32PacketSize = (uint32) i64Size;
    uint8 *pui8FileBuf = (uint8*) malloc (ui32PacketSize);
    if (ui32PacketSize != fread (pui8FileBuf, 1, ui32PacketSize, fileInput)) {
        fprintf (stderr, "could not read contents of file <%s>\n", argv[0]);
        return -4;
    }
    fclose (fileInput);

    printf ("read a packet of size %lu\n", ui32PacketSize);
    //NOTE: The +18 below is a hack for BlueRadio encapsulated messages
    NetworkMessage *pNetMsg = MessageFactory::createNetworkMessageFromBuffer (pui8FileBuf+18, ui32PacketSize-18, NetworkMessage::getVersionFromBuffer ((const char*)pui8FileBuf+18, ui32PacketSize-18));
    if (pNetMsg == NULL) {
        fprintf (stderr, "failed to instantiate a network message from the contents of file <%s>\n", argv[0]);
    }

    uint8 ui8MsgType = pNetMsg->getMsgType();
    if (!DisServiceMsgHelper::isDisServiceMessage (ui8MsgType)) {
        fprintf (stderr, "this message is not of type DisService\n");
        return -5;
    }

    const void *pMsgMetaData = pNetMsg->getMetaData();
    uint16 ui16MsgMetaDataLen = pNetMsg->getMetaDataLen();
    const void *pMsg = pNetMsg->getMsg();
    uint16 ui16MsgLen = pNetMsg->getMsgLen();

    uint8 ui8DSMsgType;
    if (DisServiceMsgHelper::getMessageType (pMsgMetaData, ui16MsgMetaDataLen, ui8DSMsgType) < 0) {
        fprintf (stderr, "cannot read DisService message of type %d\n", (int) ui8DSMsgType);
        return -6;
    }

    DisServiceMsg *pDSMsg = DisServiceMsgHelper::getInstance (ui8DSMsgType);
    if (pDSMsg == NULL) {
        fprintf (stderr, "could not instantiate DisService message of type %d\n", (int) ui8DSMsgType);
        return -7;
    }

    int rc;
    BufferReader br (pMsg, ui16MsgLen);
    if (0 != (rc = pDSMsg->read (&br, ui16MsgLen))) {
        fprintf (stderr, "reading a message of type %d failed with rc = %d\n", (int) ui8DSMsgType, rc);
        return -8;
    }

    switch (ui8DSMsgType) {
        case DisServiceMsg::DSMT_Data:
        {
            DisServiceDataMsg *pDSDMsg = (DisServiceDataMsg*) pDSMsg;
            Message *pMessage = pDSDMsg->getMessage();
            if (pMessage == NULL) {
                return -9;
            }
            MessageHeader *pMH = pMessage->getMessageHeader();
            printf ("read a Data message with id %s\n", pMH->getMsgId());
            FILE *fileOutput = fopen ("dsdata.out", "wb");
            fwrite (pMessage->getData(), 1, pMH->getFragmentLength(), fileOutput);
            fclose (fileOutput);
            break;
        }

        default:
            printf ("currently not processing DisService message of type %d\n", (int) ui8DSMsgType);
            break;
    }
    return 0;
}
