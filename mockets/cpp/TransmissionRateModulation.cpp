/*
 * TransmissionRateModulation.cpp
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

#include "TransmissionRateModulation.h"
#include "Transmitter.h"

#include "Logger.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

TransmissionRateModulation::TransmissionRateModulation (Mocket *pMocket)
{
    _pMocket = pMocket;
    // Activate bandwidth estimation since it is part of the congestion control process
    // and initialize it with the appropriate initial bandwidth: half the transmit rate.
    // Note that the TransmissionRateModulationInitialThreshold is in byte per sec but Bandwidth Estimator works with byte per milliseconds.
    _pMocket->getTransmitter()->setBandwidthEstimationActive (_pMocket->getTransmissionRateModulationInitialThreshold()/1000/2);
    // Set an initial bandwidth limit
    _pMocket->getTransmitter()->setTransmitRateLimit (_pMocket->getTransmissionRateModulationInitialThreshold());

    _bInitializationPhase = true;
    checkAndLogMsg ("TransmissionRateModulation::TransmissionRateModulation", Logger::L_MediumDetailDebug, "created a new instance of TransmissionRateModulation\n");
}

void TransmissionRateModulation::update (void)
{
    // We increase the bandwidth limit every time the bandwidth estimation reaches
    // the limit imposed meaning that there could be more bandwidth available to be used
    if (_pMocket->getStatistics()->getEstimatedBandwidth() >= (int32)_pMocket->getTransmitter()->getTransmitRateLimit()) {
        // Increase bandwidth limit to 10% more than the estimated bandwidth
        _pMocket->getTransmitter()->setTransmitRateLimit ((uint32)(_pMocket->getStatistics()->getEstimatedBandwidth()*1.1));
        // At the beginning just increase the limit until the bandwidth estimation reaches the limit for the first time
        if (_bInitializationPhase) {
            _bInitializationPhase = false;
        }
    }
    else {
        // If we are not in the initialization phase
        if (!_bInitializationPhase) {
            // We decrease the bandwidth limit if the bandwidth estimation is 20% lower than the limit
            if (_pMocket->getStatistics()->getEstimatedBandwidth() < _pMocket->getTransmitter()->getTransmitRateLimit()*0.8) {
                // Decrease the bandwidth limit to the estimated bandwidth
                _pMocket->getTransmitter()->setTransmitRateLimit ((uint32)(_pMocket->getStatistics()->getEstimatedBandwidth()));
            }
        }
    }

}
