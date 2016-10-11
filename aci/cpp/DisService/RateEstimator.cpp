/*
 * RateEstimator.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#include "RateEstimator.h"

#include "DisServiceMsg.h"
#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "NetUtils.h"
#include "TransmissionService.h"

#include "Logger.h"
#include "NLFLib.h"

using namespace NOMADSUtil;
using namespace IHMC_ACI;

/******************************
 *  NOTE: in this class 100% is represented by 10000, to reduce approximation errors
 *****************************/

RateEstimator::RateEstimator (TransmissionService *pTrSvc, uint8 ui8UpdateFactor,
                              uint8 ui8DecreaseFactor, uint32 ui32StartingCapacity)
    : _ui32IdleTime (DEFAULT_IDLE_TIME),
      _ui32SequenceNumber (0U),
      _ui32StartingCapacity (ui32StartingCapacity * 1024 / 8),
      _fUpdateFactor ((float) ui8UpdateFactor / 100),
      _fDecreaseFactor ((float) ui8DecreaseFactor / 100),
      _pTrSvc (pTrSvc),
      _m (24),
      _tInterfaces (true,  // bCaseSensitiveKeys
                    true,  // bCloneKeys
                    true,  // bDeleteKeys
                    false) // bDeleteValues
{
}

RateEstimator::~RateEstimator()
{
    _pTrSvc = NULL;

    for (StringHashtable<InterfaceStats>::Iterator iInt = _tInterfaces.getAllElements(); !iInt.end(); iInt.nextElement()){
        InterfaceStats *pInt = iInt.getValue();
        for(UInt32Hashtable<NeighbourStats>::Iterator iNeig = pInt->_tStats.getAllElements(); !iNeig.end(); iNeig.nextElement()){
            NeighbourStats *pNeig = iNeig.getValue();
            delete pNeig;
        }
        pInt->_tStats.removeAll();
        delete pInt;
    }
    _tInterfaces.removeAll();
}

int RateEstimator::init (void)
{
    // The assumption here is that the interfaces never change at runtime
    char **ppszInterfaces = _pTrSvc->getActiveInterfacesAddress();
    if (ppszInterfaces == NULL) {
        return -1;
    }

    for (int i = 0; ppszInterfaces[i]; i++) {
        InterfaceStats* iStats = new InterfaceStats();
        _tInterfaces.put (ppszInterfaces[i],iStats);
        iStats->_ui32NetworkCapacity = _ui32StartingCapacity;
        checkAndLogMsg ("RateControl::RateControl", Logger::L_LowDetailDebug,
                        "Interface %s has capacity %u\n",ppszInterfaces[i],
                        iStats->_ui32NetworkCapacity);
        free (ppszInterfaces[i]);
        ppszInterfaces[i] = NULL;
    }
    free (ppszInterfaces);
    ppszInterfaces = NULL;

    return 0;
}

void RateEstimator::newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                        DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                        const char *pszIncomingInterface)
{
    if (pDisServiceMsg->getType() == DisServiceMsg::DSMT_Data) {
        DisServiceDataMsg *pDSDMsg = (DisServiceDataMsg *) pDisServiceMsg;
        _m.lock (187);
        // Check if we are keeping stats for that interface
        InterfaceStats *pInterface = _tInterfaces.get (pszIncomingInterface);
        if (pInterface == NULL) {
            checkAndLogMsg ("RateControl::newIncomingMessage", Logger::L_Info,
                            "interface %s not present\n",pszIncomingInterface);
            _m.unlock (187);
            return;
        }

        //fetch the stats for the node that sent the message
        NeighbourStats *pNeighbour = pInterface->_tStats.get (ui32SourceIPAddress);
        if (pNeighbour == NULL) {
            pNeighbour = new NeighbourStats();
            pInterface->_tStats.put (ui32SourceIPAddress, pNeighbour);
        }
        if (pDSDMsg->hasRateEstimate()) {
            pNeighbour->_ui32EstimateRate = pDSDMsg->getRateEstimationInfo();
            pNeighbour->_ui8MissedUpdates = 0;
        }
        else if (pDSDMsg->hasSendRate()) {
            pNeighbour->_ui32SendRate = pDSDMsg->getRateEstimationInfo();
            pNeighbour->_ui8MissedUpdates = 0;
        }
        _m.unlock (187);
    }

}

void RateEstimator::run()
{
    setName ("RateEstimator::run");

    started();
    uint32 ui32TotalNeighbourSendRate;
    uint32 ui32MyDropPercentage;
    uint32 ui32MySendRate;
    uint32 ui32MyReceiveRate = 0;
    uint32 ui32MyEstimatedNetCapapacity;
    uint32 ui32MyDropRate;
    uint32 ui32OldDrop = 0;

    uint32 ui32OldCap = 0;
    int iTrend = 0;
    int iDropTrend = 0;

    while (!terminationRequested()) {
        sleepForMilliseconds (_ui32IdleTime);

        _ui32SequenceNumber++;
        for (StringHashtable<InterfaceStats>::Iterator iInterface = _tInterfaces.getAllElements(); !iInterface.end(); iInterface.nextElement()) {
            InterfaceStats *pInterface = iInterface.getValue();
            char *pInterfaceName = strDup (iInterface.getKey());

            /***********************
            ***  NEIGHBOUR STATS ***
            ************************/
            // count the send rate total of all the neighbours and their max drop rate
            ui32TotalNeighbourSendRate=0;
            uint32 ui32MinNeighbourRateEstimate=0;

            for (UInt32Hashtable<NeighbourStats>::Iterator iNeighbour = pInterface->_tStats.getAllElements(); !iNeighbour.end(); iNeighbour.nextElement()) {
                NeighbourStats *pNeighStats = iNeighbour.getValue();
                if (pNeighStats != NULL) {
                    // Update the total amount sent by neighbours
                    ui32TotalNeighbourSendRate+=pNeighStats->_ui32SendRate;
                    // Keep the minimum network estimate of the ones broadcasted by the neighbours,
                    // to avoid choking the ones that receive more traffic
                    if (ui32MinNeighbourRateEstimate==0){
                        ui32MinNeighbourRateEstimate=pNeighStats->_ui32EstimateRate;
                    }
                    else {
                        if (pNeighStats->_ui32EstimateRate > 0) {
                            ui32MinNeighbourRateEstimate = minimum (ui32MinNeighbourRateEstimate, pNeighStats->_ui32EstimateRate);
                        }
                    }

                    pNeighStats->_ui8MissedUpdates = pNeighStats->_ui8MissedUpdates +1;
                    //reset stats for neighbours that seem to have disappeared
                    if (pNeighStats->_ui8MissedUpdates > 3) {
                        pNeighStats->_ui32SendRate = 0;
                        pNeighStats->_ui32EstimateRate = 0;
                        pNeighStats->_ui8MissedUpdates = 0;
                        //lower the capacity, to recover from heavy collision

                        //NO NEED TO DO IT NOW. EVEN WITH HIGH DROP RATE, SINCE WE USE DATA MSGS, WE'LL STILL RECEIVE SOMETHING
                        //SO IF WE MISS UPDATES FOR 3 TIMES IN A ROW, IT MEANS THE NODE IS JUST DEAD
                        //pInterface->_ui32NetworkCapacity /=2;
                    }
                }
            }

            /****************
            ***  MY STATS ***
            *****************/
            //extract the send and receive rate of this node on the current interface
            if (ui32MyReceiveRate != 0) {
                ui32MyReceiveRate = (uint32) (0.8f * ui32MyReceiveRate + 0.2f * _pTrSvc->getReceiveRate (pInterfaceName));
            }
            else {
                //to initialize the rate
                ui32MyReceiveRate = _pTrSvc->getReceiveRate(pInterfaceName);
            }
            ui32MySendRate = (pInterface->_bActive) ? _pTrSvc->getTransmitRateLimit (pInterfaceName) : 0;
            pInterface->_bActive = false;

            ui32MyDropRate = (ui32TotalNeighbourSendRate > ui32MyReceiveRate ) ? (ui32TotalNeighbourSendRate - ui32MyReceiveRate) : 0;
            ui32MyDropPercentage = (ui32TotalNeighbourSendRate>0) ? 10000 * (ui32MyDropRate)/ui32TotalNeighbourSendRate : 0;

            //calculate the estimate of the network capacity
            ui32MyEstimatedNetCapapacity = estimateLinkCapacity (ui32MySendRate, ui32MyReceiveRate, ui32MyDropPercentage , pInterface->_ui32FixedDrop);

            /*
             * the following part recognizes the trends of drop rate and estimated capacity, and sues them to differentiate the behaviour
             *
             * if the capacity is increasing, reduce the fixed drop (to avoid increasing both and getting infinite speed). it's like a negative feedback control
             *
             * if the capacity is decreasing
             *      and the drop rate is increasing, probably the network capacity has decreased. decrese the fixed drop
             *      and the drop rate is not increasing, probably our estimate of the fixed drop is too low. update it
             */

            if (ui32MyDropPercentage < ui32OldDrop) {
                iDropTrend--;
                iDropTrend = maximum (iDropTrend, -2);
            }
            else {
                iDropTrend++;
                iDropTrend = minimum (iDropTrend, 2);
            }

            ui32OldDrop = ui32MyDropPercentage;

            // If the capacity seems to be decreasing, advertise the old one, to avoid
            // slowing down everyone because of a single unlucky case
            if (ui32MyEstimatedNetCapapacity < ui32OldCap) {
                pInterface->_ui32MyEstimatedNetworkCapacity = ui32OldCap;
                // if we're lowering the capacity, it's a good moment to update the fixed
                // drop value, since it's less probable that we incur in the continuous
                // increase of fixed (caused by congestion)
                iTrend--;
                if (iTrend <= -3) {
                    iTrend = -3;
                    if (iDropTrend < 2) {
                        // Drop is not increasing
                        pInterface->_ui32FixedDrop = (uint32)(pInterface->_ui32FixedDrop * (1 - _fUpdateFactor)
                                                   + ui32MyDropPercentage * _fUpdateFactor);
                        checkAndLogMsg ("RateControl::run", Logger::L_Warning, "Cap - Drop -\n");
                    }
                    else {
                        // Drop is definitely increasing
                        pInterface->_ui32FixedDrop = (uint32) (pInterface->_ui32FixedDrop * _fDecreaseFactor);
                        checkAndLogMsg ("RateControl::run", Logger::L_Warning, "Cap - Drop +\n");
                    }
                }
            }
            else {
                // if the capacity seems to the same, or better than the old one, advertise something
                // higher, to let others try to increase the speed a bit
                pInterface->_ui32MyEstimatedNetworkCapacity = (uint32)(ui32MyEstimatedNetCapapacity * 1.1f);

                iTrend++;
                // if the capacity has increased, it's a good time to reduce the fixed drop.
                // We're probably not in congestion
                if(iTrend >= 3){
                    iTrend = 3;
                    pInterface->_ui32FixedDrop = (uint32) (pInterface->_ui32FixedDrop * _fDecreaseFactor);
                    checkAndLogMsg ("RateControl::run", Logger::L_Warning, "Cap +\n");
                }
            }

            ui32OldCap = ui32MyEstimatedNetCapapacity;

            checkAndLogMsg ("RateControl::run", Logger::L_Warning, "Table,%u,%u,%u,%u,%u\n",
                            ui32TotalNeighbourSendRate, ui32MyReceiveRate, ui32MyEstimatedNetCapapacity,
                            ui32MyDropPercentage, pInterface->_ui32FixedDrop);

            /****************
            ***   ADJUST  ***
            *****************/
            //if the interface didn't send anything, don't adjust the capacity
            if(ui32MySendRate > 0){
                if(ui32MinNeighbourRateEstimate >= pInterface->_ui32NetworkCapacity){
                    //if the neighbours think the capacity is better than the value we're using, just follow their advice
                    pInterface->_ui32NetworkCapacity = ui32MinNeighbourRateEstimate;
                }
                else{
                    //if they suggest it's lower, decrease it
                    //don't decrease it more than half of its value
                    uint32 offset = (uint32) minimum (ui32MinNeighbourRateEstimate * 0.15f, pInterface->_ui32NetworkCapacity * 0.5f);
                    pInterface->_ui32NetworkCapacity = (uint32) (pInterface->_ui32NetworkCapacity * 0.85f + offset);
                    pInterface->_ui32NetworkCapacity = maximum (pInterface->_ui32NetworkCapacity, (uint32) 56*128);//don't go below 56Kbps
                }
            }

            _pTrSvc->setLinkCapacity (pInterfaceName, pInterface->_ui32NetworkCapacity);

            checkAndLogMsg ("RateControl::run", Logger::L_LowDetailDebug,
                            "Interface %s: Min Neighbor Estimate %u, Current Capacity %u\n",
                            pInterfaceName, ui32MinNeighbourRateEstimate,
                            pInterface->_ui32NetworkCapacity);
        }
    }
    terminating();
    checkAndLogMsg ("RateControl::run", Logger::L_LowDetailDebug, "Terminating\n");
}

uint32 RateEstimator::getNetworkCapacity (const char *pszInterface)
{
    InterfaceStats *pInterface;
    uint32 ui32Capacity;
    pInterface = _tInterfaces.get (pszInterface);
    if (pInterface){
        ui32Capacity = pInterface->_ui32NetworkCapacity;
        return ui32Capacity;
    }
    else{
        return 0;
    }
}

uint32 RateEstimator::getNetworkCapacityToAdvertise (const char *pszInterface)
{
    InterfaceStats *pInterface;
    uint32 ui32Capacity;
    pInterface = _tInterfaces.get (pszInterface);
    if (pInterface) {
        ui32Capacity = pInterface->_ui32MyEstimatedNetworkCapacity;
        return ui32Capacity;
    }
    else {
        return 0;
    }
}

void RateEstimator::setInterfaceIsActive (const char **ppszInterfaces)
{
    //
    if (ppszInterfaces != NULL) {
        for (int i = 0; ppszInterfaces[i]; i++) {
            InterfaceStats *pInterface = _tInterfaces.get(ppszInterfaces[i]);
            if (pInterface) {
                if (!pInterface->_bActive) {
                    pInterface->_bActive = true;
                }
            }
            else{
                checkAndLogMsg ("RateControl::setInterfaceIsActive", Logger::L_MildError,
                                "Interface %s not found\n", ppszInterfaces[i]);
            }
        }
    }
    //
}

uint32 RateEstimator::estimateLinkCapacity (uint32 ui32SendRate, uint32 ui32RecvRate, uint32 ui32DropRate, uint32 ui32FixedDrop)
{
    uint32 FixedDrop = ui32FixedDrop + 100;//assume a fixed drop of 1%, to avoid thinking that there is drop when there is only a precision problem in the calculation
    //calculate the drop due to congestion
    uint32 ui32CongestionDrop = (ui32DropRate > FixedDrop) ? ui32DropRate - FixedDrop : 0;
    uint32 ui32Ris = ui32RecvRate * (10000 - ui32CongestionDrop) / (10000 - minimum((uint32) 9900,ui32DropRate));
    ui32Ris += ui32SendRate * (10000 - ui32CongestionDrop) / 10000;

    return ui32Ris;
}

RateEstimator::NeighbourStats::NeighbourStats()
{
    _ui32SendRate = 0;
    _ui32EstimateRate = 0;
    _ui8MissedUpdates = 0;

}

RateEstimator::NeighbourStats::~NeighbourStats()
{
}

RateEstimator::InterfaceStats::InterfaceStats()
    : _tStats (4, true)
{
    _bActive = false;
    _ui32FixedDrop = 0;
    _ui32NetworkCapacity = 0;
    _ui32MyEstimatedNetworkCapacity = 0;
}

RateEstimator::InterfaceStats::~InterfaceStats()
{
}
