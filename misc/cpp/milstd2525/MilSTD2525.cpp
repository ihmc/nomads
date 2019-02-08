/*
 * MilSTD2525.cpp
 *
 * This file is part of the IHMC Database Connectivity Library.
 * Copyright (c) 2014-2016 IHMC.
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#include "MilSTD2525.h"

using namespace IHMC_MISC_MIL_STD_2525;

const char * IHMC_MISC_MIL_STD_2525::getAsString (BattleDimension bd)
{
    switch (bd) {
        case BD_Space:
            return "Space";

        case BD_Air:
            return "Air";

        case BD_Ground:
            return "Ground";

        case BD_Sea_Surface:
            return "Sea_Surface";

        case BD_Sea_Subsurface:
            return "Sea_Subsurface";

        case BD_SOF:
            return "SOF";

        case BD_Other:
            return "Other";

        case BD_Unset:
            return "Unset";

        case BD_Error:
        default:
            return "Error";
    }
}

const char * IHMC_MISC_MIL_STD_2525::getAsString (OrderOfBattle ob)
{
    switch (ob) {
        case OB_Air:
            return "Air";

        case OB_Electronic:
            return "Electronic";

        case OB_Civilian:
            return "Civilian";

        case OB_Ground:
            return "Ground";

        case OB_Maritime:
            return "Maritime";

        case OB_Strategic_Force_Related:
            return "Strategic Force Related";

        case OB_Control_Markings:
            return "Control Markings";

        case OB_Unset:
            return "Unset";

        case OB_Error:
        default:
            return "Error";
    }
}

