/*
 * C45RulesPrediction.h
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

#ifndef INCL_C45_RULES_PREDICTION_H
#define INCL_C45_RULES_PREDICTION_H

#include "Prediction.h"

namespace IHMC_C45
{
    class C45RulesPrediction : public Prediction
    {
        public:
            C45RulesPrediction();
            C45RulesPrediction(const char * className, float probability, bool isDefault);
            virtual ~C45RulesPrediction();
            bool isDefaultClass(void);
            void copyPrediction(C45RulesPrediction * oldPrediction);

        protected:
            bool _isDefault; // = true if the resulted class is the default class
    };

    inline bool C45RulesPrediction::isDefaultClass(void)
    {
        return _isDefault;
    }
}

#endif // INCL_C45_RULES_PREDICTION_H
