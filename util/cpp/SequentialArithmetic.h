/*
 * SequentialArithmetic.h
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

#ifndef INCL_SEQUENTIAL_ARITHMETIC_H
#define INCL_SEQUENTIAL_ARITHMETIC_H

/*
 * This class handles comparison of sequence numbers that may wrap around.
 * The assumption is that even if numbers wrap around, they are "closer" to each
 * other than half of the range of numbers.
 */

#include "FTypes.h"

namespace NOMADSUtil
{

    class SequentialArithmetic
    {
        public:
            static bool greaterThan (uint32 ui32LHS, uint32 ui32RHS);
            static bool greaterThan (uint16 ui16LHS, uint16 ui16RHS);
            static bool greaterThan (uint8 ui8LHS, uint8 ui8RHS);
            static bool greaterThanOrEqual (uint32 ui32LHS, uint32 ui32RHS);
            static bool greaterThanOrEqual (uint16 ui16LHS, uint16 ui16RHS);
            static bool greaterThanOrEqual (uint8 ui8LHS, uint8 ui8RHS);
            static bool lessThan (uint32 ui32LHS, uint32 ui32RHS);
            static bool lessThan (uint16 ui16LHS, uint16 ui16RHS);
            static bool lessThan (uint8 ui8LHS, uint8 ui8RHS);
            static bool lessThanOrEqual (uint32 ui32LHS, uint32 ui32RHS);
            static bool lessThanOrEqual (uint16 ui16LHS, uint16 ui16RHS);
            static bool lessThanOrEqual (uint8 ui8LHS, uint8 ui8RHS);
            static uint32 delta (uint32 ui32A, uint32 ui32B);
            static uint16 delta (uint16 ui16A, uint16 ui16B);
            static uint8 delta (uint8 ui8A, uint8 ui8B);
    };

    inline bool SequentialArithmetic::greaterThan (uint32 ui32LHS, uint32 ui32RHS)
    {
        return (ui32LHS > ui32RHS) ? ((ui32LHS - ui32RHS) <= (2147483648UL)) : ((ui32RHS - ui32LHS) > (2147483648UL));
    }

    inline bool SequentialArithmetic::greaterThan (uint16 ui16LHS, uint16 ui16RHS)
    {
        return (ui16LHS > ui16RHS) ? ((uint16)(ui16LHS - ui16RHS) <= (32768UL)) : ((uint16)(ui16RHS - ui16LHS) > (32768UL));
    }

    inline bool SequentialArithmetic::greaterThan (uint8 ui8LHS, uint8 ui8RHS)
    {
        return (ui8LHS > ui8RHS) ? ((uint16)(ui8LHS - ui8RHS) <= (256UL)) : ((uint16)(ui8RHS - ui8LHS) > (256UL));
    }

    inline bool SequentialArithmetic::greaterThanOrEqual (uint32 ui32LHS, uint32 ui32RHS)
    {
        return (ui32LHS == ui32RHS) || greaterThan (ui32LHS, ui32RHS);
    }

    inline bool SequentialArithmetic::greaterThanOrEqual (uint16 ui16LHS, uint16 ui16RHS)
    {
        return (ui16LHS == ui16RHS) || greaterThan (ui16LHS, ui16RHS);
    }

    inline bool SequentialArithmetic::greaterThanOrEqual (uint8 ui8LHS, uint8 ui8RHS)
    {
        return (ui8LHS == ui8RHS) || greaterThan (ui8LHS, ui8RHS);
    }

    inline bool SequentialArithmetic::lessThan (uint32 ui32LHS, uint32 ui32RHS)
    {
        return greaterThan (ui32RHS, ui32LHS);
    }

    inline bool SequentialArithmetic::lessThan (uint16 ui16LHS, uint16 ui16RHS)
    {
        return greaterThan (ui16RHS, ui16LHS);
    }

    inline bool SequentialArithmetic::lessThan (uint8 ui8LHS, uint8 ui8RHS)
    {
        return greaterThan (ui8RHS, ui8LHS);
    }

    inline bool SequentialArithmetic::lessThanOrEqual (uint32 ui32LHS, uint32 ui32RHS)
    {
        return (ui32LHS == ui32RHS) || lessThan (ui32LHS, ui32RHS);
    }

    inline bool SequentialArithmetic::lessThanOrEqual (uint16 ui16LHS, uint16 ui16RHS)
    {
        return (ui16LHS == ui16RHS) || lessThan (ui16LHS, ui16RHS);
    }

    inline bool SequentialArithmetic::lessThanOrEqual (uint8 ui8LHS, uint8 ui8RHS)
    {
        return (ui8LHS == ui8RHS) || lessThan (ui8LHS, ui8RHS);
    }

    inline uint32 SequentialArithmetic::delta (uint32 ui32A, uint32 ui32B)
    {
        uint32 diffA, diffB;
        if (ui32A > ui32B) {
            diffA = ui32A - ui32B;
            diffB = (0xFFFFFFFFU - ui32A) + ui32B + 1;
        }
        else {
            diffA = ui32B - ui32A;
            diffB = (0xFFFFFFFFU - ui32B) + ui32A + 1;
        }
        return diffA < diffB ? diffA : diffB;
    }

    inline uint16 SequentialArithmetic::delta (uint16 ui16A, uint16 ui16B)
    {
        uint16 diffA, diffB;
        if (ui16A > ui16B) {
            diffA = (uint16) (ui16A - ui16B);
            diffB = (0xFFFFU - ui16A) + ui16B + 1;
        }
        else {
            diffA = (uint16) (ui16B - ui16A);
            diffB = (0xFFFFU - ui16B) + ui16A + 1;
        }
        return diffA < diffB ? diffA : diffB;
    }

    inline uint8 SequentialArithmetic::delta (uint8 ui8A, uint8 ui8B)
    {
        uint8 diffA, diffB;
        if (ui8A > ui8B) {
            diffA = (uint8) (ui8A - ui8B);
            diffB = (0xFFU - ui8A) + ui8B + 1;
        }
        else {
            diffA = (uint8) (ui8B - ui8A);
            diffB = (0xFFU - ui8B) + ui8A + 1;
        }
        return diffA < diffB ? diffA : diffB;
    }
}

#endif   // #ifndef INCL_SEQUENTIAL_ARITHMETIC
