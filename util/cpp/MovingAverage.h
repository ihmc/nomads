/*
 * MovingAverage.h
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

#ifndef INCL_MOVING_AVERAGE_H
#define INCL_MOVING_AVERAGE_H

#include <stddef.h>

namespace NOMADSUtil
{

    template <class T> class MovingAverage
    {
        public:
            MovingAverage (unsigned short usWindowSize);
            ~MovingAverage (void);

            void add (T value);
            T getSum (void);
            double getAverage (void);

            void reset (void);

        private:
            unsigned short _usWindowSize;
            T *_pValues;
            T _sum;
            unsigned short _usNextPos;
            unsigned short _usNumValues;
    };

    template <class T> MovingAverage<T>::MovingAverage (unsigned short usWindowSize)
    {
        _usWindowSize = usWindowSize;
        if (usWindowSize > 0) {
            _pValues = new T [usWindowSize];
            for (unsigned short us = 0; us < usWindowSize; us++) {
                _pValues[us] = 0;
            }
        }
        else {
            _pValues = NULL;
        }
        _sum = 0;
        _usNextPos = 0;
        _usNumValues = 0;
    }

    template <class T> MovingAverage<T>::~MovingAverage (void)
    {
        delete[] _pValues;
        _pValues = NULL;
    }

    template <class T> void MovingAverage<T>::add (T value)
    {
        if (_pValues) {
            _sum -= _pValues[_usNextPos];
            _sum += value;
            _pValues[_usNextPos] = value;

            if (_usNumValues < _usWindowSize) {
                _usNumValues++;
            }

            _usNextPos++;
            if (_usNextPos >= _usWindowSize) {
                _usNextPos = 0;
            }
        }
    }

    template <class T> T MovingAverage<T>::getSum (void)
    {
        return _sum;
    }

    template <class T> double MovingAverage<T>::getAverage (void)
    {
        return _sum / ((double) _usNumValues);
    }

    template <class T> void MovingAverage<T>::reset (void)
    {
        _sum = 0;
        _usNextPos = 0;
        _usNumValues = 0;
    }
}

#endif   // #ifndef INCL_MOVING_AVERAGE_H
