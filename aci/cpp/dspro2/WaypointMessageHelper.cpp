/*
 * WaypointMessageHelper.cpp
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

#include "WaypointMessageHelper.h"

#include "BufferReader.h"
#include "BufferWriter.h"
#include "Logger.h"

#include <stdlib.h>
#include <string.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

int WaypointMessageHelper::readWaypointMessageForTarget (const void *pWaypointMsgPayload,
                                                         uint32 ui32Offset, uint32 ui32TotalLen,
                                                         PreviousMessageIds &previouMessagesSentToTargets,
                                                         uint32 &ui32WaypointMsgPayloadLen)
{
    ui32WaypointMsgPayloadLen = ui32TotalLen;
    BufferReader br (pWaypointMsgPayload, ui32TotalLen);
    br.setPosition (ui32Offset);
    uint32 ui32LatestMessageSentLen = 0;
    if (br.read32 (&ui32LatestMessageSentLen) < 0) {
        return -1;
    }
    ui32WaypointMsgPayloadLen -= 4;

    if (ui32LatestMessageSentLen > 0) {
        static const uint16 BUF_LEN = 1024;
        char buf[BUF_LEN];
        if ((ui32LatestMessageSentLen + 1) > BUF_LEN) {
            return -2;
        }
        if (br.readBytes (buf, ui32LatestMessageSentLen) < 0) {
            return -3;
        }
        buf[ui32LatestMessageSentLen] = '\0';
        previouMessagesSentToTargets = buf;
        ui32WaypointMsgPayloadLen -= ui32LatestMessageSentLen;
    }

    return 0;
}

void * WaypointMessageHelper::writeWaypointMessageForTarget (PreviousMessageIds &previouMessagesSentToTargets,
                                                             const void *pWaypointMsgPayload,
                                                             uint32 ui32WaypointMsgPayloadLen,
                                                             uint32 &ui32TotalLen)
{
    ui32TotalLen = 0;
    if (pWaypointMsgPayload == NULL) {
        return NULL;
    }

    const String sPrevMsgs (previouMessagesSentToTargets);
    uint32 ui32LatestMsgLen = (sPrevMsgs.length() > 0 ? sPrevMsgs.length() : 0U);

    BufferWriter bw (4, 0);
    bw.write32 (&ui32LatestMsgLen);

    // Allocate buffer
    char *pData = (char *) malloc (bw.getBufferLength() + ui32LatestMsgLen +
                                   ui32WaypointMsgPayloadLen);
    if (pData == NULL) {
        checkAndLogMsg ("WaypointMessageHelper::writeWaypointMessageForTarget", memoryExhausted);
        return NULL;
    }

    // Write latest pushed message ID's length
    memcpy (pData, bw.getBuffer(), bw.getBufferLength());
    ui32TotalLen += bw.getBufferLength();

    // Write latest pushed message ID
    if (ui32LatestMsgLen > 0U) {
        memcpy (pData+bw.getBufferLength(), sPrevMsgs.c_str(), ui32LatestMsgLen);
        ui32TotalLen += ui32LatestMsgLen;
    }

    // Set the waypoint message payload
    memcpy (pData+(bw.getBufferLength() + ui32LatestMsgLen),
            pWaypointMsgPayload, ui32WaypointMsgPayloadLen);
    ui32TotalLen += ui32WaypointMsgPayloadLen;

    return pData;
}

