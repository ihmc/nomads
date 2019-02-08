/*
 * TagGenerator.cpp
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
 */

#include "TagGenerator.h"

using namespace IHMC_ACI;

TagGenerator * TagGenerator::_pTagGenerator = nullptr;
const uint16 TagGenerator::_ui16DefaultTag = 0;
const uint16 TagGenerator::_ui16VersionTag = 1;

TagGenerator::TagGenerator()
    : _waypointTagByPeerId (true, // bCaseSensitiveKeys
                            true, // bCloneKeys
                            true, // bDeleteKeys
                            true) // bDeleteValues
{
    _ui16CurrTag = _ui16VersionTag + 1;
}

TagGenerator::~TagGenerator()
{
}

