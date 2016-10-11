/* 
 * File:   NetMetrics.cpp
 * Author: gbenincasa
 * 
 * Created on May 13, 2015, 2:27 PM
 */

#include "NetMetrics.h"

#include <assert.h>

using namespace IHMC_MISC;

//------------------------------------------------------------------------------
// InterArrival
//------------------------------------------------------------------------------


InterArrival::InterArrival()
    : _bInitialized (false),
      _ui32Min (0xFFFFFFFFU),
      _ui32CurrInterArrivalTime (0U),
      _i64PrevPktArrivalTime (0U)
{}

InterArrival::~InterArrival() {}

bool InterArrival::add (int64 i64ArrivalTime)
{
    const bool bWasInitialized = _bInitialized;
    if (_bInitialized) {
        _ui32CurrInterArrivalTime = (i64ArrivalTime - _i64PrevPktArrivalTime);
        if (_ui32CurrInterArrivalTime < _ui32Min) {
            _ui32Min = _ui32CurrInterArrivalTime;
        }
    }
    else {
        _bInitialized = true;
    }
    _i64PrevPktArrivalTime = i64ArrivalTime;
    return bWasInitialized;
}

uint32 InterArrival::get (void) const
{
    return _ui32CurrInterArrivalTime;
}

uint32 InterArrival::getMin (void) const
{
    return _ui32Min;
}

//------------------------------------------------------------------------------
// ROTT
//------------------------------------------------------------------------------

Rott::Rott()
    : _bInitialized (false),
      _i64Min (0xFFFFFFFFU),
      _i64Max (0U),
      _i64CurrROTT (0U)
{
}

Rott::~Rott()
{
}

bool Rott::add (int64 i64ArrivalTime, int64 i64SendingTime)
{
    const bool bWasInitialized = _bInitialized;
    _i64CurrROTT = (i64ArrivalTime - i64SendingTime);
    if (_bInitialized) {
        if (_i64CurrROTT < _i64Min) {
            _i64Min = _i64CurrROTT;
        }
        if (_i64CurrROTT > _i64Max) {
            _i64Max = _i64CurrROTT;
        }
    }
    else {
        _i64Min = _i64CurrROTT;
        _i64Max = _i64CurrROTT;
        _bInitialized = true;
    }
    _i64PrevROTT = _i64CurrROTT;
    return bWasInitialized;
}

int64 Rott::get (void) const
{
    return _i64PrevROTT;
}

int64 Rott::getMin (void) const
{
    return _i64Min;
}

int64 Rott::getMax (void) const
{
    return _i64Max;
}

//------------------------------------------------------------------------------
// Throughput
//------------------------------------------------------------------------------

Rate::Rate (int64 i64StartTime)
    : _bps (0.0f),
      _pktps (0.0f),
      _ui32NPackets (0U),
      _i64StartTime (i64StartTime),
      _ui64Bytes (0U)
{
}

Rate::~Rate (void)
{
}

void Rate::init (int64 i64StartTime)
{
    _i64StartTime = i64StartTime;
    _ui64Bytes = 0U;
    _bps = 0.0f;
    _pktps = 0.0f;
}

float Rate::add (int64 i64CurrTime, uint32 ui32Bytes)
{
    _ui32NPackets++;
    _ui64Bytes += ui32Bytes;
    const uint32 ui32ElapsedTime = (uint32) (i64CurrTime - _i64StartTime);
    _bps = (_ui64Bytes / (ui32ElapsedTime / 1000.0f)) * 8.0f;
    _pktps = (_ui32NPackets / (ui32ElapsedTime / 1000.0f));
    return get (bps);
}

float Rate::get (Scale scale)
{
    float bitsPerSec = _bps;
    float bytesPerSec = _bps / 8.0f; // Bytes per second
    switch (scale) {
        case MBps:  bytesPerSec /= 1024.0f;
        case KBps:  bytesPerSec /= 1024.0f;
        case Bps:   return bytesPerSec;
        case Mbps:  bitsPerSec /= 1024.0f;
        case Kbps:  bitsPerSec /= 1024.0f;
        case bps:   return bitsPerSec;
        case pktps: return _pktps;
        default: {
            assert (false);
            return bps;  // this case should never happen
        }
    }
}

//------------------------------------------------------------------------------
// Loss
//------------------------------------------------------------------------------

Loss::Loss()
    : _bInitialized (false),
      _ui32FirstSeqId (0U),
      _ui32LatestSeqId (0U),
      _ui32Count (0U)
{}

Loss::~Loss(){}

float Loss::add (uint32 ui32SeqId)
{
    if (!_bInitialized) {
        _ui32FirstSeqId = ui32SeqId;
        _bInitialized = true;
    }
    _ui32LatestSeqId = ui32SeqId;
    _ui32Count++;
    return get();
}

float Loss::get (void)
{
    if (!_bInitialized) {
        return 0.0f;
    }
    return 1.0f - ((float)_ui32Count / ((float)(_ui32LatestSeqId - _ui32FirstSeqId + 1.0f)));
}

