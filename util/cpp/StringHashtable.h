/*
 * StringHashtable.h
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

#ifndef INCL_STRING_HASHTABLE_H
#define INCL_STRING_HASHTABLE_H

#pragma warning (disable:4786)

#include "FTypes.h"
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

    template <class T> class StringHashtable
    {
        public:
            StringHashtable (bool bCaseSensitiveKeys = true,
                             bool bCloneKeys = true,
                             bool bDeleteKeys = true,
                             bool bDeleteValues = false);

            StringHashtable (uint32 ui32InitSize,
                             bool bCaseSensitiveKeys = true,
                             bool bCloneKeys = true,
                             bool bDeleteKeys = true,
                             bool bDeleteValues = false);

            virtual ~StringHashtable (void);

            virtual void configure (bool bCaseSensitiveKeys,
                                    bool bCloneKeys,
                                    bool bDeleteKeys,
                                    bool bDeleteValues);

            class Iterator
            {
                public:
                    ~Iterator (void) {
                        _pTable = NULL;
                        _ulState = 0;
                        _ui32Index = 0;
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
                        while (++_ui32Index < _pTable->_ui32TableSize) {
                            if (_pTable->_pHashtable[_ui32Index].pszKey) {
                                _pCurrElement = &_pTable->_pHashtable[_ui32Index];
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

                    T * getValue (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pValue;
                        }
                        return NULL;
                    }

                    //bool IPCompare (const char *pszString, const char *pszTemplate);
                //private:
                    Iterator (StringHashtable<T> *pTable, unsigned long ulState) {
                        _pTable = pTable;
                        _ulState = ulState;
                        _ui32Index = 0;
                        _pCurrElement = NULL;
                        while (_ui32Index < _pTable->_ui32TableSize) {
                            if (_pTable->_pHashtable[_ui32Index].pszKey) {
                                _pCurrElement = &_pTable->_pHashtable[_ui32Index];
                                break;
                            }
                            _ui32Index++;
                        }
                    }

                private:
                    StringHashtable<T> *_pTable;
                    unsigned long _ulState;
                    uint32 _ui32Index;
                    void *_pCurrElement;
                private:
                    friend class StringHashtable<T>;
            };

            virtual T * put (const char *pszKey, T *pValue);
            virtual T * get (const char *pszKey) const;        
            virtual T * getWild (const char *pszKey);

            // Removes the matching entry from the hashtable and returns the value associated with the key
            // Returns NULL if the key does not exist
            // NOTE: The value is NOT DELETED even if bDeleteValues is set to true, so the caller is responsible for deleting the removed value
            virtual T * remove (const char *pszKey);

            void removeAll (void);

            bool containsKey (const char *pszKey) const;

            // Returns the size of the hashtable (not the number of elements in the hashtable)
            virtual uint32 getSize (void);

            // Returns the number of elements in the hashtable
            virtual uint32 getCount (void) const;

            virtual Iterator getAllElements (void);

            virtual void printStructure (void);

        protected:
            struct HashtableEntry
            {
                HashtableEntry (void) {
                    pszKey = NULL;
                    pValue = NULL;
                    pNext = NULL;
                }
                char *pszKey;
                T *pValue;
                struct HashtableEntry *pNext;
            };

        protected:
            virtual int keycomp (const char *pszKeyOne, const char *pszKeyTwo) const;
            virtual int hashCode (const char *pszKey) const;
            virtual int hashCode2 (const char *pszKey) const;
            void rehash (void);
            void deleteTable (HashtableEntry *pTable, uint32 ui32Size, bool bRehashing);

        private:
            HashtableEntry *_pHashtable;
            uint32 _ui32TableSize;
            uint32 _ui32Count;
            unsigned long _ulState;
            bool _bCaseSensitiveKeys;
            bool _bCloneKeys;
            bool _bDeleteKeys;
            bool _bDeleteValues;

        private:
            friend class Iterator;
    };

    template <class T> uint32 StringHashtable<T>::getCount (void) const
    {
        return _ui32Count;
    }

    template <class T> StringHashtable<T>::StringHashtable (bool bCaseSensitiveKeys,
                                                            bool bCloneKeys,
                                                            bool bDeleteKeys,
                                                            bool bDeleteValues)
    {
        _ui32Count = 0;
        _ui32TableSize = 23;
        _pHashtable = new HashtableEntry[_ui32TableSize];
        _ulState = 0;
        _bCaseSensitiveKeys = bCaseSensitiveKeys;
        if (bCloneKeys) {
            _bCloneKeys = true;
            _bDeleteKeys = true;
        }
        else {
            _bCloneKeys = false;
            _bDeleteKeys = bDeleteKeys;
        }
        _bDeleteValues = bDeleteValues;
    }

    template <class T> StringHashtable<T>::StringHashtable (uint32 ui32InitSize,
                                                            bool bCaseSensitiveKeys,
                                                            bool bCloneKeys,
                                                            bool bDeleteKeys,
                                                            bool bDeleteValues)
    {
        _ui32Count = 0;
        _ui32TableSize = ui32InitSize;
        _pHashtable = new HashtableEntry[_ui32TableSize];
        _ulState = 0;
        _bCaseSensitiveKeys = bCaseSensitiveKeys;
        if (bCloneKeys) {
            _bCloneKeys = true;
            _bDeleteKeys = true;
        }
        else {
            _bCloneKeys = false;
            _bDeleteKeys = bDeleteKeys;
        }
        _bDeleteValues = bDeleteValues;
    }

    template <class T> void StringHashtable<T>::configure (bool bCaseSensitiveKeys,
                                                           bool bCloneKeys,
                                                           bool bDeleteKeys,
                                                           bool bDeleteValues) 
    {
        _bCaseSensitiveKeys = bCaseSensitiveKeys;
        if (bCloneKeys) {
            _bCloneKeys = true;
            _bDeleteKeys = true;
        }
        else {
            _bCloneKeys = false;
            _bDeleteKeys = bDeleteKeys;
        }
        _bDeleteValues = bDeleteValues;
    }

    template <class T> StringHashtable<T>::~StringHashtable (void)
    {
        deleteTable (_pHashtable, _ui32TableSize, false);
        _pHashtable = NULL;
        _ui32Count = 0;
        _ui32TableSize = 0;
    }

    template <class T> T * StringHashtable<T>::put (const char *pszKey, T *pValue)
    {
        if (pszKey == NULL) {
            return NULL;
        }
        if (pValue == NULL) {
            return NULL;
        }
        if (pszKey[0] == 0) {
            return NULL;    // Cannot have a key of length 0
        }
        float fThreshold = 0.75;
        if (_ui32Count >= _ui32TableSize*fThreshold) {
            rehash();
        }
        int hashValue = hashCode(pszKey);
        HashtableEntry *pHTE = &_pHashtable[hashValue];
        if (pHTE->pszKey == NULL) {
            if (_bCloneKeys) {
                pHTE->pszKey = new char[strlen(pszKey)+1];
                strcpy (pHTE->pszKey, pszKey);
            }
            else {
                pHTE->pszKey = (char*) pszKey;
            }
            pHTE->pValue = pValue;
            pHTE->pNext = NULL;
            _ui32Count++;
        }
        else if (0 == keycomp (pHTE->pszKey, pszKey)) {
            T *pOldValue = pHTE->pValue;
            pHTE->pValue = pValue;
            // Check to see if key should be deleted
            if (_bDeleteKeys) {
                delete[] pHTE->pszKey;
                pHTE->pszKey = NULL;
            }
            // Don't check to see if value should be deleted because
            //     the value is returned
            if (_bCloneKeys) {
                pHTE->pszKey = new char [strlen(pszKey)+1];
                strcpy (pHTE->pszKey, pszKey);
            }
            else {
                pHTE->pszKey = (char*) pszKey;
            }
            return pOldValue;
        }
        else {
            while (pHTE->pNext != NULL) {
                pHTE = pHTE->pNext;
                if (0 == keycomp (pHTE->pszKey, pszKey)) {
                    T *pOldValue = pHTE->pValue;
                    pHTE->pValue = pValue;
                    // Check to see if key should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKey;
                        pHTE->pszKey = NULL;
                    }
                    // Don't check to see if value should be deleted because
                    //     the value is returned
                    if (_bCloneKeys) {
                        pHTE->pszKey = new char [strlen(pszKey)+1];
                        strcpy (pHTE->pszKey, pszKey);
                    }
                    else {
                        pHTE->pszKey = (char*) pszKey;
                    }
                    return pOldValue;
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
            pNew->pValue = pValue;
            pNew->pNext = NULL;
            pHTE->pNext = pNew;
            _ui32Count++;
        }
        return NULL;
    }

    template <class T> bool StringHashtable<T>::containsKey (const char *pszKey) const
    {
	    return (get(pszKey) != NULL);
    }

    template <class T> T * StringHashtable<T>::get (const char *pszKey) const
    {
        if (pszKey != NULL) {
            int hashValue = hashCode (pszKey);
            struct HashtableEntry *pHTE;
            for (pHTE = &_pHashtable[hashValue]; pHTE != NULL; pHTE = pHTE->pNext) {
                if (pHTE->pszKey != NULL) {
                    if (keycomp (pszKey, pHTE->pszKey) == 0) {
                        return (pHTE->pValue);
                    }
                }
            }
        }
        return NULL;
    }

    template <class T> uint32 StringHashtable<T>::getSize (void)
    {
        return _ui32TableSize;
    }

    template <class T> T * StringHashtable<T>::getWild (const char *pszKey)
    {
        int hashValue = hashCode (pszKey);
        struct HashtableEntry *pHTE;
        for (pHTE = &_pHashtable[hashValue]; pHTE != NULL; pHTE = pHTE->pNext) {
            if (pHTE->pszKey != NULL) {
                if (wildcardStringCompare (pszKey, pHTE->pszKey) == true) {
                    return (pHTE->pValue);
                }
            }
        }
        return NULL;
    }

    template <class T> int StringHashtable<T>::keycomp (const char *pszKeyOne, const char *pszKeyTwo) const
    {
        if (_bCaseSensitiveKeys) {
            return strcmp (pszKeyOne, pszKeyTwo);
        }
        else {
            return stricmp (pszKeyOne, pszKeyTwo);
        }
    }

    template <class T> int StringHashtable<T>::hashCode (const char *pszKey) const
    {
        // Horner's method for strings calculating the hashcode on each character.
        if (pszKey == NULL) {
            return 0;
        }
        int iHashValue;
        if (_bCaseSensitiveKeys) {
            for (iHashValue=0; *pszKey != '\0'; pszKey++) {
                iHashValue = (64*iHashValue + *pszKey) % _ui32TableSize;
            }
        }
        else {
            for (iHashValue=0; *pszKey != '\0'; pszKey++) {
                iHashValue = (64*iHashValue + tolower(*pszKey)) % _ui32TableSize;
            }
        }
        return iHashValue;
    }

    template <class T> int StringHashtable<T>::hashCode2 (const char *pszKey) const
    {
        // Horner's method for strings calculating the hashcode on every two characters.
        if (pszKey == NULL) {
            return 0;
        }
        int iHashValue;
        if (_bCaseSensitiveKeys) {
            for (iHashValue=0; *pszKey != '\0'; pszKey = pszKey + 2) {
                iHashValue = (64*iHashValue + *pszKey) % _ui32TableSize;
            }
        }
        else {
            for (iHashValue=0; *pszKey != '\0'; pszKey = pszKey + 2) {
                iHashValue = (64*iHashValue + tolower(*pszKey)) % _ui32TableSize;
            }
        }
        return iHashValue;
    }

    template <class T> T * StringHashtable<T>::remove (const char *pszKey)
    {
        int hashValue = hashCode(pszKey);
        HashtableEntry *pHTE = &_pHashtable[hashValue];
        if (pHTE->pszKey == NULL) {
            // Hashtable bucket is empty - therefore the key must not exist
            return NULL;
        }
        else {
            if (0 == keycomp (pHTE->pszKey, pszKey)) {
                // Deleting entry in hashtable array
                T *pOldValue = pHTE->pValue;
                if (pHTE->pNext) {
                    HashtableEntry *pTemp = pHTE->pNext;
                    // Check to see if key should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKey;
                    }
                    // Don't check to see if value should be deleted because
                    //     the value is returned
                    pHTE->pszKey = pTemp->pszKey;
                    pHTE->pValue = pTemp->pValue;
                    pHTE->pNext = pTemp->pNext;
                    delete pTemp;
                }
                else {
                    // Check to see if key should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKey;
                    }
                    // Otherwise set the pointers to null.
                    // Don't check to see if value should be deleted because
                    //     the value is returned
                    pHTE->pszKey = NULL;
                    pHTE->pValue = NULL;
                }
                _ui32Count--;
                return pOldValue;
            }
            else {
                HashtableEntry *pPrev = pHTE;
                pHTE = pHTE->pNext;
                while (pHTE != NULL) {
                    if (0 == keycomp (pHTE->pszKey, pszKey)) {
                        // Found the node
                        T *pOldValue = pHTE->pValue;
                        pPrev->pNext = pHTE->pNext;
                        // Check to see if key should be deleted
                        if (_bDeleteKeys) {
                            delete[] pHTE->pszKey;
                        }
                        // Don't check to see if value should be deleted because
                        //     the value is returned
                        delete pHTE;
                        _ui32Count--;
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

    template <class T> void StringHashtable<T>::removeAll()
    {
        deleteTable (_pHashtable, _ui32TableSize, false);    
        _pHashtable = new HashtableEntry[_ui32TableSize];
        _ui32Count = 0;
        _ulState = 0;
    }

    template <class T> typename StringHashtable<T>::Iterator StringHashtable<T>::getAllElements (void)
    {
        return Iterator (this, _ulState);
    }

    template <class T> void StringHashtable<T>::printStructure (void)
    {
        for (uint32 ui32 = 0; ui32 < _ui32TableSize; ui32++) {
            uint32 ui32Count = 0;
            if (_pHashtable[ui32].pszKey) {
                ui32Count++;
                HashtableEntry *pHTE = _pHashtable[ui32].pNext;
                while (pHTE != NULL) {
                    ui32Count++;
                    pHTE = pHTE->pNext;
                }
            }
            printf ("[%d] %d ", (int) ui32, (int) ui32Count);
            while (ui32Count-- > 0) {
                printf ("*");
            }
            printf ("\n");
        }
    }

    template <class T> void StringHashtable<T>::rehash()
    {
        HashtableEntry *pOldHashtable = _pHashtable;
        uint32 ui32OldSize = _ui32TableSize;
        _ui32TableSize = ui32OldSize * 2 + 1;
        _pHashtable = new HashtableEntry[_ui32TableSize];
        _ui32Count = 0;

        for (uint32 ui32 = 0; ui32 < ui32OldSize; ui32++) {
            for (HashtableEntry *pHTE = &pOldHashtable[ui32]; pHTE != NULL; pHTE = pHTE->pNext) {
                if (pHTE->pszKey) {
                    put (pHTE->pszKey, pHTE->pValue);
                }
            }
        }

        deleteTable (pOldHashtable, ui32OldSize, true);
    }

    template <class T> void StringHashtable<T>::deleteTable (HashtableEntry *pTable, uint32 ui32Size, bool bRehashing)
    {
        for (uint32 ui32 = 0; ui32 < ui32Size; ui32++) {
            HashtableEntry *pHTE = &pTable[ui32];

            // Need to special case the first entry because the HTE
            // is part of the array and is not deleted
            if (_bDeleteKeys) {
                delete[] pHTE->pszKey;
                pHTE->pszKey = NULL;
            }
            if ((!bRehashing) && (_bDeleteValues)) {
                delete pHTE->pValue;
                pHTE->pValue = NULL;
            }

            pHTE = pHTE->pNext;
            while (pHTE != NULL) {
                HashtableEntry *pTmp = pHTE;
                pHTE = pHTE->pNext;
                if (_bDeleteKeys) {
                    delete[] pTmp->pszKey;
                }
                if ((!bRehashing) && (_bDeleteValues)) {
                    delete pTmp->pValue;
                }
                delete pTmp;
            }
        }
        delete[] pTable;
    }

    /*
    template <class T> StringHashtable<T>::Iterator::Iterator (StringHashtable<T> *pTable, unsigned long ulState)
    {
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

    template <class T> StringHashtable<T>::Iterator::~Iterator (void)
    {
        _pTable = NULL;
        _ulState = 0;
        _usIndex = 0;
        _pCurrElement = NULL;
    }

    template <class T> bool StringHashtable<T>::Iterator::end (void)
    {
        return (_pCurrElement == NULL);
    }

    template <class T> bool StringHashtable<T>::Iterator::nextElement (void)
    {
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

    template <class T> const char * StringHashtable<T>::Iterator::getKey (void)
    {
        if (_pCurrElement) {
            return ((HashtableEntry*)_pCurrElement)->pszKey;
        }
        return NULL;
    }

    template <class T> T * StringHashtable<T>::Iterator::getValue (void)
    {
        if (_pCurrElement) {
            return ((HashtableEntry*)_pCurrElement)->pValue;
        }
        return NULL;
    }
    */

    /*template <class T> StringHashtable<T>::HashtableEntry::HashtableEntry (void)
    {
        pszKey = NULL;
        pValue = NULL;
        pNext = NULL;
    }*/

}

#endif   // #ifndef INCL_STRING_HASHTABLE_H
