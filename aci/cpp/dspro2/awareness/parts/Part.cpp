/**
 * Part.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on January 24, 2016, 8:23 PM
 */

#include "Part.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

Part::Part (void)
    : _ui16Version (0)
{
}

Part::~Part (void)
{
}

uint16 Part::getVersion (void) const
{
    return _ui16Version;
}

void Part::incrementVersion (void)
{
    _ui16Version++;
}

void Part::setVersion (uint16 ui16NewVersion)
{
    _ui16Version = ui16NewVersion;
}

