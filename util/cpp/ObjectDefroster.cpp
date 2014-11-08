/*
 * ObjectDefroster.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#include "ObjectDefroster.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace NOMADSUtil;

ObjectDefroster::ObjectDefroster (void)
{
    bFailed = true;
    pReader = NULL;
    bSwapNeeded = false;
}

ObjectDefroster::~ObjectDefroster (void)
{
}

int ObjectDefroster::init (Reader *pr)
{
    if (pr == NULL) {
        return -1;
    }
    uint32 ui32MagicCode = 0;
    if (pr->readBytes (&ui32MagicCode, sizeof (uint32))) {
        return -2;
    }
    if (ui32MagicCode == 0x12345678) {
        bSwapNeeded = false;
    }
    else {
        Reader::byteSwap32 (&ui32MagicCode);
        if (ui32MagicCode == 0x12345678) {
            bSwapNeeded = true;
        }
        else {
            return -3;
        }
    }
    pReader = pr;
    bFailed = false;
    return 0;
}

int ObjectDefroster::beginNewObject (const char *pszObjectType)
{
    char *pszStoredObjectType = NULL;
    if (getString (pszStoredObjectType)) {
        bFailed = true;
        return -1;
    }
    if (0 != strcmp (pszObjectType, pszStoredObjectType)) {
        bFailed = true;
        return -2;
    }
    delete[] pszStoredObjectType;
    return 0;
}

int ObjectDefroster::endObject (void)
{
    if (bFailed) {
        return -1;
    }
    return 0;
}

int ObjectDefroster::getBool (bool &b)
{
    beginGet (Bool);
    unsigned char uch;
    if (pReader->readBytes (&uch, sizeof (unsigned char))) {
        bFailed = true;
    }
    if (uch) {
        b = true;
    }
    else {
        b = false;
    }
    return endGet();
}

int ObjectDefroster::getChar (char &ch)
{
    beginGet (Char);
    if (pReader->readBytes (&ch, sizeof (char))) {
        bFailed = true;
    }
    return endGet();
}

int ObjectDefroster::getUnsignedChar (unsigned char &uch)
{
    beginGet (UnsignedChar);
    if (pReader->readBytes (&uch, sizeof (unsigned char))) {
        bFailed = true;
    }
    return endGet();
}

int ObjectDefroster::getInt16 (int16 &i16)
{
    beginGet (Int16);
    if (pReader->readBytes ((char*)&i16, sizeof (int16))) {
        bFailed = true;
    }
    if (bSwapNeeded) {
        Reader::byteSwap16 (&i16);
    }
    return endGet();
}

int ObjectDefroster::getUInt16 (uint16 &ui16)
{
    beginGet (UInt16);
    if (pReader->readBytes ((char*)&ui16, sizeof (uint16))) {
        bFailed = true;
    }
    if (bSwapNeeded) {
        Reader::byteSwap16 (&ui16);
    }
    return endGet();
}

int ObjectDefroster::getInt32 (int32 &i32)
{
    beginGet (Int32);
    if (pReader->readBytes ((char*)&i32, sizeof (int32))) {
        bFailed = true;
    }
    if (bSwapNeeded) {
        Reader::byteSwap32 (&i32);
    }
    return endGet();
}

int ObjectDefroster::getUInt32 (uint32 &ui32)
{
    beginGet (UInt32);
    if (pReader->readBytes ((char*)&ui32, sizeof (uint32))) {
        bFailed = true;
    }
    if (bSwapNeeded) {
        Reader::byteSwap32 (&ui32);
    }
    return endGet();
}

int ObjectDefroster::getInt64 (int64 &i64)
{
    beginGet (Int64);
    union {
        int32 i32[2];
        int64 i64;
    } buf;
    if (pReader->readBytes ((char*)buf.i32, sizeof(int32)*2)) {
        bFailed = true;
    }
    i64 = buf.i64;
    if (bSwapNeeded) {
        Reader::byteSwap64 (&i64);
    }
    return endGet();
}

int ObjectDefroster::getFloat (float &f)
{
    beginGet (Float);
    if (pReader->readBytes((char*)&f, sizeof (float))) {
        bFailed = true;
    }
    if (bSwapNeeded) {
        Reader::byteSwap32 (&f);
    }
    return endGet();
}

int ObjectDefroster::getDouble (double &d)
{
    beginGet (Double);
    if (pReader->readBytes((char*)&d, sizeof (double))) {
        bFailed = true;
    }
    if (bSwapNeeded) {
        Reader::byteSwap64 (&d);
    }
    return endGet();
}

int ObjectDefroster::getString (char *&pszString)
{
    beginGet (String);
    uint32 len;
    if (pReader->readBytes((char*)&len, sizeof (uint32))) {
        bFailed = true;
        pszString = NULL;
        return -1;
    }
    if (bSwapNeeded) {
        Reader::byteSwap32 (&len);
    }
    if (len > 0) {
        if (NULL == (pszString = new char[len+1])) {
            bFailed = true;
            return -2;
        }
        if (pReader->readBytes (pszString, len)) {
            bFailed = true;
            delete[] pszString;
            pszString = NULL;
            return -3;
        }
        pszString[len] = '\0';
    }
    else {
        pszString = NULL;
    }
    if (endGet()) {
        return -4;
    }
    return 0;
}

int ObjectDefroster::getBlob (void *&pBlob)
{
    beginGet (Blob);
    uint32 length;
    if (pReader->readBytes ((char*)&length, sizeof (uint32))) {
        bFailed = true;
        pBlob = NULL;
        return -1;
    }
    if (bSwapNeeded) {
        Reader::byteSwap32 (&length);
    }
    if (length > 0) {
        if (NULL == (pBlob = malloc (length))) {
            bFailed = true;
            return -2;
        }
        if (pReader->readBytes (pBlob, length)) {
            bFailed = true;
            free (pBlob);
            pBlob = NULL;
        }
    }
    else {
        pBlob = NULL;
    }
    if (endGet()) {
        return -3;
    }
    return 0;
}
