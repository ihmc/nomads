/*
 * C45LocalNodeContext.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
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
 */

#include "C45LocalNodeContext.h"

#include "C45DecisionTree.h"
#include "C45AVList.h"

using namespace IHMC_C45;
using namespace IHMC_ACI;

const char * C45LocalNodeContext::TYPE = "C4.5";
const char * C45LocalNodeContext::CYCLE_ALGORITHM = "cycle";
const char * C45LocalNodeContext::WINDOW_ALGORITHM = "window";

C45LocalNodeContext::C45LocalNodeContext (const char *pszNodeID, double dTooFarCoeff, double dApproxCoeff)
    : LocalNodeContext (pszNodeID, new C45DecisionTree(), dTooFarCoeff, dApproxCoeff)
{
}

C45LocalNodeContext::~C45LocalNodeContext()
{
}

int C45LocalNodeContext::configureCycleModeClassifier (IHMC_C45::C45AVList *pFields, int iInitWinSize, int iMaxWinSize,
                                                       int iIncrement, float fPercCycleErr, float fPercCycleWin)
{
    int i = static_cast<C45DecisionTree*>(_pClassifier)->configureClassifier (pFields);
    if(i != 0) {
        return i;
    }
    return static_cast<C45DecisionTree*>(_pClassifier)->setupCycleMode (iInitWinSize, iMaxWinSize, iIncrement,
                                                                        fPercCycleErr, fPercCycleWin);
}

int C45LocalNodeContext::configureWindowModeClassifier (IHMC_C45::C45AVList *pFields,int iInitWinSize,
                                                        int iMaxWinSize, int iIncrement)
{
    int i = static_cast<C45DecisionTree*>(_pClassifier)->configureClassifier (pFields);
    if(i != 0) {
        return i;
    }
    return static_cast<C45DecisionTree*>(_pClassifier)->setupWindowMode (iInitWinSize, iMaxWinSize, iIncrement);
}
