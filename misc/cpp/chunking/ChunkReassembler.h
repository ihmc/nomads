/*
 * ChunkReassembler.h
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

#ifndef INCL_CHUNK_REASSEMBLER_H
#define INCL_CHUNK_REASSEMBLER_H

#include "Chunker.h"

namespace NOMADSUtil
{
    class BufferReader;
}

namespace IHMC_MISC
{
    class BMPReassembler;
    class FFmpegReassembler;
    class MPEG1Reassembler;

    class ChunkReassembler
    {
        public:
            enum Type {
                Image    = 0x10,
                Audio    = 0x11,
                Video    = 0x12,

                UNSUPPORTED = 0x00
            };

            ChunkReassembler (void);
            ~ChunkReassembler (void);

            int init (Type reassemblerType, uint8 ui8NoOfChunks);

            int incorporateChunk (const void *pBuf, uint32 ui32BufLen, Chunker::Type chunkType, uint8 ui8ChunkId);
            int incorporateAnnotation (Chunker::Interval **ppIntervals, const void *pBuf, uint32 ui32BufLen,
                                       Chunker::Type chunkType);

            NOMADSUtil::BufferReader * getReassembledObject (Chunker::Type outputType, uint8 ui8CompressionQuality);
            NOMADSUtil::BufferReader * getAnnotatedObject (Chunker::Type outputType, uint8 ui8CompressionQuality);

        private:
            Type _type;
            uint8 _ui8NoOfChunks;
            BMPReassembler *_pBMPReassembler;
            MPEG1Reassembler *_pMpeg1Reassembler;
            FFmpegReassembler *_pFFMPEGReassembler;
    };
}

#endif   // #ifndef INCL_CHUNK_REASSEMBLER_H
