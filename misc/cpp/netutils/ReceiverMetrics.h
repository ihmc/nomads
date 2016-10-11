/* 
 * File:   Stats.h
 * Author: gbenincasa
 *
 * Created on May 12, 2015, 1:07 PM
 */

#ifndef INCL_RECEIVER_METRICS_H
#define	INCL_RECEIVER_METRICS_H

#include "LossClassifiers.h"
#include "NetMetrics.h"
#include "MovingAverage.h"

namespace IHMC_MISC
{
    struct Metrics
    {
        Metrics (void);
        ~Metrics (void);

        void display (void);

        float fbPS;                  // bits per second
        float fBPS;                  // Bytes per second
        uint32 _ui32MinInterArrival; // Average inter-arrival time in millisec
        float _loss;
        float fAvgInterArr;          // Average inter-arrival time in millisec
        float fAvgROTT;              // Average Relative One-way Trip Time (ROTT))
        LossType _biaz;
        LossType _mBiaz;
        LossType _spike;
        LossType _zigzag;
        LossType _zbs;
        LinkState _linkState;
    };

    class ReceiverMetrics
    {
        public:
            /**
             * 
             * @param ui32SteadyStateTime - the time after which the system should update the stats
             */
            ReceiverMetrics (uint32 ui32UpdateTimeoutInMillis = 1000);
            virtual ~ReceiverMetrics();

            /**
             * 
             * @param iBytes
             * @param ui32SeqId
             * @param i64SendingTime
             * @param metrics
             * @return true whether the metrics were updated, false otherwise
             */
            bool packetArrived (int iBytes, uint32 ui32SeqId, int64 i64SendingTime, Metrics &metrics);

        private:
            const uint32 _ui32UpdateTimeoutInMillis;
            int64 _i64IntervalStartTime;
            InterArrival _interArr;
            Loss _loss;
            Rate _rate;
            Rott _rott;
            Biaz _biaz;
            Biaz _mbiaz;
            Spike _spike;
            ZigZag _zigzag;
            ZBS _zbs;
            LinkStateClassifiers _linkState;
            NOMADSUtil::MovingAverage<uint32> _interArrAvg;
            NOMADSUtil::MovingAverage<int64> _rottAvg;
    };
}

#endif	/* INCL_RECEIVER_METRICS_H */

