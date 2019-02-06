/*
* NetSensor.cpp
* Author: rfronteddu@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
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
#define checkAndLogMsg if (pLogger) pLogger->logMsg

#include "Netsensor2.h"
#include "CommandLineParser.h"
#include "InterfaceDetector.h"
#include "Logger.h"
#include "ManageableThread.h"
#include "NetSensorConfigurationManager.h"
#include "NetsensorStatus.h"

namespace IHMC_NETSENSOR
{
    Netsensor2::Netsensor2(void) {

    }

    Netsensor2::~Netsensor2(void) {

    }

    void Netsensor2::run (void)
    {

    }

    bool Netsensor2::init (int argc, char * argv[]) {
        CommandLineConfigs clc;
        CommandLineParser clParser (&clc);

        if (clParser.hasHelp()) {
            clParser.printParamMeanings();
            return false;
        }
        else if (clParser.hasError()) {
            clParser.printError();
            return -2;
        } 
        return initWithCommandLineCfgs (&clc);
    }

    bool Netsensor2::initWithCommandLineCfgs (CommandLineConfigs * pCLC) {
        if (pCLC->sConfigPath == nullptr) {
            _ns.bUseProtobufCompression = pCLC->bUseCompression;
            _ns.bExternalNodesDetection = pCLC->bUseExternalTopology;
            _ns.bCalculateTCPrtt = pCLC->bCalculateTCPRTT;
            _ns.ui32DeliveryPeriodMs = 1000;
            _ns.bAutomaticInterfaceDetection = !pCLC->bHasInterfaces;
            if (pCLC->bHasInterfaces) {
                NOMADSUtil::String interf;
                while (pCLC->interfaceNameList.getNext (interf)) {
                    _ns.interfacesList.add(interf);
                }
            }
            else {
                InterfaceDetector id;
                id.init();
                char * pNextInterface = nullptr;
                while (id.getNext (&pNextInterface)) {
                    _ns.interfacesList.add (pNextInterface);
                }
            }
            _ns.sRecipient = pCLC->sRecipient;
        }
        else {
            // init with cfg file
        }
        return init();
    }

    bool Netsensor2::init (void) {
        if (_ns.bUseProtobufCompression) {
            _pms.configureToUseCompression();
        }
        
        _ns.bExternalNodesDetection;


        return true;
    }
}