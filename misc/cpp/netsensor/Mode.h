#ifndef NETSENSOR_Embedding_Mode__INCLUDED
#define NETSENSOR_Embedding_Mode__INCLUDED
/*
* Mode.h
* Author: rfronteddu@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
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

namespace IHMC_NETSENSOR
{
    enum class Mode
    {
        EM_NONE = 0,
        EM_NETPROXY = 1,
        EM_REPLAY = 2,
    };
}
#endif