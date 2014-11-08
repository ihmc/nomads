/*
 * =====================================================================================
 *
 *       Filename:  RateBandwidthEstimator.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/05/2009 06:21:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Nino Burini (Research Associate), 
 *        Company:  IHMC, Universita' degli studi di Ferrara
 *
 * =====================================================================================
 */
#include "ACKBasedBandwidthEstimator.h"
#include <fstream>
#include <sstream>

using namespace NOMADSUtil;

class RateBandwidthEstimator : public ACKBasedBandwidthEstimator
{
    private:
        int64 _i64TimeSlidingWindowInMicroseconds;
        double computeNewBandwidthSample( DLList<ACKSample> *ackList );
        double computeNewBandwidthEstimation( DLList<ACKSample> *ackList, DLList<double> *bwSamplesList, DLList<double> *bwEstimationsList );

    public:
        RateBandwidthEstimator ( uint32 ui32MaxSamplesNumber, int64 i64TimeSlidingWindowInMicroSecs );
        void addSample ( int64 i64Timestamp, int64 i64AcknowledgedData );
        void printList ( void );
        double getBandwidthEstimation (void);
};

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  RateBandwidthEstimator
 *  Description:  Constructor for RateBandwidthEstimator.
 * =====================================================================================
 */
RateBandwidthEstimator::RateBandwidthEstimator ( uint32 ui32MaxSamplesNumber, int64 i64TimeSlidingWindowInMicroSecs ) : ACKBasedBandwidthEstimator( ui32MaxSamplesNumber )
{
    _i64TimeSlidingWindowInMicroseconds = i64TimeSlidingWindowInMicroSecs;
    _bandwidthSamplesList = new DLList<double> ( ui32MaxSamplesNumber );
    _bandwidthEstimationsList = new DLList<double> ( ui32MaxSamplesNumber );
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  addSample
 *  Description:  Adds an ACK sample to the internal lists and accordingly updates
 *                the bandwidth samples and estimations.
 * =====================================================================================
 */
void RateBandwidthEstimator::addSample ( int64 i64ACKSampleTime, int64 i64AcknowledgedData )
{
    ACKSample *ackSample;
    ackSample = new ACKSample;
    ackSample->i64Timestamp = i64ACKSampleTime; 
    ackSample->i64AcknowledgedData =i64AcknowledgedData; 

    _ackSamplesList->pushTail( *ackSample );

    // If this is the first sample that is inserted then
    // bandwidth samples and estimates need to be initialized 
    if( _ackSamplesList->getNumNodes() == 1 ) {
        double bandwidthSample = computeNewBandwidthSample( _ackSamplesList ); 

        _bandwidthSamplesList->pushTail( bandwidthSample );
        _bandwidthEstimationsList->pushTail( bandwidthSample );
        return;
    }
    // else insert the new sample and update the bandwidth samples and estimations list
    else {
        double bandwidthSample = computeNewBandwidthSample( _ackSamplesList ); 
        _bandwidthSamplesList->pushTail( bandwidthSample );        

        double bandwidthEstimation = computeNewBandwidthEstimation( _ackSamplesList, _bandwidthSamplesList, _bandwidthEstimationsList ); 
        _bandwidthEstimationsList->pushTail( bandwidthEstimation );        

        return;
    }
}

double RateBandwidthEstimator::computeNewBandwidthSample( DLList<ACKSample> *ackList ) {
    //
    //                        SUM( AcknowledgedData(j) ), [ACKs j received in Time Sliding Window] 
    // BandwidthSample(k) = ------------------------------------------------------------------------
    //                                        TimeSlidingWindowInMicroseconds
    //
    int64 i64ackDataSum = 0;
    ACKSample tempAck, lastAck;
    ackList->resetToTail();
    ackList->getPrev( lastAck );
    while( ackList->getPrev( tempAck ) ) {
        if( tempAck.i64Timestamp > lastAck.i64Timestamp - _i64TimeSlidingWindowInMicroseconds )
            i64ackDataSum += tempAck.i64AcknowledgedData;
        else break;
    }

    i64ackDataSum += lastAck.i64AcknowledgedData;
    double bandwidthSample = i64ackDataSum / (double)_i64TimeSlidingWindowInMicroseconds;
    return bandwidthSample;
}

    
double RateBandwidthEstimator::computeNewBandwidthEstimation( DLList<ACKSample> *ackList, 
                                                              DLList<double> *bwSamplesList, 
                                                              DLList<double> *bwEstimationsList ) {
    //
    //                           19                                   1
    // BandwidthEstimation(k) = ---- * BandwidthEstimation(k-1)  +  ---- * ( BandwidthSample(k) + BandwidthSample(k-1) )
    //                           21                                  21
    //
    bwSamplesList->resetToTail();
    double currentBandwidthSample, lastBandwidthSample;
    bwSamplesList->getPrev( currentBandwidthSample );
    bwSamplesList->getPrev( lastBandwidthSample );

    bwEstimationsList->resetToTail();
    double lastBandwidthEstimation;
    bwEstimationsList->getPrev( lastBandwidthEstimation );

    double bandwidthEstimation = (19/21.0) * lastBandwidthEstimation + (1/21.0) * ( currentBandwidthSample + lastBandwidthSample );
    return bandwidthEstimation;
}

double RateBandwidthEstimator::getBandwidthEstimation()
{
    double lastBandwidthEstimation;
    _bandwidthEstimationsList->getLast( lastBandwidthEstimation );
    return lastBandwidthEstimation;
}

void RateBandwidthEstimator::printList ( void )
{
    ACKSample ackSample;
    double dBwSample, dBwEstimation;

    std::cout << "ACK time (us)  \t ACK Data (bytes) \t Bandwidth Sample (MB/s) \t Bandwidth Estimation (MB/s)" << std::endl;
    std::cout << "----------------------------------------------------------------------------------------------------" << std::endl;

    _ackSamplesList->resetToHead();
    _bandwidthSamplesList->resetToHead();
    _bandwidthEstimationsList->resetToHead();

    while ( _ackSamplesList->getNext( ackSample ) ) {
        _bandwidthSamplesList->getNext( dBwSample );
        _bandwidthEstimationsList->getNext( dBwEstimation );

        std::cout << ackSample.i64Timestamp << " \t " << ackSample.i64AcknowledgedData << " \t\t\t " << dBwSample << " \t\t\t " << dBwEstimation << std::endl;
    }
}




int main ( int argc, char *argv[] )
{
    RateBandwidthEstimator rate(0, 5000);

    std::ifstream inputFile( argv[1] );
    std::string line, temp;
    int64 i64time, i64data;
    std::stringstream ss;
    
    if (inputFile.is_open())
    {
        getline (inputFile,line);
        while ( !inputFile.eof() )
        {
            ss.str("");
            ss.clear();
            temp = line.substr(0,11);
            ss << temp;
            ss >> i64time;
            ss.str("");
            ss.clear();
            temp = line.substr(12);
            ss << temp;
            ss >> i64data;
            rate.addSample(i64time, i64data);
            
            getline (inputFile,line);
        }
        inputFile.close();
    }
    else std::cout << "Unable to open file" << std::endl;

    rate.printList();
}

