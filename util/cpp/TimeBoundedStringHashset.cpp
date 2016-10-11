/*
 * TimeBoundedStringHashset.cpp
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

#include "TimeBoundedStringHashset.h"

using namespace NOMADSUtil;

TimeBoundedStringHashset::TimeBoundedStringHashset (uint32 ui32StorageDuration, unsigned short usInitSize, bool bCaseSensitiveKeys,
                                                    bool bCloneKeys, bool bDeleteKeys)
{
    _ui32StorageDuration = ui32StorageDuration;
    _usCount = 0;
    _usTableSize = usInitSize;
    _pHashtable = new HashtableEntry[_usTableSize];
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
}

void TimeBoundedStringHashset::configure (uint32 ui32StorageDuration,
                                          bool bCaseSensitiveKeys,
                                          bool bCloneKeys,
                                          bool bDeleteKeys)
{
    _ui32StorageDuration = ui32StorageDuration;
    _bCaseSensitiveKeys = bCaseSensitiveKeys;
    if (bCloneKeys) {
        _bCloneKeys = true;
        _bDeleteKeys = true;
    }
    else {
        _bCloneKeys = false;
        _bDeleteKeys = bDeleteKeys;
    }
}

TimeBoundedStringHashset::~TimeBoundedStringHashset (void)
{
    deleteTable (_pHashtable, _usTableSize);
    _pHashtable = NULL;
    _usCount = 0;
    _usTableSize = 0;
}

void TimeBoundedStringHashset::remove (const char *pszKey)
{
    if (pszKey == NULL) {
        return;
    }
    int hashValue = hashCode (pszKey);
    HashtableEntry *pHTE = &_pHashtable[hashValue];
    if (pHTE->pszKey == NULL) {
        // Hashset bucket is empty - therefore the key must not exist
        return;
    }
    else {
        if (0 == keycomp (pHTE->pszKey, pszKey)) {
            // Deleting entry in hashset array
            if (pHTE->pNext) {
                HashtableEntry *pTemp = pHTE->pNext;
                // Check to see if key should be deleted
                if (_bDeleteKeys) {
                    delete[] pHTE->pszKey;
                }
                pHTE->pszKey = pTemp->pszKey;
                pHTE->pNext = pTemp->pNext;
                delete pTemp;
            }
            else {
                // Check to see if key should be deleted
                if (_bDeleteKeys) {
                    delete[] pHTE->pszKey;
                }
                // Otherwise set the pointers to null.
                pHTE->pszKey = NULL;
            }
            _usCount--;
            return;
        }
        else {
            HashtableEntry *pPrev = pHTE;
            pHTE = pHTE->pNext;
            while (pHTE != NULL) {
                if (0 == keycomp (pHTE->pszKey, pszKey)) {
                    // Found the node
                    pPrev->pNext = pHTE->pNext;
                    // Check to see if key should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKey;
                    }
                    delete pHTE;
                    _usCount--;
                    return;
                }
                else {
                    pPrev = pHTE;
                    pHTE = pHTE->pNext;
                }
            }
        }
    }
}

void TimeBoundedStringHashset::removeAll()
{
    deleteTable (_pHashtable, _usTableSize);    
    _pHashtable = new HashtableEntry[_usTableSize];
    _usCount = 0;
    _ulState = 0;
}

TimeBoundedStringHashset::Iterator TimeBoundedStringHashset::getAllElements (void)
{
    expireEntries();
    return Iterator (this, _ulState);
}

void TimeBoundedStringHashset::printStructure (void)
{
    for (unsigned short us = 0; us < _usTableSize; us++) {
        unsigned short usCount = 0;
        if (_pHashtable[us].pszKey) {
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

void TimeBoundedStringHashset::rehash()
{
    HashtableEntry *pOldHashtable = _pHashtable;
    unsigned short usOldSize = _usTableSize;
    _usTableSize = usOldSize * 2 + 1;
    _pHashtable = new HashtableEntry[_usTableSize];
    _usCount = 0;

    for (unsigned short us = 0; us < usOldSize; us++) {
        for (HashtableEntry *pHTE = &pOldHashtable[us]; pHTE != NULL; pHTE = pHTE->pNext) {
            if (pHTE->pszKey) {
                put (pHTE->pszKey, pHTE->i64ExpirationTime);
            }
        }
    }

    deleteTable (pOldHashtable, usOldSize);
}

void TimeBoundedStringHashset::expireEntry (HashtableEntry *pHTE)
{
    if (_bDeleteKeys) {
        delete[] pHTE->pszKey;
    }
    if (pHTE->pNext != NULL) {
        HashtableEntry *pTmp = pHTE->pNext;
        pHTE->pszKey = pHTE->pNext->pszKey;
        pHTE->i64ExpirationTime = pHTE->pNext->i64ExpirationTime;
        pHTE->pNext = pHTE->pNext->pNext;
        delete pTmp;
    }
    else {
        pHTE->pszKey = NULL;
        pHTE->pNext = NULL;
        pHTE->i64ExpirationTime = 0;
    }
    _usCount--;
}

void TimeBoundedStringHashset::expireEntries (void)
{
    int64 i64CurrTime = getTimeInMilliseconds();
    for (unsigned short us = 0; us < _usTableSize; us++) {
        HashtableEntry *pHTE = &_pHashtable[us];
        if ((pHTE->pszKey != NULL) && (pHTE->i64ExpirationTime < i64CurrTime)) {
            expireEntry (pHTE);
        }
    }
}

void TimeBoundedStringHashset::deleteTable (HashtableEntry *pTable, unsigned short usSize)
{
    for (unsigned short us = 0; us < usSize; us++) {
        HashtableEntry *pHTE = &pTable[us];

        // Need to special case the first entry because the HTE
        // is part of the array and is not deleted
        if (_bDeleteKeys) {
            delete[] pHTE->pszKey;
            pHTE->pszKey = NULL;
        }

        pHTE = pHTE->pNext;
        while (pHTE != NULL) {
            HashtableEntry *pTmp = pHTE;
            pHTE = pHTE->pNext;
            if (_bDeleteKeys) {
                delete[] pTmp->pszKey;
            }
            delete pTmp;
        }
    }
    delete[] pTable;
}
