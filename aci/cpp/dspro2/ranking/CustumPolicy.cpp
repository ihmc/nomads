/* 
 * CustumRanker.cpp
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
 * Created on February 12, 2015, 7:09 PM
 */

#include "CustumPolicy.h"
#include "InstrumentedReader.h"
#include "InstrumentedWriter.h"
#include "MetadataInterface.h"

#include <limits.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

CustumPolicy::CustumPolicy (Type type)
    : _type (type),
      _rankWeight (0.0f)
{
    assert (_type < 0xFF);  // because it is serialized ad uint8
}

CustumPolicy::CustumPolicy (Type type, float rankWeight, const String &attributeName)
    : _type (type),
      _rankWeight (rankWeight),
      _attributeName (attributeName)
{
    assert (_type < 0xFF);  // because it is serialized ad uint8
}

CustumPolicy::~CustumPolicy()
{
}

float CustumPolicy::getRankWeight (void) const
{
    return _rankWeight;
}

CustumPolicy::Type CustumPolicy::getType (void) const
{
    return _type;
}

int CustumPolicy::read (Reader *pReader, uint32 i32MaxLen)
{
    if (pReader == NULL) {
        return -1;
    }
    if (pReader->readFloat (&_rankWeight) < 0) {
        return -2;
    }
    uint16 ui16Len = 0;
    if (pReader->read16 (&ui16Len) < 0) {
        return -3;
    }
    char buf[1024];
    if (ui16Len > 0 && pReader->readBytes (buf, ui16Len) < 0) {
        return -4;
    }
    buf[ui16Len] = '\0';
    _attributeName = buf;
    return 0;
}

int CustumPolicy::write (Writer *pWriter, uint32 i32MaxLen)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (pWriter->writeFloat (&_rankWeight) < 0) {
        return -2;
    }
    uint16 ui16Len = _attributeName.length();
    if (pWriter->write16 (&ui16Len) < 0) {
        return -3;
    }
    if (ui16Len > 0 && pWriter->writeBytes (_attributeName.c_str(), ui16Len) < 0) {
        return -4;
    }
    return 0;
}

int CustumPolicy::writeLength (void)
{
    int i = 4; // _rankWeight
    i += 2;    // _attributeName's length
    i += _attributeName.length();

    return i;
}

CustumPolicies::CustumPolicies (void)
    : _ui8Count (0)
{
}

CustumPolicies::~CustumPolicies (void)
{
}

int CustumPolicies::init (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return -1;
    }

    return -2;
}

void CustumPolicies::prepend (CustumPolicy *pPolicy)
{
    PtrLList<CustumPolicy>::prepend (pPolicy);
    _ui8Count++;
}

int CustumPolicies::read (Reader *pReader, uint32 i32MaxLen)
{
    if (pReader == NULL) {
        return -1;
    }
    if (pReader->read8 (&_ui8Count) < 0) {
        return -2;
    }
    int iBytes = 1;
    if (_ui8Count > 0) {
        uint8 i = 0;
        for (; i < _ui8Count; i++) {
            uint8 ui8Type = 0;
            if (pReader->read8 (&ui8Type) < 0) {
                return -3;
            }
            iBytes += 1;
            CustumPolicy *pPolicy = NULL;
            switch (ui8Type) {
                case CustumPolicy::STATIC:
                    pPolicy = new StaticPolicy();
                    break;

                default:
                    return -4;
            }
            if (pPolicy == NULL) {
                return -5;
            }
            const int rc = pPolicy->read (pReader, i32MaxLen);
            if (rc < 0) {
                return -6;
            }
            iBytes += rc;
        }
        assert (i == _ui8Count);
        if (i != _ui8Count) {
            return -7;
        }
    }
    return iBytes;
}

int CustumPolicies::write (Writer *pWriter, uint32 i32MaxLen)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (pWriter->write8 (&_ui8Count) < 0) {
        return -2;
    }
    int iBytes = 1;
    if (_ui8Count > 0) {
        for (CustumPolicy *pPolicy = getFirst(); pPolicy != NULL; pPolicy = getNext()) {
            uint8 ui8 = pPolicy->getType();
            if (pWriter->write8 (&ui8) < 0) {
                return -3;
            }
            int rc = pPolicy->write (pWriter, (i32MaxLen - iBytes));
            if (rc < 0) {
                return -4;
            }
            else {
                iBytes += rc;
            }
        }
    }
    return iBytes;
}

int CustumPolicies::getWriteLength (void)
{
    int iBytes = 1;  // iCount
    if (_ui8Count > 0) {
        for (CustumPolicy *pPolicy = getFirst(); pPolicy != NULL; pPolicy = getNext()) {
            iBytes += 1;    // _type
            int rc = pPolicy->writeLength();
            if (rc < 0) {
                return -1;
            }
            else {
                iBytes += rc;
            }
        }
    }
    return iBytes;
}

StaticPolicy::StaticPolicy (void)
    : CustumPolicy (STATIC)
{
    
}

StaticPolicy::StaticPolicy (float rankWeight, const NOMADSUtil::String &attributeName)
    : CustumPolicy (STATIC, rankWeight, attributeName)
{   
}

StaticPolicy::~StaticPolicy (void)
{
}

float StaticPolicy::rank (MetadataInterface *pMetadata)
{
    if (pMetadata == NULL) {
        return 0.0f;
    }
    String value;
    if (pMetadata->getFieldValue (_attributeName, value) < 0) {
        return 0.0f;
    }
    Rank *pRank = _valueToRank.get (value);
    if (pRank == NULL) {
        return 0.0f;
    }
    return pRank->_rank;
}

int StaticPolicy::read (Reader *pReader, uint32 i32MaxLen)
{
    InstrumentedReader ir (pReader, false);
    if (CustumPolicy::read (&ir, i32MaxLen) < 0) {
        return -1;
    }
    uint16 ui16 = 0;
    if (ir.read16 (&ui16) < 0) {
        return -2;
    }
    const uint16 ui16NElements = ui16;
    for (uint16 i = 0; i < ui16NElements; i++) {
        if (ir.read16 (&ui16) < 0) {
            return -3;
        }
        char buf[1024];
        if (ui16 > 0 && ir.readBytes (buf, ui16) < 0) {
            return -4;
        }
        buf[ui16] = '\0';
        float rank = 0.0f;
        if (ir.readFloat (&rank) < 0) {
            return -5;
        }
        Rank *pRank = new Rank (rank);
        if (pRank != NULL) {
            _valueToRank.put (buf, pRank);
        }
    }
    return ir.getBytesRead();
}

int StaticPolicy::write (Writer *pWriter, uint32 i32MaxLen)
{
    InstrumentedWriter iw (pWriter, false);
    if (CustumPolicy::write (&iw, i32MaxLen) < 0) {
        return -1;
    }
    unsigned int uiCount = _valueToRank.getCount();
    if (uiCount > 0xFFFF) {
        return -2;
    }
    if (iw.write16 (&uiCount) < 0) {
        return -3;
    }
    StringHashtable<Rank>::Iterator iter = _valueToRank.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        String attr (iter.getKey());
        uint16 ui16Len = attr.length();
        if (iw.write16 (&ui16Len) < 0) {
            return -4;
        }
        if (ui16Len > 0 && iw.writeBytes (attr.c_str(), ui16Len) < 0) {
            return -5;
        }
        Rank *pRank = iter.getValue();
        float rank = (pRank->_rank);
        if (iw.writeFloat (&rank) < 0) {
            return -2;
        }
    }
    if (iw.getBytesWritten() > INT_MAX) {
        return -3;
    }
    return (int) iw.getBytesWritten();
}

int StaticPolicy::writeLength (void)
{
    int i = CustumPolicy::writeLength();
    i += 2;    // uiCount
    StringHashtable<Rank>::Iterator iter = _valueToRank.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        String attr (iter.getKey());
        uint16 ui16Len = attr.length();
        i += 2;          // ui16Len
        i += ui16Len;    // attr
        i += 4;          // rank
    }

    return i;
}

StaticPolicy::Rank::Rank (float rank)
    : _rank (rank)
{
}

StaticPolicy::Rank::~Rank()
{
}

