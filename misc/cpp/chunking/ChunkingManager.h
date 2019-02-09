/*
 * ChunkingManager.h
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

#ifndef INCL_CHUNKING_MANAGER_H
#define	INCL_CHUNKING_MANAGER_H

#include "Chunker.h"

#include "BufferReader.h"
#include "DArray2.h"
#include "Mutex.h"
#include "StringHashtable.h"

namespace IHMC_MISC
{
    struct Annotation
    {
        explicit Annotation (bool bDelIntervals);
        ~Annotation (void);

        int read (NOMADSUtil::Reader *pReader);
        int write (NOMADSUtil::Writer *pWriter);

        const bool bDeleteIntervals;
        NOMADSUtil::BufferReader bw;
        Chunker::Interval **ppIntervals;
    };

    typedef NOMADSUtil::PtrLList<Annotation> Annotations;

    class ChunkReassemblerInterface
    {
        public:
            virtual ~ChunkReassemblerInterface (void);
            virtual NOMADSUtil::BufferReader * reassemble (NOMADSUtil::DArray2<NOMADSUtil::BufferReader> *pFragments,
                                                           Annotations *pAnnotations, const char *pszChunkMimeType,
                                                           uint8 ui8NoOfChunks, uint8 ui8CompressionQuality) = 0;
    };

    class ChunkingManager
    {
        public:
            ChunkingManager (void);
            ~ChunkingManager (void);

            bool supportsFragmentation (const char *pszMimeType);

            // Fragmenting

            Chunker::Fragments * fragmentFile (const char *pszFileName, const char *inputObjectMimeType,
                                               uint8 ui8NoOfChunks, const char *outputObjectMimeType,
                                               uint8 ui8ChunkCompressionQuality);
            Chunker::Fragments * fragmentBuffer (const void *pBuf, uint32 ui32Len, const char *inputObjectMimeType,
                                                 uint8 ui8NoOfChunks, const char *outputObjectMimeType,
                                                 uint8 ui8ChunkCompressionQuality);

            Chunker::Fragment * extractFromFile (const char *pszFileName, const char *inputObjectMimeType, const char *outputObjectMimeType,
                                                 uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals);
            Chunker::Fragment * extractFromBuffer (const void *pBuf, uint32 ui32Len, const char *inputObjectMimeType, const char *outputObjectMimeType,
                                                   uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals);

            // Reassembling

            /**
             * The indexId of the DArray2 is the chunkId
             */
            NOMADSUtil::BufferReader * reassemble (NOMADSUtil::DArray2<NOMADSUtil::BufferReader> *pFragments,
                                                   Annotations *pAnnotations, const char *chumkMimeType,
                                                   uint8 ui8NoOfChunks, uint8 ui8CompressionQuality);

            // Plugins Registration

            int registerChunker (const char *pszMimeType, ChunkerInterface *pChunker);
            int registerReassembler (const char *pszMimeTypee, ChunkReassemblerInterface *pReassembler);

            int deregisterChunker (const char *pszMimeType);
            int deregisterReassembler (const char *pszMimeTypee);

        private:
            NOMADSUtil::Mutex _mChunkers;
            NOMADSUtil::Mutex _mReassemblers;
            NOMADSUtil::StringHashtable<ChunkerInterface> _chunkers;
            NOMADSUtil::StringHashtable<ChunkReassemblerInterface> _reassemblers;
    };

}

#endif  /* INCL_CHUNKING_MANAGER_H */
