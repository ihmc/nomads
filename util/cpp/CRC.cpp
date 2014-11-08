/*
 * CRC.cpp
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Created on Nov 24, 2009
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 *
 * This code is derived from Michael Barr implementation of CRC
 *
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 *
 */

#include "CRC.h"

#include "Reader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define WIDTH (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))

using namespace NOMADSUtil;

CRC::CRC (void)
{
    _pInternalBuf = NULL;
    _ulInternalBufSize = 0;
    _pCrcTable = new crc [256];
}

CRC::~CRC (void)
{
    delete[] (char*)_pInternalBuf;
    _pInternalBuf = NULL;
    delete[] _pCrcTable;
    _pCrcTable = NULL;
}

int CRC::init (void)
{
    crc remainder;
    int dividend;
    unsigned char bit;

    // Compute the remainder of each possible dividend.
    for (dividend = 0; dividend < 256; ++dividend) {
        //Start with the dividend followed by zeros.
        remainder = dividend << (WIDTH - 8);

        //Perform modulo-2 division, a bit at a time.
        for (bit = 8; bit > 0; --bit) {
            //Try to divide the current data bit.
            if (remainder & TOPBIT) {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            }
            else {
                remainder = (remainder << 1);
            }
        }
        //Store the result into the table.
        _pCrcTable [dividend] = remainder;
    }
    return 0;
}

int CRC::update8 (const void *pBuf)
{
    if (update (pBuf, 1)) {
        return -1;
    }
    return 0;
}

int CRC::update16 (void *pBuf)
{
    #if defined (LITTLE_ENDIAN_SYSTEM)
        Reader::byteSwap16 (pBuf);
    #endif
    if (update (pBuf, 2)) {
        return -1;
    }
    #if defined (LITTLE_ENDIAN_SYSTEM)
        Reader::byteSwap16 (pBuf);
    #endif
    return 0;
}

int CRC::update32 (void *pBuf)
{
    #if defined (LITTLE_ENDIAN_SYSTEM)
        Reader::byteSwap32 (pBuf);
    #endif
    if (update (pBuf, 4)) {
        return -1;
    }
    #if defined (LITTLE_ENDIAN_SYSTEM)
        Reader::byteSwap32 (pBuf);
    #endif
    return 0;
}

int CRC::update64 (void *pBuf)
{
    #if defined (LITTLE_ENDIAN_SYSTEM)
        Reader::byteSwap64 (pBuf);
    #endif
    if (update (pBuf, 8)) {
        return -1;
    }
    #if defined (LITTLE_ENDIAN_SYSTEM)
        Reader::byteSwap64 (pBuf);
    #endif
    return 0;
}

int CRC::update (const char *pszString)
{
    return update (pszString, (unsigned long)strlen (pszString));
}

int CRC::update (const void *pBuf, unsigned long ulBufSize)
{
    if (!pBuf) {
        return -1;
    }
    if (!_pInternalBuf) {
        _pInternalBuf = (char *) malloc (ulBufSize);
        memcpy (_pInternalBuf, pBuf, ulBufSize);
        _ulInternalBufSize = ulBufSize;
    }
    else {
        void *pOldBuf = _pInternalBuf;
        void *pTmpBuf = malloc (_ulInternalBufSize + ulBufSize);
        memcpy (pTmpBuf, _pInternalBuf, _ulInternalBufSize);
        memcpy ((char*)pTmpBuf + _ulInternalBufSize, pBuf, ulBufSize);
        _pInternalBuf = pTmpBuf;
        _ulInternalBufSize = _ulInternalBufSize + ulBufSize;
        pTmpBuf = NULL;
        if (pOldBuf != NULL) {
            free (pOldBuf);
        }
        pOldBuf = NULL;
    }
    return 0;
}

int CRC::reset (void)
{
    if (_pInternalBuf != NULL) {
        free (_pInternalBuf);
        _pInternalBuf = NULL;
    }
    _ulInternalBufSize = 0;
    return 0;
}

crc CRC::getChecksum (void)
{
    return getChecksum (_pInternalBuf, _ulInternalBufSize);
}

crc CRC::getChecksum (const void *message, unsigned long ulSize)
{
    if (message == NULL) {
        return 0x00000000;
    }

    crc remainder = INITIAL_REMAINDER;
    unsigned char data;
    unsigned long byte;

    //Divide the message by the polynomial, a byte at a time.
    for (byte = 0; byte < ulSize; ++byte) {
        data = (unsigned char)reflect((((char*)message)[byte]), 8) ^ (remainder >> (WIDTH - 8));
        remainder = _pCrcTable[data] ^ (remainder << 8);
    }

    //The final remainder is the CRC.
    return (crc)reflect ((remainder), WIDTH) ^ FINAL_XOR_VALUE;
}

unsigned int CRC::reflect (unsigned int uiData, unsigned char nBits)
{
    unsigned int uiReflection = 0x00000000;
    unsigned char bit;

    //Reflect the data about the center bit.
    for (bit = 0; bit < nBits; ++bit) {
        //If the LSB bit is set, set the reflection of it.
        if (uiData & 0x01) {
            uiReflection |= (1 << ((nBits - 1) - bit));
        }
        uiData = (uiData >> 1);
    }
    return uiReflection;
}
