/*
 * Prediction.h
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

#ifndef INCL_PREDICTION_H
#define INCL_PREDICTION_H

namespace IHMC_C45
{
    class Prediction
    {
        public:
            Prediction (void);
            Prediction (const char *pClassName, float probability);
            virtual ~Prediction (void);
            virtual const char * getPrediction (void);
            virtual float getProbability (void);

        protected:
            const char *_pszClassName;  // prediction
            float _probability;	        // probabilty that the prediction is
                                        // correct
    };

    inline const char * Prediction::getPrediction()
    {
        return _pszClassName;
    }

    inline float Prediction::getProbability()
    {
        return _probability;
    }
}

#endif // INCL_PREDICTION_H
