/*
 * StateCookie.cpp
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

#include "StateCookie.h"

#include "EndianHelper.h"


using namespace NOMADSUtil;

StateCookie::StateCookie (int64 i64GenerationTime, int64 i64LifeSpan,
                          uint32 ui32ValidationA, uint32 ui32ValidationZ,
                          uint32 ui32ControlTSNA, uint32 ui32ControlTSNZ,
                          uint32 ui32ReliableSequencedTSNA, uint32 ui32ReliableSequencedTSNZ,
                          uint32 ui32UnreliableSequencedTSNA, uint32 ui32UnreliableSequencedTSNZ,
                          uint32 ui32ReliableUnsequencedIDA, uint32 ui32ReliableUnsequencedIDZ,
                          uint32 ui32UnreliableUnsequencedIDA, uint32 ui32UnreliableUnsequencedIDZ,
                          uint16 ui16PortA, uint16 ui16PortZ)
{
    _i64GenerationTime = i64GenerationTime;
    _i64LifeSpan = i64LifeSpan;
    _ui32ValidationA = ui32ValidationA;
    _ui32ValidationZ = ui32ValidationZ;
    _ui32ControlTSNA = ui32ControlTSNA;
    _ui32ControlTSNZ = ui32ControlTSNZ;
    _ui32ReliableSequencedTSNA = ui32ReliableSequencedTSNA;
    _ui32ReliableSequencedTSNZ = ui32ReliableSequencedTSNZ;
    _ui32UnreliableSequencedTSNA = ui32UnreliableSequencedTSNA;
    _ui32UnreliableSequencedTSNZ = ui32UnreliableSequencedTSNZ;
    _ui32ReliableUnsequencedIDA = ui32ReliableUnsequencedIDA;
    _ui32ReliableUnsequencedIDZ = ui32ReliableUnsequencedIDZ;
    _ui32UnreliableUnsequencedIDA = ui32UnreliableUnsequencedIDA;
    _ui32UnreliableUnsequencedIDZ = ui32UnreliableUnsequencedIDZ;
    _ui16PortA = ui16PortA;
    _ui16PortZ = ui16PortZ;
}

StateCookie::StateCookie (const char *pBuf)
{
    _i64GenerationTime = EndianHelper::ntoh64 (*((int64*)(pBuf + 0)));
    _i64LifeSpan = EndianHelper::ntoh64 (*((int64*)(pBuf + 8)));
    _ui32ValidationA = EndianHelper::ntohl (*((uint32*)(pBuf + 16)));
    _ui32ValidationZ = EndianHelper::ntohl (*((uint32*)(pBuf + 20)));
    _ui32ControlTSNA = EndianHelper::ntohl (*((uint32*)(pBuf + 24)));
    _ui32ControlTSNZ = EndianHelper::ntohl (*((uint32*)(pBuf + 28)));
    _ui32ReliableSequencedTSNA = EndianHelper::ntohl (*((uint32*)(pBuf + 32)));
    _ui32ReliableSequencedTSNZ = EndianHelper::ntohl (*((uint32*)(pBuf + 36)));
    _ui32UnreliableSequencedTSNA = EndianHelper::ntohl (*((uint32*)(pBuf + 40)));
    _ui32UnreliableSequencedTSNZ = EndianHelper::ntohl (*((uint32*)(pBuf + 44)));
    _ui32ReliableUnsequencedIDA = EndianHelper::ntohl (*((uint32*)(pBuf + 48)));
    _ui32ReliableUnsequencedIDZ = EndianHelper::ntohl (*((uint32*)(pBuf + 52)));
    _ui32UnreliableUnsequencedIDA = EndianHelper::ntohl (*((uint32*)(pBuf + 56)));
    _ui32UnreliableUnsequencedIDZ = EndianHelper::ntohl (*((uint32*)(pBuf + 60)));
    _ui16PortA = EndianHelper::ntohs (*((uint16*)(pBuf + 64)));
    _ui16PortZ = EndianHelper::ntohs (*((uint16*)(pBuf + 66)));
}

int StateCookie::freeze (ObjectFreezer &objectFreezer)
{
    objectFreezer.putInt64 (_i64GenerationTime);
    objectFreezer.putInt64 (_i64LifeSpan);
    objectFreezer.putUInt32 (_ui32ValidationA);
    objectFreezer.putUInt32 (_ui32ValidationZ);
    objectFreezer.putUInt32 (_ui32ControlTSNA);
    objectFreezer.putUInt32 (_ui32ControlTSNZ);
    objectFreezer.putUInt32 (_ui32ReliableSequencedTSNA);
    objectFreezer.putUInt32 (_ui32ReliableSequencedTSNZ);
    objectFreezer.putUInt32 (_ui32UnreliableSequencedTSNA);
    objectFreezer.putUInt32 (_ui32UnreliableSequencedTSNZ);
    objectFreezer.putUInt32 (_ui32ReliableUnsequencedIDA);
    objectFreezer.putUInt32 (_ui32ReliableUnsequencedIDZ);
    objectFreezer.putUInt32 (_ui32UnreliableUnsequencedIDA);
    objectFreezer.putUInt32 (_ui32UnreliableUnsequencedIDZ);
    objectFreezer.putUInt16 (_ui16PortA);
    objectFreezer.putUInt16 (_ui16PortZ);
    
    return 0;
}

int StateCookie::defrost (ObjectDefroster &objectDefroster)
{
    objectDefroster >> _i64GenerationTime;
    objectDefroster >> _i64LifeSpan;
    objectDefroster >> _ui32ValidationA;
    objectDefroster >> _ui32ValidationZ;
    objectDefroster >> _ui32ControlTSNA;
    objectDefroster >> _ui32ControlTSNZ;
    objectDefroster >> _ui32ReliableSequencedTSNA;
    objectDefroster >> _ui32ReliableSequencedTSNZ;
    objectDefroster >> _ui32UnreliableSequencedTSNA;
    objectDefroster >> _ui32UnreliableSequencedTSNZ;
    objectDefroster >> _ui32ReliableUnsequencedIDA;
    objectDefroster >> _ui32ReliableUnsequencedIDZ;
    objectDefroster >> _ui32UnreliableUnsequencedIDA;
    objectDefroster >> _ui32UnreliableUnsequencedIDZ;
    objectDefroster >> _ui16PortA;
    objectDefroster >> _ui16PortZ;
    
    return 0;
}

int StateCookie::write (char *pBuf)
{
    *((int64*)(pBuf + 0)) = EndianHelper::hton64 (_i64GenerationTime);
    *((int64*)(pBuf + 8)) = EndianHelper::hton64 (_i64LifeSpan);
    *((uint32*)(pBuf + 16)) = EndianHelper::htonl (_ui32ValidationA);
    *((uint32*)(pBuf + 20)) = EndianHelper::htonl (_ui32ValidationZ);
    *((uint32*)(pBuf + 24)) = EndianHelper::htonl (_ui32ControlTSNA);
    *((uint32*)(pBuf + 28)) = EndianHelper::htonl (_ui32ControlTSNZ);
    *((uint32*)(pBuf + 32)) = EndianHelper::htonl (_ui32ReliableSequencedTSNA);
    *((uint32*)(pBuf + 36)) = EndianHelper::htonl (_ui32ReliableSequencedTSNZ);
    *((uint32*)(pBuf + 40)) = EndianHelper::htonl (_ui32UnreliableSequencedTSNA);
    *((uint32*)(pBuf + 44)) = EndianHelper::htonl (_ui32UnreliableSequencedTSNZ);
    *((uint32*)(pBuf + 48)) = EndianHelper::htonl (_ui32ReliableUnsequencedIDA);
    *((uint32*)(pBuf + 52)) = EndianHelper::htonl (_ui32ReliableUnsequencedIDZ);
    *((uint32*)(pBuf + 56)) = EndianHelper::htonl (_ui32UnreliableUnsequencedIDA);
    *((uint32*)(pBuf + 60)) = EndianHelper::htonl (_ui32UnreliableUnsequencedIDZ);
    *((uint16*)(pBuf + 64)) = EndianHelper::htons (_ui16PortA);
    *((uint16*)(pBuf + 66)) = EndianHelper::htons (_ui16PortZ);
    return 0;
}
