#ifndef INCL_CONGESTION_CONTROLLER_H
#define    INCL_CONGESTION_CONTROLLER_H

/*
 * CongestionController.h
 * Author: nino
 *
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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
 */

#if !defined (ANDROID) //No std support on ANDROID
    #include <fstream>
    #include <sstream>
#endif
#include "FTypes.h"
#include "Mocket.h"
#include "TimeIntervalAverage.h"
#include "BandwidthEstimator.h"
#include "CongestionControl.h"


class CongestionController : public CongestionControl
{
    public:
        CongestionController (Mocket *pMocket);
        ~CongestionController (void);

        void update (void);
        void reactToLosses (uint8 ui8Code);
        uint32 adaptToCongestionWindow (uint32 ui32SpaceAvailable);
    void reset (void);
        void printState (void);

    private:
        uint32 getCongestionWindow (void);

        uint32 _ui32CongestionWindowSize;
        uint16 _ui16InitialSlowStartThresholdSegments;
        uint32 _ui32SlowStartThreshold;
        uint16 _ui16MaximumMTU;

        TimeIntervalAverage<double> *_pRTTRecord;

        bool _bCongestionAvoidance;
        uint64 _ui64LastUpdateTimestamp;

        uint16 _bFirstCycleThreshold;
        bool _bFastRecovery;

#if !defined (ANDROID) //No std support on ANDROID
        std::ofstream _logFile;
        std::stringstream ss;
#endif
};

inline uint32 CongestionController::getCongestionWindow (void)
{
    return _ui32CongestionWindowSize;
}

#endif    /* _CONGESTION_CONTROLLER_H */
