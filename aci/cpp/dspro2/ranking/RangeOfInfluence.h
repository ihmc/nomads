/*
* RangeOfInfluence.h
*
* This file is part of the IHMC DSPro Library/Component
* Copyright (c) 2008-2016 IHMC.
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
*/

#ifndef INCL_RANGE_OF_INFLUENCE_H
#define INCL_RANGE_OF_INFLUENCE_H

#include "SymbolCodeTemplateTable.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    class JsonObject;
}

namespace IHMC_ACI
{
    class RangeOfInfluence
    {
        public:
            static const char * RANGE_OF_INFLUENCE_BY_MILSTD2525_SYMBOL_CODE;

            RangeOfInfluence (void);
            ~RangeOfInfluence (void);

            void clear (void);

            uint32 getRangeOfInfluence (const char *pszAttribute);
            uint32 getMaximumRangeOfInfluence (void);
            bool setRangeOfInfluence (const char *pszAttribute, uint32 ui32RangeOfInfluenceInMeters);

            void reset (void);

            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

        private:
            uint32 getRangeOfInfluence (IHMC_MISC_MIL_STD_2525::SymbolCode &milSTD2525Symbol);
            bool setRangeOfInfluence (const IHMC_MISC_MIL_STD_2525::SymbolCodeTemplate &milSTD2525SymbolTemplate,
                                      uint32 ui32RangeOfInfluenceInMeters);

            IHMC_MISC_MIL_STD_2525::SymbolCodeTemplateTable _symbols;
            NOMADSUtil::StringHashtable<uint32> _symbolCodeToRangeOfInfluence;
    };
}

#endif  /* INCL_RANGE_OF_INFLUENCE_H */
    