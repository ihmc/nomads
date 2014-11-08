/*
 * RollingBoundedBitmap.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#include "RollingBoundedBitmap.h"

#include "SequentialArithmetic.h"

#include <stdio.h>

using namespace NOMADSUtil;

RollingBoundedBitmap::RollingBoundedBitmap (uint32 ui32NumberOfBits)
{
    if ((ui32NumberOfBits % 32) != 0) {
        // Increment ui32NumberOfBits to make it a multiple of 32
        ui32NumberOfBits = ((ui32NumberOfBits / 32) + 1) * 32;
    }
    _ui32ArraySize = ui32NumberOfBits / 32;
    _pui32Bits = new uint32 [_ui32ArraySize];
    reset();    // Sets _ui32Floor and _ui32Ceiling
}

RollingBoundedBitmap::~RollingBoundedBitmap (void)
{
    delete[] _pui32Bits;
    _pui32Bits = NULL;
}

void RollingBoundedBitmap::reset (void)
{
    for (uint32 ui32 = 0; ui32 < _ui32ArraySize; ui32++) {
        _pui32Bits[ui32] = 0UL;
    }
    _ui32Floor = 0;
    _ui32Ceiling = _ui32ArraySize * 32;
}

void RollingBoundedBitmap::set (uint32 ui32Bit)
{
    if (SequentialArithmetic::lessThan (ui32Bit, _ui32Floor)) {
        // Below the floor of the bitmap we are keeping track of, so ignore
    }
    else if (SequentialArithmetic::lessThan (ui32Bit, _ui32Ceiling)) {
        uint32 ui32BitArrayIndex = (ui32Bit - _ui32Floor) / 32;
        uint32 ui32BitOffset = ui32Bit % 32;     // Should be the same as (ui32Bit - _ui32Floor) % 32
        _pui32Bits[ui32BitArrayIndex] |= (0x00000001UL << ui32BitOffset);
    }
    else {
        // Need to move the bitmap
        uint32 ui32BitArrayIndex = (ui32Bit - _ui32Floor) / 32;
        uint32 ui32MoveFactor = ui32BitArrayIndex - (_ui32ArraySize - 1);
        for (uint32 ui32 = 0; ui32 < _ui32ArraySize; ui32++) {
            if ((ui32 + ui32MoveFactor) < _ui32ArraySize) {
                _pui32Bits[ui32] = _pui32Bits[ui32 + ui32MoveFactor];
            }
            else {
                _pui32Bits[ui32] = 0UL;
            }
        }
        _ui32Floor += (ui32MoveFactor * 32);
        _ui32Ceiling += (ui32MoveFactor * 32);

        // Now - we are ready to set the bit
        ui32BitArrayIndex = (ui32Bit - _ui32Floor) / 32;
        uint32 ui32BitOffset = ui32Bit % 32;     // Should be the same as (ui32Bit - _ui32Floor) % 32
        _pui32Bits[ui32BitArrayIndex] |= (0x00000001UL << ui32BitOffset);
    }
}

bool RollingBoundedBitmap::isSet (uint32 ui32Bit)
{
    if (SequentialArithmetic::lessThan (ui32Bit, _ui32Floor)) {
        return true;
    }
    else if (SequentialArithmetic::greaterThanOrEqual (ui32Bit, _ui32Ceiling)) {
        return false;
    }
    else {
        uint32 ui32BitArrayIndex = (ui32Bit - _ui32Floor) / 32;
        uint32 ui32BitOffset = ui32Bit % 32;     // Should be the same as (ui32Bit - _ui32Floor) % 32
        if (_pui32Bits[ui32BitArrayIndex] & (0x00000001UL << ui32BitOffset)) {
            return true;
        }
        else {
            return false;
        }
    }
}

void RollingBoundedBitmap::display (void)
{
    printf ("floor = %lu (%lx)\n", _ui32Floor, _ui32Floor);
    for (uint32 ui = 0; ui < _ui32ArraySize; ui++) {
        for (uint32 ui2 = 0; ui2 < 32; ui2++) {
            if (_pui32Bits[ui] & (0x00000001UL << ui2)) {
                printf ("1 ");
            }
            else {
                printf ("0 ");
            }
        }
        printf ("\n");
    }
    printf ("ceiling = %lu (%lx)\n", _ui32Ceiling, _ui32Ceiling);
}
