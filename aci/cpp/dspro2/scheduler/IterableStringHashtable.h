/*
 * Scheduler.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on October 03, 2015, 8:57 AM
 */

#ifndef INCL_ITERABLE_STRING_HASHTABLE_H
#define INCL_ITERABLE_STRING_HASHTABLE_H

#include "PtrLList.h"
#include "StrClass.h"
#include "StringHashtable.h"

#include "Mutex.h"

namespace IHMC_ACI
{
    template <class T>
    class IterableStringHashtable
    {
        public:
            explicit IterableStringHashtable (bool bCaseSensitiveKeys = true,
                                              bool bCloneKeys = true,
                                              bool bDeleteKeys = true,
                                              bool bDeleteValues = false);
            explicit IterableStringHashtable (uint32 ui32InitSize,
                                              bool bCaseSensitiveKeys = true,
                                              bool bCloneKeys = true,
                                              bool bDeleteKeys = true,
                                              bool bDeleteValues = false);
            ~IterableStringHashtable (void);

            T * put (const char *pszKey, T *pValue);

            // Access via hash table
            T * get (const char *pszKey) const;

            // Iterative access
            void resetGetNext (void);
            T * getNext (NOMADSUtil::String &key);

        private:
            struct Entry
            {
                Entry (const char *pszString, T*pVal);
                ~Entry (void);

                const NOMADSUtil::String _key;
                T *_pVal;
            };

            mutable NOMADSUtil::Mutex _mList;
            NOMADSUtil::StringHashtable<T> _hashtable;
            NOMADSUtil::PtrLList<Entry> _list;
    };

    template <class T>
    IterableStringHashtable<T>::IterableStringHashtable (bool bCaseSensitiveKeys, bool bCloneKeys,
                                                         bool bDeleteKeys, bool bDeleteValues)
        : _hashtable (bCaseSensitiveKeys, bCloneKeys, bDeleteKeys, bDeleteValues)
    {
    }

    template <class T>
    IterableStringHashtable<T>::IterableStringHashtable (uint32 ui32InitSize, bool bCaseSensitiveKeys,
                                                         bool bCloneKeys, bool bDeleteKeys, bool bDeleteValues)
        : _hashtable (ui32InitSize, bCaseSensitiveKeys, bCloneKeys, bDeleteKeys, bDeleteValues)
    {
    }

    template <class T>
    IterableStringHashtable<T>::~IterableStringHashtable (void)
    {
    }

    template <class T>
    T * IterableStringHashtable<T>::put (const char *pszKey, T *pValue)
    {
        T *pOldValue = _hashtable.put (pszKey, pValue);
        assert (pOldValue == nullptr);
        if (nullptr == pOldValue) {
            _mList.lock();
            _list.prepend (new Entry (pszKey, pValue));
            _mList.unlock();
        }
        return pOldValue;
    }

    template <class T>
    T * IterableStringHashtable<T>::get (const char *pszKey) const
    {
        return _hashtable.get (pszKey);
    }

    template <class T>
    void IterableStringHashtable<T>::resetGetNext (void)
    {
        _mList.lock();
        _list.resetGet();
        _mList.unlock();
    }

    template <class T>
    T * IterableStringHashtable<T>::getNext (NOMADSUtil::String &key)
    {
        _mList.lock();
        Entry *pNext = _list.getNext();
        if (pNext != nullptr) {
            key = pNext->_key;
            _mList.unlock();
            return pNext->_pVal;
        }
        _mList.unlock();
        return nullptr;
    }

    template <class T>
    IterableStringHashtable<T>::Entry::Entry (const char *pszString, T*pVal)
        : _key (pszString), _pVal (pVal)
    {
    }

    template <class T>
    IterableStringHashtable<T>::Entry::~Entry (void)
    {
    }
}

#endif  // INCL_ITERABLE_STRING_HASHTABLE_H

