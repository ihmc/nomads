/*
 * Message.h
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

#ifndef INCL_MESSAGE_H
#define INCL_MESSAGE_H

namespace IHMC_ACI
{
    class MessageInfo;
    class MessageHeader;
    class ChunkMsgInfo;

    class Message
    {
        public:
            Message (void);
            Message (MessageHeader *pMsgHeader, const void *pData);

            virtual ~Message (void);

            Message * clone (void);

            // NOTE: This class will NOT delete pMsgInfo in the destructor
            void setMessageHeader (MessageHeader *pMsgHeader);

            // NOTE: This class will NOT delete pData in the destructor
            void setData (const void *pData);

            MessageHeader * getMessageHeader (void) const;
            MessageHeader * relinquishMessageHeader (void);
            MessageInfo * getMessageInfo (void) const;
            MessageInfo * relinquishMessageInfo (void);
            ChunkMsgInfo * getChunkMsgInfo (void) const;
            ChunkMsgInfo * relinquishChunkMsgInfo (void);
            const void * getData (void) const;
            const void * relinquishData (void);

            bool operator > (Message &msgToMatch); // !! // For now I remove const
            bool operator < (Message &msgToMatch); // !! // For now I remove const
            bool operator == (Message &msgToMatch);

        private:
            MessageHeader *_pMsgHeader;
            const void *_pData;
    };
}

#endif   // #ifndef INCL_MESSAGE_H
