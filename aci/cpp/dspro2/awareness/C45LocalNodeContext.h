/* 
 * C45LocalNodeContext.h
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

#ifndef INCL_C45_LOCAL_NODE_CONTEXT_H
#define INCL_C45_LOCAL_NODE_CONTEXT_H

#include "LocalNodeContext.h"

namespace IHMC_ACI
{
    class C45LocalNodeContext : public LocalNodeContext
    {
        public:
            static const char * TYPE;
            static const char * CYCLE_ALGORITHM;
            static const char * WINDOW_ALGORITHM;

            C45LocalNodeContext (const char *pszNodeID, double dTooFarCoeff, double dApproxCoeff);
            virtual ~C45LocalNodeContext (void);

            /**
             * Returns the error code present in C45DecisionTree
             * class in case of errors, else returns 0.
             */
            int configureCycleModeClassifier (IHMC_C45::C45AVList *pFields, int iInitWinSize=30, int iMaxWinSize=60,
                                              int iIncrement=5, float fPercCycleErr=0.0, float fPercCycleWin=0.0);
            int configureWindowModeClassifier (IHMC_C45::C45AVList *pFields,int iInitWinSize=30,
                                               int iMaxWinSize=200, int iIncrement=1);
    };
}

#endif // INCL_C45_LOCAL_NODE_CONTEXT_H

