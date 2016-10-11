/*
 * Prediction.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "Prediction.h"

#include <stddef.h>

using namespace IHMC_C45;

Prediction::Prediction()
{
    _pszClassName = NULL;
    _probability = 0;
}

Prediction::Prediction(const char * className, float probability)
{
    _pszClassName = className;
    _probability = probability;
}

Prediction::~Prediction()
{
}
