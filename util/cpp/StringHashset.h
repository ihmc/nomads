/*
 * StringHashset.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on January 14, 2012, 1:08 AM
 */

#ifndef INCL_STRING_HASHSET_H
#define INCL_STRING_HASHSET_H

#pragma warning (disable:4786)

#include "NLFLib.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>

#if defined (WIN32)
    #define stricmp _stricmp
#elif defined (UNIX)
    #include <strings.h>
    #define stricmp strcasecmp
#endif

namespace NOMADSUtil
{

    class StringHashset
    {
        public:
            StringHashset (bool bCaseSensitiveKeys = true,
                           bool bCloneKeys = true,
                           bool bDeleteKeys = true);

            StringHashset (unsigned short usInitSize,
                           bool bCaseSensitiveKeys = true,
                           bool bCloneKeys = true,
                           bool bDeleteKeys = true);

            virtual ~StringHashset (void);

            virtual void configure (bool bCaseSensitiveKeys,
                                    bool bCloneKeys,
                                    bool bDeleteKeys);

            class Iterator
            {
                public:
                    ~Iterator (void) {
                        _pTable = NULL;
                        _ulState = 0;
                        _usIndex = 0;
                        _pCurrElement = NULL;
                    }

                    bool end (void) {
                        return (_pCurrElement == NULL);
                    }

                    // Advances the enumeration to the next element
                    // NOTE: nextElement() works differently than in Java enumerations.
                    //       In Java, you have to call nextElement() in order to get the
                    //       first element. In this case, when an enumeration is returned,
                    //       the enumeration is already pointing to the first element, which
                    //       can be retrieved using getKey() and getValue(). Calling nextElement()
                    //       will advance the enumeration to the second element.
                    bool nextElement (void) {
                        if (_pCurrElement) {
                            if (((HashtableEntry*)_pCurrElement)->pNext) {
                                _pCurrElement = ((HashtableEntry*)_pCurrElement)->pNext;
                                return true;
                            }
                        }
                        while (++_usIndex < _pTable->_usTableSize) {
                            if (_pTable->_pHashtable[_usIndex].pszKey) {
                                _pCurrElement = &_pTable->_pHashtable[_usIndex];
                                return true;
                            }
                        }
                        _pCurrElement = NULL;
                        return false;
                    }

                    const char * getKey (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pszKey;
                        }
                        return NULL;
                    }

                    Iterator (StringHashset *pTable, unsigned long ulState) {
                        _pTable = pTable;
                        _ulState = ulState;
                        _usIndex = 0;
                        _pCurrElement = NULL;
                        while (_usIndex < _pTable->_usTableSize) {
                            if (_pTable->_pHashtable[_usIndex].pszKey) {
                                _pCurrElement = &_pTable->_pHashtable[_usIndex];
                                break;
                            }
                            _usIndex++;
                        }
                    }

                private:
                    StringHashset *_pTable;
                    unsigned long _ulState;
                    unsigned short _usIndex;
                    void *_pCurrElement;
                private:
                    friend class StringHashset;
            };

            // Returns true if pszKey was not already in the set and was
            // successfully inserted.  Returns false otherwise
            virtual bool put (const char *pszKey);

            // Removes the matching entry from the hashset and deallocates it if
            // bDeleteKeys is set on true.
            // Returns true if the element was found and removed, false otherwise
            virtual bool remove (const char *pszKey);

            void removeAll (void);
            void removeAll (StringHashset &set);

            bool containsKey (const char *pszKey) const;
            bool containsKeyWild (const char *pszKey);

            // Returns the size of the hashset (not the number of elements in
            // the hashset)
            virtual unsigned short getSize (void) const;

            // Returns the number of elements in the hashset
            virtual unsigned short getCount (void) const;

            virtual Iterator getAllElements (void);

            virtual void printStructure (void);

        protected:
            struct HashtableEntry
            {
                HashtableEntry (void) {
                    pszKey = NULL;
                    pNext = NULL;
                }
                char *pszKey;
                struct HashtableEntry *pNext;
            };

        protected:
            virtual int keycomp (const char *pszKeyOne, const char *pszKeyTwo) const;
            virtual int hashCode (const char *pszKey) const;
            virtual int hashCode2 (const char *pszKey) const;
            void rehash (void);
            void deleteTable (HashtableEntry *pTable, unsigned short usSize);

        private:
            HashtableEntry *_pHashtable;
            unsigned short _usTableSize;
            unsigned short _usCount;
            unsigned long _ulState;
            bool _bCaseSensitiveKeys;
            bool _bCloneKeys;
            bool _bDeleteKeys;

        private:
            friend class Iterator;
    };

    inline unsigned short StringHashset::getCount (void) const
    {
        return _usCount;
    }

    inline bool StringHashset::put (const char *pszKey)
    {
        if (pszKey == NULL) {
            return false;
        }
        if (pszKey[0] == 0) {
            return false;    // Cannot have a key of length 0
        }
        float fThreshold = 0.75;
        if (_usCount >= _usTableSize*fThreshold) {
            rehash();
        }
        int hashValue = hashCode (pszKey);
        HashtableEntry *pHTE = &_pHashtable[hashValue];
        if (pHTE->pszKey == NULL) {
            if (_bCloneKeys) {
                pHTE->pszKey = new char[strlen(pszKey)+1];
                strcpy (pHTE->pszKey, pszKey);
            }
            else {
                pHTE->pszKey = (char*) pszKey;
            }
            pHTE->pNext = NULL;
            _usCount++;
        }
        else if (0 == keycomp (pHTE->pszKey, pszKey)) {
            // Check to see if key should be deleted
            if (_bDeleteKeys) {
                delete[] pHTE->pszKey;
                pHTE->pszKey = NULL;
            }
            if (_bCloneKeys) {
                pHTE->pszKey = new char [strlen(pszKey)+1];
                strcpy (pHTE->pszKey, pszKey);
            }
            else {
                pHTE->pszKey = (char*) pszKey;
            }
            return false;
        }
        else {
            while (pHTE->pNext != NULL) {
                pHTE = pHTE->pNext;
                if (0 == keycomp (pHTE->pszKey, pszKey)) {
                    // Check to see if key should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKey;
                        pHTE->pszKey = NULL;
                    }
                    if (_bCloneKeys) {
                        pHTE->pszKey = new char [strlen(pszKey)+1];
                        strcpy (pHTE->pszKey, pszKey);
                    }
                    else {
                        pHTE->pszKey = (char*) pszKey;
                    }
                    return false;
                }
            }
            HashtableEntry *pNew = new HashtableEntry;
            if (_bCloneKeys) {
                pNew->pszKey = new char [strlen(pszKey)+1];
                strcpy (pNew->pszKey, pszKey);
            }
            else {
                pNew->pszKey = (char*) pszKey;
            }
            pNew->pNext = NULL;
            pHTE->pNext = pNew;
            _usCount++;
        }
        return true;
    }

    inline bool StringHashset::containsKey (const char *pszKey) const
    {
        if (pszKey == NULL) {
            return false;
        }
        int hashValue = hashCode (pszKey);
        struct HashtableEntry *pHTE;
        for (pHTE = &_pHashtable[hashValue]; pHTE != NULL; pHTE = pHTE->pNext) {
            if (pHTE->pszKey != NULL) {
                if (keycomp (pszKey, pHTE->pszKey) == 0) {
                    return true;
                }
            }
        }
        return false;
    }

    inline bool StringHashset::containsKeyWild (const char *pszKey)
    {
        if (pszKey == NULL) {
            return false;
        }
        if (containsKey (pszKey)) {
            return true;
        }
        for (Iterator iter = getAllElements(); !iter.end(); iter.nextElement()) {
            if (wildcardStringCompare (iter.getKey(), pszKey) ||
                wildcardStringCompare (pszKey, iter.getKey())) {
                return true;
            }
        }
        return false;
    }

    inline unsigned short StringHashset::getSize (void) const
    {
        return _usTableSize;
    }

    inline int StringHashset::keycomp (const char *pszKeyOne, const char *pszKeyTwo) const
    {
        if (_bCaseSensitiveKeys) {
            return strcmp (pszKeyOne, pszKeyTwo);
        }
        else {
            return stricmp (pszKeyOne, pszKeyTwo);
        }
    }

    inline int StringHashset::hashCode (const char *pszKey) const
    {
        // Horner's method for strings calculating the hashcode on each character.
        if (pszKey == NULL) {
            return 0;
        }
        int iHashValue;
        if (_bCaseSensitiveKeys) {
            for (iHashValue=0; *pszKey != '\0'; pszKey++) {
                iHashValue = (64*iHashValue + *pszKey) % _usTableSize;
            }
        }
        else {
            for (iHashValue=0; *pszKey != '\0'; pszKey++) {
                iHashValue = (64*iHashValue + tolower(*pszKey)) % _usTableSize;
            }
        }
        return iHashValue;
    }

    inline int StringHashset::hashCode2 (const char *pszKey) const
    {
        // Horner's method for strings calculating the hashcode on every two characters.
        if (pszKey == NULL) {
            return 0;
        }
        int iHashValue;
        if (_bCaseSensitiveKeys) {
            for (iHashValue=0; *pszKey != '\0'; pszKey = pszKey + 2) {
                iHashValue = (64*iHashValue + *pszKey) % _usTableSize;
            }
        }
        else {
            for (iHashValue=0; *pszKey != '\0'; pszKey = pszKey + 2) {
                iHashValue = (64*iHashValue + tolower(*pszKey)) % _usTableSize;
            }
        }
        return iHashValue;
    }
}

#endif   // #ifndef INCL_STRING_HASHSET_H

