/*
 * Chunker.h
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

#ifndef INCL_CHUNKER_H
#define	INCL_CHUNKER_H

#include "FTypes.h"
#include "PtrLList.h"
#include "StringStringHashtable.h"

namespace NOMADSUtil
{
    class Writer;
    class Reader;
}

namespace IHMC_MISC
{
    class Chunker
    {
        public:
            enum Dimension {
                X = 0x00,  // x-axes (longitude)
                Y = 0x01,  // y-axes (latitude)
                T = 0x03   // time
            };

            enum Type {
                UNSUPPORTED = 0x00,
                BMP         = 0x01,
                JPEG        = 0x02,
                JPEG2000    = 0x03,
                PNG         = 0x04,
                A_MPEG      = 0x05, // Audio MPEG
                AVI         = 0x06,
                MOV         = 0x07,
                V_MPEG      = 0x08, // Video MPEG
                ZIP         = 0x09  // Compressed Zip
            };

            struct Fragment {
                NOMADSUtil::Reader *pReader;
                uint64 ui64FragLen;     // The length of the fragment
                NOMADSUtil::String src_type;          // The type of the source object
                NOMADSUtil::String out_type;          // The type of the output fragments
                uint8 ui8Part;          // The number of this fragment
                uint8 ui8TotParts;      // The total number of fragments

                bool operator == (Fragment &rhsFragment);
            };

            typedef NOMADSUtil::PtrLList<Fragment> Fragments;

            struct Interval {
                Interval (void);
                Interval (const Interval &interval);
                ~Interval (void);

                int read (NOMADSUtil::Reader *pReader);
                int write (NOMADSUtil::Writer *pWriter);

                Dimension dimension;
                uint64 uiStart;
                uint64 uiEnd;
            };

            static Fragments * fragmentFile (const char *pszFileName, Type inputObjectType,
                                             uint8 ui8NoOfChunks, Type outputChunkType,
                                             uint8 ui8ChunkCompressionQuality);
            static Fragments * fragmentBuffer (const void *pBuf, uint32 ui32Len, Type inputObjectType,
                                               uint8 ui8NoOfChunks, Type outputChunkType,
                                               uint8 ui8ChunkCompressionQuality);

            static Fragment * extractFromFile (const char *pszFileName, Type inputObjectType, Type outputChunkType,
                                                 uint8 ui8ChunkCompressionQuality, Interval **ppPortionIntervals);
            static Fragment * extractFromBuffer (const void *pBuf, uint32 ui32Len, Type inputObjectType, Type outputChunkType,
                                                 uint8 ui8ChunkCompressionQuality, Interval **ppPortionIntervals);
    };

    class ChunkerInterface
    {
        public:
            virtual ~ChunkerInterface (void);
            virtual NOMADSUtil::PtrLList<Chunker::Fragment> * fragmentFile (const char *pszFileName, const char *pszInputChunkMimeType,
                                                                            uint8 ui8NoOfChunks, const char *pszOutputChunkMimeType,
                                                                            uint8 ui8ChunkCompressionQuality) = 0;
            virtual NOMADSUtil::PtrLList<Chunker::Fragment> * fragmentBuffer (const void *pBuf, uint32 ui32Len, const char *pszInputChunkMimeType,
                                                                              uint8 ui8NoOfChunks, const char *pszOutputChunkMimeType,
                                                                              uint8 ui8ChunkCompressionQuality) = 0;

            virtual Chunker::Fragment * extractFromFile (const char *pszFileName, const char *pszInputChunkMimeType, const char *pszOutputChunkMimeType,
                                                         uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals) = 0;
            virtual Chunker::Fragment * extractFromBuffer (const void *pBuf, uint32 ui32Len, const char *pszInputChunkMimeType, const char *pszOutputChunkMimeType,
                                                           uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals) = 0;
    };

    inline bool Chunker::Fragment::operator == (Chunker::Fragment &rhsFragment)
    {
        return ui8Part == rhsFragment.ui8Part;
    }
}

#endif	// INCL_FRAGMENTER_H
