/* 
 * File:   LinkState.cpp
 * Author: gbenincasa
 * 
 * Created on May 14, 2015, 3:52 PM
 */

#include "LinkStateClassifiers.h"

using namespace IHMC_MISC;


const char * IHMC_MISC::linkStateAsString (LinkState state)
{
    switch (state) {
        case UNDER_UTILIZED: return "UNDER_UTILIZED";
        case UTILIZED_NOT_SHARED: return "UTILIZED_NOT_SHARED";
        case UTILIZED_AMBIGUOS: return "UTILIZED_AMBIGUOS";
        case UTILIZED_COMPETING_FLOWS: return "UTILIZED_COMPETING_FLOWS";
        default: return "no description for state";
    }         
}

double LinkStateClassifiers::DEFAULT_ALPHA = 0.125;

LinkStateClassifiers::LinkStateClassifiers (double alpha)
    : _bInitialized (false),
      _alpha (alpha),
      _beta (1.0 - _alpha),
      _interArrEMA (_alpha)
{
}

LinkStateClassifiers::~LinkStateClassifiers()
{
}

LinkState LinkStateClassifiers::packetArrived (int64 ui32CurrROTT, int64 ui32MinROTT,
                                               uint32 ui32InterArrivalTime, uint32 ui32MinInterArrivalTime)
{
    const double dNormalizedAvgInterArr = _interArrEMA.add (ui32InterArrivalTime) / (double) ui32MinInterArrivalTime;
    if (ui32CurrROTT < (ui32MinROTT + (0.05 * ui32MinInterArrivalTime))) {
        // The bottleneck link is under-utilized. Biaz and ZigZag do not perform well
        return UNDER_UTILIZED;
    }
    // The bottleneck link is not under-utilized
    else if (dNormalizedAvgInterArr < _beta) {
        // beginning of the connection
        return UTILIZED_AMBIGUOS;
    }
    else if (dNormalizedAvgInterArr < 1.5) {
        // wireless link is the bottleneck, and it's not shared
        return UTILIZED_NOT_SHARED;
    }
    else if (dNormalizedAvgInterArr < 2.0) {
        // middle of the connection
        return UTILIZED_AMBIGUOS;
    }
    else {
        return UTILIZED_COMPETING_FLOWS;
    }
}

