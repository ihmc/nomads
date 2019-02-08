/*
 * C45TreeTestInfo.h
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

#ifndef INCL_C45_TREE_TEST_INFO_H
#define INCL_C45_TREE_TEST_INFO_H

#include "TestInfo.h"

namespace IHMC_C45
{
    class C45TreeTestInfo : public TestInfo
    {
        public:
            C45TreeTestInfo();
            C45TreeTestInfo(int treeSize, int noErrors, int noItems, float percErrors,
                            float estimate, int * confMat, int confMatSize);
            virtual ~C45TreeTestInfo();
            int getTreeSize(void);
            float getEstimateErrorRate(void);
            int * getConfusionMatrix(void);
            int getConfusionMatrixSize(void);
            void copyTest(C45TreeTestInfo * treeTest);

        protected:
            int _treeSize;              // size of the tree: number of nodes in the tree
            float _estimate;		// estimate the % of errors (only for pruned trees)
            int * _pConfusionMatrix;	// an array of int[noClasses * noClasses] that represent
                                        // the confusion matrix for this tree
            int _confusionMatrixSize;
    };

    inline int C45TreeTestInfo::getTreeSize(void)
    {
        return _treeSize;
    }

    inline float C45TreeTestInfo::getEstimateErrorRate(void)
    {
        return _estimate;
    }

    inline int * C45TreeTestInfo::getConfusionMatrix(void)
    {
        return _pConfusionMatrix;
    }

    inline int C45TreeTestInfo::getConfusionMatrixSize(void)
    {
        return _confusionMatrixSize;
    }
}

#endif // INCL_C45_TREE_TEST_INFO_H
