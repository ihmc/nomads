#ifndef BANDWIDTH_ESTIMATOR_H
#define BANDWIDTH_ESTIMATOR_H

/*
 * BandwidthEstimator.h
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

#include "DLList.h"
#include "FTypes.h"
#include "StrClass.h"

#if !defined (ANDROID) //No std support on ANDROID
	#include <iostream>
	#include <fstream>
	#include <sstream>
	#include <cstdlib>
	#include <ctime>
#endif


class BandwidthEstimator
{
    protected:
        struct ACKSample
        {
            ACKSample (void);

            uint64 ui64Timestamp;
            uint64 ui64AcknowledgedData;

            NOMADSUtil::String toString (void);
        };

    public:
        //Constructor for BandwidthEstimator. Parameters: max lenght of the internals lists,
        // duration of the time interval considered by the bandwidth samples, max time between two ack samples, initial assumed bandwidth.
        BandwidthEstimator (uint32 ui32MaxSamplesNumber, uint32 ui32TimeIntervalInMs, uint64 ui64SamplingTime, uint16 ui16InitialAssumedBandwidth);

        virtual ~BandwidthEstimator (void);

        // Adds a new ACK sample to the internal list and consequentely computes bandwidth
        // samples and estimations. Returns the bandwidth estimated in bytes per second.
        // If it is the first sample a default value is used as bandwidth estimation.
        // param i64Timestamp timestamp of the ACK in milliseconds
        // param i64AcknowledgedData number of acknowledged bytes
        int32 addSample (uint64 i64Timestamp, uint64 i64AcknowledgedData, uint32 ui32PacketInQueue);

        void reset (void);
        
        void printList (void);

        // Returns the latest bandwidth estimation in bytes per milliseconds
        double getBandwidthEstimationBpms (void);

        void setNewTimeInterval (uint32 ui32NewTimeInt);

        // Computes a new bandwidth sample from the ACK samples over a time interval.
        //
        //                             SUM( AcknowledgedData(j*) )
        //    BandwidthSample(k) = -----------------------------------
        //                           TimeSlidingWindowInMilliseconds
        //
        //    * [ACKs j received in Time Sliding Window]
        //
        // param *pACKList list of ACK samples
        // return the bandwidth sample
        virtual double computeNewBandwidthSample (DLList<ACKSample> *pACKList);
        
        // Computes a new bandwidth estimation from the ACK samples over a time interval,
        // the bandwidth samples and the previous estimations.
        // param *pACKList list of ACK samples
        // param *pBWSamplesList list of bandwidth samples
        // param *pBWEstimationsList list of bandwidth estimations
        // return the bandwidth estimation
        virtual double computeNewBandwidthEstimation (DLList<ACKSample> *pACKList, DLList<double> *pBWSamplesList, DLList<double> *pBWEstimationsList, uint32 ui32PacketInQueue);

        DLList<ACKSample> *_pACKSamplesList;
        DLList<double> *_pBandwidthSamplesList;
        DLList<double> *_pBandwidthEstimationsList;

        uint32 _ui32TimeIntervalInMs;
        uint64 _ui64MaxSamplingTime;
        uint16 _ui16InitialAssumedBandwidth;
		
		#if !defined (ANDROID) //No std support on ANDROID
        	std::ofstream _logFile;
        	std::stringstream _ss;
		#endif
};

class WestwoodBandwidthEstimator : BandwidthEstimator
{
    public:
        // Constructor for WestwoodBandwidthEstimator
        // param ui32MaxSamplesNumber max lenght of the internals lists
        // param ui32TimeIntervalInMs duration of the time interval considered by the bandwidth samples
        // param ui64MaxSamplingTime max time between two ack samples
        WestwoodBandwidthEstimator (uint32 ui32MaxSamplesNumber, uint32 ui32TimeIntervalInMs, uint64 ui64SamplingTime, uint16 ui16InitialAssumedBandwidth);

        ~WestwoodBandwidthEstimator (void);

    protected:
        // Computes a new bandwidth sample from the ACK samples over a time interval
        //
        //                                 currentAck.ui64AcknowledgedData
        //      bandwidthSample = ----------------------------------------------------
        //                          currentAck.ui64Timestamp - lastAck.ui64Timestamp
        //
        // param *pACKList list of ACK samples
        // return the bandwidth sample
        double computeNewBandwidthSample (DLList<ACKSample> *pACKList);

        // Computes a new bandwidth estimation from the ACK samples over a time interval,
        // the bandwidth samples and the previous estimations.
        // param *pACKList list of ACK samples
        // param *pBWSamplesList list of bandwidth samples
        // param *pBWEstimationsList list of bandwidth estimations
        // return the bandwidth estimation
        double computeNewBandwidthEstimation (DLList<ACKSample> *pACKList, DLList<double> *pBWSamplesList, DLList<double> *pBWEstimationsList);
};

#endif   // #ifndef INCL_BANDWIDTH_ESTIMATOR_H
