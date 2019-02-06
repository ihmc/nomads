/*
 * CongestionController.cpp
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

#include "CongestionController.h"
#include "Logger.h"
#include "NLFLib.h"
#include "Transmitter.h"
#if !defined (ANDROID) //No std/STL support on ANDROID
    #include <algorithm>
    #include <iostream>
#endif
#include "math.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

CongestionController::CongestionController (Mocket *pMocket)
{
    _pMocket = pMocket;

    _ui16MaximumMTU = _pMocket->getMTU();
    _ui16InitialSlowStartThresholdSegments = 4;
    _ui32SlowStartThreshold = _ui16InitialSlowStartThresholdSegments * _ui16MaximumMTU;
    _ui32CongestionWindowSize = _ui32SlowStartThreshold;
    _bCongestionAvoidance = false;

    _pRTTRecord = new TimeIntervalAverage<double> (5000);

    _ui64LastUpdateTimestamp = NOMADSUtil::getTimeInMilliseconds();

    _bFirstCycleThreshold = 100;
    _bFastRecovery = false;

    _pMocket->getTransmitter()->setBandwidthEstimationActive (_pMocket->getInitialAssumedBandwidth());

    checkAndLogMsg ("CongestionController::CongestionController", Logger::L_MediumDetailDebug, "created a new instance of CongestionController\n");
}

CongestionController::~CongestionController (void)
{
    delete _pRTTRecord;
    _pRTTRecord = nullptr;
}

void CongestionController::update (void)
{
    double dRTT = _pMocket->getStatistics()->_fSRTT;
    _pRTTRecord->add(NOMADSUtil::getTimeInMilliseconds(), dRTT);
    double dBandwidthEstimation = _pMocket->getStatistics()->getEstimatedBandwidth();

    _bFirstCycleThreshold--;
    if (!_bFirstCycleThreshold) {
        _ui32SlowStartThreshold = NOMADSUtil::maximum ( (uint32)2, (uint32)(dBandwidthEstimation * _pRTTRecord->getMin() / _ui16MaximumMTU ) ) * _ui16MaximumMTU;
        if (_ui32CongestionWindowSize >= _ui32SlowStartThreshold)
           _bCongestionAvoidance = true;
        else _bCongestionAvoidance = false;
    }

    if (_bCongestionAvoidance) {
        //Increase the Window size by 1 MMTU every RTT (approximatively)
        _ui32CongestionWindowSize += _pMocket->getTransmitter()->getNumberOfAcknowledgedPackets() * ( _ui16MaximumMTU * _ui16MaximumMTU ) / _ui32CongestionWindowSize;
    }
    else {
        //Increase the Window size by 1 MMTU for every received ACK
        _ui32CongestionWindowSize += _pMocket->getTransmitter()->getNumberOfAcknowledgedPackets() * _ui16MaximumMTU;
    }

    if (!_ui32CongestionWindowSize) _ui32CongestionWindowSize = _ui16MaximumMTU;
    if (_ui32CongestionWindowSize >= _ui32SlowStartThreshold)
        _bCongestionAvoidance = true;

    _ui64LastUpdateTimestamp = NOMADSUtil::getTimeInMilliseconds();
}

uint32 CongestionController::adaptToCongestionWindow (uint32 ui32SpaceAvailable)
{
    return NOMADSUtil::minimum (ui32SpaceAvailable, getCongestionWindow());
}

//note: should consider using some defines for codes
void CongestionController::reactToLosses (uint8 ui8Code)
{
    double dRTT = _pRTTRecord->getMin();

    _ui32SlowStartThreshold = NOMADSUtil::maximum ( (uint16)(_ui16InitialSlowStartThresholdSegments * _ui16MaximumMTU), (uint16)(_pMocket->getStatistics()->getEstimatedBandwidth() * dRTT) ) ;

    if (ui8Code == 0) { //Timeout
        _ui32CongestionWindowSize = _ui16InitialSlowStartThresholdSegments * _ui16MaximumMTU;
    }
    else {
        _ui32CongestionWindowSize = _ui32SlowStartThreshold + (ui8Code-1)*_ui16MaximumMTU;
    }

    if (_ui32CongestionWindowSize < _ui16MaximumMTU) _ui32CongestionWindowSize = _ui16MaximumMTU;
    if (_ui32CongestionWindowSize >= _ui32SlowStartThreshold)
        _bCongestionAvoidance = true;
    else _bCongestionAvoidance = false;

}

//TODO: comments
//      remember to synchronize with constructor
void CongestionController::reset (void)
{
    _ui16MaximumMTU = _pMocket->getMTU();
    _ui16InitialSlowStartThresholdSegments = 4;
    _ui32SlowStartThreshold = _ui16InitialSlowStartThresholdSegments * _ui16MaximumMTU;
    _ui32CongestionWindowSize = _ui32SlowStartThreshold;
    _bCongestionAvoidance = false;

    delete _pRTTRecord;
    _pRTTRecord = new TimeIntervalAverage<double> (5000);

    _ui64LastUpdateTimestamp = NOMADSUtil::getTimeInMilliseconds();

    _bFirstCycleThreshold = 100;
    _bFastRecovery = false;
}

