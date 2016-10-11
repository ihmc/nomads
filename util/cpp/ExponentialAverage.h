/*
 * ExponentialAverage.h
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
 * Created on May 14, 2015, 12:58 PM
 */

#ifndef INCL_EXPONENTIAL_AVERAGE_H
#define	INCL_EXPONENTIAL_AVERAGE_H

namespace NOMADSUtil
{
    template <class T> class ExponentialAverage
    {
        public:
            explicit ExponentialAverage (double alpha);
            ~ExponentialAverage (void);

            double add (T value);
            void reset (void);

        private:
            const double _dAlpha;
            const double _dBeta;
            double _dExpAvg;
            double _dCount;
    };

    template <class T>
    ExponentialAverage<T>::ExponentialAverage (double alpha)
        : _dAlpha (alpha),
          _dBeta (1.0 - _dAlpha),
          _dExpAvg (0.0),
          _dCount (0U)
    {
    }

    template <class T>
    ExponentialAverage<T>::~ExponentialAverage (void)
    {
    }

    template <class T>
    double ExponentialAverage<T>::add (T value)
    {
        _dCount += 1.0;
        _dExpAvg = (_dBeta * _dExpAvg) + (_dAlpha * (value/_dCount));
        return _dExpAvg;
    }

    template <class T>
    void ExponentialAverage<T>::reset (void)
    {
        _dExpAvg = 0.0;
        _dCount = 0.0;
    }
}

#endif	/* INCL_EXPONENTIAL_AVERAGE_H */

