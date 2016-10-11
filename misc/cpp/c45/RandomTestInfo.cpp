/*
 * RandomTestInfo.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "RandomTestInfo.h"

#include <stdlib.h>
#include <string.h>

using namespace IHMC_C45;

RandomTestInfo::RandomTestInfo(int noErrors, int noItems, float percErrors, int * confusionMatrix,
										  int confusionMatrixSize)
 : TestInfo(noErrors, noItems, percErrors)
{
	if(confusionMatrixSize > 0) {
		_confusionMatrixSize = confusionMatrixSize;
		_pConfusionMatrix = (int *) calloc(_confusionMatrixSize, sizeof(int));
		memcpy(_pConfusionMatrix, confusionMatrix, _confusionMatrixSize*sizeof(int));
	}
	else {
		_pConfusionMatrix = NULL;
		_confusionMatrixSize = 0;
	}
}

RandomTestInfo::~RandomTestInfo()
{
    if(_pConfusionMatrix != NULL) {
        free(_pConfusionMatrix);
    }
}
