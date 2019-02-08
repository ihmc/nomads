/*
 * DSProMessage.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on August 3, 2011, 6:52 PM
 */

#ifndef INCL_DS_PRO_MESSAGE_H
#define INCL_DS_PRO_MESSAGE_H

#include "DisServiceMsg.h"
#include "MessageHeaders.h"

#include "Writer.h"

namespace IHMC_ACI
{
    class MetaData;

    class MessageInfo;

    class DSProMessage : public ControllerToControllerMsg
    {
        public:
            static const uint32 METADATA_LENGTH;

            virtual ~DSProMessage (void);

            static DSProMessage * getDSProMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                                                   MessageHeaders::MsgType type, void *pData, uint32 ui32DataLength);

            static char * readMessagePublisher (const void *pBuf, uint32 ui32Len, uint32 &ui32BytesRead);

        protected:
            DSProMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                          uint8 *pui8Type, void *pData, uint32 ui32DataLength);
    };

    class DSProMessageHelper
    {
        public:
            static int writesCtrlMsg (NOMADSUtil::Writer *pWriter, const char *pszPublisherNodeId,
                                      uint32 ui32PubLen, const void *pBuf, uint32 ui32BufLen);
    };

    class ContextUpdateMessage : public DSProMessage
    {
        public:
            ContextUpdateMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                                  uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~ContextUpdateMessage (void);
    };

    class ContextVersionMessage : public DSProMessage
    {
        public:
            ContextVersionMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                                   uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~ContextVersionMessage (void);
    };

    class PositionMessage : public DSProMessage
    {
        public:
            PositionMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                             uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~PositionMessage (void);
    };

    class TopologyReply : public DSProMessage
    {
        public:
            TopologyReply (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                           uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~TopologyReply (void);
    };

    class SearchMessage : public DSProMessage
    {
        public:
            SearchMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                           uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~SearchMessage (void);
    };

    class TopologyReplyMessage : public DSProMessage
    {
        public:
            TopologyReplyMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                                  uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~TopologyReplyMessage (void);
    };

    class TopologyRequestMessage : public DSProMessage
    {
        public:
            TopologyRequestMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                                    uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~TopologyRequestMessage (void);
    };

    class UpdateMessage : public DSProMessage
    {
        public:
            UpdateMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                           uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~UpdateMessage (void);
    };

    class VersionMessage : public DSProMessage
    {
        public:
            VersionMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                            uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~VersionMessage (void);
    };

    class WayPointMessage : public DSProMessage
    {
        public:
            WayPointMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                             uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~WayPointMessage (void);
    };

    class WholeMessage : public DSProMessage
    {
        public:
            WholeMessage (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                          uint8 *pui8Type, void *pData, uint32 ui32DataLength);
            virtual ~WholeMessage (void);
    };

    inline int DSProMessageHelper::writesCtrlMsg (NOMADSUtil::Writer* pWriter, const char* pszPublisherNodeId,
                                                  uint32 ui32PubLen, const void* pBuf, uint32 ui32BufLen)
    {
        if (pWriter == nullptr || ((ui32PubLen > 0) && (pszPublisherNodeId == nullptr))) {
            return -1;
        }

        if (ui32PubLen > 0) {
            if (pWriter->write32 (&ui32PubLen) != 0 ||
                pWriter->writeBytes (pszPublisherNodeId, ui32PubLen) != 0) {
                return -2;
            }
        }
        if (pWriter->writeBytes (pBuf, ui32BufLen) != 0) { // DSProMessage's destructor
            return -3;                               // deletes pBuf! Pass a copy to it!
        }

        return 0;
    }
}

#endif    // INCL_DS_PRO_MESSAGE_H

