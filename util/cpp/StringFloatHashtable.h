/*
 * StringFloatHashtable.h
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
 */

#ifndef INCL_STRING_FLOAT_HASHTABLE_H
#define INCL_STRING_FLOAT_HASHTABLE_H

#include "StringHashtable.h"

namespace NOMADSUtil
{

    class StringFloatHashtable : public StringHashtable<float>
    {
        public:
            StringFloatHashtable (bool bCaseSensitiveKeys = true,
                                   bool bCloneKeys = true,
                                   bool bDeleteKeys = true,
                                   bool bCloneValues = true,
                                   bool bDeleteValues = true);
            StringFloatHashtable (unsigned short usInitSize,
                                   bool bCaseSensitiveKeys = true,
                                   bool bCloneKeys = true,
                                   bool bDeleteKeys = true,
                                   bool bCloneValues = true,
                                   bool bDeleteValues = true);

            // Puts the specified value into the hashtable using the specified key
            // Returns NULL if this is a new element or the value of the old element if
            //     an old element is being replaced
            // NOTE: If either pszKey or pszValue is NULL, then the method will simply
            //       return NULL
            // NOTE: Whether pszKey and pszValue are copied (cloned) depends on the
			//       parameters passed into the constructor
            float * put (const char *pszKey, float *pszValue);
        private:
            bool _bCloneValues;
    };

    inline StringFloatHashtable::StringFloatHashtable (bool bCaseSensitiveKeys,
                                                         bool bCloneKeys,
                                                         bool bDeleteKeys,
                                                         bool bCloneValues,
                                                         bool bDeleteValues)
        : StringHashtable<float> (bCaseSensitiveKeys,
                                 bCloneKeys,
                                 bDeleteKeys,
                                 bDeleteValues)
    {
        _bCloneValues = bCloneValues;
    }

    inline StringFloatHashtable::StringFloatHashtable (unsigned short usInitSize,
                                                         bool bCaseSensitiveKeys,
                                                         bool bCloneKeys,
                                                         bool bDeleteKeys,
                                                         bool bCloneValues,
                                                         bool bDeleteValues)
        : StringHashtable<float> (usInitSize,
                                 bCaseSensitiveKeys,
                                 bCloneKeys,
                                 bDeleteKeys,
                                 bDeleteValues)
    {
        _bCloneValues = bCloneValues;
    }

    inline float * StringFloatHashtable::put (const char *pszKey, float *pszValue)
    {
        if (pszKey == NULL) {
            return NULL;
        }
        if (pszValue == NULL) {
            return NULL;
        }
        if (_bCloneValues) {
            float *pszClonedValue = new float (*pszValue);
            return StringHashtable<float>::put (pszKey, pszClonedValue);
        }
        else {
            return StringHashtable<float>::put (pszKey, pszValue);
        }
    }

}

#endif   // #ifndef INCL_STRING_FLOAT_HASHTABLE_H
