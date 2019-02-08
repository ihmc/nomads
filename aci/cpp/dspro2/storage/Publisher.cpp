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
#include "MetadataInterface.h"
#include "MetaData.h"
#include "MetadataHelper.h"
#include "NLFLib.h"

#include "ChunkingManager.h"
#include "MimeUtils.h"

#include "Logger.h"
#include "ConfigManager.h"
#include "BufferWriter.h"
#include "MD5.h"

#define checksumErr Logger::L_Warning, "could not compute checksum\n"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    void * readToBuffer (Reader *pReader, uint32 ui32Len)
    {
        if (pReader == nullptr) {
            checkAndLogMsg ("DSPro - readToBuffer", Logger::L_SevereError,
                "pChunk->pReader is nullptr\n");
            return nullptr;
        }
        void *pBuf = malloc (ui32Len);
        if (pBuf == nullptr) {
            checkAndLogMsg ("DSPro - readToBuffer", memoryExhausted);
            return nullptr;
        }
        if (pReader->readBytes (pBuf, ui32Len) < 0) {
            checkAndLogMsg ("DSPro - readToBuffer", Logger::L_MildError,
                "error reading from pReader\n");
            return nullptr;
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
      _idGen (_nodeId, pszRootGroupName, pDataStore->getPropertyStore()),
      _pDataStore (pDataStore),
      _pInfoStore (pInfoStore)
{
}

Publisher::~Publisher (void)
{
}

int Publisher::init (NOMADSUtil::ConfigManager *pCfgMgr)
{
    return 0;
}

int Publisher::setAndAddMetadata (PublicationInfo &pubInfo, MetadataInterface *pMetadata,
                                  String &msgId, bool bStoreInInfoStore, bool bDisseminate)
{
    const char *pszMethodName = "Publisher::setAndAddMetadata";
    if (pMetadata == nullptr) {
        return -1;
    }

    String refersTo;
    pMetadata->getReferredDataMsgId (refersTo);
    if ((refersTo.length() <= 0) && (pubInfo.pszReferredObjectId == nullptr)) {
        return -2;
    }

    const int64 i64 = getTimeInMilliseconds();
    setFixedMetadataFields (pMetadata, pubInfo.i64ExpirationTime, i64, _nodeId, i64, _nodeId);
    pMetadata->setFieldValue (MetadataInterface::SOURCE, _nodeId); // Force the value of the source
    if ((pubInfo.pszObjectId != nullptr) && (strlen (pubInfo.pszObjectId) > 0)) {
        pMetadata->setFieldValue (MetadataInterface::REFERRED_DATA_OBJECT_ID, pubInfo.pszObjectId);
    }
    if ((pubInfo.pszInstanceId != nullptr) && (strlen (pubInfo.pszInstanceId) > 0)) {
        pMetadata->setFieldValue (MetadataInterface::REFERRED_DATA_INSTANCE_ID, pubInfo.pszInstanceId);
    }
    if ((pubInfo.pszReferredObjectId != nullptr) && (strlen (pubInfo.pszReferredObjectId) > 0) && (refersTo.length() <= 0)) {
        pMetadata->setFieldValue (MetadataInterface::REFERS_TO, pubInfo.pszReferredObjectId);
        if (pubInfo.ui32RefDataSize > 0) {
            char refDataSize[22];
            i64toa (refDataSize, pubInfo.ui32RefDataSize);
            pMetadata->setFieldValue (MetadataInterface::REFERRED_DATA_SIZE, refDataSize);
        }
    }

    pMetadata->getReferredDataMsgId (refersTo);
    if (refersTo.length() <= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "%s attribute not set for metadata %s:%s\n.",
                        MetadataInterface::REFERS_TO, pubInfo.pszObjectId, pubInfo.pszInstanceId);
        return -3;
    }

    BufferWriter bw (1024, 1024);
    if (toBuffer (pMetadata, &bw, true) < 0) {
        return -4;
    }

    char *pszChecksum = MD5Utils::getMD5Checksum (bw.getBuffer(), bw.getBufferLength());
    char *ptmp = _idGen.getMsgId (pubInfo.pszGroupName, bDisseminate);
    if (ptmp != nullptr) {
        msgId = ptmp;
        free (ptmp);
    }
    int rc = addMetadata (msgId, nullptr, nullptr, MetaData::JSON_METADATA_MIME_TYPE, pszChecksum,
                          refersTo, bw.getBuffer(), bw.getBufferLength(), pubInfo.i64ExpirationTime, false);

    if (pszChecksum == nullptr) {
        checkAndLogMsg (pszMethodName, checksumErr);
    }
    else {
        free (pszChecksum);
    }

    if (rc >= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "added annotation for message with objectId <%s> "
                        "and instanceId <%s> into message store with dsproId <%s>, and referring to %s. "
                        "Return code: %d\n", pubInfo.pszObjectId, pubInfo.pszInstanceId, msgId.c_str(),
                        (refersTo.length () <= 0 ? "NULL" : refersTo.c_str()), rc);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not add data. Return code: %d\n", rc);
        return -5;
    }

    pMetadata->setFieldValue (MetaData::MESSAGE_ID, msgId);
    if (bStoreInInfoStore && (rc = _pInfoStore->insert (pMetadata)) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not add metadata. Return code: %d\n", rc);
        return -6;
    }

    return 0;
}

int Publisher::chunkAndAddData (PublicationInfo &pubInfo, const char *pszAnnotatedObjMsgId,
                                const void *pAnnotationMetadata, uint32 ui32AnnotationMetdataLen,
                                const void *pData, uint32 ui32DataLen, const char *pszDataMimeType,
                                char **ppszId, uint8 ui8NChunks, bool bDisseminate)
{
    const char *pszMethodName = "Publisher::chunkAndAddData";
    if ((ui32DataLen == 0) || (ppszId == nullptr)) {
        return -1;
    }
    *ppszId = nullptr;
    if (ui8NChunks > 1) {
        assert (pszAnnotatedObjMsgId == nullptr);
        assert (pAnnotationMetadata == nullptr);
        assert (ui32AnnotationMetdataLen == 0U);
    }

    bool bChunked = (ui8NChunks > 1) && (pszDataMimeType != nullptr);

    // Store the data (and possibly chunk it)
    char *pszLargeObjectId = nullptr;
    int rc = 0;
    if (bChunked) {
        PtrLList<Chunker::Fragment> *pChunks = nullptr;
        if (!_pDataStore->getChunkingManager()->supportsFragmentation (pszDataMimeType)) {
            pChunks = new PtrLList<Chunker::Fragment> (ChunkingAdaptor::getChunkerFragment (pData, ui32DataLen));
        }
        else {
            pChunks = _pDataStore->getChunkingManager()->fragmentBuffer (pData, ui32DataLen, pszDataMimeType, ui8NChunks, pszDataMimeType, 80);
        }
        if (pChunks == nullptr) {
            return -2;
        }

        int iNChunks = pChunks->getCount();
        if (iNChunks < 1) {
            delete pChunks;
            return -3;
        }
        if (iNChunks == 1) {
            bChunked = false;
        }
        else {
            pszLargeObjectId = nullptr;
            addChunkedData (pubInfo, pChunks, pszDataMimeType, bDisseminate, pszLargeObjectId);
        }
        Chunker::Fragment *pNext = pChunks->getFirst();
        for (Chunker::Fragment *pCurr; (pCurr = pNext) != nullptr;) {
            pNext = pChunks->getNext();
            pChunks->remove (pCurr);
            delete pCurr->pReader;
            delete pCurr;
        }
        deallocateAllPtrLListElements<Chunker::Fragment> (pChunks);
        delete pChunks;
    }

    if (!bChunked) { // do not use "else", because bChunked may be set to false in the previus "if" block
        pszLargeObjectId = _idGen.getMsgId (pubInfo.pszGroupName, bDisseminate);
        //pszLargeObjectId = _idGen.chunkId (pubInfo.pszGroupName, bDisseminate);
        if (pszLargeObjectId != nullptr) {
            //pMetadata
            char *pszChecksum = MD5Utils::getMD5Checksum (pData, ui32DataLen);
            rc = addData (pszLargeObjectId, pubInfo.pszObjectId, pubInfo.pszInstanceId,
                          pszAnnotatedObjMsgId, pAnnotationMetadata,
                          ui32AnnotationMetdataLen, pszDataMimeType, pszChecksum,
                          pData, ui32DataLen, pubInfo.i64ExpirationTime);
            if (pszChecksum == nullptr) {
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
                        pszLargeObjectId, pubInfo.pszObjectId == nullptr ? "" : pubInfo.pszObjectId,
                        pubInfo.pszInstanceId == nullptr ? "" : pubInfo.pszInstanceId, rc);
    }

    return (rc < 0 ? -5 : 0);
}

int Publisher::addChunkedData (PublicationInfo &pubInfo, PtrLList<Chunker::Fragment> *pChunks, const char *pszDataMimeType,
                               bool bDisseminate, char *&pszLargeObjectId)
{
    Chunker::Fragment *pCurr, *pNext;
    pNext = pChunks->getFirst();
    pszLargeObjectId = nullptr;
    int rc = 0;
    for (uint8 uiChunkId = MessageHeader::MIN_CHUNK_ID; (pCurr = pNext) != nullptr; uiChunkId++) {
        pNext = pChunks->getNext();
        if (pCurr->ui64FragLen > 0) {
            if (pszLargeObjectId == nullptr) {
                pszLargeObjectId = _idGen.chunkId (pubInfo.pszGroupName, bDisseminate);
            }
            if (pCurr->ui64FragLen > 0xFFFFFFFF) {
                return -4;
            }
            assert (pCurr->ui8Part == uiChunkId);
            pCurr->ui8Part = (uint8) uiChunkId;
            if (addChunkedData (pszLargeObjectId, pubInfo, pCurr, pszDataMimeType) < 0) {
                rc = -5;
            }
        }
    }
    return rc;
}

int Publisher::addChunkedData (const char *pszLargeObjectId, PublicationInfo &pubInfo, Chunker::Fragment *pChunk, const char *pszDataMimeType)
{
    const char *pszMethodName = "Publisher::addChunkedData";
    if (pChunk == nullptr) {
        return -1;
    }
    const uint32 ui32FragLen = (uint32) pChunk->ui64FragLen;
    void *pChunkBuf = readToBuffer (pChunk->pReader, ui32FragLen);
    if (pChunkBuf == nullptr) {
        return -2;
    }
    char *pszChecksum = MD5Utils::getMD5Checksum (pChunkBuf, ui32FragLen);
    // assert (pszAnnotatedObjMsgId == nullptr);
    int rc = addData (pszLargeObjectId, pubInfo.pszObjectId, pubInfo.pszInstanceId, nullptr, nullptr, 0, // annotations cannot be chunked!
                      pszDataMimeType, pszChecksum, pChunkBuf, ui32FragLen,
                      pubInfo.i64ExpirationTime, pChunk->ui8Part, pChunk->ui8TotParts);
    if (pszChecksum == nullptr) {
        checkAndLogMsg (pszMethodName, checksumErr);
    }
    else {
        free (pszChecksum);
    }
    free (pChunkBuf);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not insert chunk "
                        "%d/%d of message %s. Return code %d\n", pChunk->ui8Part, pChunk->ui8TotParts,
                        pszLargeObjectId, rc);
    }
    return rc;
}

int Publisher::addData (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                        const char *pszAnnotatedObjMsgId, const void *pAnnotationMetadata, uint32 ui32AnnotationMetdataLen,
                        const char *pszMimeType, const char *pszChecksum,
                        const void *pBuf, uint32 ui32Len, int64 i64Expiration, uint8 ui8NChunks, uint8 ui8TotNChunks)
{
    if ((pszId == nullptr) || (pBuf == nullptr) || (ui32Len == 0)) {
        return -1;
    }

    int rc = _pDataStore->insert (pszId, pszObjectId, pszInstanceId, pszAnnotatedObjMsgId,
                                  pAnnotationMetadata, ui32AnnotationMetdataLen, pszMimeType,
                                  pszChecksum, nullptr, pBuf, ui32Len, false, i64Expiration,
                                  ui8NChunks, ui8TotNChunks);
    if (rc < 0) {
        checkAndLogMsg ("Publisher::addData", Logger::L_Warning,
                        "could not insert data. Return code: %d\n", rc);
    }
    return (rc < 0 ? -2 : 0);
}

char * Publisher::assignIdAndAddMetadata (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                          const char *pszMimeType, const char *pszChecksum, const char *pszReferredObjectId,
                                          const void *pBuf, uint32 ui32Len, int64 i64Expiration, bool bDisseminate)
{
    const char *pszMethodName = "Publisher::assignIdAndAddMetadata";
    char *pszId = _idGen.getMsgId (pszGroupName, bDisseminate);
    int rc = addMetadata (pszId, pszObjectId, pszInstanceId, pszMimeType, pszChecksum,
                          pszReferredObjectId, pBuf, ui32Len, i64Expiration, bDisseminate);
    if (0 == rc) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Added metadata with id %s\n", pszId);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Could not add metadata. Returned code: %d\n", rc);
    }
    return pszId;
}

int Publisher::addMetadata (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                            const char *pszMimeType, const char *pszChecksum, const char *pszReferredObjectId,
                            const void *pBuf, uint32 ui32Len, int64 i64Expiration, bool bDSProMetadata)
{
    if ((pszId == nullptr) || (pBuf == nullptr) || (ui32Len == 0)) {
        return -1;
    }

    int rc = _pDataStore->insert (pszId, pszObjectId, pszInstanceId, nullptr, nullptr, 0U,
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
      pszReferredObjectId (nullptr)
{
}

Publisher::PublicationInfo::PublicationInfo (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId, int64 i64ExpTime)
    : ui32RefDataSize (0U), i64ExpirationTime (i64ExpTime),
      pszGroupName (pszGroupName),
      pszObjectId (pszObjectId),
      pszInstanceId (pszInstanceId),
      pszReferredObjectId (nullptr)
{
}

Publisher::PublicationInfo::~PublicationInfo (void)
{
}

