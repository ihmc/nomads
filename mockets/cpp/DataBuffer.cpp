/*
 * DataBuffer.cpp
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

#include "DataBuffer.h"

#include "PacketProcessor.h"


DataBuffer::~DataBuffer (void)
{
    if (_pPacket) {
        delete _pPacket;
        _pPacket = nullptr;
    }
    else if (_pFragments) {
        Packet *pPacket;
        _pFragments->resetGet();
        while (_pFragments->getNext (pPacket)) {
            delete pPacket;
        }
        delete _pFragments;
        _pFragments = nullptr;
    }
}
