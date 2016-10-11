/* 
 * ServingRequestProbability.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on April 15, 2015, 6:13 PM
 */

#ifndef INCL_SERVING_REQUEST_PROBABILITY_H
#define	INCL_SERVING_REQUEST_PROBABILITY_H

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class ServingRequestProbability
    {
        public:
            ServingRequestProbability (void);
            ~ServingRequestProbability (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);
            bool serveRequest (unsigned int uiNumberOfActiveNeighbors);
            
        private:
            float getProbability (unsigned int uiNumberOfActiveNeighbors);

        private:
            enum MissingFragmentRequestReplyMode
            {
                FIXED_REPLY_PROB = 0x00,       // The prob. value is static
                NEIGHBOR_DEPENDENT_PROB = 0x01 // The prob. depends on the number of neighbors
            };

            MissingFragmentRequestReplyMode _mode;
            float _fMissingFragReqReplyFixedProb;
    };
}

#endif	/* SERVINGREQUESTPROBABILITY_H */

