/*
 * MessageInfo.h
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

#ifndef INCL_MESSAGE_INFO_H
#define INCL_MESSAGE_INFO_H

#include "FTypes.h"
#include "StrClass.h"

#include "BufferReader.h"

#include <stdio.h>

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    //--------------------------------------------------------------------------
    // MessageHeader
    //--------------------------------------------------------------------------

    class MessageHeader
    {
        public:
            static const NOMADSUtil::String MSG_ID_SEPARATOR;
            static const int64 NO_EXPIRATION;
            static const uint8 UNDEFINED_CHUNK_ID;
            static const uint8 MIN_CHUNK_ID;
            static const uint8 MAX_CHUNK_ID;
            static const NOMADSUtil::String DEFAULT_MIME_TYPE;

            enum {
                DEFAULT_MIN_PRIORITY = 0,
                DEFAULT_AVG_PRIORITY = 5,
                DEFAULT_MAX_PRIORITY = 10
            };

            enum Type {
                MessageInfo = 0x00,
                ChunkMessageInfo = 0x01
            };

            virtual ~MessageHeader (void);

            /**
             * Returns the ID of the message in the form
             * groupName:publisherNodeId:msgSeqId:chunkId:fragmentOffset:fragmentLength
             */
            const char * getMsgId (void) const;

            /**
             * Returns the ID of the complete message (of which this might be a fragment)
             * The returned ID will include the chunk id (or 0), but will not include fragment offset and fragment length
             * Caller is responsible for freeing this string
             */
            char * getIdForCompleteMsg (void) const;

            char * getLargeObjectId (void);

            bool getAcknowledgment (void);
            const char * getAnnotates (void) const;
            const void * getAnnotationMetadata (uint32 &ui32BufLen) const;
            const char * getGroupName (void) const;
            const char * getPublisherNodeId (void) const;
            const char * getObjectId (void) const;
            const char * getInstanceId (void) const;
            uint32 getMsgSeqId (void) const;
            uint16 getClientId (void) const;
            uint8 getClientType (void) const;
            const char * getMimeType (void) const;
            const char * getChecksum (void) const;
            uint8 getChunkId (void) const;
            uint32 getFragmentOffset (void) const;
            uint32 getFragmentLength (void) const;
            uint32 getTotalMessageLength (void) const;
            uint16 getTag (void) const;
            uint16 getHistoryWindow (void) const;
            uint8 getPriority (void) const;
            int64 getExpiration (void) const;

            void setAcknowledgment (bool bAcknoledgement);
            void setAnnotates (const char *pszAnnotatdObjMsgId);
            // it makes a copy of the annotation metadata
            int setAnnotationMetadata (const void *pBuf, uint32 ui32BufLen);
            void setFragmentOffset (uint32 ui32FragmentOffset);
            void setFragmentLength (uint32 ui32FragmentLength);
            void setMsgSeqId (uint32 ui32MsgSeqId);
            void setPriority (uint8 ui8Priority);
            void setTotalMessageLength (uint32 ui32TotalMessageLength);
            virtual uint8 getTotalNumberOfChunks (void) const = 0;

            bool isCompleteMessage (void) const;
            bool isChunk (void) const;

            bool operator == (MessageHeader &rhsMessageHeader);

            virtual int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            virtual int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            virtual MessageHeader * clone (void) = 0;

        protected:
            MessageHeader (void);
            /**
             * NB: MessageHeader makes a copy of pszGroupName and pszPublisherNodeId
             */
            MessageHeader (Type type, const char *pszGroupName,
                           const char *pszPublisherNodeId, uint32 ui32SeqId, uint8 ui8ChunkId,
                           const char *pszObjectId, const char *pszInstanceId,
                           uint16 ui16Tag, uint16 ui16ClientId, uint8 ui8ClientType,
                           const char *pszMimeType, const char *pszChecksum,
                           uint32 ui32FragmentOffset, uint32 ui32FragmentLength,
                           uint32 ui32TotalMessageLength, uint16 ui16HistoryWindow,
                           uint8 ui8Priority, int64 i64Expiration, bool bAcknowledgment);

        protected:
            /*
             * NOTE: generateMsgId method MUST be implemented and called in the
             * child constructor as well as in the following methods:
             * 1) setFragmentOffset (uint32 ui32FragmentOffset);
             * 2) setFragmentLength (uint32 ui32FragmentLenght);
             * 3) setMsgSeqId (uint32 ui32SeqId);
             */
            void generateLargeMsgId (NOMADSUtil::String &id);
            void generateMsgId (void);

            NOMADSUtil::String _sGroupName;
            NOMADSUtil::String _sPublisherNodeId;
            NOMADSUtil::String _sMsgId;
            NOMADSUtil::String _objectId;
            NOMADSUtil::String _instanceId;
            NOMADSUtil::String _mimeType;
            NOMADSUtil::String _checksum;
            NOMADSUtil::String _annotatedObjMsgId;
            NOMADSUtil::BufferReader _annotationMetadata;

            uint8 _ui8ChunkId;
            uint32 _ui32SeqId;
            uint32 _ui32FragmentOffset;
            uint32 _ui32FragmentLength;

        private:
            Type _type;
            bool _bAcknowledgment;

            uint8 _ui8ClientType;
            uint8 _ui8Priority;
            uint16 _ui16Tag;
            uint16 _ui16ClientId;
            uint16 _ui16HistoryWindow;
            uint32 _ui32TotalMessageLength;
            int64 _i64Expiration;
    };

    //--------------------------------------------------------------------------
    // MessageInfo
    //--------------------------------------------------------------------------

    class MessageInfo : public MessageHeader
    {
        public:
            MessageInfo (void);
            MessageInfo (bool bMetaData);

            /**
             * NB: MessageInfo makes a copy of pszGroupName and pszPublisherNodeId
             */
            MessageInfo (const char *pszGroupName, const char *pszPublisherNodeId,
                         uint32 ui32SeqId, const char *pszObjectId,
                         const char *pszInstanceId, uint16 ui16Tag, uint16 ui16ClientId,
                         uint8 ui8ClientType, const char *pszMimeType,
                         const char *pszChecksum, uint32 ui32TotalMessageLength,
                         uint32 ui32FragmentLength, uint32 ui32FragmentOffest,
                         uint32 ui32MetaDataLength=0, uint16 ui16HistoryWindow=0,
                         uint8 ui8Priority=DEFAULT_AVG_PRIORITY, int64 i64Expiration=0,
                         bool bAcknowledgment=false, bool bMetaData=false,
                         const char *pszReferredObjectId = NULL);

            virtual ~MessageInfo (void);

            MessageInfo * clone (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            uint32 getMetaDataLength (void) const;
            const char * getReferredObject (void);

            void setReferredObject (const char *pszMessageId);
            void setMetaData (bool bMetaData);

            bool isMetaData (void) const;
            bool isMetaDataWrappedInData (void) const;

            uint8 getTotalNumberOfChunks (void) const;

        private:
            // Masks to read and write MessageInfo flags
            enum BinaryFlags {
                WRITE_ACK = 0x01,
                WRITE_META = 0x02,
                WRITE_SUBSTATE = 0x04,
            };

            enum ReadingMask {
                READ_ACK = 0xFE,
                READ_META = 0xFD,
            };

            // Prints each field of the MessageInfo.
            void display (FILE *pFileOut);

        private:
            uint32 _ui32MetaDataLength;

            bool _bMetaData;

            NOMADSUtil::String _sRefObj;
    };

    //--------------------------------------------------------------------------
    // ChunkMsgInfo
    //--------------------------------------------------------------------------

    class ChunkMsgInfo : public MessageHeader
    {
        public:
            ChunkMsgInfo (void);
            ChunkMsgInfo (const char *pszGroupName, const char *pszPublisherNodeId,
                          uint32 ui32SeqId, uint8 ui8ChunkId, const char *pszObjectId,
                          const char *pszInstanceId, uint16 ui16Tag, uint16 ui16ClientId,
                          uint8 ui8ClientType, const char *pszMimeType, const char *pszChecksum,
                          uint32 ui32FragmentOffset, uint32 ui32FragmentLength,
                          uint32 ui32TotalMessageLength, uint8 ui8TotalNumOfChunks,
                          uint16 ui16HistoryWindow=0, uint8 ui8Priority=DEFAULT_AVG_PRIORITY,
                          int64 i64Expiration=0, bool bAcknowledgment=false);

            virtual ~ChunkMsgInfo (void);

            uint8 getTotalNumberOfChunks (void) const;

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            ChunkMsgInfo * clone (void);

            void setFragmentOffset (uint32 ui32FragmentOffset);
            void setFragmentLength (uint32 ui32FragmentLength);

        private:
            uint8 _ui8TotalNumOfChunks;
    };

    //--------------------------------------------------------------------------
    // MessageHeaderHelper
    //--------------------------------------------------------------------------

    class MessageHeaderHelper
    {
        public:
            static MessageHeader * getMessageHeader (const char *pszGroupName, const char *pszPublisherNodeId,
                                                     uint32 ui32SeqId, uint8 ui8ChunkId, const char *pszObjectId,
                                                     const char *pszInstanceId, uint16 ui16Tag, uint16 ui16ClientId,
                                                     uint8 ui8ClientType, const char *pszMimeType, const char *pszChecksum,
                                                     uint32 ui32TotalMessageLength, uint32 ui32FragmentOffest,
                                                     uint32 ui32FragmentLength, uint32 ui32MetaDataLength, uint16 ui16HistoryWindow,
                                                     uint8 ui8Priority, int64 i64Expiration, bool bAcknowledgment, bool bMetaData,
                                                     uint8 ui8TotalNumOfChunks, const char *pszReferredObjectId=NULL);
    };

    // ---- MessageHeader ------------------------------------------------------

    inline bool MessageHeader::getAcknowledgment (void)
    {
        return _bAcknowledgment;
    }

    inline uint8 MessageHeader::getChunkId (void) const
    {
        return _ui8ChunkId;
    }

    inline void MessageHeader::setPriority(uint8 ui8Priority)
    {
        _ui8Priority = ui8Priority;
    }

    inline void MessageHeader::setTotalMessageLength (uint32 ui32TotalMessageLength)
    {
        _ui32TotalMessageLength = ui32TotalMessageLength;
    }

    inline uint16 MessageHeader::getHistoryWindow (void) const
    {
        return _ui16HistoryWindow;
    }

    inline uint8 MessageHeader::getPriority (void) const
    {
        return _ui8Priority;
    }

    inline int64 MessageHeader::getExpiration (void) const
    {
        return _i64Expiration;
    }

    inline bool MessageHeader::isChunk (void) const
    {
        return (_type == ChunkMessageInfo);
    }

    inline void MessageHeader::setAcknowledgment (bool bAcknoledgement)
    {
        _bAcknowledgment = bAcknoledgement;
    }

    // ---- MessageInfo --------------------------------------------------------

    inline uint32 MessageInfo::getMetaDataLength (void) const
    {
        return _ui32MetaDataLength;
    }

    inline void MessageInfo::setMetaData (bool bMetaData)
    {
        _bMetaData = bMetaData;
    }

    inline bool MessageInfo::isMetaData (void) const
    {
        return _bMetaData;
    }

    inline bool MessageInfo::isMetaDataWrappedInData (void) const
    {
        return ((_ui32FragmentOffset == 0) &&
                (_ui32FragmentLength == _ui32MetaDataLength) &&
                (_ui32MetaDataLength < getTotalMessageLength()));
    }

    inline uint8 MessageInfo::getTotalNumberOfChunks (void) const
    {
        return 1;
    }

    // ---- ChunkMsgInfo -------------------------------------------------------

    inline uint8 ChunkMsgInfo::getTotalNumberOfChunks (void) const
    {
        return _ui8TotalNumOfChunks;
    }
}

#endif   // #ifndef INCL_MESSAGE_INFO_H

