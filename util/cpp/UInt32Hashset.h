/*
 * UInt32Hashset.h
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
 * Hashset for UInt32 values.
 *
 * This is the Hashset of choice when the keys are,
 * for example, memory addresses.  The advantages of using
 * uint32 keys instead of strings are that the storage needed
 * is considerably smaller, and also, key comparisons and
 * hash value computations are faster.
 *
 * By Raul Saavedra, May/20-29/2003
 */

#ifndef INCL_UINT32_HASHSET_H
#define INCL_UINT32_HASHSET_H

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

    class UInt32Hashset
    {
        public:
            UInt32Hashset (const unsigned long ulInitSize = US_INITSIZE);

            virtual ~UInt32Hashset (void);

            class Iterator
            {
                public:
                    ~Iterator (void) {
                        _pSet = NULL;
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
                            if (((HashsetEntry*)_pCurrElement)->pNext) {
                                _pCurrElement = ((HashsetEntry*)_pCurrElement)->pNext;
                                return true;
                            }
                        }
                        while (++_ulIndex < _pSet->_ulSetSize) {
                            if (_pSet->_pHashset[_ulIndex].bInUse) {
                                _pCurrElement = &(_pSet->_pHashset[_ulIndex]);
                                return true;
                            }
                        }
                        _pCurrElement = NULL;
                        return false;
                    }

                    uint32 getKey (void) {
                        if (_pCurrElement) {
                            return ((HashsetEntry*)_pCurrElement)->ui32Key;
                        }
                        return 0;
                    }

                    Iterator (const UInt32Hashset *pSet) {
                        _pSet = pSet;
                        _ulIndex = 0;
                        _pCurrElement = NULL;
                        while (_ulIndex < _pSet->_ulSetSize) {
                            if (_pSet->_pHashset[_ulIndex].bInUse) {
                                _pCurrElement = &_pSet->_pHashset[_ulIndex];
                                break;
                            }
                            _ulIndex++;
                        }
                    }

                private:
                    const UInt32Hashset *_pSet;
                    unsigned long _ulIndex;
                    void *_pCurrElement;
                private:
                    friend class UInt32Hashset;
            };

            virtual bool contains (uint32 ui32Key) const;
            virtual void put (uint32 ui32Key);
            virtual bool remove (uint32 ui32Key);
            void removeAll (void);

            // Returns a count of the number of elements currently in the Hashset
            virtual unsigned long getCount (void) const;

            // Returns the current size of the Hashset (NOTE: this is NOT the number of elements in the Hashset)
            virtual unsigned long getSetSize (void) const;

            virtual Iterator getAllElements (void) const;

            virtual void printStructure (void) const;

        protected:
            struct HashsetEntry
            {
                HashsetEntry (void) {
                    ui32Key  = 0;
                    bInUse = false;
                    pNext  = NULL;
                }
                uint32 ui32Key;
                bool bInUse;
                struct HashsetEntry *pNext;
            };

        protected:
            virtual int hashCode (uint32 ui32Key) const;
            void rehash (void);
            void deleteSet (HashsetEntry *pTable, unsigned long ulSize, bool bRehashing = false);

        private:
            HashsetEntry *_pHashset;
            unsigned long _ulInitSize;
            unsigned long _ulSetSize;
            unsigned long _ulCount;

        private:
            friend class Iterator;
    };

    inline UInt32Hashset::UInt32Hashset (unsigned long ulInitSize)
    {
        _ulCount = 0;
        if (ulInitSize > 0) {
            _ulInitSize = ulInitSize;
        }
        else {
            _ulInitSize = US_INITSIZE;
        }
        _pHashset = new HashsetEntry[_ulInitSize];
        _ulSetSize = _ulInitSize;
    }

    inline UInt32Hashset::~UInt32Hashset (void)
    {
        deleteSet (_pHashset, _ulSetSize);
        _pHashset = NULL;
        _ulCount = 0;
        _ulSetSize = 0;
        _ulInitSize  = 0;
    }

    inline void UInt32Hashset::put (uint32 ui32Key)
    {
        // If Hashset is getting quite full for its size, then rehash
        unsigned long ulThreshold = (unsigned long) (_ulSetSize * F_THRESHOLD);
        if (_ulCount >= ulThreshold) {
            if (pLogger) {
                pLogger->logMsg ("UInt32Hashset::put", Logger::L_MediumDetailDebug,
                                 "this UInt32Hashset has %lu elems with set size: %lu and threshold %lu\n", _ulCount, _ulSetSize, ulThreshold);
            }
            if (_ulSetSize > MAXTABLESIZE) {
                // HERE WE RUN INTO AN OUT OF MEMORY ERROR
                // We can't have a larger set
                if (pLogger) {
                    pLogger->logMsg ("UInt32Hashset::put", Logger::L_SevereError,
                                     "this is too large a set, with %lu\n", _ulSetSize);
                }
            }
            rehash();
        }
        // Compute hashvalue which will be the bucket index
        int hashValue = hashCode (ui32Key);
        HashsetEntry *pHTE = &_pHashset[hashValue];
        if (!pHTE->bInUse) {
            // This is the case where the bucket is empty, add entry to the bucket
            pHTE->bInUse = true;
            pHTE->ui32Key = ui32Key;
            pHTE->pNext = NULL;
            _ulCount++;
            return;
        }
        // Bucket is not empty
        if (pHTE->ui32Key == ui32Key) {
            // First element in bucket has the key
            return;
        }
        // Search the bucket for an entry with that key
        while (pHTE->pNext) {
            pHTE = pHTE->pNext;
            if (pHTE->ui32Key == ui32Key) {
                // Bucket contains an entry with that key,
                return;
            }
        }
        // This is a new key, add a new entry to the bucket and return null
        HashsetEntry *pNew = new HashsetEntry;
        pNew->ui32Key = ui32Key;
        pNew->bInUse = true;
        pNew->pNext = NULL;
        pHTE->pNext = pNew;
        _ulCount++;
        return;
    }

    inline bool UInt32Hashset::contains (uint32 ui32Key) const
    {
        int hashValue = hashCode (ui32Key);
        struct HashsetEntry *pHTE;
        for (pHTE = &_pHashset[hashValue];  pHTE;  pHTE = pHTE->pNext) {
            if ((ui32Key == pHTE->ui32Key) && (pHTE->bInUse)) {
                return true;
            }
        }
        return false;
    }

    inline bool UInt32Hashset::remove (uint32 ui32Key)
    {
        // Compute hashvalue which will be the bucket index
        int hashValue = hashCode(ui32Key);
        HashsetEntry *pHTE = &_pHashset[hashValue];
        if (!pHTE->bInUse) {
            // Hashset bucket is empty - key not in the Hashset
            return false;
        }
        if (pHTE->ui32Key == ui32Key) {
            // First element in bucket contains that key
            // Deleting entry in Hashset array
            if (pHTE->pNext) {
                // There are more elements in the bucket, shrink list
                HashsetEntry *pTemp = pHTE->pNext;
                pHTE->ui32Key = pTemp->ui32Key;
                pHTE->bInUse = pTemp->bInUse;
                pHTE->pNext = pTemp->pNext;
                // Delete entry
                delete pTemp;
            }
            else {
                // Bucket will be empty, set the pointer to null.
                pHTE->bInUse = false;
                pHTE->ui32Key = 0;
            }
            _ulCount--;
            // Don't check to see if value should be deleted because
            //     the value is returned
            return true;
        }
        // Search the bucket for an entry with that key
        HashsetEntry *pPrev = pHTE;
        pHTE = pHTE->pNext;
        while (pHTE) {
            if ((pHTE->ui32Key == ui32Key) && (pHTE->bInUse)) {
                // Found the node
                pPrev->pNext = pHTE->pNext;
                // Delete the entry
                delete pHTE;
                _ulCount--;
                // But don't check to see if value should be deleted because
                //     the value is returned
                return true;
            }
            else {
                pPrev = pHTE;
                pHTE = pHTE->pNext;
            }
        }
        return false;
    }

    inline void UInt32Hashset::removeAll()
    {
        deleteSet (_pHashset, _ulSetSize);
        _pHashset = new HashsetEntry[_ulInitSize];
        _ulSetSize = _ulInitSize;
        _ulCount = 0;
    }

    inline unsigned long UInt32Hashset::getCount (void) const
    {
        return _ulCount;
    }

    inline unsigned long UInt32Hashset::getSetSize (void) const
    {
        return _ulSetSize;
    }

    inline UInt32Hashset::Iterator UInt32Hashset::getAllElements (void) const
    {
        return Iterator (this);
    }

    inline void UInt32Hashset::printStructure (void) const
    {
        for (unsigned long ul = 0; ul < _ulSetSize; ul++) {
            unsigned long ulCount = 0;
            if (_pHashset[ul].bInUse) {
                ulCount++;
                HashsetEntry *pHTE = _pHashset[ul].pNext;
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

    inline int UInt32Hashset::hashCode (uint32 ui32Key) const
    {
        // Horner's method calculating the hashcode
        // byte per byte within the long number,
        // as if char per char within a string

        // Check later whether by bit-shifting it might be faster
        // This scheme is somewhat simple because I don't need to know
        // how many bytes a long occupies in memory
        int iHashValue = 0;
        do {
            iHashValue = (64*iHashValue + ((int) (ui32Key % 256))) % _ulSetSize;
        } while ((ui32Key /= 256) > 0);
        return iHashValue;
    }

    inline void UInt32Hashset::rehash()
    {
        HashsetEntry *pOldHashset = _pHashset;
        unsigned long ulOldSize = _ulSetSize;
        if (ulOldSize > 1048576) {
            // Don't grow the Hashset exponentially after a certain threshold
            _ulSetSize = ulOldSize + 1048576;
        }
        else {
            _ulSetSize = ulOldSize * 2 + 1;
        }
        _pHashset = new HashsetEntry[_ulSetSize];
        _ulCount = 0;

        for (unsigned long ul = 0; ul < ulOldSize; ul++) {
            for (HashsetEntry *pHTE = &pOldHashset[ul];  pHTE;  pHTE = pHTE->pNext) {
                if (pHTE->bInUse) {
                    put (pHTE->ui32Key);
                }
            }
        }

        deleteSet (pOldHashset, ulOldSize, true);
    }

    inline void UInt32Hashset::deleteSet (HashsetEntry *pSet, unsigned long ulSize, bool bRehashing)
    {
        for (unsigned long ul = 0; ul < ulSize; ul++) {
            HashsetEntry *pHTE = &pSet[ul];
            pHTE = pHTE->pNext;
            while (pHTE) {
                HashsetEntry *pTmp = pHTE;
                pHTE = pHTE->pNext;
                delete pTmp;
            }
        }
        delete[] pSet;
    }

}

#endif   // #ifndef INCL_UINT32_Hashset_H
