/*
 * SymbolCodeTemplateTable.h
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

#ifndef INCL_MIL_STANDARD_2525_SYMBOL_CODE_TEMPLATE_TABLE_H
#define INCL_MIL_STANDARD_2525_SYMBOL_CODE_TEMPLATE_TABLE_H

#include "SymbolCodeTemplate.h"

#include "DArray2.h"
#include "PtrLList.h"

namespace IHMC_MISC_MIL_STD_2525
{
    class SymbolCodeTemplateTable
    {
        public:
            SymbolCodeTemplateTable (void);
            ~SymbolCodeTemplateTable (void);

            int addTemplate (const SymbolCodeTemplate &symbolCodeTemplate);
            void clear (void);
            SymbolCodeTemplate getBestMatchingTemplate (const SymbolCode &symbolCode);
            unsigned int getCount (void);

        private:
            typedef NOMADSUtil::PtrLList<SymbolCodeTemplate> MatchingTemplates;
            NOMADSUtil::DArray2<MatchingTemplates> _templates;
    };
}

#endif	// INCL_MIL_STANDARD_2525_SYMBOL_CODE_TEMPLATE_TABLE_H

