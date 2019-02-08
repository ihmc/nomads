/*
 * C45TreePrediction.h
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

#ifndef INCL_C45_TREE_PREDICTION_H
#define INCL_C45_TREE_PREDICTION_H

#include "Prediction.h"

#include "types.h"

namespace IHMC_C45
{
    class C45TreePrediction : public Prediction
    {
        public:
            C45TreePrediction();
            C45TreePrediction(consultTreeResults * results);
            virtual ~C45TreePrediction();
            const char * getClassName(int noPredictions);
            float getGuessProbability(int noPredictions);
            float getLowProbability(int noPredictions);
            float getUpperProbability(int noPredictions);
            int getNoPredictions(void);
            void copyPrediction(C45TreePrediction * prediction);

            const char * getPrediction(void);  // Returns the most probable class.
            float getProbability(void);        // Returns the guess probability of the class
                                               // returned by getPrediction().

       private:
          struct _pred {
             char * _pszClassName; // resulted class
             float _guessProb;     // best guess probability
             float _lowProb;       // the probability that the predicted class is correct is
             float _upperProb;     // in the intervall: [lowProbability, upperProbability]
          } * _pPrediction;
          short _noPred;
    };

    inline int C45TreePrediction::getNoPredictions(void)
    {
        return _noPred + 1;
    }
}

#endif // INCL_C45_TREE_PREDICTION_H
