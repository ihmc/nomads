/* 
 * File:   CongestionClassifiers.h
 * Author: gbenincasa
 *
 * Created on May 13, 2015, 2:35 PM
 */

#ifndef INCL_CONGESTION_CLASSIFIERS_H
#define	INCL_CONGESTION_CLASSIFIERS_H

#include "ExponentialAverage.h"
#include "FTypes.h"

namespace IHMC_MISC
{
    enum LossType
    {
        NO_LOSS         = 0x00,
        WIRELESS_LOSS   = 0x01,
        CONGESTION_LOSS = 0x02
    };

    class Biaz
    {
        public:
            Biaz (double dUpperLimit);
            ~Biaz (void);

            LossType packetArrived (uint32 ui32SeqId, uint32 ui32InterArrivalTime, uint32 ui32MinInterArrivalTime);

        public:
            static const double DEFAULT_UPPER_LIMIT;
            static const double MODIFIED_UPPER_LIMIT;  // mBiaz

        private:
            bool _bInitialized;
            uint32 _ui32LatestPktSeqId;
            const double _dUpperLimit;
    };

    class Spike
    {
        public:
            static float DEFAULT_ALPHA;
            static float DEFAULT_BETA;

        public:
            Spike (float alpha = DEFAULT_ALPHA, float beta = DEFAULT_BETA);
            ~Spike (void);

            LossType packetArrived (uint32 ui32SeqId, int64 i64CurrROTT, int64 ui32MinROTT, int64 ui32MaxROTT);

        private:
            bool spikeStatus (bool spiking, int64 i64MinROTT, int64 i64MaxROTT, int64 i64CurrROTT);

        private:
            bool _bInitialized;
            bool _spiking;
            const float _alpha;
            const float _beta;
            uint32 _ui32LatestPktSeqId;
    };

    class ZigZag
    {
        public:
            static double DEFAULT_ALPHA;

        public:
            ZigZag (double alpha = DEFAULT_ALPHA);
            ~ZigZag (void);

            LossType packetArrived (uint32 ui32SeqId, int64 ui32CurrROTT);

        private:
            bool _bInitialized;
            uint32 _ui32LatestPktSeqId;
            NOMADSUtil::ExponentialAverage<uint32> _rottEMA;
            NOMADSUtil::ExponentialAverage<double> _rootEMStdDev;
    };

    class HybridFilter
    {
        public:
            static double DEFAULT_ALPHA;

        public:
            HybridFilter (double alpha = DEFAULT_ALPHA);
            ~HybridFilter (void);

            LossType packetArrived (int64 ui32CurrROTT, int64 ui32MinROTT,
                                    uint32 ui32InterArrivalTime, uint32 ui32MinInterArrivalTime,
                                    LossType mbiaz, LossType spike, LossType zigzag);

        private:
            bool _bInitialized;
            const double _alpha;
            const double _beta;
            NOMADSUtil::ExponentialAverage<uint32> _interArrEMA;
    };
}

#endif	/* INCL_CONGESTION_CLASSIFIERS_H */

