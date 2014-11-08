/*
 * Utils.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#include "Utils.h"

#include "NLFLib.h"

#include <stdlib.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

bool Utils::sameElements (char **pszStringsA, char **pszStringsB)
{
    if (pszStringsA == NULL) {
        if (pszStringsB == NULL) {
            return true;
        }
        return false;
    }
    if (pszStringsB == NULL) {
        if (pszStringsA == NULL) {
            return true;
        }
        return false;
    }

    if (pszStringsA[0] == NULL) {
        if (pszStringsB[0] == NULL) {
            return true;
        }
        return false;
    }

    bool bFound;
    for (int i = 0; pszStringsA[i] != NULL; i++) {
        bFound = false;
        for (int j = 0; (pszStringsB[j] != NULL) && (!bFound); j++) {
            if (strNotNullAndEqual (pszStringsA[i], pszStringsA[j])) {
                bFound = true;
            }
        }
        if (!bFound) {
            return false;
        }
    }
    return true;
}

