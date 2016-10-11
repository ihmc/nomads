/*
 * NetworkMessage.h
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_NETWORK_MESSAGE_INTERFACE_H
#define INCL_NETWORK_MESSAGE_INTERFACE_H

#include "FTypes.h"
#include "SequentialArithmetic.h"

namespace NOMADSUtil
{
    /**
     * NetworkMessage wraps a buffer containing the NetworkMessageService's
     * payload and the NetworkMessageService's header and provides convenient
     * methods to retrieve them.
     * When a NetworkMessage is created, a new buffer of the proper size is
     * allocated and both the header and payload are <em>copied</em> into it.
     */
    class NetworkMessage
    {
        public:
            static const uint8 NO_TARGET =  0;

            enum ChunkType {
                CT_SAck = 0x00,
                CT_DataMsgComplete = 0x01,
                CT_DataMsgStart = 0x02,
                CT_DataMsgInter = 0x03,
                CT_DataMsgEnd = 0x04
            };

            virtual ~NetworkMessage (void);

            static uint8 getVersionFromBuffer (const char* pBuf, uint32 ui32BufSize);

            virtual bool isMalformed (void) const;

            virtual uint16 getFixedHeaderLength (void) const;

            uint8 getVersion (void) const;
            uint8 getType (void) const;
            
            uint16 getLength (void) const;
            uint8 getMsgType (void) const;
            uint32 getSourceAddr (void) const;
            uint32 getDestinationAddr (void) const;
            uint16 getSessionId (void) const;

            uint16 getMsgId (void) const;
            
            uint8 getHopCount  (void) const;
            uint8 getTTL (void) const;
            uint8 getChunkType (void) const;
            
            bool isReliableMsg (void) const;

            virtual void * getMetaData (void) const;
            virtual uint16 getMetaDataLen (void) const;
            virtual void * getMsg (void) const;
            virtual uint16 getMsgLen (void) const;

            void * getBuf(void) const;
            void incrementHopCount(void);

            void setTargetAddress (uint32 ui32TargetAddress);

            void display (void) const;

            bool operator == (NetworkMessage &rhsMessageInfo);

            /**
             * This methods handle comparison of sequence numbers that may wrap
             * around. The assumption is that even if numbers wrap around, they
             * are "closer" to each other than half of the range of numbers.
             */
            bool operator > (NetworkMessage &rhsMessageInfo);
            bool operator < (NetworkMessage &rhsMessageInfo);

            static const uint16 FIXED_HEADER_LENGTH = 20;
            static const uint16 MAX_BUF_SIZE = 2048;

        protected:
            NetworkMessage (void);

        private:
            NetworkMessage (const NetworkMessage& nm);
            NetworkMessage (const void *pBuf, uint16 ui16BufSize);

        protected:
            //Fields offsets:
            static const uint16 VERSION_AND_TYPE_FIELD_OFFSET = 0;
            static const uint16 LENGTH_FIELD_OFFSET = 1;
            static const uint16 MSG_TYPE_FIELD_OFFSET = 3;
            static const uint16 SOURCE_ADDRESS_FIELD_OFFSET = 4;
            static const uint16 TARGET_ADDRESS_FIELD_OFFSET = 8;
            static const uint16 SESSION_ID_FIELD_OFFSET = 12;
            static const uint16 MSG_ID_FIELD_OFFSET = 14;
            static const uint16 HOP_COUNT_FIELD_OFFSET = 16;
            static const uint16 TTL_FIELD_OFFSET = 17;
            static const uint16 CHUNK_TYPE_FIELD = 18;
            static const uint16 RELIABLE_FIELD = 19;

            char *_pBuf;
            uint16 _ui16NetMsgLen;     // Size of the complete network message including all the header fields
    };

    inline bool NetworkMessage::operator == (NetworkMessage &rhsMessageInfo)
    {
        return ((getSourceAddr() == rhsMessageInfo.getSourceAddr()) && (getMsgId() == rhsMessageInfo.getMsgId()));
    }

    inline bool NetworkMessage::operator > (NetworkMessage &rhsMessageInfo)
    {
        //return ((getSourceAddr() == rhsMessageInfo.getSourceAddr()) && (getMsgId() > rhsMessageInfo.getSourceAddr()) ? ((getMsgId() - rhsMessageInfo.getSourceAddr()) <= (32768UL)) : ((rhsMessageInfo.getSourceAddr() - getMsgId()) > (32768UL)));
        return ((getSourceAddr() == rhsMessageInfo.getSourceAddr()) && SequentialArithmetic::greaterThan(getMsgId(), rhsMessageInfo.getMsgId()));
    }

    inline bool NetworkMessage::operator < (NetworkMessage &rhsMessageInfo)
    {
        return (rhsMessageInfo > *this);
    }

    inline uint16 NetworkMessage::getLength (void) const
    {
        //return *((uint16*)(_pBuf + LENGTH_FIELD_OFFSET));
        return _ui16NetMsgLen;
    }
}

#endif   // #ifndef INCL_NETWORK_MESSAGE_INTERFACE_H
