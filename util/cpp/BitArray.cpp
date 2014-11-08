/**
 * BitArray.cpp
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

#include "BitArray.h"

#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)

using namespace NOMADSUtil;

BitArray::BitArray()
{
    _pui8BitArray = NULL;
}

BitArray::BitArray (unsigned int uiNBits)
{
    _pui8BitArray = (uint8*) calloc (BITNSLOTS(uiNBits), sizeof (uint8));
}

BitArray::~BitArray()
{
}

int BitArray::clearBit (unsigned int uiBitIndex)
{
    BITCLEAR(_pui8BitArray, uiBitIndex);
}

bool BitArray::isBitSet (unsigned int uiBitIndex)
{
    return BITTEST(_pui8BitArray, uiBitIndex) != 0;
}

int BitArray::setBit (unsigned int uiBitIndex)
{
    BITSET(_pui8BitArray, uiBitIndex);
}

int BitArray::read (Reader *pReader, uint32 ui32MaxLen)
{
    if (pReader == NULL) {
        return -1;
    }

    uint8 ui8Bytes = 0;
    pReader->read8 (&ui8Bytes);
    if ((ui8Bytes+1) > ui32MaxLen) {
        return -2;
    }
    if (ui8Bytes == 0) {
        return 0;
    }

    _pui8BitArray = (uint8 *) calloc (ui8Bytes, sizeof (ui8Bytes));
    for (uint8 i = 0; i < ui8Bytes; i++) {
        uint8 ui8 = 0;
        pReader->read8 (&ui8);
        _pui8BitArray[i] = ui8;
    }

}

int BitArray::write (Writer *pWriter, uint32 ui32MaxLen)
{
    if (pWriter == NULL) {
        return -1;
    }

    uint8 ui8Bytes = _pui8BitArray == NULL ? 0 : sizeof (_pui8BitArray);
    if ((ui8Bytes+1) > ui32MaxLen) {
        return -2;
    }
    pWriter->write8 (&ui8Bytes);
    for (uint8 i = 0; i < ui8Bytes; i++) {
        uint8 ui8 = _pui8BitArray[i];
        pWriter->write8 (&ui8);
    }

    return 0;
}

