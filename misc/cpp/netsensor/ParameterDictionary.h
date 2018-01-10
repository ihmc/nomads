#ifndef NETSENSOR_ParameterDictionary_INCLUDED
#define NETSENSOR_ParameterDictionary_INCLUDED
/*
* ParameterDictionary.h
* Author: bordway@ihmc.us
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
* Holds types of parameter arguments that can be passed to NetSensor via
* command line
*/
#include "NetSensorConstants.h"
#include "StrClass.h"
#include "StringHashtable.h"

namespace IHMC_NETSENSOR
{
    class ParameterDictionary
    {
    public:
        ParameterDictionary(void);
        ~ParameterDictionary(void);
        uint8 getIntParam(NOMADSUtil::String sParam);
        void printMeanings(void);

    private:
        NOMADSUtil::StringHashtable<const uint8> _stringToEnum;
    };
}
#endif
