/* 
 * File:   Stats.cpp
 * Author: gbenincasa
 * 
 * Created on May 12, 2015, 1:07 PM
 */

#include "ReceiverMetrics.h"

#include "NLFLib.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

ReceiverMetrics::ReceiverMetrics (uint32 ui32SteadyStateTime)
    : _ui32UpdateTimeoutInMillis (ui32SteadyStateTime),
      _i64IntervalStartTime (ui32SteadyStateTime),
      _rate (getTimeInMilliseconds()),
      _biaz (Biaz::DEFAULT_UPPER_LIMIT),
      _mbiaz (Biaz::MODIFIED_UPPER_LIMIT),
      _interArrAvg (1000),
      _rottAvg (1000)
{
}

ReceiverMetrics::~ReceiverMetrics()
{
}

bool ReceiverMetrics::packetArrived (int iBytes, uint32 ui32SeqId, int64 i64SendingTime, Metrics &metrics)
{
    if (iBytes <= 0) {
        return false;
    }
    
    const int64 i64Now = getTimeInMilliseconds();
    const uint32 ui32ElapsedTime = i64Now - _i64IntervalStartTime;

    // Update Metrics
    metrics._loss = _loss.add (ui32SeqId);
    
    if (_interArr.add (i64Now)) {
        _interArrAvg.add (_interArr.get());
        metrics.fAvgInterArr = static_cast<float>(_interArrAvg.getAverage());
    }
    else {
        metrics.fAvgInterArr = 0.0f;
    }
    metrics._ui32MinInterArrival = _interArr.getMin();

    if (_rott.add (i64Now, i64SendingTime)) {
        _rottAvg.add (_rott.get());
        metrics.fAvgROTT = static_cast<float>(_rottAvg.getAverage());
    }
    else {
        metrics.fAvgROTT = 0.0f;
    }

    metrics.fbPS = _rate.add (i64Now, iBytes);
    metrics.fBPS = _rate.get (Rate::Bps);

    // Update classifiers
    metrics._biaz  = _biaz.packetArrived (ui32SeqId, _interArr.get(), _interArr.getMin());
    metrics._mBiaz = _mbiaz.packetArrived (ui32SeqId, _interArr.get(), _interArr.getMin());
    metrics._spike = _spike.packetArrived (ui32SeqId, _rott.get(), _rott.getMin(), _rott.getMax());
    metrics._zigzag = _zigzag.packetArrived (ui32SeqId, _rott.getMax());
    metrics._linkState = _linkState.packetArrived (_rott.get(), _rott.getMin(), _interArr.get(), _interArr.getMin());
    metrics._zbs = _zbs.packetArrived (metrics._linkState, metrics._mBiaz, metrics._spike, metrics._zigzag);

    if (ui32ElapsedTime > _ui32UpdateTimeoutInMillis) {
        _i64IntervalStartTime = getTimeInMilliseconds();
        return true;
    }
    return false;
}

Metrics::Metrics (void)
    : fbPS (0.0f),                // bits per second
      fBPS (0.0f),                  // Bytes per second
      _ui32MinInterArrival (0U), // Average inter-arrival time in millisec
      _loss (0.0f),
      fAvgInterArr (0.0f),          // Average inter-arrival time in millisec
      fAvgROTT (0.0f),              // Average Relative One-way Trip Time (ROTT))
      _biaz (NO_LOSS),
      _mBiaz (NO_LOSS),
      _spike (NO_LOSS),
      _zigzag (NO_LOSS)
{
}

Metrics::~Metrics (void)
{
}

void Metrics::display (void)
{
    printf ("Rate %.1f bps\t%.1f Kbps\t%.1f Mbps\t%.lf Bps\t%.1f KBps\t%.1f MBps\t Loss %.5f.\tAvg Interarrival %.1f ms (%u minimum).\t"
            "Avg ROTT %.1f ms.\tbiaz %d mbiaz %d spike %d zigzag %d zbs %d (link state %s).\n",
            fbPS, fbPS / 1024, fbPS / (1024*1024), fBPS, fBPS / 1024, fBPS / (1024*1024), _loss, fAvgInterArr,
            _ui32MinInterArrival, fAvgROTT, _biaz, _mBiaz, _spike, _zigzag, _zbs, linkStateAsString (_linkState));
}

