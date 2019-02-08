/*
 * RandomTestInfo.h
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

#ifndef INCL_RANDOMTESTINFO_H
#define INCL_RANDOMTESTINFO_H

#include "TestInfo.h"

#include <stddef.h>

namespace IHMC_C45
{
    class RandomTestInfo : public TestInfo
    {
        public:
            RandomTestInfo();
            RandomTestInfo(int noErrors, int noItems, float percErrors, int * confusionMatrix, int confusionMatrixSize);
            virtual ~RandomTestInfo();
            int * getConfusionMatrix(void);
            int getConfusionMatrixSize(void);

        protected:
            int * _pConfusionMatrix;
            int _confusionMatrixSize;
    };

    inline RandomTestInfo::RandomTestInfo()
        : TestInfo()
    {
        _pConfusionMatrix = NULL;
        _confusionMatrixSize = 0;
    }

    inline int * RandomTestInfo::getConfusionMatrix(void)
    {
        return _pConfusionMatrix;
    }

    inline int RandomTestInfo::getConfusionMatrixSize(void)
    {
        return _confusionMatrixSize;
    }
}

#endif // INCL_RANDOMTESTINFO_H
