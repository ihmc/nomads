/*
* FINHandshake.cpp
* Author: bordway@ihmc.us rfronteddu@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2018 IHMC.
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
#include "FINHandshake.h"
using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
    FINHandshake::FINHandshake(void)
    {
        _bStepOneInitiated = false;
        _bStepOneComplete = false;
        _bStepTwoInitiated = false;
        _bStepTwoComplete = false;
    }

    bool FINHandshake::isInitiated(void)
    {
        return _bStepOneInitiated;
    }

    bool FINHandshake::isComplete(void)
    {
        return _bStepTwoComplete;
    }

    void FINHandshake::offerFlag(uint8 ui8Flag)
    {
        if (!_bStepOneInitiated) {
            _bStepOneInitiated = true;
            return;
        }
        else if (!_bStepOneComplete) {
            if (hasACKFlag(ui8Flag)) {
                _bStepOneComplete = true;
            }
            else {
                _bStepOneInitiated = false;
            }
        }

        else if (!_bStepTwoInitiated) {
            _bStepTwoInitiated = true;
        }

        else if (!_bStepTwoComplete) {
            if (hasACKFlag(ui8Flag)) {
                _bStepTwoComplete = true;
            }
            else {
                _bStepOneComplete = false;
                _bStepTwoComplete = false;
                _bStepOneInitiated = false;
            }
        }
    }
}