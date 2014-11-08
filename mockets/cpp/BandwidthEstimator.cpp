/*
 * BandwidthEstimator.cpp
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

#include "BandwidthEstimator.h"
#include "NLFLib.h"
#include "Logger.h"
#include <stdio.h>


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

BandwidthEstimator::BandwidthEstimator (uint32 ui32MaxSamplesNumber, uint32 ui32TimeIntervalInMs, uint64 ui64MaxSamplingTime, uint16 ui16InitialAssumedBandwidth)
{
    _pACKSamplesList = new DLList<ACKSample> (ui32MaxSamplesNumber);
    _pBandwidthSamplesList = new DLList<double> (ui32MaxSamplesNumber);
    _pBandwidthEstimationsList = new DLList<double> (ui32MaxSamplesNumber);

    _ui64MaxSamplingTime = ui64MaxSamplingTime;
    _ui32TimeIntervalInMs = ui32TimeIntervalInMs;
    _ui16InitialAssumedBandwidth = ui16InitialAssumedBandwidth;
}

BandwidthEstimator::~BandwidthEstimator (void)
{
    delete _pACKSamplesList;
    _pACKSamplesList = NULL;
    delete _pBandwidthSamplesList;
    _pBandwidthSamplesList = NULL;
    delete _pBandwidthEstimationsList;
    _pBandwidthEstimationsList = NULL;

	#if !defined (ANDROID) //No stringstream support on ANDROID
    	_logFile << _ss.str();
    	_logFile.close();
	#endif
}

int32 BandwidthEstimator::addSample (uint64 ui64Timestamp, uint64 ui64AcknowledgedData, uint32 ui32PacketInQueue)
{
    ACKSample currentAckSample;

    currentAckSample.ui64Timestamp = ui64Timestamp;
    currentAckSample.ui64AcknowledgedData = ui64AcknowledgedData;

    // If the queue is empty then this is the first sample to be added
    if (_pACKSamplesList->getNumNodes() == 0) {
        //The new sample is added
        _pACKSamplesList->pushTail (currentAckSample);
        // A default value is used as first bandwidth estimation
        _pBandwidthSamplesList->pushTail (_ui16InitialAssumedBandwidth);
        _pBandwidthEstimationsList->pushTail (_ui16InitialAssumedBandwidth);
        return _ui16InitialAssumedBandwidth;
    }
    else {
        _pACKSamplesList->pushTail (currentAckSample);
        // Compute new bandwidth sample and bandwidth estimation
        double bandwidthSample = computeNewBandwidthSample (_pACKSamplesList);
        _pBandwidthSamplesList->pushTail (bandwidthSample);

        double dBandwidthEstimation = computeNewBandwidthEstimation (_pACKSamplesList, _pBandwidthSamplesList, _pBandwidthEstimationsList, ui32PacketInQueue);
        _pBandwidthEstimationsList->pushTail (dBandwidthEstimation);
        return (int32)(dBandwidthEstimation * 1000);
    }
}

void BandwidthEstimator::reset (void)
{
    uint32 ui32size = _pACKSamplesList->getMaxNumNodes();
    delete _pACKSamplesList;
    _pACKSamplesList = new DLList<ACKSample> (ui32size);
    delete _pBandwidthSamplesList;
    _pBandwidthSamplesList = new DLList<double> (ui32size);
    delete _pBandwidthEstimationsList;
    _pBandwidthEstimationsList = new DLList<double> (ui32size);
}

double BandwidthEstimator::computeNewBandwidthSample (DLList<ACKSample> *pACKList)
{
    uint64 ui64ackDataSum = 0;
    ACKSample tempAck, lastAck;
    pACKList->getLast (lastAck);
    // Save the timestamp and the amount of acknowledged data of the oldest sample within the TimeInterval
    uint64 ui64OldestTimestamp = lastAck.ui64Timestamp;
    uint64 ui64OldestAcknowledgedData = 0;

    while (pACKList->getPrev (tempAck)) {
        if ((tempAck.ui64Timestamp > (lastAck.ui64Timestamp - _ui32TimeIntervalInMs)) && (tempAck.ui64AcknowledgedData != 0)){
            ui64ackDataSum += tempAck.ui64AcknowledgedData;
            // Update oldest timestamp and acknowldged data
            ui64OldestTimestamp = tempAck.ui64Timestamp;
            ui64OldestAcknowledgedData = tempAck.ui64AcknowledgedData;
            //checkAndLogMsg ("BandwidthEstimator::computeNewBandwidthSample", Logger::L_MediumDetailDebug,
            //                " ui64ackDataSum %llu ui64OldestTimestamp %llu\n", ui64ackDataSum, ui64OldestTimestamp);
        }
        else {
            break;
        }
    }

    // Note that we are summing the number of acknowledged bytes over a period
    // of time from the most recent sack to the oldest one within the time interval,
    // so the most recent sack sample should be added to the sum but the oldest one
    // is out of the interval we are considering
    ui64ackDataSum -= ui64OldestAcknowledgedData;
    ui64ackDataSum += lastAck.ui64AcknowledgedData;

    // Divide the sum of bytes acknowledged by the current timestamp - the last timestamp within the time interval.
    double dBandwidthSample = 0;
    uint64 ui64ActualInterval = lastAck.ui64Timestamp - ui64OldestTimestamp;
    //checkAndLogMsg ("BandwidthEstimator::computeNewBandwidthSample", Logger::L_MediumDetailDebug,
    //                " most recent timestamp: %llu actual interval: %llu actual data sum: %llu\n", lastAck.ui64Timestamp, ui64ActualInterval, ui64ackDataSum);
    if (ui64ActualInterval != 0) {
        dBandwidthSample = (double) (ui64ackDataSum / (lastAck.ui64Timestamp - ui64OldestTimestamp));
        checkAndLogMsg ("BandwidthEstimator::computeNewBandwidthSample", Logger::L_MediumDetailDebug,
                        " new bandwidth sample is %f\n", dBandwidthSample);
    }
    else {
        dBandwidthSample = (double) (ui64ackDataSum / _ui32TimeIntervalInMs);
        checkAndLogMsg ("BandwidthEstimator::computeNewBandwidthSample", Logger::L_MediumDetailDebug,
                        " there is a single ack within the time interval. New bandwidth sample %f\n", dBandwidthSample);
    }
    return dBandwidthSample;
}

double BandwidthEstimator::computeNewBandwidthEstimation (DLList<ACKSample> *pACKList,
                                                          DLList<double> *pBWSamplesList,
                                                          DLList<double> *pBWEstimationsList,
                                                          uint32 ui32PacketInQueue)
{
    double dCurrentBandwidthSample, dLastBandwidthSample;
    pBWSamplesList->getLast (dCurrentBandwidthSample);
    pBWSamplesList->getPrev (dLastBandwidthSample);

    double dLastBandwidthEstimation;
    pBWEstimationsList->getLast (dLastBandwidthEstimation);

    // Depending on how many samples are in the queue the importance of the last bandwidth estimation and the new one should change.
    // It should also change depending on the sending rate of the sender (sender sending at full speed=accurate bandwidth estimation)
    if (_pACKSamplesList->getNumNodes() < _pACKSamplesList->getMaxNumNodes()/2) {
        return 0.2 * dLastBandwidthEstimation + 0.8 * (dCurrentBandwidthSample + dLastBandwidthSample) / 2;
    }
    if (_pACKSamplesList->getNumNodes() < _pACKSamplesList->getMaxNumNodes()) {
        return 0.6 * dLastBandwidthEstimation + 0.4 * (dCurrentBandwidthSample + dLastBandwidthSample) / 2;
    }
    if (ui32PacketInQueue > 10) {
        // If the number of packets in the pending packet queue is considerable (>10)
        // we can assume the sender is sending at a high rate so the new sample is highly valuable
        return 0.6 * dLastBandwidthEstimation + 0.4 * (dCurrentBandwidthSample + dLastBandwidthSample) / 2;
    }
    return 0.9 * dLastBandwidthEstimation + 0.1 * (dCurrentBandwidthSample + dLastBandwidthSample) / 2;
}

double BandwidthEstimator::getBandwidthEstimationBpms (void)
{
    double dLastBandwidthEstimation = -1.0;
    if (_pBandwidthEstimationsList->getNumNodes() > 0) {
       _pBandwidthEstimationsList->getLast (dLastBandwidthEstimation);
    }
    return dLastBandwidthEstimation;
}

void BandwidthEstimator::setNewTimeInterval (uint32 ui32NewTimeInt)
{
    _ui32TimeIntervalInMs = ui32NewTimeInt;
}

void BandwidthEstimator::printList (void)
{
    ACKSample ackSample; 
    double dBwSample, dBwEstimation;
	
	#if !defined (ANDROID) //No std support on ANDROID
    	std::cout << "ACK time (us)  \t ACK Data (bytes) \t Bandwidth Sample (kB/s) \t Bandwidth Estimation (kB/s)" << std::endl;
    	std::cout << "----------------------------------------------------------------------------------------------------" << std::endl;
	#endif

    _pACKSamplesList->resetToHead();
    _pBandwidthSamplesList->resetToHead();
    _pBandwidthEstimationsList->resetToHead();

	#if !defined (ANDROID) //No std support on ANDROID
   		while (_pACKSamplesList->getNext(ackSample)) {
        	std::cout << ackSample.ui64Timestamp << " \t " << ackSample.ui64AcknowledgedData << " \t\t\t "; 
        	if (_pBandwidthSamplesList->getNext (dBwSample)) {
            	std::cout << dBwSample << " \t\t\t ";
        	}
        	else {
            	std::cout << " \t\t\t ";
        	}
        	if (_pBandwidthEstimationsList->getNext (dBwEstimation)) {
            	std::cout << dBwEstimation << " \t\t\t ";
        	}
       	 	else {
            	std::cout << " \t\t\t ";
        	}
        	std::cout << std::endl;
  		}
	#endif
}

BandwidthEstimator::ACKSample::ACKSample (void)
{
    ui64Timestamp = 0;
    ui64AcknowledgedData = 0;
}

NOMADSUtil::String BandwidthEstimator::ACKSample::toString (void)
{
    char szBuf[128];
    sprintf(szBuf, "(%I64d) %I64d\n", ui64Timestamp, ui64AcknowledgedData);

    // This doesn't work. It converts the int values to a char
    // which probably isn't what you want. - jk 11/2010
    //NOMADSUtil::String out("(");
    //out += ui64Timestamp;
    //out += ") ";
    //out += ui64AcknowledgedData;
    //out += "\n";
    return NOMADSUtil::String(szBuf);
}

WestwoodBandwidthEstimator::WestwoodBandwidthEstimator (uint32 ui32MaxSamplesNumber, uint32 ui32TimeIntervalInMs, uint64 ui64MaxSamplingTime, uint16 ui16InitialAssumedBandwidth)
                          : BandwidthEstimator (ui32MaxSamplesNumber, ui32TimeIntervalInMs, ui64MaxSamplingTime, ui16InitialAssumedBandwidth)
{
}

WestwoodBandwidthEstimator::~WestwoodBandwidthEstimator (void)
{
    delete _pACKSamplesList;
    _pACKSamplesList = NULL;
    delete _pBandwidthSamplesList;
    _pBandwidthSamplesList = NULL;
    delete _pBandwidthEstimationsList;
    _pBandwidthEstimationsList = NULL;

	#if !defined (ANDROID) //No stringstream support on ANDROID
    	_logFile << _ss.str();
    	_logFile.close();
	#endif
}

double WestwoodBandwidthEstimator::computeNewBandwidthSample (DLList<ACKSample> *pACKList)
{
    ACKSample currentAck, lastAck;
    pACKList->getLast (currentAck);
    pACKList->getPrev (lastAck);

    uint64 ui64Delta = currentAck.ui64Timestamp - lastAck.ui64Timestamp;

    // If, for some reason, the time difference between the two ack samples is zero no division is
    // performed and the current amount of acknowledged data is returned
    if (ui64Delta) {
	return currentAck.ui64AcknowledgedData / (double)ui64Delta;
    }
    else {
	return (double)currentAck.ui64AcknowledgedData;
    }
}

double WestwoodBandwidthEstimator::computeNewBandwidthEstimation (DLList<ACKSample> *pACKList,
                                                                  DLList<double> *pBWSamplesList,
                                                                  DLList<double> *pBWEstimationsList)
{
    ACKSample currentAck, lastAck;
    pACKList->getLast( currentAck );
    pACKList->getPrev( lastAck );
    uint64 ui64ACKDelta = currentAck.ui64Timestamp - lastAck.ui64Timestamp;  

    double currentBandwidthSample, lastBandwidthSample;
    pBWSamplesList->getLast( currentBandwidthSample );
    if (!pBWSamplesList->getPrev( lastBandwidthSample )) {
        return currentBandwidthSample;
    }

    double lastBandwidthEstimation;
    pBWEstimationsList->getLast( lastBandwidthEstimation );

    double dAlpha = (4 * _ui64MaxSamplingTime - ui64ACKDelta ) / (double)(4 * _ui64MaxSamplingTime + ui64ACKDelta );
    return dAlpha * lastBandwidthEstimation + (1 - dAlpha) * currentBandwidthSample;
}


