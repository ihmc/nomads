/*
 * Statistics.cpp
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

#include "Statistics.h"

#include <math.h>

using namespace NOMADSUtil;

Statistics::Statistics()
{
    _sumValues = 0.0;
    _sumSqValues = 0.0;
    _totalNumValues = 0;
    _max = 0.0;
}

Statistics::~Statistics()
{
}

void Statistics::update (double value)
{
    _sumValues += value;
    _sumSqValues += (value*value);
    if ((_totalNumValues == 0) || (value > _max)) {
        _max = value;
    }
    _totalNumValues++;
}

void Statistics::reset()
{
    _sumValues = 0.0;
    _sumSqValues = 0.0;
    _totalNumValues = 0;
}

int Statistics::getNumValues()
{
    return _totalNumValues;
}

double Statistics::getMax()
{
    return _max;
}

double Statistics::getAverage()
{
    return (_sumValues/_totalNumValues);
}

double Statistics::getStDev()
{
    double avg = getAverage();
    double aux = (_totalNumValues * avg * avg)
                  - (2 * avg * _sumValues)
                  + _sumSqValues;
    aux = (double) aux / (_totalNumValues - 1);
    aux = sqrt (aux);
    return aux;
}

