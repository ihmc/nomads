/*
 * Chunker.h
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2014 IHMC.
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

#include <stdio.h>

#include "FTypes.h"
#include "PtrLList.h"

namespace NOMADSUtil
{
    template <class T> class PtrLList;
    class Reader;
}

namespace IHMC_MISC
{
    class Chunker
    {
        public:
            enum Type {
                BMP      = 0x01,
                JPEG     = 0x02,
                JPEG2000 = 0x03,
                A_MPEG   = 0x04, // Audio MPEG
                V_MPEG   = 0x05, // Video MPEG
                PNG      = 0x06,
                UNSUPPORTED = 0x00
            };

            struct Fragment {
                NOMADSUtil::Reader *pReader;
                uint64 ui64FragLen;     // The length of the fragment
                Type src_type;          // The type of the source object
                Type out_type;          // The type of the output fragments
                uint8 ui8Part;          // The number of this fragment
                uint8 ui8TotParts;      // The total number of fragments

                bool operator == (Fragment &rhsFragment);
            };

            static NOMADSUtil::PtrLList<Fragment> * fragmentBuffer (const void *pBuf, uint32 ui32Len, Type inputObjectType,
                                                                    uint8 ui8NoOfChunks, Type outputChunkType,
                                                                    uint8 ui8ChunkCompressionQuality);
    };

    inline bool Chunker::Fragment::operator == (Chunker::Fragment &rhsFragment)
    {
        return ui8Part == rhsFragment.ui8Part;
    }
}

#endif	// INCL_FRAGMENTER_H
