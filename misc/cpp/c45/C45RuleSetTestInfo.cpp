/*
 * C45RuleSetTestInfo.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "C45RuleSetTestInfo.h"

#include <stdlib.h>

using namespace IHMC_C45;

C45RuleSetTestInfo::C45RuleSetTestInfo()
    : TestInfo()
{
    _noRules = 0;
    _pConfusionMatrix = NULL;
    _confusionMatrixSize = 0;
}

C45RuleSetTestInfo::C45RuleSetTestInfo(int noRules, int noItems, int noErrors, float percErrors,
                                       int * confusionMatrix, int confusionMatrixSize)
    : TestInfo(noErrors, noItems, percErrors)
{
    _noRules = noRules;
    if(confusionMatrixSize >= 0) {
        _confusionMatrixSize = confusionMatrixSize;
        _pConfusionMatrix = (int *) calloc(_confusionMatrixSize, sizeof(int));
        for(int i = 0; i < _confusionMatrixSize; i ++) {
            _pConfusionMatrix[i] = confusionMatrix[i];
        }
    }
    else {
        _confusionMatrixSize = 0;
        _pConfusionMatrix = NULL;
    }
}

void C45RuleSetTestInfo::copyTest(C45RuleSetTestInfo * test)
{
    _noErrors = test->_noErrors;
    _noItems = test->_noItems;
    _percErrors = test->_percErrors;
    _noRules = test->_noRules;
    _confusionMatrixSize = test->_confusionMatrixSize;
    if(_pConfusionMatrix != NULL) {
        free(_pConfusionMatrix);
        _pConfusionMatrix = NULL;
    }
    _pConfusionMatrix = (int *) calloc(_confusionMatrixSize, sizeof(int));
    for(int i = 0; i < _confusionMatrixSize; i ++) {
        _pConfusionMatrix[i] = test->_pConfusionMatrix[i];
    }
}

C45RuleSetTestInfo::~C45RuleSetTestInfo()
{
    if(_pConfusionMatrix != NULL) {
        free(_pConfusionMatrix);
    }
}
