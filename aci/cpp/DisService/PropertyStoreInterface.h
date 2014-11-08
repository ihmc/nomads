/*
 * PropertyStoreInterface.h
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

#ifndef INCL_PROPERTY_STORE_INTERFACE_H
#define INCL_PROPERTY_STORE_INTERFACE_H

#include "StrClass.h"

#include <stddef.h>

namespace IHMC_ACI
{
    class PropertyStoreInterface
    {
        public:
            virtual ~PropertyStoreInterface (void);

            virtual int set (const char *pszNodeID, const char *pszAttr, const char *pszValue) = 0;
            virtual NOMADSUtil::String get (const char *pszNodeID, const char *pszAttr) = 0;
            virtual int remove (const char *pszNodeID, const char *pszAttr) = 0;
            virtual int update (const char *pszNodeID, const char *pszAttr, const char *pszNewValue) = 0;

        protected:
            PropertyStoreInterface (void);

    };
}

#endif   // #ifndef INCL_PROPERTY_STORE_INTERFACE_H
