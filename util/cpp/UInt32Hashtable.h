/*
 * UInt32Hashtable.h
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
 * Hashtable with uint32 numbers as keys.
 *
 * This is the hashtable of choice when the keys are,
 * for example, memory addresses.  The advantages of using
 * uint32 keys instead of strings are that the storage needed
 * is considerably smaller, and also, key comparisons and
 * hash value computations are faster.
 *
 * By Raul Saavedra, May/20-29/2003
 */

#ifndef INCL_UINT32_HASHTABLE_H
#define INCL_UINT32_HASHTABLE_H

#include "FTypes.h"
#include "Logger.h"
#include "NLFLib.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>

#define F_THRESHOLD   0.75f
#define US_INITSIZE   53
#define MAXTABLESIZE  10000000l

namespace NOMADSUtil
{

    template <class T> class UInt32Hashtable
    {
        public:
            UInt32Hashtable (bool bDelValues);
            UInt32Hashtable (const unsigned long ulInitSize = US_INITSIZE, bool bDelValues = false);

            virtual ~UInt32Hashtable (void);

            class Iterator
            {
                public:
                    ~Iterator (void) {
                        _pTable = NULL;
                        _ulIndex = 0;
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
                        while (++_ulIndex < _pTable->_ulTableSize) {
                            if (_pTable->_pHashtable[_ulIndex].pValue) {
                                _pCurrElement = &_pTable->_pHashtable[_ulIndex];
                                return true;
                            }
                        }
                        _pCurrElement = NULL;
                        return false;
                    }

                    uint32 getKey (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->ui32Key;
                        }
                        return 0;
                    }

                    T * getValue (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pValue;
                        }
                        return NULL;
                    }

                    Iterator (UInt32Hashtable<T> *pTable) {
                        _pTable = pTable;
                        _ulIndex = 0;
                        _pCurrElement = NULL;
                        while (_ulIndex < _pTable->_ulTableSize) {
                            if (_pTable->_pHashtable[_ulIndex].pValue) {
                                _pCurrElement = &_pTable->_pHashtable[_ulIndex];
                                break;
                            }
                            _ulIndex++;
                        }
                    }

                private:
                    UInt32Hashtable<T> *_pTable;
                    unsigned long _ulIndex;
                    void *_pCurrElement;
                private:
                    friend class UInt32Hashtable<T>;
            };

            virtual bool contains (uint32 ui32Key) const;
            virtual T * get (uint32 ui32Key) const;
            virtual T * put (uint32 ui32Key, T *pValue);
            virtual T * remove (uint32 ui32Key);
            void removeAll (void);

            // Returns a count of the number of elements currently in the hashtable
            virtual unsigned long getCount (void) const;

            // Returns the current size of the hashtable (NOTE: this is NOT the number of elements in the hashtable)
            virtual unsigned long getTableSize (void) const;

            virtual Iterator getAllElements (void);

            virtual void printStructure (void) const;

        protected:
            struct HashtableEntry
            {
                HashtableEntry (void) {
                    ui32Key  = 0;
                    pValue = NULL;
                    pNext  = NULL;
                }
                uint32 ui32Key;
                T *pValue;
                struct HashtableEntry *pNext;
            };

        protected:
            virtual int hashCode (uint32 ui32Key) const;
            void rehash (void);
            void deleteTable (HashtableEntry *pTable, unsigned long ulSize, bool bRehashing = false);

        private:
            HashtableEntry *_pHashtable;
            unsigned long _ulInitSize;
            unsigned long _ulTableSize;
            unsigned long _ulCount;
            bool _bDelValues;

        private:
            friend class Iterator;

    };

    template <class T> UInt32Hashtable<T>::UInt32Hashtable (bool bDelValues)
    {
        _ulCount = 0;
        _ulInitSize = US_INITSIZE;
        _pHashtable = new HashtableEntry[_ulInitSize];
        _ulTableSize = _ulInitSize;
        _bDelValues = bDelValues;
    }

    template <class T> UInt32Hashtable<T>::UInt32Hashtable (unsigned long ulInitSize, bool bDelValues)
    {
        _ulCount = 0;
        if (ulInitSize > 0) {
            _ulInitSize = ulInitSize;
        }
        else {
            _ulInitSize = US_INITSIZE;
        }
        _pHashtable = new HashtableEntry[_ulInitSize];
        _ulTableSize = _ulInitSize;
        _bDelValues = bDelValues;
    }

    template <class T> UInt32Hashtable<T>::~UInt32Hashtable (void)
    {
        deleteTable (_pHashtable, _ulTableSize);
        _pHashtable = NULL;
        _ulCount = 0;
        _ulTableSize = 0;
        _ulInitSize  = 0;
    }

    template <class T> T * UInt32Hashtable<T>::put (uint32 ui32Key, T *pValue)
    {
        // Can't accept a null value as element in the hashtable
        if (pValue == NULL) {
            return NULL;
        }
        // If hashtable is getting quite full for its size, then rehash
        unsigned long ulThreshold = (unsigned long) (_ulTableSize * F_THRESHOLD);
        if (_ulCount >= ulThreshold) {
            if (pLogger) {
                pLogger->logMsg ("UInt32Hashtable<T>::put", Logger::L_MediumDetailDebug, 
                                 "this uint32table has %lu elems with table size: %lu and threshold %lu\n", _ulCount, _ulTableSize, ulThreshold);
            }
            if (_ulTableSize > MAXTABLESIZE) {
                // HERE WE RUN INTO AN OUT OF MEMORY ERROR
                // We can't have a larger table
                if (pLogger) {
                    pLogger->logMsg ("UInt32Hashtable<T>::put", Logger::L_SevereError, 
                                     "this is too large a table, with %lu\n", _ulTableSize);
                }
            }
            rehash();
        }
        // Compute hashvalue which will be the bucket index
        int hashValue = hashCode (ui32Key);
        HashtableEntry *pHTE = &_pHashtable[hashValue];
        if (pHTE->pValue == NULL) {
            // This is the case where the bucket is empty, add entry to the bucket
            pHTE->pValue = pValue;
            pHTE->ui32Key = ui32Key;
            pHTE->pNext = NULL;
            _ulCount++;
            return NULL;
        }
        // Bucket is not empty
        if (pHTE->ui32Key == ui32Key) {
            // First element in bucket has the key
            T *pOldValue = pHTE->pValue;
            pHTE->pValue = pValue;
            // Don't check to see if old value should be deleted because
            //     that value gets returned
            return pOldValue;
        }
        // Search the bucket for an entry with that key 
        while (pHTE->pNext) {
            pHTE = pHTE->pNext;
            if (pHTE->ui32Key == ui32Key) {
                // Bucket contains an entry with that key,
                // replace value and return old value
                T *pOldValue = pHTE->pValue;
                pHTE->pValue = pValue;
                // Don't check to see if value should be deleted because
                //     the value is returned
                return pOldValue;
            }
        }
        // This is a new key, add a new entry to the bucket and return null
        HashtableEntry *pNew = new HashtableEntry;
        pNew->ui32Key = ui32Key;
        pNew->pValue = pValue;
        pNew->pNext = NULL;
        pHTE->pNext = pNew;
        _ulCount++;
        return NULL;
    }

    template <class T> bool UInt32Hashtable<T>::contains (uint32 ui32Key) const
    {
        int hashValue = hashCode (ui32Key);
        struct HashtableEntry *pHTE;
        for (pHTE = &_pHashtable[hashValue];  pHTE;  pHTE = pHTE->pNext) {
            if ((ui32Key == pHTE->ui32Key) && (pHTE->pValue)) {
                return true;
            }
        }
        return false;
    }

    template <class T> T * UInt32Hashtable<T>::get (uint32 ui32Key) const
    {
        int hashValue = hashCode(ui32Key);
        struct HashtableEntry *pHTE;
        for (pHTE = &_pHashtable[hashValue];  pHTE;  pHTE = pHTE->pNext) {
            if ((ui32Key == pHTE->ui32Key) && (pHTE->pValue)) {
                return (pHTE->pValue);
            }
        }
        return NULL;
    }

    template <class T> T * UInt32Hashtable<T>::remove (uint32 ui32Key)
    {
        // Compute hashvalue which will be the bucket index
        int hashValue = hashCode(ui32Key);
        HashtableEntry *pHTE = &_pHashtable[hashValue];
        if (pHTE->pValue == NULL) {
            // Hashtable bucket is empty - key not in the hashtable
            return NULL;
        }
        if (pHTE->ui32Key == ui32Key) {
            // First element in bucket contains that key
            // Deleting entry in hashtable array
            T *pOldValue = pHTE->pValue;
            if (pHTE->pNext) {
                // There are more elements in the bucket, shrink list
                HashtableEntry *pTemp = pHTE->pNext;
                pHTE->ui32Key = pTemp->ui32Key;
                pHTE->pValue = pTemp->pValue;
                pHTE->pNext = pTemp->pNext;
                // Delete entry
                delete pTemp;
            }
            else {
                // Bucket will be empty, set the pointer to null.
                pHTE->pValue = NULL;
                pHTE->ui32Key = 0;
            }
            _ulCount--;
            // Don't check to see if value should be deleted because
            //     the value is returned
            return pOldValue;
        }
        // Search the bucket for an entry with that key 
        HashtableEntry *pPrev = pHTE;
        pHTE = pHTE->pNext;
        while (pHTE) {
            if ((pHTE->ui32Key == ui32Key) && (pHTE->pValue)) {
                // Found the node
                T *pOldValue = pHTE->pValue;
                pPrev->pNext = pHTE->pNext;
                // Delete the entry
                delete pHTE;
                _ulCount--;
                // But don't check to see if value should be deleted because
                //     the value is returned
                return pOldValue;
            }
            else {
                pPrev = pHTE;
                pHTE = pHTE->pNext;
            }
        }
        return NULL;
    }

    template <class T> void UInt32Hashtable<T>::removeAll()
    {
        deleteTable (_pHashtable, _ulTableSize);
        _pHashtable = new HashtableEntry[_ulInitSize];
        _ulTableSize = _ulInitSize;
        _ulCount = 0;
    }

    template <class T> inline unsigned long UInt32Hashtable<T>::getCount (void) const
    {
        return _ulCount;
    }

    template <class T> inline unsigned long UInt32Hashtable<T>::getTableSize (void) const
    {
        return _ulTableSize;
    }

    template <class T> inline typename UInt32Hashtable<T>::Iterator UInt32Hashtable<T>::getAllElements (void)
    {
        return Iterator (this);
    }

    template <class T> void UInt32Hashtable<T>::printStructure (void) const
    {
        for (unsigned long ul = 0; ul < _ulTableSize; ul++) {
            unsigned long ulCount = 0;
            if (_pHashtable[ul].pValue) {
                ulCount++;
                HashtableEntry *pHTE = _pHashtable[ul].pNext;
                while (pHTE != NULL) {
                    ulCount++;
                    pHTE = pHTE->pNext;
                }
            }
            printf ("[%d] %d ", (int) ul, (int) ulCount);
            while (ulCount-- > 0) {
                printf ("*");
            }
            printf ("\n");
        }
    }

    template <class T> int UInt32Hashtable<T>::hashCode (uint32 ui32Key) const
    {
        // Horner's method calculating the hashcode
        // byte per byte within the long number,
        // as if char per char within a string

        // Check later whether by bit-shifting it might be faster
        // This scheme is somewhat simple because I don't need to know
        // how many bytes a long occupies in memory
        int iHashValue = 0;
        do {
            iHashValue = (64*iHashValue + ((int) (ui32Key % 256))) % _ulTableSize;
        } while ((ui32Key /= 256) > 0);
        return iHashValue;
    }

    template <class T> void UInt32Hashtable<T>::rehash()
    {
        HashtableEntry *pOldHashtable = _pHashtable;
        unsigned long ulOldSize = _ulTableSize;
        if (ulOldSize > 1048576) {
            // Don't grow the hashtable exponentially after a certain threshold
            _ulTableSize = ulOldSize + 1048576;
        }
        else {
            _ulTableSize = ulOldSize * 2 + 1;
        }
        _pHashtable = new HashtableEntry[_ulTableSize];
        _ulCount = 0;

        for (unsigned long ul = 0; ul < ulOldSize; ul++) {
            for (HashtableEntry *pHTE = &pOldHashtable[ul];  pHTE;  pHTE = pHTE->pNext) {
                if (pHTE->pValue) {
                    put (pHTE->ui32Key, pHTE->pValue);
                }
            }
        }

        deleteTable (pOldHashtable, ulOldSize, true);
    }

    template <class T> void UInt32Hashtable<T>::deleteTable (HashtableEntry *pTable, unsigned long ulSize, bool bRehashing)
    {
        for (unsigned long ul = 0; ul < ulSize; ul++) {
            HashtableEntry *pHTE = &pTable[ul];

            // Need to special case the first entry because the HTE
            // is part of the array and is not deleted
            if ((_bDelValues) && (!bRehashing)) {
                delete pHTE->pValue;
                pHTE->pValue = NULL;
            }
            pHTE = pHTE->pNext;
            while (pHTE) {
                HashtableEntry *pTmp = pHTE;
                pHTE = pHTE->pNext;
                if ((_bDelValues) && (!bRehashing)) {
                    delete pTmp->pValue;
                }
                delete pTmp;
            }
        }
        delete[] pTable;
    }

}

#endif   // #ifndef INCL_UINT32_HASHTABLE_H
