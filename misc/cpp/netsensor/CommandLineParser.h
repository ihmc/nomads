#ifndef NETSENSOR_CommandLineParser__INCLUDED
#define NETSENSOR_CommandLineParser__INCLUDED
/*
* CommandLineParser.h
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
*
* Netsensor class that handles the parsing of command line arguments
*/
#include "NetSensorConstants.h"
#include "ParameterDictionary.h"
#include "CommandLineConfigs.h"
#include "Logger.h"

namespace IHMC_NETSENSOR
{
class CommandLineParser
{
public:
    CommandLineParser       (CommandLineConfigs *pCLC);
    ~CommandLineParser      (void);
    bool hasError           (void);
    bool hasHelp            (void);
    void parseArgs          (const int argCount, const char *args[]);
    void printError         (void);
    void printParamMeanings (void);
    //<----------------------------------------------------->
private:
    bool _bHasError;
    bool _bHasHelp;
    ParameterDictionary *_pParamDictionary;
    CommandLineConfigs  *_pCommandLineConfigs;
};
}
#endif
