/*
 * ObjectFreezer.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#include "ObjectFreezer.h"

#include <string.h>

using namespace NOMADSUtil;

ObjectFreezer::ObjectFreezer (void)
{
    _bFailed = true;
    _pWriter = NULL;
}

ObjectFreezer::~ObjectFreezer (void)
{
}

int ObjectFreezer::init (Writer *pw)
{
    if (pw == NULL) {
        return -1;
    }
    uint32 ui32MagicCode = 0x12345678;
    if (pw->writeBytes (&ui32MagicCode, sizeof (uint32))) {
        return -2;
    }
    _pWriter = pw;
    _bFailed = false;
    return 0;
}

int ObjectFreezer::beginNewObject (const char *pszObjectType)
{
    if (putString (pszObjectType)) {
        _bFailed = true;
        return -1;
    }
    return 0;
}

int ObjectFreezer::endObject (void)
{
    if (_bFailed) {
        return -1;
    }
    return 0;
}

int ObjectFreezer::putBool (bool b)
{
    beginPut (Bool);
    unsigned char uch = b;
    if (_pWriter->writeBytes (&uch, sizeof (unsigned char))) {
        _bFailed = true;
    }
    return endPut();
}

int ObjectFreezer::putChar (char ch)
{
    beginPut (Char);
    if (_pWriter->writeBytes (&ch, sizeof (char))) {
        _bFailed = true;
    }
    return endPut();
}

int ObjectFreezer::putUnsignedChar (unsigned char uch)
{
    beginPut (UnsignedChar);
    if (_pWriter->writeBytes (&uch, sizeof (unsigned char))) {
        _bFailed = true;
    }
    return endPut();
}

int ObjectFreezer::putInt16 (int16 i16)
{
    beginPut (Int16);
    if (_pWriter->writeBytes ((char*)&i16, sizeof (int16))) {
        _bFailed = true;
    }
    return endPut();
}

int ObjectFreezer::putUInt16 (uint16 ui16)
{
    beginPut (UInt16);
    if (_pWriter->writeBytes ((char*)&ui16, sizeof (uint16))) {
        _bFailed = true;
    }
    return endPut();
}

int ObjectFreezer::putInt32 (int32 i32)
{
    beginPut (Int32);
    if (_pWriter->writeBytes ((char*)&i32, sizeof (int32))) {
        _bFailed = true;
    }
    return endPut();
}

int ObjectFreezer::putUInt32 (uint32 ui32)
{
    beginPut (UInt32);
    if (_pWriter->writeBytes ((char*)&ui32, sizeof (uint32))) {
        _bFailed = true;
    }
    return endPut();
}

int ObjectFreezer::putInt64 (int64 i64)
{
    beginPut (Int64);
    union {
        int32 i32[2];
        int64 i64;
    } buf;
    buf.i64 = i64;
    if (_pWriter->writeBytes ((char*)buf.i32, sizeof (int32)*2)) {
        _bFailed = true;
    }
    return endPut();
}

int ObjectFreezer::putFloat (float f)
{
    beginPut (Float);
    if (_pWriter->writeBytes ((char*)&f, sizeof (float))) {
        _bFailed = true;
    }
    return endPut();
}

int ObjectFreezer::putDouble (double d)
{
    beginPut (Double);
    if (_pWriter->writeBytes ((char*)&d, sizeof (double))) {
        _bFailed = true;
    }
    return endPut();
}

int ObjectFreezer::putString (const char *pszString)
{
    beginPut (String);
    uint32 len = 0;
    if (pszString) {
        len = (uint32)strlen (pszString);
    }
    if (_pWriter->writeBytes ((char*)&len, sizeof (uint32))) {
        _bFailed = true;
    }
    else {
        if (len > 0) {
            if (_pWriter->writeBytes ((void*)pszString, len)) {
                _bFailed = true;
            }
        }
    }
    return endPut();
}

int ObjectFreezer::putBlob (void *pBlob, uint32 length)
{
    beginPut (Blob);
    if (_pWriter->writeBytes ((char*)&length, sizeof (uint32))) {
        _bFailed = true;
    }
    else {
        if (length > 0) {
            if (_pWriter->writeBytes ((char*)pBlob, length)) {
                _bFailed = true;
            }
        }
    }
    return endPut();
}
