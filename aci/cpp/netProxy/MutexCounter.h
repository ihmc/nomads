#ifndef INCL_MUTEXT_COUNTER_H
#define INCL_MUTEXT_COUNTER_H

/*
 * MutexCounter.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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
 * MutexCounter is a template class that implements the functionality
 * of a counter with mutually exclusive access. At every call to the
 * method tick(), the method locks the counter, increments its value
 * and finally returns its value previous to the increment.
 * Any class that implements the post-increment operator can
 * be used as a template for the MutexCounter class.
 */

#include <atomic>


namespace ACMNetProxy
{
    template <class T> class MutexCounter
    {
    public:
        MutexCounter (void);
        MutexCounter (const T & rStartValue);
        ~MutexCounter (void);

        const T tick (void);
        void increment (void);
        void decrement (void);
        void reset (void);


    private:
        std::atomic<T> _aCounter;
        const T _tStartValue;
    };


    template <class T> inline MutexCounter<T>::MutexCounter (void) :
        _tStartValue{}
    { }

    template <class T> inline MutexCounter<T>::MutexCounter (const T & rStartValue) :
        _tStartValue{rStartValue}
    { }

    template <class T> inline MutexCounter<T>::~MutexCounter (void) { }

    template <class T> inline const T MutexCounter<T>::tick (void)
    {
        return _aCounter++;
    }

    template <class T> inline void MutexCounter<T>::increment (void)
    {
        ++_aCounter;
    }

    template <class T> inline void MutexCounter<T>::decrement (void)
   {
        --_aCounter;
    }

    template <class T> inline void MutexCounter<T>::reset (void)
    {
        _aCounter = _tStartValue;
    }
}

#endif      // INCL_MUTEXT_COUNTER_H
