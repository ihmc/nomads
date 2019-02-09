/*
 * ChunkerArguments.h
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

#ifndef INCL_CHUNKER_ARGUMENTS_H
#define	INCL_CHUNKER_ARGUMENTS_H

#include "FTypes.h"
#include "Chunker.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_MISC
{
    struct FragmentBufferArguments
    {
        FragmentBufferArguments (void);
        FragmentBufferArguments (const void *pBuf, uint32 ui32Len, const char *pszInputMimeType,
                                 uint8 ui8NoOfChunks, const char *pszOutputMimeType,
                                 uint8 ui8ChunkCompressionQuality);
        ~FragmentBufferArguments (void);

        int read (NOMADSUtil::Reader *pReader);
        int write (NOMADSUtil::Writer *pWriter);

        uint8 _ui8NoOfChunks;
        uint8 _ui8ChunkCompressionQuality;
        NOMADSUtil::String _inputObjectType;
        NOMADSUtil::String _outputChunkType;
        uint32 _ui32Len;
        const void *_pBuf;
    };

    struct ExtractFromBufferArguments : private FragmentBufferArguments
    {
        ExtractFromBufferArguments (void);
        ExtractFromBufferArguments (const void *pBuf, uint32 ui32Len,
                                    const char *pszInputMimeType, const char *pszOutputMimeType,
                                    uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals);
        ~ExtractFromBufferArguments (void);

        int read (NOMADSUtil::Reader *pReader);
        int write (NOMADSUtil::Writer *pWriter);

        Chunker::Interval **_ppPortionIntervals;
    };
}

#endif  /* INCL_CHUNKER_ARGUMENTS_H */

