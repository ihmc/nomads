/* 
 * File:   NetMetrics.h
 * Author: gbenincasa
 *
 * Created on May 13, 2015, 2:27 PM
 */

#ifndef INCL_NETWORK_METRICS_H
#define	INCL_NETWORK_METRICS_H

#include "FTypes.h"

namespace IHMC_MISC
{
    class InterArrival
    {
        public:
            InterArrival (void);
            ~InterArrival (void);

            bool add (int64 i64ArrivalTime);
            uint32 get (void) const;
            uint32 getMin (void) const;
            double getExpAvg (void) const;

        private:
            bool _bInitialized;
            uint32 _ui32Min;
            uint32 _ui32CurrInterArrivalTime;
            int64 _i64PrevPktArrivalTime;
    };

    class Loss
    {
        public:
            Loss (void);
            ~Loss (void);

            float add (uint32 ui32SeqId);
            float get (void);

        private:
            bool _bInitialized;
            uint32 _ui32FirstSeqId;
            uint32 _ui32LatestSeqId;
            uint32 _ui32Count;
    };

    class Rott
    {
        public:
            Rott (void);
            ~Rott (void);

            bool add (int64 i64ArrivalTime, int64 i64RelTime);
            int64 get (void) const;
            int64 getMin (void) const;
            int64 getMax (void) const;

        private:
            bool _bInitialized;
            int64 _i64Max;
            int64 _i64Min;
            int64 _i64CurrROTT;
            int64 _i64PrevROTT;
    };

    class Rate
    {
        public:
            Rate (int64 i64StartTime);
            ~Rate (void);

            void init (int64 i64StartTime);
            float add (int64 i64CurrTime, uint32 ui32Bytes);

            enum Scale
            {
                MBps,  // mega-bytes per second
                KBps,  // kilo-bytes per second
                Bps,   // bytes per seconds

                Mbps,  // mega-bits per second
                Kbps,  // kilo-bits per second
                bps,   // bits per seconds

                pktps  // packets per second
            };
            float get (Scale scale);

        private:
            float _bps;   // bits per second
            float _pktps; // packets per second
            uint32 _ui32NPackets;
            int64 _i64StartTime;
            uint64 _ui64Bytes;
    };
}

#endif	/* INCL_NETWORK_METRICS_H */

