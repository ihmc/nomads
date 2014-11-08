#ifndef INCL_MUTEXT_COUNTER_H
#define INCL_MUTEXT_COUNTER_H

/*
 * MutexCounter.h
 * 
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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

#include "Mutex.h"


namespace ACMNetProxy
{
    template <class T> class MutexCounter
    {
    public:
        MutexCounter (void);
        MutexCounter (const T &rStartValue);
        ~MutexCounter (void);

        const T tick (void);
        void increment (void);
        void decrement (void);
        void reset (void);

    private:
        T _IncrementableObj;
        const T _StartValue;

        NOMADSUtil::Mutex _m;
    };


    template <class T> inline MutexCounter<T>::MutexCounter (void)
        : _StartValue() {}

    template <class T> inline MutexCounter<T>::MutexCounter (const T &rStartValue)
        : _StartValue (rStartValue) {}

    template <class T> inline MutexCounter<T>::~MutexCounter (void) {};

    template <class T> inline const T MutexCounter<T>::tick (void)
    {
        _m.lock();
        T retVal = _IncrementableObj++;
        _m.unlock();

        return retVal;
    }

    template <class T> inline void MutexCounter<T>::increment (void)
    {
        _m.lock();
        _IncrementableObj++;
        _m.unlock();
    }

    template <class T> inline void MutexCounter<T>::decrement (void)
   {
        _m.lock();
        _IncrementableObj--;
        _m.unlock();
    }

    template <class T> inline void MutexCounter<T>::reset (void)
    {
        _IncrementableObj = _StartValue;
    }
}

#endif      // INCL_MUTEXT_COUNTER_H
