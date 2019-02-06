/*
* CommandLineParser.cpp
* Author: bordway@ihmc.us rfronteddu@ihmc.us
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
#include "CommandLineParser.h"

using namespace NOMADSUtil;
#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_NETSENSOR
{
CommandLineParser::CommandLineParser (CommandLineConfigs * pCLC)
{
    _pParamDictionary    = new ParameterDictionary();
    _pCommandLineConfigs = pCLC;
    _bHasError           = false;
    _bHasHelp            = false;
}

CommandLineParser::~CommandLineParser (void)
{
    delete _pParamDictionary;
    _pParamDictionary = nullptr;
}

bool CommandLineParser::checkForError (
    const char * args[], 
    const int * pWordCounter, 
    const char * pszMsg, 
    const char * const id)
{
    auto pszMethodName = "CommandLineParser::checkForError";
    if (args[*pWordCounter][0] == '-') {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "%s %s; exiting now\n\n", pszMsg, id);
        return true;
    }
    return false;
}

void CommandLineParser::manageInterfaces (int* pWordCounter, const char* args[], const int argCount)
{
    auto pszMethodName = "CommandLineParser::manageInterfaces";

	printf("TEST2: %s, %d, %d\n", args[*pWordCounter + 2], *pWordCounter, argCount);

    if ((*pWordCounter) + 1 != argCount) {
        (*pWordCounter)++;
        auto pszMsg = "Please specify a interfaces after the command ";
        if (checkForError (args, pWordCounter, pszMsg, C_SPECIFY_INTERFACE)) {
            return;
        }
        _pCommandLineConfigs->bHasInterfaces = true;
        /*
		int tempCounter = *pWordCounter;
        while ((tempCounter < argCount) && (args[tempCounter][0] != '-')) {
            _pCommandLineConfigs->interfaceNameList.add (args[tempCounter++]);
        }
		*/
		int counter = *pWordCounter;
		while ((counter < argCount) && (args[counter][0] != '-')) {
			_pCommandLineConfigs->interfaceNameList.add(args[counter++]);
		}
		*pWordCounter = (counter - 1);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, 
			"Please specify an interface after the command %s; exiting now\n\n", 
            C_SPECIFY_INTERFACE);
        _bHasError = true;
        return;
    }
}

void CommandLineParser::manageForcedInterfaces (int* pWordCounter, const char * args[], const int argCount)
{
    auto pszMethodName = "CommandLineParser::manageForcedInterfaces";
    checkAndLogMsg (pszMethodName, Logger::L_Warning, "THIS MODE IS NOT COMPATIBLE WITH OTHER OVERRIDES, INTERFACES NEED TO HAVE DIFFERENT NAMES\n");

    if ((*pWordCounter) + 1 != argCount) {
        (*pWordCounter)++;
        auto pszMsg = "Please specify an interface after the command ";
        if (checkForError (args, pWordCounter, pszMsg, C_FORCED_INTERFACES)) {
            return;
        }
        _pCommandLineConfigs->bHasInterfaces = true;
        
		int counter = *pWordCounter;
		while ((counter < argCount) && (args[counter][0] != '-')) {
			_pCommandLineConfigs->interfaceNameList.add(args[counter++]);
		}
		*pWordCounter = (counter - 1);
		_pCommandLineConfigs->bHasForcedInterfaces = true;
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Please specify an interface after the command %s; exiting now\n", C_FORCED_INTERFACES);
        _bHasError = true;
        return;
    }
}

void CommandLineParser::manageConfigPath (int * pWordCounter, const char * args[], const int argCount)
{
    auto pszMethodName = "CommandLineParser::manageConfigPath";
    if ((*pWordCounter) + 1 != argCount) {
        (*pWordCounter)++;
        auto pszMsg = "Please specify a correct path after the command ";
        if (checkForError (args, pWordCounter, pszMsg, C_SPECIFIY_CONFIG_PATH)) {
            return;
        }
        _pCommandLineConfigs->sConfigPath = args[*pWordCounter];
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Please specify a path after the command " 
            " %s; exiting now\n\n", C_SPECIFIY_CONFIG_PATH);
        _bHasError = true;
        return;
    }
}

void CommandLineParser::manageNSRecipients (int * pWordCounter, const char * args[], const int argCount)
{
    auto pszMethodName = "CommandLineParser::manageNSRecipients";
    if ((*pWordCounter) + 1 != argCount) {
        (*pWordCounter)++;
        auto pszMsg = "Please specify a recipient that starts without a \'-\' after the command ";
        if (checkForError (args, pWordCounter, pszMsg, C_SPECIFY_NS_RECIPIENT)) {
            return;
        }
        _pCommandLineConfigs->sRecipient = args[*pWordCounter];
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Please specify a recipient after the command "
            "%s; exiting now\n\n", C_SPECIFY_NS_RECIPIENT);
        _bHasError = true;
        return;
    }
}

void CommandLineParser::manageReplayMode (int * pWordCounter, const char * args[], const int argCount)
{
    auto pszMethodName = "CommandLineParser::manageReplayMode";
    if ((*pWordCounter) + 1 != argCount) {
        (*pWordCounter)++;

        auto pszMsg = "Please specify a correct path after the command ";
        if (checkForError(args, pWordCounter, pszMsg, C_REPLAY_MODE)) {
            return;
        }
        _pCommandLineConfigs->sReplayConfigPath = args[*pWordCounter];
        _pCommandLineConfigs->em = Mode::EM_REPLAY;
    }
    else {
        checkAndLogMsg(pszMethodName, Logger::L_SevereError, "Please specify a path after the command "
            "%s; exiting now\n\n", C_REPLAY_MODE);
        _bHasError = true;
        return;
    }
}

void CommandLineParser::manageDeliveryTime (int * pWordCounter, const char * args[], const int argCount)
{
    if ((*pWordCounter) + 1 != argCount) {
        (*pWordCounter)++;

        auto pszMsg = "Please specify a correct value after the command ";
        if (checkForError (args, pWordCounter, pszMsg, C_DELIVERY_TIME)) {
            return;
        }

        std::string::size_type sz;
        _pCommandLineConfigs->ui32msStatsDeliveryTime = std::stoi (args[*pWordCounter], &sz);
        _pCommandLineConfigs->bHasDifferentDeliveryTime = true;
    }
}

void CommandLineParser::manageForcedAddress (int * pWordCounter, const char * args[], const int argCount)
{
    if ((*pWordCounter) + 1 != argCount) {
        (*pWordCounter)++;

        auto pszMsg = "Please specify a correct value after the command ";
        if (checkForError (args, pWordCounter, pszMsg, C_FORCED_ADRR)) {
            return;
        }
        _pCommandLineConfigs->ui32ForcedInterfaceAddr = InetAddr (args[*pWordCounter]).getIPAddress();
        _pCommandLineConfigs->bHasForcedAddr = true;
    }
}

void CommandLineParser::manageForcedNetmask (int * pWordCounter, const char * args[], const int argCount)
{
    if ((*pWordCounter) + 1 != argCount) {
        (*pWordCounter)++;

        auto pszMsg = "Please specify a correct value after the command ";
        if (checkForError (args, pWordCounter, pszMsg, C_FORCED_NMSK)) {
            return;
        }

        _pCommandLineConfigs->ui32ForcedNetmask = InetAddr (args[*pWordCounter]).getIPAddress();
        _pCommandLineConfigs->bHasForcedNetmask = true;
    }
}

void CommandLineParser::manageEnableIW (int * pWordCounter, const char * args[], const int argCount)
{
    if ((*pWordCounter) + 1 != argCount) {
        (*pWordCounter)++;

        auto pszMsg = "Please specify a correct value after the command ";
        if (checkForError (args, pWordCounter, pszMsg, C_ENABLE_IW)) {
            return;
        }

        _pCommandLineConfigs->sIWIface = args[*pWordCounter];
        _pCommandLineConfigs->bEnableIW = true;
    }
}

void CommandLineParser::manageUnknown (const char * pszParam)
{
    auto pszMethodName = "CommandLineParser::manageUnknown";
    checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Unknown command line option %s; exiting now\n", pszParam);
    _bHasError = true;
}

void CommandLineParser::parseArgs (const int argCount, const char * args[])
{
    auto pszMethodName = "CommandLineParser::parseArgs";
    if (_pCommandLineConfigs == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "No config object set!\n");
        return;
    }

    // Start at 1 to skip over the first .exe param
    for (int wordCounter = 1; wordCounter < argCount; wordCounter++) {
        String sParam = args[wordCounter];
        uint8 ui8Param = _pParamDictionary->getIntParam (sParam);
		
        switch (ui8Param) {
        case (C_INT_USE_COMPRESSION):
            _pCommandLineConfigs->bUseCompression = true;
            break;

        case (C_INT_STORE_EXTERNALS_TOPOLOGY):
            _pCommandLineConfigs->bUseExternalTopology = true;
            break;

        case (C_INT_CALCULATE_TCP_RTT):
            _pCommandLineConfigs->bCalculateTCPRTT = true;
            break;

        case (C_INT_SPECIFY_INTERFACE):
            manageInterfaces (&wordCounter, args, argCount);
            break;

        case (C_INT_SPECIFY_CONFIG_PATH):
            manageConfigPath (&wordCounter, args, argCount);
            break;

        case (C_INT_SPECIFY_NS_RECIPIENT):
            manageNSRecipients (&wordCounter, args, argCount);
            break;

        case (C_INT_REPLAY_MODE):
            manageReplayMode (&wordCounter, args, argCount);
            break;

        case (C_INT_SHOW_HELP):
            _bHasHelp = true;
            break;

        case (C_INT_DELIVERY_TIME):
            manageDeliveryTime (&wordCounter, args, argCount);
            break;

        case (C_INT_FORCED_ADRR): 
            manageForcedAddress (&wordCounter, args, argCount);
            break;

        case (C_INT_FORCED_NMSK):
            manageForcedNetmask (&wordCounter, args, argCount);
            break;

        case (C_INT_FORCED_INTERFACES):
            manageForcedInterfaces (&wordCounter, args, argCount);
            break;

        case (C_INT_ENABLE_IW):
            manageEnableIW (&wordCounter, args, argCount);
            break;

        case (C_INT_PARAM_UNKNOWN):
            manageUnknown (sParam);
            return;
        }
    }
    return;
}

void CommandLineParser::printError (void)
{
    printf("Type -h to see available parameter options\n");
}

void CommandLineParser::printParamMeanings()
{
    _pParamDictionary->printMeanings();
}

bool CommandLineParser::hasError(void)
{
    return _bHasError;
}

bool CommandLineParser::hasHelp(void)
{
    return _bHasHelp;
}

}