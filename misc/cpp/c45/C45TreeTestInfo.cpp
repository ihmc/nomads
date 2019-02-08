/*
 * C45TreeTestInfo.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "C45TreeTestInfo.h"

#include <stdlib.h>
#include <string.h>

using namespace IHMC_C45;

C45TreeTestInfo::C45TreeTestInfo()
    : TestInfo()
{
    _treeSize = 0;
    _estimate = 0;
    _pConfusionMatrix = NULL;
    _confusionMatrixSize = 0;
}

C45TreeTestInfo::C45TreeTestInfo(int treeSize, int noErrors, int noItems,
                                 float percErrors, float estimate, int * confMat, int confMatSize)
    : TestInfo(noErrors, noItems, percErrors)
{
    _treeSize  = treeSize;
    _estimate = estimate;
    if(confMatSize > 0) {
        _confusionMatrixSize = confMatSize;
        _pConfusionMatrix = (int *) calloc(_confusionMatrixSize, sizeof(int));
        memcpy(_pConfusionMatrix, confMat, _confusionMatrixSize * sizeof(int));
    }
    else {
        _pConfusionMatrix = NULL;
        _confusionMatrixSize = 0;
    }
}

void C45TreeTestInfo::copyTest(C45TreeTestInfo * treeTest)
{
    _noErrors = treeTest->_noErrors;
    _percErrors = treeTest->_percErrors;
    _noItems = treeTest->_noItems;
    _treeSize = treeTest->_treeSize;
    _estimate = treeTest->_estimate;
    _confusionMatrixSize = treeTest->_confusionMatrixSize;
    if(_pConfusionMatrix != NULL) {
        free(_pConfusionMatrix);
    }
    _pConfusionMatrix = (int *) calloc(_confusionMatrixSize, sizeof(int));
    for(int i = 0; i < _confusionMatrixSize; i ++) {
        _pConfusionMatrix[i] = treeTest->_pConfusionMatrix[i];
    }
}

C45TreeTestInfo::~C45TreeTestInfo()
{
    if(_pConfusionMatrix != NULL) {
        free(_pConfusionMatrix);
    }
}
