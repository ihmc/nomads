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
#include"CommandLineParser.h"

using namespace NOMADSUtil;
#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_NETSENSOR
{

CommandLineParser::CommandLineParser(CommandLineConfigs *pCLC)
{
    _pParamDictionary    = new ParameterDictionary();
    _pCommandLineConfigs = pCLC;
    _bHasError = false;
    _bHasHelp = false;
}

CommandLineParser::~CommandLineParser()
{
    delete _pParamDictionary;
    _pParamDictionary    = nullptr;
}

void CommandLineParser::parseArgs(const int argCount, const char *args[])
{
    static const char *pszMethodName = "parseArgs";
    if (_pCommandLineConfigs == nullptr)
    {
        checkAndLogMsg(pszMethodName, Logger::L_SevereError,
            "No config object set!\n");
        return;
    }

    // Start at 1 to skip over the first .exe param
    for (int wordCounter = 1; wordCounter < argCount; wordCounter++)
    {
        String sParam = args[wordCounter];
        uint8 ui8Param = _pParamDictionary->getIntParam(sParam);

        switch (ui8Param)
        {
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
            if (wordCounter + 1 != argCount) {
                wordCounter++;
                if (args[wordCounter][0] == '-') {
                    checkAndLogMsg(pszMethodName, Logger::L_SevereError,
                        "Please specify a interfaces after the command "
                        " \"%s\"; exiting now\n\n", C_SPECIFY_INTERFACE);
                    return;
                }

                _pCommandLineConfigs->bHasInterfaces = true;

                int tempCounter = wordCounter;
                while ((tempCounter < argCount) && (args[tempCounter][0] != '-')) {
                    _pCommandLineConfigs->interfaceNameList.add(args[tempCounter++]);
                }
            }
            else {
                checkAndLogMsg(pszMethodName, Logger::L_SevereError,
                    "Please specify an interface after the command"
                    " \"%s\"; exiting now\n\n", C_SPECIFY_INTERFACE);
                _bHasError = true;
                return;
            }
            break;
        case (C_INT_SPECIFY_CONFIG_PATH):
            if (wordCounter + 1 != argCount) {
                wordCounter++;
                if (args[wordCounter][0] == '-')
                {
                    checkAndLogMsg(pszMethodName, Logger::L_SevereError,
                        "Please specify a correct path after the command "
                        " \"%s\"; exiting now\n", C_SPECIFIY_CONFIG_PATH);
                    _bHasError = true;
                    return;
                }
                _pCommandLineConfigs->sConfigPath = args[wordCounter];
            }
            else{
                checkAndLogMsg(pszMethodName, Logger::L_SevereError,
                    "Please specify a path after the command "
                    " \"%s\"; exiting now\n\n", C_SPECIFIY_CONFIG_PATH);
                _bHasError = true;
                return;
            }
            break;
        case (C_INT_SPECIFY_NS_RECIPIENT):
            if (wordCounter + 1 != argCount) {
                wordCounter++;
                if (args[wordCounter][0] == '-')
                {
                    checkAndLogMsg(pszMethodName, Logger::L_SevereError,
                        "Please specify a recipient that starts without a "
                        " \'-\' after the command "
                        " \"%s\"; exiting now\n\n", C_SPECIFY_NS_RECIPIENT);
                    _bHasError = true;
                    return;
                }
                _pCommandLineConfigs->sRecipient = args[wordCounter];
            }
            else {
                checkAndLogMsg(pszMethodName, Logger::L_SevereError,
                    "Please specify a recipient after the command "
                    " \"%s\"; exiting now\n\n", C_SPECIFY_NS_RECIPIENT);
                _bHasError = true;
                return;
            }
            break;
        case (C_INT_REPLAY_MODE):
            if (wordCounter + 1 != argCount) {
                wordCounter++;
                if (args[wordCounter][0] == '-')
                {
                    checkAndLogMsg(pszMethodName, Logger::L_SevereError,
                                   "Please specify a correct path after the command "
                                   " \"%s\"; exiting now\n", C_REPLAY_MODE);
                    _bHasError = true;
                    return;
                }
                _pCommandLineConfigs->sReplayConfigPath = args[wordCounter];
                _pCommandLineConfigs->em = EmbeddingMode::EM_REPLAY;
            }
            else{
                checkAndLogMsg(pszMethodName, Logger::L_SevereError,
                               "Please specify a path after the command "
                               " \"%s\"; exiting now\n\n", C_REPLAY_MODE);
                _bHasError = true;
                return;
            }
            break;
        case (C_INT_SHOW_HELP):
            _bHasHelp = true;
            break;
        case (C_INT_PARAM_UNKNOWN):
            const char *pszMessage = "Unknown command line option \"%s\"; exiting now\n\n";
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, pszMessage, sParam.c_str());
            _bHasError = true;
            return;
        }
    }
    return;
}

void CommandLineParser::printError()
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