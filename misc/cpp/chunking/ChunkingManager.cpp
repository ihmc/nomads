/*
 * ChunkingManager.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2016 IHMC.
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

#include "ChunkingManager.h"
#include "ChunkReassembler.h"
#include "DArray2.h"
#include "MimeUtils.h"
#include "Writer.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

namespace CHUNKING_MANAGER
{
    int toType (const char *pszMimeType, Chunker::Type &type)
    {
        type = MimeUtils::mimeTypeToFragmentType (pszMimeType);
        if (type == Chunker::UNSUPPORTED) {
            return -1;
        }
        return 0;
    }

    int toType (const char *pszInputMimeType, const char *pszOutputMimeType, Chunker::Type &inType, Chunker::Type &outType)
    {
        if ((toType (pszInputMimeType, inType) < 0) || (toType (pszOutputMimeType, outType) < 0)) {
            return -1;
        }
        return 0;
    }
}

ChunkingManager::ChunkingManager (void)
{
}

ChunkingManager::~ChunkingManager (void)
{
}

bool ChunkingManager::supportsFragmentation (const char *pszMimeType)
{
    _mChunkers.lock();
    bool bPlugin = _chunkers.get (pszMimeType) != NULL;
    _mChunkers.unlock();
    return bPlugin || (MimeUtils::mimeTypeToFragmentType (pszMimeType) != Chunker::UNSUPPORTED);
}

Chunker::Fragments * ChunkingManager::fragmentFile (const char *pszFileName, const char *inputObjectMimeType,
                                                    uint8 ui8NoOfChunks, const char *outputObjectMimeType,
                                                    uint8 ui8ChunkCompressionQuality)
{
    _mChunkers.lock();
    ChunkerInterface *pChunker = _chunkers.get (inputObjectMimeType);
    if (pChunker != NULL) {
        Chunker::Fragments *pFrags = pChunker->fragmentFile (pszFileName, inputObjectMimeType, ui8NoOfChunks,
                                                             outputObjectMimeType, ui8ChunkCompressionQuality);
        _mChunkers.unlock();
        return pFrags;
    }
    _mChunkers.unlock();
    Chunker::Type inType, outType;
    if (CHUNKING_MANAGER::toType (inputObjectMimeType, outputObjectMimeType, inType, outType) < 0) {
        return NULL;
    }
    return Chunker::fragmentFile (pszFileName, inType, ui8NoOfChunks, outType, ui8ChunkCompressionQuality);
}

Chunker::Fragments * ChunkingManager::fragmentBuffer (const void *pBuf, uint32 ui32Len, const char *inputObjectMimeType,
                                                      uint8 ui8NoOfChunks, const char *outputObjectMimeType,
                                                      uint8 ui8ChunkCompressionQuality)
{
    _mChunkers.lock();
    ChunkerInterface *pChunker = _chunkers.get (inputObjectMimeType);
    if (pChunker != NULL) {
        Chunker::Fragments *pFrags = pChunker->fragmentBuffer (pBuf, ui32Len, inputObjectMimeType, ui8NoOfChunks,
                                                               outputObjectMimeType, ui8ChunkCompressionQuality);
        _mChunkers.unlock();
        return pFrags;
    }
    _mChunkers.unlock();
    Chunker::Type inType, outType;
    if (CHUNKING_MANAGER::toType (inputObjectMimeType, outputObjectMimeType, inType, outType) < 0) {
        return NULL;
    }
    return Chunker::fragmentBuffer (pBuf, ui32Len, inType, ui8NoOfChunks, outType, ui8ChunkCompressionQuality);
}

Chunker::Fragment *ChunkingManager::extractFromFile (const char *pszFileName, const char *inputObjectMimeType, const char *outputObjectMimeType,
                                                     uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals)
{
    _mChunkers.lock();
    ChunkerInterface *pChunker = _chunkers.get (inputObjectMimeType);
    if (pChunker != NULL) {
        Chunker::Fragment *pFrag = pChunker->extractFromFile (pszFileName, inputObjectMimeType, outputObjectMimeType,
                                                              ui8ChunkCompressionQuality, ppPortionIntervals);
        _mChunkers.unlock();
        return pFrag;
    }
    _mChunkers.unlock();
    Chunker::Type inType, outType;
    if (CHUNKING_MANAGER::toType (inputObjectMimeType, outputObjectMimeType, inType, outType) < 0) {
        return NULL;
    }
    return Chunker::extractFromFile (pszFileName, inType, outType, ui8ChunkCompressionQuality, ppPortionIntervals);
}

Chunker::Fragment * ChunkingManager::extractFromBuffer (const void *pBuf, uint32 ui32Len, const char *inputObjectMimeType, const char *outputObjectMimeType,
                                                        uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals)
{
    _mChunkers.lock();
    ChunkerInterface *pChunker = _chunkers.get (inputObjectMimeType);
    if (pChunker != NULL) {
        Chunker::Fragment *pFrag = pChunker->extractFromBuffer (pBuf, ui32Len, inputObjectMimeType, outputObjectMimeType,
                                                                ui8ChunkCompressionQuality, ppPortionIntervals);
        _mChunkers.unlock();
        return pFrag;
    }
    _mChunkers.unlock();
    Chunker::Type inType, outType;
    if (CHUNKING_MANAGER::toType (inputObjectMimeType, outputObjectMimeType, inType, outType) < 0) {
        return NULL;
    }
    return Chunker::extractFromBuffer (pBuf, ui32Len, inType, outType, ui8ChunkCompressionQuality, ppPortionIntervals);
}

BufferReader * ChunkingManager::reassemble (DArray2<BufferReader> *pFragments, Annotations *pAnnotations,
                                            const char *chunkMimeType, uint8 ui8NoOfChunks, uint8 ui8CompressionQuality)
{
    if (pFragments == NULL) {
        return NULL;
    }

    _mReassemblers.lock();
    ChunkReassemblerInterface *pReassembler = _reassemblers.get (chunkMimeType);
    if (pReassembler != NULL) {
        BufferReader *pBR = pReassembler->reassemble (pFragments, pAnnotations, chunkMimeType, ui8NoOfChunks, ui8CompressionQuality);
        _mReassemblers.unlock();
        return pBR;
    }
    _mReassemblers.unlock();

    Chunker::Type chunkType;
    if (CHUNKING_MANAGER::toType (chunkMimeType, chunkType) < 0) {
        return NULL;
    }
    ChunkReassembler reass;
    if (reass.init (MimeUtils::toReassemblerType (chunkType), ui8NoOfChunks) < 0) {
        return NULL;
    }

    for (unsigned int i = 1; i < pFragments->size(); i++) {
        if (pFragments->used (i) && (i < 0xFF)) {
            const uint32 ui32Len = (*pFragments)[i].getBufferLength();
            if (reass.incorporateChunk ((*pFragments)[i].getBuffer(), ui32Len, chunkType, static_cast<uint8>(i)) < 0) {
                return NULL;
            }
        }
    }

    if ((pAnnotations == NULL) || pAnnotations->isEmpty()) {
        return reass.getReassembledObject (chunkType, ui8CompressionQuality);
    }

    Annotation *pAnnotation = pAnnotations->getFirst();
    for (; pAnnotation != NULL; pAnnotation = pAnnotations->getNext()) {
        reass.incorporateAnnotation (pAnnotation->ppIntervals, pAnnotation->bw.getBuffer(),
                                     pAnnotation->bw.getBufferLength(), chunkType);
    }

    return reass.getAnnotatedObject (chunkType, ui8CompressionQuality);
}

// Plugins Registration

int ChunkingManager::registerChunker (const char *pszMimeType, ChunkerInterface *pChunker)
{
    _mChunkers.lock();
    if (_chunkers.containsKey (pszMimeType)) {
        _mChunkers.unlock();
        return -1;
    }
    _chunkers.put (pszMimeType, pChunker);
    _mChunkers.unlock();
    return 0;
}

int ChunkingManager::registerReassembler (const char *pszMimeType, ChunkReassemblerInterface *pReassembler)
{
    _mReassemblers.lock();
    if (_reassemblers.containsKey (pszMimeType)) {
        _mReassemblers.unlock();
        return -1;
    }
    _reassemblers.put (pszMimeType, pReassembler);
    _mReassemblers.unlock();
    return 0;
}

int ChunkingManager::deregisterChunker (const char *pszMimeType)
{
    _mChunkers.lock();
    _chunkers.remove (pszMimeType);
    _mChunkers.unlock();
    return 0;
}

int ChunkingManager::deregisterReassembler (const char *pszMimeType)
{
    _mReassemblers.lock();
    _reassemblers.remove (pszMimeType);
    _mReassemblers.unlock();
    return 0;
}

Annotation::Annotation (bool bDelIntervals)
    : bDeleteIntervals (bDelIntervals),
      ppIntervals (NULL)
{
}

Annotation::~Annotation (void)
{
    if (bDeleteIntervals && (ppIntervals != NULL)) {
        for (unsigned int i = 0; ppIntervals[i] != NULL; i++) {
            free (ppIntervals[i]);
        }
        free (ppIntervals);
    }
}

int Annotation::read (Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }
    if (ppIntervals == NULL) {
        return -2;
    }
    uint32 ui32BufLen = 0U;
    if (pReader->read32 (&ui32BufLen) < 0) {
        return -3;
    }
    if (ui32BufLen > 0) {
        void *pBuf = malloc (ui32BufLen);
        if (pBuf == NULL) {
            return -4;
        }
        if (pReader->readBytes (pBuf, ui32BufLen) < 0) {
            free (pBuf);
            return -5;
        }
        bw.init (pBuf, ui32BufLen, true);
    }

    uint8 ui8Count = 0;
    if (pReader->read8 (&ui8Count) < 0) {
        return -6;
    }
    if (ui8Count == 0) {
        return -7;
    }
    ppIntervals = static_cast<Chunker::Interval **>(calloc (ui8Count + 1, sizeof (Chunker::Interval *)));
    if (ppIntervals == NULL) {
        return -8;
    }
    for (uint8 i = 0; i < ui8Count; i++) {
        ppIntervals[i] = new Chunker::Interval();
        if (ppIntervals[i] == NULL) {
            return -9;
        }
        if (ppIntervals[i]->read (pReader) < 0) {
            return -10;
        }
    }

    return 0;
}

int Annotation::write (NOMADSUtil::Writer *pWriter)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (ppIntervals == NULL) {
        return -2;
    }
    uint32 ui32BufLen = bw.getBufferLength();
    if (pWriter->write32 (&ui32BufLen) < 0) {
        return -3;
    }
    if (pWriter->writeBytes (bw.getBuffer(), ui32BufLen) < 0) {
        return -4;
    }
    uint8 ui8Count = 0;
    for (; ppIntervals[ui8Count] != NULL; ui8Count++);
    if (pWriter->write8 (&ui8Count) < 0) {
        return -8;
    }
    for (; ppIntervals[ui8Count] != NULL; ui8Count++) {
        if (ppIntervals[ui8Count]->write (pWriter) < 0) {
            return -9;
        }
    }
    return 0;
}

ChunkReassemblerInterface::~ChunkReassemblerInterface (void)
{
}

