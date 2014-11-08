/* 
 * StringStringWildMultimap.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on October 8, 2014, 11:34 PM
 */

#ifndef INCL_STRING_STRING_WILD_MULTIMAP_H
#define	INCL_STRING_STRING_WILD_MULTIMAP_H

#include "PtrLList.h"
#include "StringHashset.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class StringStringWildMultimap
    {
        public:
            StringStringWildMultimap (void);
            virtual ~StringStringWildMultimap (void);

            void put (const char *pszKey, const char *pszValue);
            bool hasKeyValue (const char *pszKey, const char *pszValue);

        private:
            struct KeyValues
            {
                KeyValues (const char *pszKey, const char *pszValue);
                ~KeyValues (void);

                const String key;
                StringHashset values;
            };
            PtrLList<KeyValues> _map;
    };
}

#endif	/* STRINGSTRINGWILDMULTIMAP_H */

