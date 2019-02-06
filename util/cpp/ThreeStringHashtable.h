/*
 * ThreeStringHashtable.h
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

#ifndef INCL_THREE_STRING_HASHTABLE_H
#define INCL_THREE_STRING_HASHTABLE_H

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <iostream.h>

#if defined (UNIX)
    #define stricmp strcasecmp
#endif

namespace NOMADSUtil
{

    template <class T> class ThreeStringHashtable
    {
        public:
            ThreeStringHashtable (bool bCaseSensitiveKeys = true,
                                  bool bCloneKeys = false,
                                  bool bDeleteKeys = false);

            ThreeStringHashtable (unsigned short usInitSize,
                                  bool bCaseSensitiveKeys = true,
                                  bool bCloneKeys = false,
                                  bool bDeleteKeys = false);

            virtual ~ThreeStringHashtable (void);

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

                    bool nextElement (void) {
                        if (_pCurrElement) {
                            if (((HashtableEntry*)_pCurrElement)->pNext) {
                                _pCurrElement = ((HashtableEntry*)_pCurrElement)->pNext;
                                return true;
                            }
                        }
                        while (++_usIndex < _pTable->_usTableSize) {
                            if (_pTable->_pHashtable[_usIndex].pszKeyOne) {
                                _pCurrElement = &_pTable->_pHashtable[_usIndex];
                                return true;
                            }
                        }
                        _pCurrElement = NULL;
                        return false;
                    }

                    const char * getKeyOne (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pszKeyOne;
                        }
                        return NULL;
                    }

                    const char * getKeyTwo (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pszKeyTwo;
                        }
                        return NULL;
                    }

                    const char * getKeyThree (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pszKeyThree;
                        }
                        return NULL;
                    }


                    T *getValue (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pValue;
                        }
                        return NULL;
                    }

                private:
                    Iterator (ThreeStringHashtable<T> *pTable, unsigned long ulState) {
                        _pTable    = pTable;
                        _ulState   = ulState;
                        _usIndex   = 0;
                        _pCurrElement = NULL;
                        while (_usIndex < _pTable->_usTableSize) {
                            if (_pTable->_pHashtable[_usIndex].pszKeyOne) {
                                _pCurrElement = &_pTable->_pHashtable[_usIndex];
                                break;
                            }
                            _usIndex++;
                        }
                    }

                    ThreeStringHashtable<T> *_pTable;
                    unsigned long           _ulState;
                    unsigned short          _usIndex;
                    void                    *_pCurrElement;
                    static const float      fTHRESHOLD;

                private:
                    friend class ThreeStringHashtable<T>;
            };

            virtual T * put      (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, T *Value);
            virtual T * get      (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree);
            virtual T * getCaseS (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree);
            virtual T * getCaseI (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree);
            virtual T * remove   (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree);
            void removeAll (void);

            virtual unsigned short getCount (void);
            virtual Iterator getAllElements (void);

            virtual void printStructure (void);

        protected:
            struct HashtableEntry
            {
                HashtableEntry (void) {
                    pszKeyOne     = NULL;
                    pszKeyTwo     = NULL;
                    pszKeyThree   = NULL;
                    pNext         = NULL;
                    pNextLinear   = NULL;
                }
                char   *pszKeyOne;
                char   *pszKeyTwo;
                char   *pszKeyThree;
                T      *pValue;
                struct HashtableEntry *pNext;
                struct HashtableEntry *pNextLinear;
            };

        protected:
            virtual int keycomp  (const char *pszKeyOne, const char *pszKeyTwo);
            virtual int hashCode (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree);
            virtual int hashCode2(const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree);
            void        rehash   (void);
            void        deleteTable (HashtableEntry *pTable, unsigned short usSize);

        private:
            HashtableEntry *_pHashtable;
            unsigned short _usInitSize;
            unsigned short _usTableSize;
            unsigned short _usCount;
            unsigned long  _ulState;
            bool           _bCaseSensitiveKeys;
            bool           _bCloneKeys;
            bool           _bDeleteKeys;

            HashtableEntry *_pFirstAdded;
            HashtableEntry *_pLastAdded;
            bool           _bUseLinear;

        private:
            friend class Iterator;
    };

    template <class T> inline unsigned short ThreeStringHashtable<T>::getCount (void)
    {
        return _usCount;
    }

    template <class T> ThreeStringHashtable<T>::ThreeStringHashtable (bool bCaseSensitiveKeys,
                                                                      bool bCloneKeys,
                                                                      bool bDeleteKeys)
    {
        _usCount     = 0;
        _usInitSize  = 23;
        _pHashtable  = new HashtableEntry[_usInitSize];
        _usTableSize = _usInitSize;
        _ulState     = 0;
        _bCaseSensitiveKeys = bCaseSensitiveKeys;
        if (bCloneKeys) {
            _bCloneKeys  = true;
            _bDeleteKeys = true;
        }
        else {
            _bCloneKeys  = false;
            _bDeleteKeys = bDeleteKeys;
        }
        _pFirstAdded = _pHashtable;
        _pLastAdded  = NULL;
    }

    template <class T> ThreeStringHashtable<T>::ThreeStringHashtable (unsigned short usInitSize,
                                                                      bool bCaseSensitiveKeys,
                                                                      bool bCloneKeys,
                                                                      bool bDeleteKeys)
    {
        _usCount     = 0;
        _usInitSize  = (usInitSize>=5? usInitSize: 23);
        _pHashtable  = new HashtableEntry[_usInitSize];
        _usTableSize = _usInitSize;
        _ulState     = 0;
        _bCaseSensitiveKeys = bCaseSensitiveKeys;
        if (bCloneKeys) {
            _bCloneKeys  = true;
            _bDeleteKeys = true;
        }
        else {
            _bCloneKeys  = false;
            _bDeleteKeys = bDeleteKeys;
        }
        _pFirstAdded = _pHashtable;
        _pLastAdded  = NULL;
    }

    template <class T> ThreeStringHashtable<T>::~ThreeStringHashtable (void)
    {
        deleteTable (_pHashtable, _usTableSize);
        _pHashtable  = NULL;
        _usCount     = 0;
        _usTableSize = 0;
        _pFirstAdded = NULL;
        _pLastAdded  = NULL;
    }

    template <class T> T * ThreeStringHashtable<T>::put (const char *pszKeyOne,
                                                         const char *pszKeyTwo,
                                                         const char *pszKeyThree,
                                                         T* pValue)
    {
        if ( (pszKeyOne == NULL) || (pszKeyTwo == NULL) || (pszKeyThree == NULL) || (pValue == NULL) ){
            return NULL;
        }

        if (_usCount >= _usTableSize*0.75) {
            rehash();
        }
        if (_usCount >= 20) {
            // No longer use linear search after that many elements
            _pFirstAdded = NULL;
        }
        int hashValue = hashCode2(pszKeyOne, pszKeyTwo, pszKeyThree);
        HashtableEntry *pHTE = &_pHashtable[hashValue];
        if (pHTE->pszKeyOne == NULL) {
            if (_bCloneKeys) {
                pHTE->pszKeyOne   = new char[strlen(pszKeyOne)+1];
                pHTE->pszKeyTwo   = new char[strlen(pszKeyTwo)+1];
                pHTE->pszKeyThree = new char[strlen(pszKeyThree)+1];
                strcpy (pHTE->pszKeyOne,   pszKeyOne);
                strcpy (pHTE->pszKeyTwo,   pszKeyTwo);
                strcpy (pHTE->pszKeyThree, pszKeyThree);
            }
            else {
                pHTE->pszKeyOne   = (char*) pszKeyOne;
                pHTE->pszKeyTwo   = (char*) pszKeyTwo;
                pHTE->pszKeyThree = (char*) pszKeyThree;
            }
            pHTE->pValue = pValue;
            pHTE->pNext = NULL;
            if (_pFirstAdded != NULL) {
                if (_usCount) {
                    _pLastAdded->pNextLinear = pHTE;
                }
                else {
                    _pFirstAdded = pHTE;
                }
                _pLastAdded = pHTE;
            }
            _usCount++;
        }
        else if (
                 (0 == keycomp (pHTE->pszKeyTwo,   pszKeyTwo)) &&
                 (0 == keycomp (pHTE->pszKeyThree, pszKeyThree)) &&
                 (0 == keycomp (pHTE->pszKeyOne,   pszKeyOne))
                 ) {
            T * pOldValue = pHTE->pValue;
            pHTE->pValue = pValue;
            // Check to see if key1 or key2 or key3 should be deleted
            if (_bDeleteKeys) {
                delete[] pHTE->pszKeyOne;
                delete[] pHTE->pszKeyTwo;
                delete[] pHTE->pszKeyThree;
                pHTE->pszKeyOne = NULL;
                pHTE->pszKeyTwo = NULL;
                pHTE->pszKeyThree = NULL;
            }
            // Don't check to see if value should be deleted because
            //     the value is returned
            if (_bCloneKeys) {
                pHTE->pszKeyOne   = new char [strlen(pszKeyOne)+1];
                pHTE->pszKeyTwo   = new char [strlen(pszKeyTwo)+1];
                pHTE->pszKeyThree = new char [strlen(pszKeyThree)+1];
                strcpy (pHTE->pszKeyOne,   pszKeyOne);
                strcpy (pHTE->pszKeyTwo,   pszKeyTwo);
                strcpy (pHTE->pszKeyThree, pszKeyThree);
            }
            else {
                pHTE->pszKeyOne   = (char*) pszKeyOne;
                pHTE->pszKeyTwo   = (char*) pszKeyTwo;
                pHTE->pszKeyThree = (char*) pszKeyThree;
            }
            return pOldValue;
        }
        else {
            while (pHTE->pNext != NULL) {
                pHTE = pHTE->pNext;
                if (
                    (0 == keycomp (pHTE->pszKeyTwo,   pszKeyTwo)) &&
                    (0 == keycomp (pHTE->pszKeyThree, pszKeyThree)) &&
                    (0 == keycomp (pHTE->pszKeyOne,   pszKeyOne))
                    ) {
                    T * pOldValue = pHTE->pValue;
                    pHTE->pValue = pValue;
                    // Check to see if key should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKeyOne;
                        delete[] pHTE->pszKeyTwo;
                        delete[] pHTE->pszKeyThree;
                        pHTE->pszKeyOne = NULL;
                        pHTE->pszKeyTwo = NULL;
                        pHTE->pszKeyThree = NULL;
                    }
                    // Don't check to see if value should be deleted because
                    //     the value is returned
                    if (_bCloneKeys) {
                        pHTE->pszKeyOne   = new char [strlen(pszKeyOne)+1];
                        pHTE->pszKeyTwo   = new char [strlen(pszKeyTwo)+1];
                        pHTE->pszKeyThree = new char [strlen(pszKeyThree)+1];
                        strcpy (pHTE->pszKeyOne,   pszKeyOne);
                        strcpy (pHTE->pszKeyTwo,   pszKeyTwo);
                        strcpy (pHTE->pszKeyThree, pszKeyThree);
                    }
                    else {
                        pHTE->pszKeyOne   = (char*) pszKeyOne;
                        pHTE->pszKeyTwo   = (char*) pszKeyTwo;
                        pHTE->pszKeyThree = (char*) pszKeyThree;
                    }
                    return pOldValue;
                }
            }
            HashtableEntry *pNew = new HashtableEntry;
            if (_bCloneKeys) {
                pNew->pszKeyOne   = new char [strlen(pszKeyOne)+1];
                pNew->pszKeyTwo   = new char [strlen(pszKeyTwo)+1];
                pNew->pszKeyThree = new char [strlen(pszKeyThree)+1];
                strcpy (pNew->pszKeyOne,   pszKeyOne);
                strcpy (pNew->pszKeyTwo,   pszKeyTwo);
                strcpy (pNew->pszKeyThree, pszKeyThree);
            }
            else {
                pNew->pszKeyOne   = (char*) pszKeyOne;
                pNew->pszKeyTwo   = (char*) pszKeyTwo;
                pNew->pszKeyThree = (char*) pszKeyThree;
            }
            pNew->pValue = pValue;
            pNew->pNext = NULL;
            pHTE->pNext = pNew;
            if (_pFirstAdded != NULL) {
                _pLastAdded->pNextLinear = pNew;
                _pLastAdded = pNew;
            }
            _usCount++;
        }
        return NULL;
    }

    template <class T> T* ThreeStringHashtable<T>::getCaseS (const char *pszKeyOne,
                                                             const char *pszKeyTwo,
                                                             const char *pszKeyThree)
    {
        struct HashtableEntry *pHTE;
        // First try linear search if _pFirstAdded is not yet NULL
        for (pHTE = _pFirstAdded; pHTE != NULL; pHTE = pHTE->pNextLinear) {
            if (pHTE->pszKeyOne && pHTE->pszKeyTwo && pHTE->pszKeyThree) {
                if ((0 == strcmp (pszKeyTwo,   pHTE->pszKeyTwo)) &&
                    (0 == strcmp (pszKeyThree, pHTE->pszKeyThree)) &&
                    (0 == strcmp (pszKeyOne,   pHTE->pszKeyOne))) {
                    return (pHTE->pValue);
                }
            }
        }
        if ((_pFirstAdded != NULL) || (pszKeyTwo==NULL)) {
            return NULL;
        }
        // Compute the hashValue right here instead of invoking hashcode2()
        // (the inline definition or hashcode2 is not being honored by the compiler)
        const char * psk2 = pszKeyTwo;
        int hashValue = 0;
        while (*psk2 != '\0') {
            hashValue = (64*hashValue + *psk2) % _usTableSize;
            if (*(++psk2)=='\0') break;
            psk2++;
        }

        for (pHTE = &_pHashtable[hashValue]; pHTE != NULL; pHTE = pHTE->pNext) {
            if (pHTE->pszKeyOne && pHTE->pszKeyTwo && pHTE->pszKeyThree) {
                if ((0 == strcmp (pszKeyTwo,   pHTE->pszKeyTwo)) &&
                    (0 == strcmp (pszKeyThree, pHTE->pszKeyThree)) &&
                    (0 == strcmp (pszKeyOne,   pHTE->pszKeyOne))) {
                    return (pHTE->pValue);
                }
            }
        }
        return NULL;
    }



    template <class T> T* ThreeStringHashtable<T>::getCaseI (const char *pszKeyOne,
                                                             const char *pszKeyTwo,
                                                             const char *pszKeyThree)
    {
        struct HashtableEntry *pHTE;
        // First try linear search if _pFirstAdded is not yet NULL
        for (pHTE = _pFirstAdded; pHTE != NULL; pHTE = pHTE->pNextLinear) {
            if (pHTE->pszKeyOne && pHTE->pszKeyTwo && pHTE->pszKeyThree) {
                if ((0 == stricmp (pszKeyTwo,   pHTE->pszKeyTwo)) &&
                    (0 == stricmp (pszKeyThree, pHTE->pszKeyThree)) &&
                    (0 == stricmp (pszKeyOne,   pHTE->pszKeyOne))) {
                    return (pHTE->pValue);
                }
            }
        }
        if ((_pFirstAdded != NULL) || (pszKeyTwo==NULL)) {
            return NULL;
        }

        // Compute the hashValue right here instead of invoking hashcode2()
        // (the inline definition or hashcode2 is not being honored by the compiler)
        const char * psk2 = pszKeyTwo;
        int hashValue = 0;
        while (*psk2 != '\0') {
            hashValue = (64*hashValue + *psk2) % _usTableSize;
            if (*(++psk2)=='\0') break;
            psk2++;
        }

        for (pHTE = &_pHashtable[hashValue]; pHTE != NULL; pHTE = pHTE->pNext) {
            if (pHTE->pszKeyOne && pHTE->pszKeyTwo && pHTE->pszKeyThree) {
                if ((0 == stricmp (pszKeyTwo,   pHTE->pszKeyTwo)) &&
                    (0 == stricmp (pszKeyThree, pHTE->pszKeyThree)) &&
                    (0 == stricmp (pszKeyOne,   pHTE->pszKeyOne))) {
                    return (pHTE->pValue);
                }
            }
        }
        return NULL;
    }

    template <class T> inline T * ThreeStringHashtable<T>::get (const char *pszKeyOne,
                                                                const char *pszKeyTwo,
                                                                const char *pszKeyThree)
    {
        if (_bCaseSensitiveKeys) {
            return getCaseS (pszKeyOne, pszKeyTwo, pszKeyThree);
        }
        else {
            return getCaseI (pszKeyOne, pszKeyTwo, pszKeyThree);
        }
    }


    template <class T> inline int ThreeStringHashtable<T>::keycomp (const char *pszKeyOne, const char *pszKeyTwo)
    {
        if (_bCaseSensitiveKeys) {
            return strcmp (pszKeyOne, pszKeyTwo);
        }
        else {
            return stricmp (pszKeyOne, pszKeyTwo);
        }
    }

    template <class T> inline int ThreeStringHashtable<T>::hashCode (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree)
    {
        // Horner's method for strings calculating the hashcode on each character.
        int iHashValue;
        if (_bCaseSensitiveKeys) {
            for (iHashValue=0; *pszKeyOne != '\0'; pszKeyOne++) {
                iHashValue = (64*iHashValue + *pszKeyOne) % _usTableSize;
            }
            for (; *pszKeyTwo != '\0'; pszKeyTwo++) {
                iHashValue = (64*iHashValue + *pszKeyTwo) % _usTableSize;
            }
            for (; *pszKeyThree != '\0'; pszKeyThree++) {
                iHashValue = (64*iHashValue + *pszKeyThree) % _usTableSize;
            }
        }
        else {
            for (iHashValue=0; *pszKeyOne != '\0'; pszKeyOne++) {
                iHashValue = (64*iHashValue + tolower(*pszKeyOne)) % _usTableSize;
            }
            for (; *pszKeyTwo != '\0'; pszKeyTwo++) {
                iHashValue = (64*iHashValue + tolower(*pszKeyTwo)) % _usTableSize;
            }
            for (; *pszKeyThree != '\0'; pszKeyThree++) {
                iHashValue = (64*iHashValue + tolower(*pszKeyThree)) % _usTableSize;
            }
        }
        return iHashValue;
    }

    template <class T> inline int ThreeStringHashtable<T>::hashCode2 (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree)
    {
        // Horner's method for strings calculating the hashcode on every two characters.
        int iHashValue = 0;
        if (_bCaseSensitiveKeys) {
            while (*pszKeyOne != '\0') {
                   iHashValue = (64*iHashValue + *pszKeyOne) % _usTableSize;
                if (*(++pszKeyOne)=='\0') break;
                      pszKeyOne++;
            }
            while (*pszKeyTwo != '\0') {
                iHashValue = (64*iHashValue + *pszKeyTwo) % _usTableSize;
                if (*(++pszKeyTwo)=='\0') break;
                    pszKeyTwo++;
            }
            while (*pszKeyThree != '\0') {
                iHashValue = (64*iHashValue + *pszKeyThree) % _usTableSize;
                if (*(++pszKeyThree)=='\0') break;
                    pszKeyThree++;
            }
        }
        else {
            while (*pszKeyOne != '\0') {
                   iHashValue = (64*iHashValue + tolower(*pszKeyOne)) % _usTableSize;
                if (*(++pszKeyOne)=='\0') break;
                      pszKeyOne++;
            }
            while (*pszKeyTwo != '\0') {
                iHashValue = (64*iHashValue + tolower(*pszKeyTwo)) % _usTableSize;
                if (*(++pszKeyTwo)=='\0') break;
                    pszKeyTwo++;
            }
            while (*pszKeyThree != '\0') {
                iHashValue = (64*iHashValue + tolower(*pszKeyThree)) % _usTableSize;
                if (*(++pszKeyThree)=='\0') break;
                    pszKeyThree++;
            }
        }
        return iHashValue;
    }

    template <class T> T* ThreeStringHashtable<T>::remove (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree)
    {
        if (pszKeyTwo == NULL) {
            return NULL;
        }

        _pFirstAdded  = NULL; // Don't use linear search any more after any removal
        int hashValue = hashCode2(pszKeyOne, pszKeyTwo, pszKeyThree);
        HashtableEntry *pHTE = &_pHashtable[hashValue];
        if ((pHTE->pszKeyOne == NULL) && (pHTE->pszKeyTwo == NULL) && (pHTE->pszKeyThree == NULL)) {
            // Hashtable bucket is empty - therefore the keys must not exist
            return NULL;
        }
        else {
            if ((0 == keycomp (pHTE->pszKeyTwo,   pszKeyTwo)) &&
                (0 == keycomp (pHTE->pszKeyThree, pszKeyThree)) &&
                (0 == keycomp (pHTE->pszKeyOne,   pszKeyOne))) {
                // Deleting entry in hashtable array
                T *pOldValue = pHTE->pValue;
                if (pHTE->pNext) {
                    HashtableEntry *pTemp = pHTE->pNext;
                    // Check to see if key should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKeyOne;
                        delete[] pHTE->pszKeyTwo;
                        delete[] pHTE->pszKeyThree;
                    }
                    // Don't check to see if value should be deleted because
                    //     the value is returned
                    pHTE->pszKeyOne   = pTemp->pszKeyOne;
                    pHTE->pszKeyTwo   = pTemp->pszKeyTwo;
                    pHTE->pszKeyThree = pTemp->pszKeyThree;
                    pHTE->pValue       = pTemp->pValue;
                    pHTE->pNext       = pTemp->pNext;
                    delete pTemp;
                }
                else {
                    // Check to see if the keys should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKeyOne;
                        delete[] pHTE->pszKeyTwo;
                        delete[] pHTE->pszKeyThree;
                    }
                    // Otherwise set the pointers to null.
                    // Don't check to see if value should be deleted because
                    //     the value is returned
                    pHTE->pszKeyOne   = NULL;
                    pHTE->pszKeyTwo   = NULL;
                    pHTE->pszKeyThree = NULL;
                    pHTE->pValue       = NULL;
                }
                return pOldValue;
            }
            else {
                HashtableEntry *pPrev = pHTE;
                pHTE = pHTE->pNext;
                while (pHTE != NULL) {
                    if ((0 == keycomp (pHTE->pszKeyOne,   pszKeyOne)) &&
                        (0 == keycomp (pHTE->pszKeyTwo,   pszKeyTwo)) &&
                        (0 == keycomp (pHTE->pszKeyThree, pszKeyThree))) {
                        // Found the node
                        T * pOldValue = pHTE->pValue;
                        pPrev->pNext = pHTE->pNext;
                        // Check to see if the keys should be deleted
                        if (_bDeleteKeys) {
                            delete[] pHTE->pszKeyOne;
                            delete[] pHTE->pszKeyTwo;
                            delete[] pHTE->pszKeyThree;
                        }
                        // Don't check to see if value should be deleted because
                        //     the value is returned
                        delete pHTE;
                        return pOldValue;
                    }
                    else {
                        pPrev = pHTE;
                        pHTE = pHTE->pNext;
                    }
                }
            }
        }
        return NULL;
    }

    template <class T> void ThreeStringHashtable<T>::removeAll(void)
    {
        deleteTable (_pHashtable, _usTableSize);
        _pHashtable  = new HashtableEntry[_usInitSize];
        _usTableSize = _usInitSize;
        _usCount     = 0;
        _ulState     = 0;
        _pFirstAdded = _pHashtable; // We can use linear search again in the get method
        _pLastAdded  = NULL;
    }

    template <class T> inline typename ThreeStringHashtable<T>::Iterator ThreeStringHashtable<T>::getAllElements (void)
    {
        return Iterator (this, _ulState);
    }

    template <class T> void ThreeStringHashtable<T>::printStructure (void)
    {
        for (unsigned short us = 0; us < _usTableSize; us++) {
            unsigned short usCount = 0;
            if ((_pHashtable[us].pszKeyOne) && (_pHashtable[us].pszKeyTwo) && (_pHashtable[us].pszKeyThree)) {
                usCount++;
                HashtableEntry *pHTE = _pHashtable[us].pNext;
                while (pHTE != NULL) {
                    usCount++;
                    pHTE = pHTE->pNext;
                }
            }
            printf ("[%d] %d ", (int) us, (int) usCount);
            while (usCount-- > 0) {
                printf ("*");
            }
            printf ("\n");
        }
    }

    template <class T> void ThreeStringHashtable<T>::rehash()
    {
        HashtableEntry *pOldHashtable = _pHashtable;
        unsigned short usOldSize = _usTableSize;
        _usTableSize = usOldSize * 2 + 1;
        _pHashtable  = new HashtableEntry[_usTableSize];
        _usCount     = 0;
        _pFirstAdded = _pHashtable;
        _pLastAdded  = NULL;

        for (unsigned short us = 0; us < usOldSize; us++) {
            for (HashtableEntry *pHTE = &pOldHashtable[us]; pHTE != NULL; pHTE = pHTE->pNext) {
                if ((pHTE->pszKeyOne) && (pHTE->pszKeyTwo) && (pHTE->pszKeyThree)) {
                    put (pHTE->pszKeyOne, pHTE->pszKeyTwo, pHTE->pszKeyThree, pHTE->pValue);
                }
            }
        }
        deleteTable (pOldHashtable, usOldSize);
    }

    template <class T> void ThreeStringHashtable<T>::deleteTable (HashtableEntry *pTable, unsigned short usSize)
    {
        for (unsigned short us = 0; us < usSize; us++) {
            HashtableEntry *pHTE = pTable[us].pNext;
            while (pHTE != NULL) {
                HashtableEntry *pTmp = pHTE;
                pHTE = pHTE->pNext;
                if (_bDeleteKeys) {
                    delete[] pTmp->pszKeyOne;
                    delete[] pTmp->pszKeyTwo;
                    delete[] pTmp->pszKeyThree;
                }
                delete pTmp;
            }
        }
        delete[] pTable;
    }

}

#endif   // #ifndef INCL_THREE_STRING_HASHTABLE_H
