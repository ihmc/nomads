#ifndef BANDWIDTH_ESTIMATOR_H
#define BANDWIDTH_ESTIMATOR_H

/*
 * BandwidthEstimator.h
 *
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2016 IHMC.
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
        /**
        Constructor and destructor for BandwidthEstimator
        - ui32MaxSamplesNumber: max lenght of the internals lists
        - ui32TimeIntervalInMs: duration of the time interval considered by the bandwidth samples
        - ui64SamplingTime: max time between two ack samples
        - ui16InitialAssumedBandwidth: initial assumed bandwidth
        */
        BandwidthEstimator (uint32 ui32MaxSamplesNumber, uint32 ui32TimeIntervalInMs, uint64 ui64SamplingTime, uint16 ui16InitialAssumedBandwidth);
        virtual ~BandwidthEstimator (void);

        /**
        Adds a new ACK sample to the internal list and consequentely computes bandwidth samples and estimations.
        If it is the first sample a default value is used as bandwidth estimation.
        - i64Timestamp: Timestamp of the ACK in milliseconds
        - i64AcknowledgedData: Number of acknowledged bytes
        - ui32PacketInQueue: Number of packets in the queue
        @retun: The bandwidth estimated in bytes per second.
        */
        int32 addSample (uint64 i64Timestamp, uint64 i64AcknowledgedData, uint32 ui32PacketInQueue);

        /**
        Destroy and then re-initialize the internal DLLISTs
        */
        void reset (void);

        /**
        Print DLL contents
        */
        void printList (void);

        /**
        Returns the latest bandwidth estimation in B/ms
        @retun: Last estimated BW or -1 if no estimation is present yet
        */
        double getBandwidthEstimationBpms (void);

        /**
        Set new time interval used for the estimation process
        - ui32NewTimeInt: New time limit
        */
        void setNewTimeInterval (uint32 ui32NewTimeInt);

        /**
        Computes a new bandwidth sample from the ACK samples over a time interval, that is:
                                        SUM( AcknowledgedData(j*) )
                BandwidthSample(k) = -----------------------------------
                                        TimeSlidingWindowInMilliseconds

                *Where j is the number of ACKs received in Time Sliding Window
        - pACKList: Pointer to the list of ACK samples
        @retun: The new computed bandwidth sample
        */
        virtual double computeNewBandwidthSample (DLList<ACKSample> *pACKList);

        /**
        Computes a new bandwidth estimation from the ACK samples over a time interval,the bandwidth samples and the previous estimations.
        - pACKList list of ACK samples
        - pBWSamplesList list of bandwidth samples
        - pBWEstimationsList list of bandwidth estimations
        @retun: the bandwidth estimation
        */
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
        /**
        Cosructor and destructor method for WestwoodBandwidthEstimator
        - ui32MaxSamplesNumber max lenght of the internals lists
        - ui32TimeIntervalInMs duration of the time interval considered by the bandwidth samples
        - ui64MaxSamplingTime max time between two ack samples
        */
        WestwoodBandwidthEstimator (uint32 ui32MaxSamplesNumber, uint32 ui32TimeIntervalInMs, uint64 ui64SamplingTime, uint16 ui16InitialAssumedBandwidth);
        ~WestwoodBandwidthEstimator (void);

    protected:
        /**
        Computes a new bandwidth sample from the ACK samples over a time interval using the following calculation:
                                         currentAck.ui64AcknowledgedData
              bandwidthSample = ----------------------------------------------------
                                  currentAck.ui64Timestamp - lastAck.ui64Timestamp
        - pACKList: list of ACK samples
        @return: Bandwidth Sample
        */
        double computeNewBandwidthSample (DLList<ACKSample> *pACKList);

        /**
        Computes a new bandwidth estimation from the ACK samples over a time interval, the bandwidth
        samples and the previous estimations.
        - pACKList: list of ACK samples
        - pBWSamplesList: list of bandwidth samples
        - pBWEstimationsList: list of bandwidth estimations
        @return: New bandwidth estimation
        */
        double computeNewBandwidthEstimation (DLList<ACKSample> *pACKList, DLList<double> *pBWSamplesList, DLList<double> *pBWEstimationsList);
};

#endif   // #ifndef INCL_BANDWIDTH_ESTIMATOR_H
