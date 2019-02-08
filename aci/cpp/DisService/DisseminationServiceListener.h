/*
 * DisseminationServiceListener.h
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

#ifndef INCL_DISSEMINATION_SERVICE_LISTENER_H
#define INCL_DISSEMINATION_SERVICE_LISTENER_H

#include "FTypes.h"

namespace IHMC_ACI
{
    class DisseminationServiceListener
    {
        public:
            DisseminationServiceListener (void);
            virtual ~DisseminationServiceListener (void);

            /**
            * Callback function that is invoked when new data arrives.
            *
            * ui16ClientId - the client id that was used when the callback was registered.
            *
            * pszSender - id of the sender.
            *
            * pszGroupName - the group to which the message was addressed.
            *
            * pszObjectId - an application-defined id that uniquely identify the object.
            *
            * pszInstanceId - an application-defined id that uniquely identify the version of the object.
            *
            * pszMimeType - the MIME type of the data/chunk.
            *
            * pData - the data and/or metadata that was received - depending on which callback was invoked.
            *
            * ui32Length - the length of the whole data in bytes (it includes the
            *              metadata part too.)
            *
            * ui8NChunks - the total number of chunks that the data has been fragmented
            *
            * ui32MetadataLength - the length of the metadata in bytes.
            *
            * ui16HistoryWindow - the number of messages previously transmitted that are recommended to be
            *                     retrieved before processing the current message.
            *
            * ui16Tag - the tag that was specified for the message.
            *
            * ui8Priority - the priority value that was specified for the message
            *
            * pszQueryId - if the data was delivered as result of a query, this parameter will contain
            *              the ID of the query that matched the data.
            *
            */
            virtual bool dataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                      uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                      const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                                      const void *pData, uint32 ui32Length, uint32 ui32MetadataLength,
                                      uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId) = 0;

            virtual bool chunkArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                       uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                       const char *pszMimeType, const void *pChunk, uint32 ui32Length,
                                       uint8 ui8NChunks, uint8 ui8TotNChunks, const char *pszChunkedMsgId,
                                       uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId) = 0;

            /**
            * Callback function that is invoked when new metadata arrives.
            *
            * ui16ClientId - the client id that was used when the callback was registered.
            *
            * pszSender - id of the sender.
            *
            * pszGroupName - the group to which the message was addressed.
            *
            * ui32SeqId - the sequence id of the message.
            *
            * pszObjectId - an application-defined id that uniquely identify the object.
            *
            * pszInstanceId - an application-defined id that uniquely identify the version of the object.
            *
            * pszDataMimeType - the MIME type of the referred data.
            *
            * pMetadata - the metadata that was received - depending on which callback was invoked.
            *
            * ui32MetadataLength - the length of the metadata in bytes.
            *
            * ui16HistoryWindow - the number of messages previously transmitted that are recommended to be
            *                     retrieved before processing the current message.ge.
            *
            * ui16Tag - the tag that was specified for the message.
            *
            * ui8Priority - the priority value that was specified for the message.
            *
            * pszQueryId - if the metadata was delivered as result of a query, this parameter will contain
            *              the ID of the query that matched the matadata.
            */
            virtual bool metadataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                          uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                          const char *pszDataMimeType, const void *pMetadata, uint32 ui32MetadataLength,
                                          bool bDataChunked, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId) = 0;

            /**
            * Callback function that is invoked when new metadata arrives and the
            * actual data is available to be retrieved.
            *
            * ui16ClientId - the client id that was used when the callback was registered.
            *
            * pszSender - id of the sender.
            *
            * pszGroupName - the group to which the message was addressed.
            *
            * ui32SeqId - the sequence id of the message.
            *
            * pszObjectId - an application-defined id that uniquely identify the object.
            *
            * pszInstanceId - an application-defined id that uniquely identify the version of the object.
            *
            * pszDataMimeType - the MIME type of the referred data.
            *
            * pszRefObjId - the id of the referenced object.
            *
            * pMetadata - the metadata that was received - depending on which callback was invoked.
            *
            * ui32MetadataLength - the length of the metadata in bytes.
            *
            * ui16HistoryWindow - the number of messages previously transmitted that are recommended to be
            *                     retrieved before processing the current message.
            *
            * ui16Tag - the tag that was specified for the message.
            *
            * ui8Priority - the priority value that was specified for the message.
            *
            * pszQueryId - if the metadata was delivered as result of a query, this parameter will contain
            *              the ID of the query that matched the metadata.
            */
            virtual bool dataAvailable (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                        uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                        const char *pszMimeType, const char *pszRefObjId, const void *pMetadata,
                                        uint32 ui32MetadataLength, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId) = 0;
    };

    inline DisseminationServiceListener::DisseminationServiceListener()
    {
    }

    inline DisseminationServiceListener::~DisseminationServiceListener()
    {
    }
}

#endif   // #ifndef INCL_DISSEMINATION_SERVICE_LISTENER_H
