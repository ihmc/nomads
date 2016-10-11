/*
 * MPEG1Handler.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#ifndef INCL_MPEG_1_HANDLER_H
#define INCL_MPEG_1_HANDLER_H

#include "PtrLList.h"
#include "MPEG1Parser.h"

namespace NOMADSUtil
{
    class BufferReader;
    class Reader;
}

namespace IHMC_MISC
{
    class MPEG1Parser;

    class MPEG1Chunker
    {
        public:
            static NOMADSUtil::Reader * fragmentMpeg1 (MPEG1Parser *pReader, uint8 ui8DesiredChunkId, uint8 ui8TotalNoOfChunks);
            static NOMADSUtil::Reader * extractFromMpeg1 (MPEG1Parser *pReader, int64 i64StartT, int64 i64EndT);
    };

    class MPEG1Reassembler
    {
        public:
            MPEG1Reassembler (void);
            ~MPEG1Reassembler (void);

            int init (uint8 ui8TotalNoOfChunks);
            int incorporateChunk (MPEG1Parser *pReader, uint8 ui8ChunkId);

            // NOTE: if new chunks are added, (part of) the changes to the image added
            // be the annotation may be overrridden. Annotations should be added to the
            // image after all chunks have been added
            // NOTE: the caller should implement the logic to avoid the addition of
            // overriding annotations
            int incorporateChunk (MPEG1Parser *pReader, int64 i64StartT, int64 i64EndT);

            // Returns the BMPImage that is a result of all of the chunks incorporated
            // in the added annotations
            // NOTE: Caller must not modify this image
            // NOTE: The video/audio object will NOT be deleted when the instance of
            //       FFmpegReassembler is deleted (conversely from BMPReassembler)
            NOMADSUtil::BufferReader * getAnnotatedVideo (void);

            // Returns the BMPImage that is a result of all of the chunks incorporated
            // NOTE: This method is not a simple accessor - it does interpolation of the
            //       pixel values for the missing chunks and is hence expensive
            // NOTE: Caller must not modify this image
            // NOTE: The video / audio object will NOT be deleted when the instance of
            //       FFmpegReassembler is deleted (conversely from BMPReassembler)
            NOMADSUtil::BufferReader * getReassembledVideo (void);

        private:
            struct ChunkWrapper
            {
                ChunkWrapper (uint8 ui8ChunkId, MPEG1Parser *pReader);
                ~ChunkWrapper (void);

                bool operator > (const ChunkWrapper &rhsChunkWr);
                bool operator < (const ChunkWrapper &rhsChunkWr);
                bool operator == (const ChunkWrapper &rhsChunkWr);

                const uint8 _ui8ChunkId;
                unsigned int _uiCurrChunk;
                MPEG1Parser *_pReader;
            };

            struct AnnotationWrapper
            {
                AnnotationWrapper (int64 i64StartT, int64 i64EndT, MPEG1Parser *pReader);
                ~AnnotationWrapper (void);

                /**
                 * Returns a negative number if i64FrameTimeInMillis is smaller than the beginning of
                 * the interval, 0 is it is included in the interval, a positive non-zero number if
                 * i64FrameTimeInMillis is grated than the end of the interval.
                 */
                int includes (int64 i64FrameTimeInMillis) const;
                bool operator > (const AnnotationWrapper &rhsAnnWr) const;
                bool operator < (const AnnotationWrapper &rhsAnnWr) const;

                const int64 _i64StartT;
                const int64 _i64EndT;
                MPEG1Parser *_pReader;
            };

            uint8 _ui8TotalNoOfChunks;
            NOMADSUtil::PtrLList<ChunkWrapper> _chunks;
            NOMADSUtil::PtrLList<AnnotationWrapper> _annotations;
    };
}

#endif  // INCL_MPEG_1_HANDLER_H

