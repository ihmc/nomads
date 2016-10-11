/*
 * FileReader.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_BOYER_MOORE_HORSPOOL_H
#define INCL_BOYER_MOORE_HORSPOOL_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class FileReader;

    class BoyerMooreHorspool
    {
        public:
            BoyerMooreHorspool (void);
            virtual ~BoyerMooreHorspool (void);

            virtual int init (FileReader *pHaystack, long lHaystackLen, uint8 *pui8Needle, uint16 ui16NeedleLen);
            virtual int init (FileReader *pHaystack, long lHaystackLen, uint32 ui32Needle);

            // It returns the position of the beginning of the first
            // substring starting from lPos, or a negative number if
            // the substring is not found.
            long findFirstNeedle (long lPos = 0) const;

        private:
            uint16 _ui16NeedleLen;
            long _lHaystackLen;
            FileReader *_pHaystack;
            uint8 *_pui8Needle;
            uint16 _table[256];
    };
}

#endif    /* INCL_BOYER_MOORE_HORSPOOL_H */

