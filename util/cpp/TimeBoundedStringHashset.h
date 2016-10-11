/*
 * TimeBoundedStringHashset.h
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
 * Keeps track of a set of keys for at least the specified length of time
 * Keys are dropped from the hashtable once they have been stored for at least the specified length of time
 *
 * NOTE: For efficiency reasons, not every key is potentially checked for expiration every time. In particular,
 * only the first element in the hash bucket is checked. This should be adequate for most situations. The only
 * occasion where it might result in entries not being expired immediately is if there is more than one entry
 * per hash bucket, or if the storage duration was changed to a lower value while items were still in the hash set.
 *
 * File:   StringHashset.h
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Author: Niranjan Suri        (nsuri@ihmc.us)
 * Created on January 14, 2012, 1:08 AM
 */

#ifndef INCL_TIME_BOUNDED_STRING_HASHSET_H
#define INCL_TIME_BOUNDED_STRING_HASHSET_H

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
    class TimeBoundedStringHashset
    {
        public:
            /**
             * - ui32StorageDuration: the minimum duration (in milliseconds)
             *                        for which a key is kept
             * - usInitSize:
             * - bCaseSensitiveKeys
             * - bCloneKeys
             *  -bDeleteKeys
             */
            TimeBoundedStringHashset (uint32 ui32StorageDuration,
                                      unsigned short usInitSize = 23,
                                      bool bCaseSensitiveKeys = true,
                                      bool bCloneKeys = true,
                                      bool bDeleteKeys = true);

            virtual ~TimeBoundedStringHashset (void);

            /**
             * - ui32StorageDuration: the minimum duration (in milliseconds)
             *                        for which a key is kept
             * - usInitSize:
             * - bCaseSensitiveKeys
             * - bCloneKeys
             *  -bDeleteKeys
             */
            virtual void configure (uint32 ui32StorageDuration,
                                    bool bCaseSensitiveKeys = true,
                                    bool bCloneKeys = true,
                                    bool bDeleteKeys = true);

            /**
             * Return the minimum duration (in milliseconds) for which
             * a key is kept
             */
            uint32 getStorageDuration (void);

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

                    Iterator (TimeBoundedStringHashset *pTable, unsigned long ulState) {
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
                    TimeBoundedStringHashset *_pTable;
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
            // bDeleteKeys is set on true
            virtual void remove (const char *pszKey);

            void removeAll (void);

            bool containsKey (const char *pszKey);

            // Returns the size of the hashset (not the number of elements in
            // the hashset)
            virtual unsigned short getSize (void);

            // Returns the number of elements in the hashset
            virtual unsigned short getCount (void);

            virtual Iterator getAllElements (void);

            virtual void printStructure (void);

        protected:
            struct HashtableEntry
            {
                HashtableEntry (void) {
                    pszKey = NULL;
                    pNext = NULL;
                    i64ExpirationTime = 0;
                }
                char *pszKey;
                struct HashtableEntry *pNext;
                int64 i64ExpirationTime;
            };

        protected:
            virtual bool put (const char *pszKey, int64 i64ExpirationTime);
            virtual int keycomp (const char *pszKeyOne, const char *pszKeyTwo);
            virtual int hashCode (const char *pszKey);
            virtual int hashCode2 (const char *pszKey);
            void rehash (void);
            void expireEntry (HashtableEntry *pHTE);
            void expireEntries (void);
            void deleteTable (HashtableEntry *pTable, unsigned short usSize);

        private:
            HashtableEntry *_pHashtable;
            uint32 _ui32StorageDuration;
            unsigned short _usTableSize;
            unsigned short _usCount;
            unsigned long _ulState;
            bool _bCaseSensitiveKeys;
            bool _bCloneKeys;
            bool _bDeleteKeys;

        private:
            friend class Iterator;
    };

    inline uint32 TimeBoundedStringHashset::getStorageDuration (void)
    {
        return _ui32StorageDuration;
    }

    inline unsigned short TimeBoundedStringHashset::getCount (void)
    {
        return _usCount;
    }

    inline bool TimeBoundedStringHashset::put (const char *pszKey)
    {
        return put (pszKey, getTimeInMilliseconds() + _ui32StorageDuration);
    }

    inline bool TimeBoundedStringHashset::put (const char *pszKey, int64 i64ExpirationTime)
    {
        if (pszKey == NULL) {
            return false;
        }
        if (pszKey[0] == 0) {
            return false;    // Cannot have a key of length 0
        }   
        int hashValue = hashCode (pszKey);
        HashtableEntry *pHTE = &_pHashtable[hashValue];
        expireEntry (pHTE);
        float fThreshold = 0.75;
        if (_usCount >= _usTableSize*fThreshold) {
            rehash();
            hashValue = hashCode (pszKey);
            pHTE = &_pHashtable[hashValue];
        }
        if (pHTE->pszKey == NULL) {
            if (_bCloneKeys) {
                pHTE->pszKey = new char[strlen(pszKey)+1];
                strcpy (pHTE->pszKey, pszKey);
            }
            else {
                pHTE->pszKey = (char*) pszKey;
            }
            pHTE->pNext = NULL;
            pHTE->i64ExpirationTime = i64ExpirationTime;
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
            pHTE->i64ExpirationTime = i64ExpirationTime;
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
                    pHTE->i64ExpirationTime = i64ExpirationTime;
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
            pNew->i64ExpirationTime = i64ExpirationTime;
            pHTE->pNext = pNew;
            _usCount++;
        }
        return true;
    }

    inline bool TimeBoundedStringHashset::containsKey (const char *pszKey)
    {
        if (pszKey == NULL) {
            return false;
        }
        int hashValue = hashCode (pszKey);
        struct HashtableEntry *pHTE;
        for (pHTE = &_pHashtable[hashValue]; pHTE != NULL; pHTE = pHTE->pNext) {
            if (pHTE->pszKey != NULL) {
                if (keycomp (pszKey, pHTE->pszKey) == 0) {
                    if (pHTE->i64ExpirationTime < getTimeInMilliseconds()) {
                        expireEntry (pHTE);
                        return false;
                    }
                    return true;
                }
            }
        }
        return false;
    }

    inline unsigned short TimeBoundedStringHashset::getSize (void)
    {
        return _usTableSize;
    }

    inline int TimeBoundedStringHashset::keycomp (const char *pszKeyOne, const char *pszKeyTwo)
    {
        if (_bCaseSensitiveKeys) {
            return strcmp (pszKeyOne, pszKeyTwo);
        }
        else {
            return stricmp (pszKeyOne, pszKeyTwo);
        }
    }

    inline int TimeBoundedStringHashset::hashCode (const char *pszKey)
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

    inline int TimeBoundedStringHashset::hashCode2 (const char *pszKey)
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

#endif   // #ifndef INCL_TIME_BOUNDED_STRING_HASHSET_H
