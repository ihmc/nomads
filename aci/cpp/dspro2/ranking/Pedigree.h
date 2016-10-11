/* 
 * Pedigree.h
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on August 12, 2011, 12:03 PM
 */

#ifndef INCL_PEDIGREE_H
#define	INCL_PEDIGREE_H

#include "StrClass.h"

namespace IHMC_ACI
{
    class Pedigree
    {
        public:
            Pedigree (const char *pszSource);
            Pedigree (const char *pszSource, const char *pszPedigree);
            virtual ~Pedigree (void);

            const char * toString (void) const;

            bool containsNodeID (const char *pszNodeID, bool bCheckSource) const;

            /**
             * Add the pedigree in ped to the current pedigree
             */
            Pedigree & operator += (const Pedigree &ped);

            /**
             * Adds the _node_ to the pedigree (the separator is automatically
             * added)
             */
            Pedigree & operator += (const NOMADSUtil::String &str);
            Pedigree & operator += (const char *pszStr);

        private:
            static const char SEPARATOR;
            NOMADSUtil::String _source;
            NOMADSUtil::String _pedigree;
    };
}

#endif	// INCL_PEDIGREE_H

