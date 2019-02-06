/*
 * StringStringHashtable.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_STRING_STRING_HASHTABLE_H
#define INCL_STRING_STRING_HASHTABLE_H

#include "StrClass.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{

    class StringStringHashtable : public StringHashtable<char>
    {
        public:
            StringStringHashtable (bool bCaseSensitiveKeys = true,
                                   bool bCloneKeys = true,
                                   bool bDeleteKeys = true,
                                   bool bCloneValues = true,
                                   bool bDeleteValues = true);
            StringStringHashtable (unsigned short usInitSize,
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
            char * put (const char *pszKey, char *pszValue);

            String toString (void);
            static StringStringHashtable * parseStringStringHashtable (const char *pszString);

        private:
            static const char INNER_SEPARATOR = ',';
            static const char OUTER_SEPARATOR = ';';

            bool _bCloneValues;
    };

    inline StringStringHashtable::StringStringHashtable (bool bCaseSensitiveKeys,
                                                         bool bCloneKeys,
                                                         bool bDeleteKeys,
                                                         bool bCloneValues,
                                                         bool bDeleteValues)
        : StringHashtable<char> (bCaseSensitiveKeys,
                                 bCloneKeys,
                                 bDeleteKeys,
                                 bDeleteValues)
    {
        _bCloneValues = bCloneValues;
    }

    inline StringStringHashtable::StringStringHashtable (unsigned short usInitSize,
                                                         bool bCaseSensitiveKeys,
                                                         bool bCloneKeys,
                                                         bool bDeleteKeys,
                                                         bool bCloneValues,
                                                         bool bDeleteValues)
        : StringHashtable<char> (usInitSize,
                                 bCaseSensitiveKeys,
                                 bCloneKeys,
                                 bDeleteKeys,
                                 bDeleteValues)
    {
        _bCloneValues = bCloneValues;
    }

    inline char * StringStringHashtable::put (const char *pszKey, char *pszValue)
    {
        if (pszKey == NULL) {
            return NULL;
        }
        if (pszValue == NULL) {
            return NULL;
        }
        if (_bCloneValues) {
            char *pszClonedValue = new char [strlen(pszValue)+1];
            strcpy (pszClonedValue, pszValue);
            return StringHashtable<char>::put (pszKey, pszClonedValue);
        }
        else {
            return StringHashtable<char>::put (pszKey, pszValue);
        }
    }

}

#endif   // #ifndef INCL_STRING_STRING_HASHTABLE_H
