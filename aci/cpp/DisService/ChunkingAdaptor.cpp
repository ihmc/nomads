/*
 * ChunkingAdaptor.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#include "BufferReader.h"
#include "Logger.h"
#include "Chunker.h"
#include "ChunkReassembler.h"

#include "ConfigManager.h"

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
    ChunkingConfiguration::Range *pRange = _sizeToNchunks.getFirst();
    while ((pRange != NULL) && (ui64Bytes > pRange->_ui64Bytes)) {
        pRange = _sizeToNchunks.getNext();
    }
    if (pRange == NULL) {
        return 4;
    }
    return pRange->_ui8NChunks;
}

//------------------------------------------------------------------------------
// ChunkingAdaptor
//------------------------------------------------------------------------------

Reader * ChunkingAdaptor::reassemble (PtrLList<Message> *pChunks, uint32 &ui32LargeObjLen)
{
    int rc;
    ui32LargeObjLen = 0;
    if (pChunks == NULL) {
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
            checkAndLogMsg ("ChunkingAdaptor::reassemble", Logger::L_MildError,
                            "failed to allocate memory to copy retrieved object of size %lu\n",
                            pMsg->getMessageHeader()->getFragmentLength());
            return NULL;
        }
        memcpy (pData, pMsg->getData(), pMsg->getMessageHeader()->getFragmentLength());

        BufferReader *pReader = new BufferReader (pData,
                                                  pMsg->getMessageHeader()->getFragmentLength());
        ui32LargeObjLen = pMsg->getMessageHeader()->getFragmentLength();
        return pReader;
    }

    // This is a fragment - so use the reassembler
    ChunkReassembler cr;
    ChunkMsgInfo *pCMI = pMsg->getChunkMsgInfo();
    if (pCMI == NULL) {
        checkAndLogMsg ("ChunkingAdaptor::reassembler", Logger::L_MildError,
                        "ChunkMsgInfo is NULL\n");
        return NULL;
    }
    if (0 != (rc = cr.init (ChunkReassembler::Image, pCMI->getTotalNumberOfChunks()))) {
        checkAndLogMsg ("ChunkingAdaptor::reassembler", Logger::L_MildError,
                        "could not initialize ChunkReassembler; rc = %d\n", rc);
        return NULL;
    }

    do {
        pCMI = pMsg->getChunkMsgInfo();
        const void *pData = pMsg->getData();
        if (pCMI == NULL) {
            checkAndLogMsg ("ChunkingAdaptor::reassembler", Logger::L_MildError,
                            "ChunkMsgInfo is NULL\n");
            return NULL;
        }
        if (pData == NULL) {
            checkAndLogMsg ("ChunkingAdaptor::reassembler", Logger::L_MildError,
                            "data is NULL\n");
            return NULL;
        }
        if (0 != (rc = cr.incorporateChunk (pData, pCMI->getTotalMessageLength(), Chunker::JPEG, pCMI->getChunkId()))) {
            checkAndLogMsg ("ChunkingAdaptor::reassembler", Logger::L_MildError,
                            "failed to incorporate chunk %d; rc = %d\n", (int) pCMI->getChunkId(), rc);
            return NULL;
        }
    } while ((pMsg = pChunks->getNext()) != NULL);
    BufferReader *pBR = cr.getReassembedObject (Chunker::JPEG, 90);
    if (pBR == NULL) {
        checkAndLogMsg ("ChunkingAdaptor::reassembler", Logger::L_MildError,
                        "failed to get reassembled object\n");
        return NULL;
    }
    ui32LargeObjLen = pBR->getBufferLength();
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
