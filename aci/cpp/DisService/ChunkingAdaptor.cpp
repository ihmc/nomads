/*
 * ChunkingAdaptor.cpp
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

#include "ChunkingAdaptor.h"

#include "DisServiceDefs.h"

#include "MessageInfo.h"

#include "ChunkingUtils.h"

#include "BufferReader.h"
#include "Logger.h"
#include "ChunkingManager.h"

#include "ConfigManager.h"
#include "Writer.h"
#include "MimeUtils.h"

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

//------------------------------------------------------------------------------
// Range
//------------------------------------------------------------------------------

ChunkingConfiguration::Range::Range (uint64 ui64Bytes, uint8 ui8NChunks)
    : _ui8NChunks (ui8NChunks), _ui64Bytes (ui64Bytes)
{
}

ChunkingConfiguration::Range::~Range (void)
{
}

bool ChunkingConfiguration::Range::operator > (const Range &range) const
{
    return (_ui64Bytes > range._ui64Bytes);
}

bool ChunkingConfiguration::Range::operator < (const Range &range) const
{
   return (_ui64Bytes < range._ui64Bytes);
}

bool ChunkingConfiguration::Range::operator == (const Range& range) const
{
    return (_ui64Bytes == range._ui64Bytes);
}

//------------------------------------------------------------------------------
// ChunkingConfiguration
//------------------------------------------------------------------------------

ChunkingConfiguration::ChunkingConfiguration (void)
    : _sizeToNchunks (false)
{
}

ChunkingConfiguration::~ChunkingConfiguration (void)
{
}

int ChunkingConfiguration::init (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return -1;
    }
    StringStringHashtable *pBytesToChunks = StringStringHashtable::parseStringStringHashtable (pCfgMgr->getValue ("aci.disService.numberOfChunks"));
    if (pBytesToChunks == NULL) {
        return 0;
    }
    StringStringHashtable::Iterator iter = pBytesToChunks->getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        uint64 ui64Bytes = atoui64 (iter.getKey());
        uint32 ui32NChunks = atoui32 (iter.getValue());
        if (ui32NChunks <= 0xFF) {
            ChunkingConfiguration::Range *pRange = new ChunkingConfiguration::Range (ui64Bytes, (uint8) ui32NChunks);
            if (_sizeToNchunks.search (pRange) == NULL) {
                checkAndLogMsg ("ChunkingConfiguration::init", Logger::L_Info, "setting number of chunks for data of "
                                " %lld bytes to %d.\n", pRange->_ui64Bytes, (int) pRange->_ui8NChunks);
                _sizeToNchunks.insert (pRange);
            }
            else {
                checkAndLogMsg ("ChunkingConfiguration::init", Logger::L_Warning, "trying to "
                                "add duplicate range for files of %lld bytes\n", ui64Bytes);
            }
        }
    }
    return 0;
}

uint8 ChunkingConfiguration::getNumberofChunks (uint64 ui64Bytes)
{
    uint8 ui8NChunks = 4;
    Range *pRange = _sizeToNchunks.getFirst();
    while ((pRange != NULL) && (ui64Bytes > pRange->_ui64Bytes)) {
        ui8NChunks = pRange->_ui8NChunks;
        pRange = _sizeToNchunks.getNext();
    }
    return ui8NChunks;
}

//------------------------------------------------------------------------------
// ChunkingAdaptor
//------------------------------------------------------------------------------

Reader * ChunkingAdaptor::reassemble (ChunkingManager *pChunkingMgr, PtrLList<Message> *pChunks, PtrLList<Message> *pAnnotations, uint32 &ui32LargeObjLen)
{
    const char *pszMethodName = "ChunkingAdaptor::reassemble";
    ui32LargeObjLen = 0U;
    if ((pChunkingMgr == NULL) || (pChunks == NULL)) {
        return NULL;
    }
    Message *pMsg = pChunks->getFirst();
    if (pMsg == NULL) {
        return NULL;
    }

    if ((!pMsg->getMessageHeader()->isChunk()) || (pMsg->getChunkMsgInfo()->getTotalNumberOfChunks() < 2)) {
        // It is not a chunk - just a complete message
        void *pData = malloc (pMsg->getMessageHeader()->getFragmentLength());
        if (pData == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "failed to allocate memory to copy retrieved object of size %lu\n",
                            pMsg->getMessageHeader()->getFragmentLength());
            return NULL;
        }
        memcpy (pData, pMsg->getData(), pMsg->getMessageHeader()->getFragmentLength());

        BufferReader *pReader = new BufferReader (pData, pMsg->getMessageHeader()->getFragmentLength());
        ui32LargeObjLen = pMsg->getMessageHeader()->getFragmentLength();
        return pReader;
    }

    // This is a fragment - so use the reassembler
    ChunkMsgInfo *pCMI = pMsg->getChunkMsgInfo();
    if (pCMI == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "ChunkMsgInfo is NULL\n");
        return NULL;
    }

    const String mimeType (pCMI->getMimeType());
    const uint8 ui8TotalNumberOfChunks = pCMI->getTotalNumberOfChunks();
    DArray2<BufferReader> fragments;
    do {
        pCMI = pMsg->getChunkMsgInfo();
        const void *pData = pMsg->getData();
        if (pCMI == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "ChunkMsgInfo is NULL\n");
            return NULL;
        }
        if (pData == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "data is NULL\n");
            return NULL;
        }

        fragments[pCMI->getChunkId()].init (pData, pCMI->getTotalMessageLength());
    } while ((pMsg = pChunks->getNext()) != NULL);

    Annotations annotations;
    if (pAnnotations != NULL) {
        for (Message *pCustumChunk = pAnnotations->getFirst(); pCustumChunk != NULL;
            pCustumChunk = pAnnotations->getNext()) {
            uint32 ui32AnnotationMetadataLen = 0U;
            const void *pAnnotationBuf = pCustumChunk->getMessageHeader()->getAnnotationMetadata (ui32AnnotationMetadataLen);
            BufferReader br (pAnnotationBuf, ui32AnnotationMetadataLen);
            CustomChunkDescription chunkDescr;
            chunkDescr.read (&br);
            MessageHeader *pMH = pCustumChunk->getMessageHeader();
            uint32 ui32TotalMsgLen = pMH->getTotalMessageLength();
            const char *pCustumChunkData = static_cast<const char *>(pCustumChunk->getData());
            if (ui32TotalMsgLen > 0) {
                const String grpName (pMH->getGroupName());
                if ((!pMH->isChunk()) && grpName.startsWith ("DSPro")) {
                    // This is an HACK to remove DSPro metadata
                    ui32TotalMsgLen -= 1;
                    pCustumChunkData++;
                }
                Annotation *pAnn = new Annotation (true);
                if (pAnn != NULL) {
                    pAnn->ppIntervals = chunkDescr.getIntervals();
                    pAnn->bw.init (pCustumChunkData, ui32TotalMsgLen);
                    annotations.append (pAnn);
                }
            }
        }
    }

    BufferReader *pBR = pChunkingMgr->reassemble (&fragments, &annotations, mimeType, ui8TotalNumberOfChunks, 90);
    if (pBR == NULL) {
        checkAndLogMsg ("ChunkingAdaptor::reassembler", Logger::L_MildError,
                        "failed to get annotated object\n");
    }
    else {
        ui32LargeObjLen = pBR->getBufferLength();
    }
    deallocateAllPtrLListElements<Annotation> (&annotations);
    return pBR;
}

ChunkMsgInfo * ChunkingAdaptor::toChunkMsgInfo (const char *pszGroupName, const char *pszSenderNodeId,
                                                uint32 ui32MsgSeqId, const char *pszObjectId,
                                                const char *pszInstanceID, uint16 ui16Tag, uint16 ui16ClientId,
                                                uint8 ui8ClientType, const char *pszMimeType, const char *pszChecksum,
                                                uint16 ui16HistoryWindow, uint8 ui8Priority,
                                                int64 i64ExpirationTime, Chunker::Fragment *pChunkFragment)
{
    if (pChunkFragment == NULL) {
        return NULL;
    }

    return new ChunkMsgInfo (pszGroupName, pszSenderNodeId, ui32MsgSeqId, pChunkFragment->ui8Part,
                             pszObjectId, pszInstanceID, ui16Tag, ui16ClientId, ui8ClientType,
                             pszMimeType, pszChecksum, 0, // ui32FragmentOffset
                             (uint32) pChunkFragment->ui64FragLen, (uint32) pChunkFragment->ui64FragLen,
                             pChunkFragment->ui8TotParts, ui16HistoryWindow, ui8Priority,
                             i64ExpirationTime);
}

Chunker::Fragment * ChunkingAdaptor::getChunkerFragment (const void *pData, uint32 ui32DataLenght)
{
    Chunker::Fragment *pFrag = new Chunker::Fragment();
    pFrag->out_type = pFrag->src_type = Chunker::UNSUPPORTED;
    pFrag->ui64FragLen = ui32DataLenght;
    pFrag->ui8Part = MessageHeader::MIN_CHUNK_ID;
    pFrag->ui8TotParts = 1;
    pFrag->pReader = new BufferReader (pData, ui32DataLenght);
    return pFrag;
}

CustomChunkDescription::CustomChunkDescription (IHMC_MISC::Chunker::Type inputType,
                                                IHMC_MISC::Chunker::Type outputType)
    : _inputType (inputType), _outputType (outputType)
{
}

CustomChunkDescription::~CustomChunkDescription (void)
{
}

void CustomChunkDescription::deleteIntervals (IHMC_MISC::Chunker::Interval **ppIntervals)
{
    for (unsigned int i = 0; ppIntervals[i] != NULL; i++) {
        free (ppIntervals[i]);
    }
    free (ppIntervals);
}

IHMC_MISC::Chunker::Interval ** CustomChunkDescription::getIntervals (void)
{
    if (_dimensions.size() <= 0) {
        return NULL;
    }
    Chunker::Interval **ppIntervals = static_cast<Chunker::Interval **>(calloc (_dimensions.size() + 1, sizeof (Chunker::Interval*)));
    if (ppIntervals != NULL) {
        for (unsigned int i = 0; i < _dimensions.size(); i++) {
            ppIntervals[i] = new Chunker::Interval (_dimensions[i]);
            if (ppIntervals[i] == NULL) {
                deleteIntervals (ppIntervals);
                return NULL;
            }
        }
    }
    return ppIntervals;
}

int CustomChunkDescription::read (NOMADSUtil::Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }
    if (pReader->readUI8 (&_inputType) < 0) {
        return -3;
    }
    if (pReader->readUI8 (&_outputType) < 0) {
        return -4;
    }
    uint8 ui8NDimensions = 0;
    if (pReader->readUI8 (&ui8NDimensions) < 0) {
        return -7;
    }
    for (uint8 i = 0; i < ui8NDimensions; i++) {
        uint8 ui8Dimension;
        Chunker::Dimension dimension;
        if ((pReader->readUI8 (&ui8Dimension) < 0) || (ChunkingUtils::toDimension (ui8Dimension, dimension) < 0)) {
            return -8;
        }
        uint32 ui32Start, uint32End;
        if ((pReader->readUI32 (&ui32Start) < 0) || (pReader->readUI32 (&uint32End) < 0)) {
            return -9;
        }
        Chunker::Interval interval;
        interval.dimension = dimension;
        interval.uiStart = ui32Start;
        interval.uiEnd = uint32End;
        _dimensions.add (interval);
    }
    return 0;
}

int CustomChunkDescription::write (NOMADSUtil::Writer *pWriter)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (pWriter->writeUI8 (&_inputType)) {
        return -3;
    }
    if (pWriter->writeUI8 (&_outputType)) {
        return -4;
    }
    const unsigned int uiNDimension = _dimensions.size();
    if (uiNDimension > 0xFF) {
        return -7;
    }
    uint8 ui8NDimensions = static_cast<uint8>(uiNDimension);
    if (pWriter->writeUI8 (&ui8NDimensions) < 0) {
        return -7;
    }
    for (uint8 i = 0; i < ui8NDimensions; i++) {
        if (_dimensions[i].dimension > 0xFF) {
            return -8;
        }
        uint8 ui8Dimension = static_cast<uint8>(_dimensions[i].dimension);
        if (pWriter->writeUI8 (&ui8Dimension) < 0) {
            return -9;
        }
        uint32 ui32Start = _dimensions[i].uiStart;
        if (pWriter->writeUI32 (&ui32Start) < 0) {
            return -10;
        }
        uint32 ui32End = _dimensions[i].uiEnd;
        if (pWriter->writeUI32 (&ui32End) < 0) {
            return -11;
        }
    }
    return 0;
}

