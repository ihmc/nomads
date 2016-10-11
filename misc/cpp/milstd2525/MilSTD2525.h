/*
 * MilSTD2525.h
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

#ifndef INCL_MIL_STANDARD_2525
#define	INCL_MIL_STANDARD_2525

namespace IHMC_MISC_MIL_STD_2525
{
    enum Affiliation
    {
        A_Pending,
        A_Unknown,
        A_Assumed_Friend,
        A_Friend,
        A_Neutral,
        A_Suspect,
        A_Hostile,
        A_Exercise_Pending,
        A_Joker,
        A_Faker,
        A_Exercise_Unknown,
        A_Exercise_Assumed_Friend,
        A_Exercise_Friend,
        A_Exercise_Neutral,
        A_None,
        A_Unset,
        A_Error
    };

    enum BattleDimension
    {
        BD_Space,
        BD_Air,
        BD_Ground,
        BD_Sea_Surface,
        BD_Sea_Subsurface,
        BD_SOF,
        BD_Other,
        BD_Unset,
        BD_Error
    };

    enum CodingScheme
    {
        CS_War_Fighting,
        CS_Tactical_Graphic,
        CS_METOC,
        CS_Intelligence,
        CS_Mapping,
        CS_Military_Operation_Other_Than_War,
        CS_Emergency_Management,
        CS_Unset,
        CS_Error
    };

    enum OrderOfBattle
    {
        OB_Air,
        OB_Electronic,
        OB_Civilian,
        OB_Ground,
        OB_Maritime,
        OB_Strategic_Force_Related,
        OB_Control_Markings,
        OB_Unset,
        OB_Error
    };

    enum Status
    {
        S_Anticipated_Or_Planned,
        S_Present,
        S_Unset,
        S_Error
    };

    const char * getAsString (BattleDimension bd);
    const char * getAsString (OrderOfBattle ob);
}

#endif	// INCL_MIL_STANDARD_2525


