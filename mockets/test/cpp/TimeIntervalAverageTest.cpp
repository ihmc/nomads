/* 
 * File:   TimeIntervalAverageTest.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses
 */

#include "Mocket.h"
#include "TimeIntervalAverage.h"

using namespace std;
using namespace NOMADSUtil;

/*
 * 
 */
int main(int argc, char** argv) {
    printf ("TestTimeIntervalAverage\n");
    TimeIntervalAverage<double> *_pRTTRecord;
    printf ("TimeIntervalAverage created\n");
    _pRTTRecord = new TimeIntervalAverage<double> (1000);
    printf ("new TimeIntervalAverage\n");
    
    double dRTT = 3.5;
    _pRTTRecord->add(NOMADSUtil::getTimeInMilliseconds(), dRTT);
    printf ("Min = %f, num values = %d, Sum = %f\n", (double)_pRTTRecord->getMin(), _pRTTRecord->getNumValues(), (double)_pRTTRecord->getSum());
    
    sleepForMilliseconds (2000);
    printf ("Min = %f, num values = %d, Sum = %f\n", (double)_pRTTRecord->getMin(), _pRTTRecord->getNumValues(), (double)_pRTTRecord->getSum());
    
    delete _pRTTRecord;
    _pRTTRecord = NULL;
    printf ("TimeIntervalAverage deleted\n");

    return 0;
}

