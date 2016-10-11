/* 
 * File:   LinkState.h
 * Author: gbenincasa
 *
 * Created on May 14, 2015, 3:52 PM
 */

#ifndef INCL_LINK_STATE_CLASSIFIERS_H
#define	INCL_LINK_STATE_CLASSIFIERS_H

#include "ExponentialAverage.h"
#include "FTypes.h"

namespace IHMC_MISC
{
    enum LinkState
    {
        UNDER_UTILIZED           = 0x00,
        UTILIZED_NOT_SHARED      = 0x01,
        UTILIZED_AMBIGUOS        = 0x02,
        UTILIZED_COMPETING_FLOWS = 0x03
    };

    const char * linkStateAsString (LinkState state);

    class LinkStateClassifiers
    {
        public:
            static double DEFAULT_ALPHA;

        public:
            LinkStateClassifiers (double alpha = DEFAULT_ALPHA);
            ~LinkStateClassifiers (void);

            LinkState packetArrived (int64 ui32CurrROTT, int64 ui32MinROTT,
                                     uint32 ui32InterArrivalTime, uint32 ui32MinInterArrivalTime);

        private:
            bool _bInitialized;
            const double _alpha;
            const double _beta;
            NOMADSUtil::ExponentialAverage<uint32> _interArrEMA;
    };
}

#endif	/* INCL_LINK_STATE_CLASSIFIERS_H */

