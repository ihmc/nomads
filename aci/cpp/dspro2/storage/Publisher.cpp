/**
 * Publisher.cpp
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on July 1, 2015, 5:01 PM
 */

#include "Publisher.h"

#include "Defs.h"
#include "DataStore.h"
#include "InformationStore.h"
#include "MetaData.h"
#include "MetadataConfiguration.h"
#include "NLFLib.h"
#include "SQLAVList.h"

#include "Chunker.h"
#include "MimeUtils.h"

#include "Logger.h"
#include "ConfigManager.h"
#include "BufferWriter.h"
#include "MD5.h"

#define checksumErr Logger::L_Warning, "could not compute checksum\n"

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    void * readToBuffer (Reader *pReader, uint32 ui32Len)
    {
        if (pReader == NULL) {
            checkAndLogMsg ("DSPro - readToBuffer", Logger::L_SevereError,
                "pChunk->pReader is NULL\n");
            return NULL;
        }
        void *pBuf = malloc (ui32Len);
        if (pBuf == NULL) {
            checkAndLogMsg ("DSPro - readToBuffer", memoryExhausted);
            return NULL;
        }
        if (pReader->readBytes (pBuf, ui32Len) < 0) {
            checkAndLogMsg ("DSPro - readToBuffer", Logger::L_MildError,
                "error reading from pReader\n");
            return NULL;
        }
        return pBuf;
    }
}

ThreadSafePublisher::ThreadSafePublisher (void)
{
}

ThreadSafePublisher::~ThreadSafePublisher (void)
{
}

Publisher::Publisher (const char *pszNodeId, const char *pszRootGroupName, InformationStore *pInfoStore, DataStore *pDataStore)
    : _nodeId (pszNodeId),
      _idGen (_nodeId, pszRootGroupName, pDataStore->getPropertyStore ()),
      _pDataStore (pDataStore),
      _pInfoStore (pInfoStore)
{
}

Publisher::~Publisher (void)
{
}

int Publisher::init (NOMADSUtil::ConfigManager *pCfgMgr)
{
    if (_chunkingConf.init (pCfgMgr) < 0) {
        return -1;
    }
    return 0;
}

int Publisher::setAndAddMetadata (PublicationInfo &pubInfo, MetadataInterface *pMetadata,
                                  String &msgId, bool bStoreInInfoStore)
{
    const char *pszMethodName = "Publisher::setAndAddMetadata";
    if (pMetadata == NULL) {
        return -1;
    }

    if (pMetadata->isFieldUnknown (MetaData::REFERS_TO) && (pubInfo.pszReferredObjectId == NULL)) {
        return -2;
    }

    const int64 i64 = getTimeInMilliseconds();
    pMetadata->setFixedMetadataFields (pubInfo.i64ExpirationTime, i64, _nodeId, i64, _nodeId);
    pMetadata->setFieldValue (MetadataInterface::SOURCE, _nodeId); // Force the value of the source
    if ((pubInfo.pszObjectId != NULL) && (strlen (pubInfo.pszObjectId) > 0)) {
        pMetadata->setFieldValue (MetadataInterface::REFERRED_DATA_OBJECT_ID, pubInfo.pszObjectId);
    }
    if ((pubInfo.pszInstanceId != NULL) && (strlen (pubInfo.pszInstanceId) > 0)) {
        pMetadata->setFieldValue (MetadataInterface::REFERRED_DATA_INSTANCE_ID, pubInfo.pszInstanceId);
    }
    if ((pubInfo.pszReferredObjectId != NULL) && (strlen (pubInfo.pszReferredObjectId) > 0) && pMetadata->isFieldUnknown (MetadataInterface::REFERS_TO)) {
        pMetadata->setFieldValue (MetadataInterface::REFERS_TO, pubInfo.pszReferredObjectId);
        if (pubInfo.ui32RefDataSize > 0) {
            char refDataSize[22];
            i64toa (refDataSize, pubInfo.ui32RefDataSize);
            pMetadata->setFieldValue (MetadataInterface::REFERRED_DATA_SIZE, refDataSize);
        }
    }
    if (pMetadata->isFieldUnknown (MetadataInterface::REFERS_TO)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "%s attribute not set for metadata %s:%s\n.",
                        MetadataInterface::REFERS_TO, pubInfo.pszObjectId, pubInfo.pszInstanceId);
        return -3;
    }

    String refersTo;
    pMetadata->getFieldValue (MetaData::REFERS_TO, refersTo);
    BufferWriter bw (1024, 1024);

    switch (pMetadata->getMetadataType()) {

        case MetadataInterface::List: {
            // TODO: this conversion may not be very efficient.  Find way to avoid it
            SQLAVList *pavlist = static_cast<SQLAVList*>(pMetadata);
            MetaData *ptmp = MetadataConfiguration::getConfiguration()->createNewMetadata (pavlist);
            ptmp->write (&bw, 0);
            delete ptmp;
            break;
        }

        case MetadataInterface::Meta: {
            static_cast<MetaData*>(pMetadata)->write (&bw, 0);
            break;
        }

        default: {
            assert (false);
            return -3;
        }
    }

    char *pszChecksum = MD5Utils::getMD5Checksum (bw.getBuffer(), bw.getBufferLength());
    msgId = _idGen.getMsgId (pubInfo.pszGroupName);
    int rc = addMetadata (msgId, NULL, NULL, MetaData::XML_METADATA_MIME_TYPE, pszChecksum,
                          refersTo, bw.getBuffer(), bw.getBufferLength(), pubInfo.i64ExpirationTime);

    if (pszChecksum == NULL) {
        checkAndLogMsg (pszMethodName, checksumErr);
    }
    else {
        free (pszChecksum);
        pszChecksum = NULL;
    }

    if (rc >= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "added annotation for message with objectId <%s> "
                        "and instanceId <%s> into message store with dsproId <%s>, and referring to %s. "
                        "Return code: %d\n", pubInfo.pszObjectId, pubInfo.pszInstanceId, msgId.c_str(),
                        (refersTo.length () <= 0 ? "NULL" : refersTo.c_str ()), rc);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not add data. Return code: %d\n", rc);
        return -3;
    }

    pMetadata->setFieldValue (MetaData::MESSAGE_ID, msgId);
    if (bStoreInInfoStore && (rc = _pInfoStore->insertIntoDB (pMetadata)) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not add metadata. Return code: %d\n", rc);
        return -4;
    }

    return 0;
}

int Publisher::chunkAndAddData (PublicationInfo &pubInfo, const char *pszAnnotatedObjMsgId,
                                const void *pAnnotationMetadata, uint32 ui32AnnotationMetdataLen,
                                const void *pData, uint32 ui32DataLen, const char *pszDataMimeType,
                                char **ppszId, bool bDoNotChunk)
{
    const char *pszMethodName = "Publisher::chunkAndAddData";
    if ((ui32DataLen == 0) || (ppszId == NULL)) {
        return -1;
    }
    *ppszId = NULL;
    if (!bDoNotChunk) {
        assert (pszAnnotatedObjMsgId == NULL);
        assert (pAnnotationMetadata == NULL);
        assert (ui32AnnotationMetdataLen == 0U);
    }

    bool bChunked = (!bDoNotChunk) && (pszDataMimeType != NULL);

    // Store the data (and possibly chunk it)
    char *pszLargeObjectId = NULL;
    int rc = 0;
    if (bChunked) {
        PtrLList<Chunker::Fragment> *pChunks = NULL;
        if (MimeUtils::mimeTypeToFragmentType (pszDataMimeType) == Chunker::UNSUPPORTED) {
            pChunks = new PtrLList<Chunker::Fragment> (ChunkingAdaptor::getChunkerFragment (pData, ui32DataLen));
        }
        else {
            const uint8 ui8NChunks = _chunkingConf.getNumberofChunks (ui32DataLen);
            pChunks = Chunker::fragmentBuffer (pData, ui32DataLen,
                                               MimeUtils::mimeTypeToFragmentType (pszDataMimeType), ui8NChunks,
                                               MimeUtils::mimeTypeToFragmentType (pszDataMimeType), 80);
        }
        if (pChunks == NULL) {
            return -2;
        }

        int iNChunks = pChunks->getCount();
        if (iNChunks < 1) {
            delete pChunks;
            return -3;
        }
        if (iNChunks == 1) {
            delete pChunks->getFirst();
            delete pChunks;
            bChunked = false;
        }
        else {
            Chunker::Fragment *pCurr, *pNext;
            pNext = pChunks->getFirst();
            pszLargeObjectId = NULL;
            for (uint8 uiChunkId = MessageHeader::MIN_CHUNK_ID; (pCurr = pNext) != NULL; uiChunkId++) {
                pNext = pChunks->getNext();
                if (pCurr->ui64FragLen > 0) {
                    if (pszLargeObjectId == NULL) {
                        pszLargeObjectId = _idGen.chunkId (pubInfo.pszGroupName);
                    }
                    if (pCurr->ui64FragLen > 0xFFFFFFFF) {
                        return -4;
                    }
                    void *pChunkBuf = readToBuffer (pCurr->pReader, (uint32)pCurr->ui64FragLen);
                    if (pChunkBuf != NULL) {
                        char *pszChecksum = MD5Utils::getMD5Checksum (pChunkBuf, (uint32)pCurr->ui64FragLen);
                        assert (pszAnnotatedObjMsgId == NULL);
                        rc = addData (pszLargeObjectId, pubInfo.pszObjectId, pubInfo.pszInstanceId, NULL, NULL, 0, // annotations cannot be chunked!
                                      pszDataMimeType, pszChecksum, pChunkBuf, (uint32) pCurr->ui64FragLen,
                                      pubInfo.i64ExpirationTime, uiChunkId, iNChunks);
                        if (pszChecksum == NULL) {
                            checkAndLogMsg (pszMethodName, checksumErr);
                        }
                        else {
                            free (pszChecksum);
                        }
                        if (rc < 0) {
                            checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not insert chunk "
                                            "%d/%d of message %s. Return code %d\n", uiChunkId, iNChunks,
                                            pszLargeObjectId, rc);
                        }
                    }
                }
                delete pChunks->remove (pCurr);
            }
        }
    }

    if (!bChunked) { // do not use "else", because bChunked may be set to false in the previus "if" block
        pszLargeObjectId = _idGen.getMsgId (pubInfo.pszGroupName);
        if (pszLargeObjectId != NULL) {
            //pMetadata
            char *pszChecksum = MD5Utils::getMD5Checksum (pData, ui32DataLen);
            rc = addData (pszLargeObjectId, pubInfo.pszObjectId, pubInfo.pszInstanceId,
                          pszAnnotatedObjMsgId, pAnnotationMetadata,
                          ui32AnnotationMetdataLen, pszDataMimeType, pszChecksum,
                          pData, ui32DataLen, pubInfo.i64ExpirationTime);
            if (pszChecksum == NULL) {
                checkAndLogMsg (pszMethodName, checksumErr);
            }
            else {
                free (pszChecksum);
            }
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not insert message %s. "
                                "Return code %d\n", pszLargeObjectId, rc);
            }
        }
    }

    *ppszId = pszLargeObjectId;
    if (rc >= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "%s message with dspro id <%s>, "
                        "object id <%s>, instance id <%s>. Return code: %d\n",
                        bChunked ? "chunked and added" : "added",
                        pszLargeObjectId, pubInfo.pszObjectId == NULL ? "" : pubInfo.pszObjectId,
                        pubInfo.pszInstanceId == NULL ? "" : pubInfo.pszInstanceId, rc);
    }
    return (rc < 0 ? -5 : 0);
}

int Publisher::addData (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                        const char *pszAnnotatedObjMsgId, const void *pAnnotationMetadata, uint32 ui32AnnotationMetdataLen, 
                        const char *pszMimeType, const char *pszChecksum,
                        const void *pBuf, uint32 ui32Len, int64 i64Expiration, uint8 ui8NChunks, uint8 ui8TotNChunks)
{
    if ((pszId == NULL) || (pBuf == NULL) || (ui32Len == 0)) {
        return -1;
    }

    int rc = _pDataStore->insert (pszId, pszObjectId, pszInstanceId, pszAnnotatedObjMsgId,
                                  pAnnotationMetadata, ui32AnnotationMetdataLen, pszMimeType,
                                  pszChecksum, NULL, pBuf, ui32Len, false, i64Expiration,
                                  ui8NChunks, ui8TotNChunks);
    if (rc < 0) {
        checkAndLogMsg ("Publisher::addData", Logger::L_Warning,
                        "could not insert data. Return code: %d\n", rc);
    }
    return (rc < 0 ? -2 : 0);
}

int Publisher::addMetadata (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                            const char *pszMimeType, const char *pszChecksum, const char *pszReferredObjectId,
                            const void *pBuf, uint32 ui32Len, int64 i64Expiration)
{
    if ((pszId == NULL) || (pBuf == NULL) || (ui32Len == 0)) {
        return -1;
    }

    int rc = _pDataStore->insert (pszId, pszObjectId, pszInstanceId, NULL, NULL, 0U,
                                  pszMimeType, pszChecksum, pszReferredObjectId, pBuf,
                                  ui32Len, true, i64Expiration, 0, 0);
    if (rc < 0) {
        checkAndLogMsg ("Publisher::addMetadata", Logger::L_Warning,
                        "could not insert data. Return code: %d\n", rc);
    }
    return (rc < 0 ? -2 : 0);
}

Publisher::PublicationInfo::PublicationInfo (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId)
    : ui32RefDataSize (0U), i64ExpirationTime (0UL),
      pszGroupName (pszGroupName),
      pszObjectId (pszObjectId),
      pszInstanceId (pszInstanceId),
      pszReferredObjectId (NULL)
{
}

Publisher::PublicationInfo::PublicationInfo (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId, int64 i64ExpTime)
    : ui32RefDataSize (0U), i64ExpirationTime (i64ExpTime),
      pszGroupName (pszGroupName),
      pszObjectId (pszObjectId),
      pszInstanceId (pszInstanceId),
      pszReferredObjectId (NULL)
{
}

Publisher::PublicationInfo::~PublicationInfo (void)
{
}

