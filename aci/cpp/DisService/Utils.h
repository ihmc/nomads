/*
 * Utils.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 * Created on May 27, 2011, 4:48 PM
 */

#ifndef INCL_STRING_UTILS_H
#define	INCL_STRING_UTILS_H

namespace IHMC_ACI
{
    class Utils
    {
        public:
            static bool sameElements (char **pszStringsA, char **pszStringsB);
    };
}

#endif	// INCL_STRING_UTILS_H

