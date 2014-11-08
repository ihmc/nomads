/* 
 * BitArray.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 8, 2013, 4:09 PM
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

