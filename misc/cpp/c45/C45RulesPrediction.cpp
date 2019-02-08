/*
 * C45RulesPrediction.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "C45RulesPrediction.h"

using namespace IHMC_C45;

void C45RulesPrediction::copyPrediction(C45RulesPrediction * oldPrediction)
{
    _pszClassName = oldPrediction->_pszClassName;
    _probability = oldPrediction->_probability;
    _isDefault = oldPrediction->isDefaultClass();
}

C45RulesPrediction::C45RulesPrediction() : Prediction()
{
    _isDefault = false;
}

C45RulesPrediction::C45RulesPrediction(const char * className, float probability, bool isDefault)
 : Prediction(className, probability)
{
    _isDefault = isDefault;
}

C45RulesPrediction::~C45RulesPrediction()
{
}

