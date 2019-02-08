/*
 * TestInfo.h
 *
 * This file is part of the IHMC C4.5 Decision Tree Library.
 * Copyright (c) 2008-2016 IHMC.
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on November 23, 2011, 12:00 PM
 */

#ifndef INCL_TESTINFO_H
#define INCL_TESTINFO_H

namespace IHMC_C45
{
    class TestInfo
    {
        public:
            TestInfo();
            TestInfo(int noErrors, int noItems, float percErrors);
            virtual ~TestInfo();
            int getNoErrors(void);
            int getNoItemsTested(void);
            float getRealErrorRate(void);

        protected:
            int _noErrors;	// number of misclassified items in the test
            int _noItems;	// total number of items analyzed in the test
            float _percErrors;	// % of errors on the total number of items analyzed
    };

    inline TestInfo::TestInfo()
    {
        _noErrors = 0;
        _noItems = 0;
        _percErrors = 0;
    }

    inline TestInfo::TestInfo(int noErrors, int noItems, float percErrors)
    {
        _noErrors = noErrors;
        _noItems = noItems;
        _percErrors = percErrors;
    }

    inline TestInfo::~TestInfo()
    {
    }

    inline int TestInfo::getNoErrors(void)
    {
        return _noErrors;
    }

    inline int TestInfo::getNoItemsTested(void)
    {
        return _noItems;
    }

    inline float TestInfo::getRealErrorRate(void)
    {
        return _percErrors;
    }
}

#endif // INCL_TESTINFO_H
