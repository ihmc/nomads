/**
 * IntegerMovingAverage.java
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

package us.ihmc.algos;

import java.io.Serializable;

public class IntegerMovingAverage implements Serializable
{
    public IntegerMovingAverage (int windowSize)
    {
        _values = new int [windowSize];
        _windowSize = windowSize;
        _nextPos = 0;
        _sum = 0;
        _numValues = 0;
    }

    public void add (int value)
    {
        _sum -= _values[_nextPos];
        _sum += value;
        _values[_nextPos] = value;

        if (_numValues < _windowSize) {
            _numValues++;
        }

        _nextPos++;
        if (_nextPos >= _windowSize) {
            _nextPos = 0;
        }
    }
    
    public long getSum()
    {
        return _sum;
    }

    public float getAverage()
    {
        return (_sum / (float) _numValues);
    }

    private int _windowSize;
    private int[] _values;
    private int _nextPos;
    private long _sum;
    private int _numValues;
}
