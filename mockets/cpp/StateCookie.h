#ifndef INCL_STATE_COOKIE_H
#define INCL_STATE_COOKIE_H

/*
 * StateCookie.h
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

#include "FTypes.h"

#include "ObjectDefroster.h"
#include "ObjectFreezer.h"


class StateCookie
{
    public:
        StateCookie (void);
        StateCookie (int64 i64GenerationTime, int64 i64LifeSpan,
                     uint32 ui32ValidationA, uint32 ui32ValidationZ,
                     uint32 ui32ControlTSNA, uint32 ui32ControlTSNZ,
                     uint32 ui32ReliableSequencedTSNA, uint32 ui32ReliableSequencedTSNZ,
                     uint32 ui32UnreliableSequencedTSNA, uint32 ui32UnreliableSequencedTSNZ,
                     uint32 ui32ReliableUnsequencedIDA, uint32 ui32ReliableUnsequencedIDZ,
                     uint32 ui32UnreliableUnsequencedIDA, uint32 ui32UnreliableUnsequencedIDZ,
                     uint16 ui16PortA, uint16 ui16PortZ);
        int64 getGenerationTime (void);
        int64 getLifeSpan (void);
        uint32 getValidationA (void);
        uint32 getValidationZ (void);
        uint32 getControlTSNA (void);
        uint32 getControlTSNZ (void);
        uint32 getReliableSequencedTSNA (void);
        uint32 getReliableSequencedTSNZ (void);
        uint32 getUnreliableSequencedTSNA (void);
        uint32 getUnreliableSequencedTSNZ (void);
        uint32 getReliableUnsequencedIDA (void);
        uint32 getReliableUnsequencedIDZ (void);
        uint32 getUnreliableUnsequencedIDA (void);
        uint32 getUnreliableUnsequencedIDZ (void);
        uint16 getPortA (void);
        uint16 getPortZ (void);

    private:
        friend class Mocket;
        friend class Packet;
        friend class InitAckChunkAccessor;
        friend class CookieEchoChunkAccessor;
        friend class SimpleConnectAckChunkAccessor;
        StateCookie (const char *pBuf);
        int write (char *pBuf);

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

        static const uint16 STATE_COOKIE_SIZE = 68;

    private:
        int64 _i64GenerationTime;
        int64 _i64LifeSpan;
        uint32 _ui32ValidationA;
        uint32 _ui32ValidationZ;
        uint32 _ui32ControlTSNA;
        uint32 _ui32ControlTSNZ;
        uint32 _ui32ReliableSequencedTSNA;
        uint32 _ui32ReliableSequencedTSNZ;
        uint32 _ui32UnreliableSequencedTSNA;
        uint32 _ui32UnreliableSequencedTSNZ;
        uint32 _ui32ReliableUnsequencedIDA;
        uint32 _ui32ReliableUnsequencedIDZ;
        uint32 _ui32UnreliableUnsequencedIDA;
        uint32 _ui32UnreliableUnsequencedIDZ;
        uint16 _ui16PortA;
        uint16 _ui16PortZ;
};

inline StateCookie::StateCookie (void)
{
    _i64GenerationTime = 0;
    _i64LifeSpan = 0;
    _ui32ValidationA = 0;
    _ui32ValidationZ = 0;
    _ui32ControlTSNA = 0;
    _ui32ControlTSNZ = 0;
    _ui32ReliableSequencedTSNA = 0;
    _ui32ReliableSequencedTSNZ = 0;
    _ui32UnreliableSequencedTSNA = 0;
    _ui32UnreliableSequencedTSNZ = 0;
    _ui32ReliableUnsequencedIDA = 0;
    _ui32ReliableUnsequencedIDZ = 0;
    _ui32UnreliableUnsequencedIDA = 0;
    _ui32UnreliableUnsequencedIDZ = 0;
    _ui16PortA = 0;
    _ui16PortZ = 0;
}

inline int64 StateCookie::getGenerationTime (void)
{
    return _i64GenerationTime;
}

inline int64 StateCookie::getLifeSpan (void)
{
    return _i64LifeSpan;
}

inline uint32 StateCookie::getValidationA (void)
{
    return _ui32ValidationA;
}

inline uint32 StateCookie::getValidationZ (void)
{
    return _ui32ValidationZ;
}

inline uint32 StateCookie::getControlTSNA (void)
{
    return _ui32ControlTSNA;
}

inline uint32 StateCookie::getControlTSNZ (void)
{
    return _ui32ControlTSNZ;
}

inline uint32 StateCookie::getReliableSequencedTSNA (void)
{
    return _ui32ReliableSequencedTSNA;
}

inline uint32 StateCookie::getReliableSequencedTSNZ (void)
{
    return _ui32ReliableSequencedTSNZ;
}

inline uint32 StateCookie::getUnreliableSequencedTSNA (void)
{
    return _ui32UnreliableSequencedTSNA;
}

inline uint32 StateCookie::getUnreliableSequencedTSNZ (void)
{
    return _ui32UnreliableSequencedTSNZ;
}

inline uint32 StateCookie::getReliableUnsequencedIDA (void)
{
    return _ui32ReliableUnsequencedIDA;
}

inline uint32 StateCookie::getReliableUnsequencedIDZ (void)
{
    return _ui32ReliableUnsequencedIDZ;
}

inline uint32 StateCookie::getUnreliableUnsequencedIDA (void)
{
    return _ui32UnreliableUnsequencedIDA;
}

inline uint32 StateCookie::getUnreliableUnsequencedIDZ (void)
{
    return _ui32UnreliableUnsequencedIDZ;
}

inline uint16 StateCookie::getPortA (void)
{
    return _ui16PortA;
}

inline uint16 StateCookie::getPortZ (void)
{
    return _ui16PortZ;
}

#endif   // #ifndef INCL_STATE_COOKIE_H
