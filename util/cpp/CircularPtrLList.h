/*
 * CircularPtrLList.h
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

#ifndef INCL_CIRCULAR_PTR_LLIST_H
#define INCL_CIRCULAR_PTR_LLIST_H

#include "PtrLList.h"

namespace NOMADSUtil
{
    template<class T>
    class CircularPtrLList
    {
        public:
            explicit CircularPtrLList (PtrLList<T> *ptrLList);
            ~CircularPtrLList (void);

            T * getFirst (void);
            T * getNext (void);

        private:
            PtrLList<T> *_ptrLList;
    };

    template<class T>
    CircularPtrLList<T>::CircularPtrLList (PtrLList<T> *ptrLList)
        : _ptrLList (ptrLList)
    {
    }

    template<class T>
    CircularPtrLList<T>::~CircularPtrLList (void)
    {
    }

    template<class T>
    T * CircularPtrLList<T>::getFirst (void)
    {
        return _ptrLList->getFirst();
    }

    template<class T>
    T * CircularPtrLList<T>::getNext (void)
    {
        T *pEl = _ptrLList->getNext();
        if (pEl == NULL) {
            return _ptrLList->getFirst();
        }
        return pEl;
    }
}



#endif    /* INCL_CIRCULAR_PTR_LLIST_H */

