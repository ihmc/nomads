/*
 * PtrStack.h
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
 */

#ifndef INCL_PTR_STACK_H
#define INCL_PTR_TRACK_H

#include "PtrLList.h"

namespace NOMADSUtil
{
    template<class T>
    class PtrStack
    {
        public:
            explicit PtrStack (PtrLList<T> *pLList);
            ~PtrStack (void);

            bool isEmpty (void);
            void push (T *pel);
            T * peek (void);
            T * pop (void);

        private:
            PtrLList<T> *_pLList;
    };

    template<class T>
    PtrStack<T>::PtrStack (PtrLList<T> *pLList)
        : _pLList (pLList)
    {
    }

    template<class T>
    PtrStack<T>::~PtrStack (void)
    {
    }

    template<class T>
    bool PtrStack<T>::isEmpty (void)
    {
        return _pLList->isEmpty();
    }

    template<class T>
    void PtrStack<T>::push (T *pel)
    {
        _pLList->prepend (pel);
    }

    template<class T>
    T * PtrStack<T>::peek (void)
    {
        return _pLList->getFirst();
    }

    template<class T>
    T * PtrStack<T>::pop (void)
    {
        return _pLList->removeFirst();
    }
}

#endif // INCL_PTR_STACK_H

