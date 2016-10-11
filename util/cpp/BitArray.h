/* 
 * BitArray.h
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
 */

#ifndef INCL_BIT_ARRAY_H
#define	INCL_BIT_ARRAY_H

#include "Writer.h"

namespace NOMADSUtil
{
    class BitArray
    {
        public:
            BitArray (void);
            BitArray (unsigned int uiNBits);
            ~BitArray (void);

            int clearBit (unsigned int uiBitIndex);
            bool isBitSet (unsigned int uiBitIndex);
            int setBit (unsigned int uiBitIndex);

            int read (Reader *pReader, uint32 ui32MaxLen);
            int write (Writer *pWriter, uint32 ui32MaxLen);

        private:
            uint8 *_pui8BitArray;
    };
}

#endif	/* INCL_BIT_ARRAY_H */

