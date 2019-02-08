/*
 * MessageId.h
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

#ifndef INCL_MESSAGE_ID_H
#define INCL_MESSAGE_ID_H

#include "FTypes.h"
#include "StrClass.h"

namespace IHMC_ACI
{
    class MessageId
    {
        public:
            MessageId (void);
            explicit MessageId (const char *pszMsgId);
            MessageId (const char *pszGroupName, const char *pszOriginatorNodeId,
                       uint32 ui32SeqId, uint8 ui8ChunkId);

            MessageId (const MessageId &msgId);

            virtual ~MessageId (void);

            int init (const char *pszMsgId);
            int init (const char *pszGroupName, const char *pszOriginatorNodeId,
                      uint32 ui32SeqId, uint8 ui8ChunkId);

            const MessageId & operator = (const MessageId &rhsId);
            bool operator == (const MessageId &rhsId) const;
            bool operator != (const MessageId &rhsId) const;

            int setGroupName (const char *pszGroupName);
            const char * getGroupName (void) const;

            int setOriginatorNodeId (const char *pszOriginatorNodeId);
            const char * getOriginatorNodeId (void) const;

            void setSeqId (uint32 ui32SeqId);
            uint32 getSeqId (void) const;

            void setChunkId (uint8 ui8ChunkId);
            uint8 getChunkId (void) const;

            const char * getId (void) const;

        protected:
            void buildMessageIdString (void);
            int parseAndInitFromString (const char *pszMsgId);

        protected:
            NOMADSUtil::String _msgId;
            NOMADSUtil::String _groupName;
            NOMADSUtil::String _originatorNodeId;
            uint32 _ui32SeqId;
            uint8 _ui8ChunkId;
    };

    inline bool MessageId::operator == (const MessageId &rhsId) const
    {
        return ((_msgId == rhsId._msgId) == 0 ? false : true);
    }

    inline bool MessageId::operator != (const MessageId &rhsId) const
    {
        return ((_msgId != rhsId._msgId) == 0 ? false : true);
    }

    inline const char * MessageId::getGroupName (void) const
    {
       return _groupName;
    }

    inline const char * MessageId::getOriginatorNodeId (void) const
    {
        return _originatorNodeId;
    }

    inline uint32 MessageId::getSeqId (void) const
    {
        return _ui32SeqId;
    }

    inline uint8 MessageId::getChunkId (void) const
    {
        return _ui8ChunkId;
    }

    inline const char * MessageId::getId (void) const
    {
        return _msgId;
    }
}

#endif   // #ifndef INCL_MESSAGE_ID_H
