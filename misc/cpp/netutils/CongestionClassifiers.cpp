/* 
 * File:   CongestionClassifiers.cpp
 * Author: gbenincasa
 * 
 * Created on May 13, 2015, 2:35 PM
 */

#include "CongestionClassifiers.h"

#include <math.h>

using namespace IHMC_MISC;

float spikeAlpha = 0.5f;
float spikeBeta = (1.0f/3.0f);

namespace IHMC_MISC
{
    uint32 getSeqIdGap (uint32 ui32Curr, uint32 uint32Prev)
    {
        return (ui32Curr - uint32Prev -1);
    }
}

const double Biaz::DEFAULT_UPPER_LIMIT = 2.0;
const double Biaz::MODIFIED_UPPER_LIMIT = 1.25;

Biaz::Biaz (double dUpperLimit)
    : _bInitialized (false),
      _ui32LatestPktSeqId (0U),
      _dUpperLimit (dUpperLimit)
{
}

Biaz::~Biaz (void)
{
    
}

LossType Biaz::packetArrived (uint32 ui32SeqId, uint32 ui32InterArrivalTime, uint32 ui32MinInterArrivalTime)
{
    uint32 ui32Gap = 0U;
    if (_bInitialized) {
        ui32Gap = getSeqIdGap (ui32SeqId, _ui32LatestPktSeqId);
    }
    else {
        _bInitialized = true;
    }
    _ui32LatestPktSeqId = ui32SeqId;

    if (ui32Gap == 0) {
        return NO_LOSS;
    }
    else if (ui32Gap > 0) {
        if ((ui32InterArrivalTime >= ((ui32Gap + 1) * ui32MinInterArrivalTime)) &&
            (ui32InterArrivalTime < ((ui32Gap + _dUpperLimit) * ui32MinInterArrivalTime))) {
            // ui32SeqId arrived right around the time that it should have arrived:
            // we can assume the missing packets were properly transmitted and lost to wireless errors.
            // If ui32SeqId arrives much earlier than it should, then at least some packets ahead of it
            // were dropped at a buffer, and if it arrives much later than expected, then it is likely
            // that queuing times at buffers have increased. Either way, congestion.
            return WIRELESS_LOSS;
        }
        return CONGESTION_LOSS;
    }
    // Packets of of order
    return NO_LOSS;
}

float Spike::DEFAULT_ALPHA = 0.5f;
float Spike::DEFAULT_BETA = (1.0f/3.0f);

Spike::Spike (float alpha, float beta)
    : _bInitialized (false),
      _spiking (false),
      _alpha (alpha),
      _beta (beta),
      _ui32LatestPktSeqId (0U)
{
}

Spike::~Spike (void)
{
}

LossType Spike::packetArrived (uint32 ui32SeqId, int64 i64CurrROTT, int64 i64MinROTT, int64 i64MaxROTT)
{
    _spiking = spikeStatus (_spiking, i64MinROTT, i64MaxROTT, i64CurrROTT);
    uint32 ui32Gap = 0U;
    if (_bInitialized) {
        ui32Gap = getSeqIdGap (ui32SeqId, _ui32LatestPktSeqId);
    }
    else {
        _bInitialized = true;
    }
    _ui32LatestPktSeqId = ui32SeqId;

    if (ui32Gap == 0) {
        return NO_LOSS;
    }
    else if (_spiking) {
        return CONGESTION_LOSS;
    }
    else {
        return WIRELESS_LOSS;
    }
}

bool Spike::spikeStatus (bool spiking, int64 i64MinROTT, int64 i64MaxROTT, int64 i64CurrROTT)
{
    const int64 i64Diff = i64MaxROTT - i64MinROTT;        
    if (spiking) {
        if (i64CurrROTT < (i64MinROTT + (_beta * i64Diff))) {
            return false;
        }     
    }
    else if (i64CurrROTT > (i64MinROTT + (_alpha * i64Diff))) {
        return true;
    }
    return spiking;
}

double ZigZag::DEFAULT_ALPHA = (1.0f/32.0f);

ZigZag::ZigZag (double alpha)
    : _bInitialized (false),
      _ui32LatestPktSeqId (0U),
      _rottEMA (alpha),
      _rootEMStdDev (2.0 * alpha)
{
}

ZigZag::~ZigZag (void)
{   
}

LossType ZigZag::packetArrived (uint32 ui32SeqId, int64 ui32CurrROTT)
{
    const double rottEMA =  _rottEMA.add (ui32CurrROTT);
    const double rootEMStdDev = _rootEMStdDev.add (fabs (ui32CurrROTT - rottEMA));

    uint32 ui32Gap = 0U;
    if (_bInitialized) {
        ui32Gap = getSeqIdGap (ui32SeqId, _ui32LatestPktSeqId);
    }
    else {
        _bInitialized = true;
    }
    _ui32LatestPktSeqId = ui32SeqId;

    switch (ui32Gap) {
        case 0: return NO_LOSS;
        case 1:
        case 3: return (ui32CurrROTT < (rottEMA - rootEMStdDev) ? WIRELESS_LOSS : CONGESTION_LOSS);
        default:
            return (ui32CurrROTT < (rottEMA - (rootEMStdDev / 2.0)) ? WIRELESS_LOSS : CONGESTION_LOSS);
    }
}

double HybridFilter::DEFAULT_ALPHA = 0.125;

HybridFilter::HybridFilter (double alpha)
    : _bInitialized (false),
      _alpha (alpha),
      _beta (1.0 - _alpha),
      _interArrEMA (_alpha)
{
}

HybridFilter::~HybridFilter (void)
{
}

LossType HybridFilter::packetArrived (int64 ui32CurrROTT, int64 ui32MinROTT,
                                      uint32 ui32InterArrivalTime, uint32 ui32MinInterArrivalTime,
                                      LossType mbiaz, LossType spike, LossType zigzag)
{
    if (_bInitialized) {
        const double dNormalizedAvgInterArr = _interArrEMA.add (ui32InterArrivalTime) / (double) ui32MinInterArrivalTime;
        if (ui32CurrROTT < (ui32MinROTT + (0.05 * ui32MinInterArrivalTime))) {
            return spike;
        }
        else if (dNormalizedAvgInterArr < _beta) {
            return zigzag;
        }
        else if (dNormalizedAvgInterArr < 1.5) {
            return mbiaz;
        }
        else {
            return zigzag;
        }
    }
    else {
        _bInitialized = true;
        return NO_LOSS;
    }
}

