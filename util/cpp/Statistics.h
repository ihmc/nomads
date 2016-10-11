/*
 * Statistics.h
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

#ifndef STATISTICS_H
#define STATISTICS_H

namespace NOMADSUtil
{
    class Statistics
    {
        public:
            Statistics (void);
            ~Statistics (void);

            // updates the statistics with the new value.
            void update (double value);
            
            // reset all counters to zero
            void reset (void);

            // returns the number of values that have been entered via the update() method.
            int getNumValues (void);

            // returns the calculation of the average of the entered values.
            double getAverage (void);

            // returns the calculation of the Standard Deviation of the entered values.
            double getStDev (void);

        private:
            double _sumValues;
            double _sumSqValues;
            int _totalNumValues;
    };
}

#endif //STATISTICS_H
