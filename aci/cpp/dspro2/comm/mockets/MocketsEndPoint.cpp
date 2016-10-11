/*
 * MocketsEndPoint.cpp
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
 * Created on April 14, 2014, 1:01 PM
 */

#include "MocketsEndPoint.h"

#include "Mocket.h"

#include "NetUtils.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

MocketEndPoint::MocketEndPoint (Mocket *pMocket, int64 i64Timeout)
    : _pMocket (pMocket),
      _i64Timeout (i64Timeout)
{
}

MocketEndPoint::~MocketEndPoint (void)
{
    _pMocket = NULL;
}

int MocketEndPoint::connect (const char *pszRemoteHost, uint16 ui16RemotePort)
{
    if (pszRemoteHost == NULL) {
        return -1;
    }
    if (_pMocket == NULL) {
        return -2;
    }
    return _pMocket->connect (pszRemoteHost, ui16RemotePort);
}

void MocketEndPoint::close (void)
{
    if (_pMocket != NULL) {
        _pMocket->close();
    }
}

String MocketEndPoint::getRemoteAddress (void)
{
    String addr;
    if (_pMocket != NULL) {
        char *pszTmp = NetUtils::ui32Inetoa (_pMocket->getRemoteAddress());
        if (pszTmp != NULL) {
            addr = pszTmp; // String makes a copy
            free (pszTmp);
        }
    }
    return addr;
}

int MocketEndPoint::send (const void *pBuf, int iSize)
{
    if (_pMocket == NULL) {
        return -1;
    }
    return _pMocket->send (true, // reliable transmission
                           true, // sequenced transmission
                           pBuf, iSize,
                           0,    // tag
                           1,    // priority
                           0,    // infinite enqueue timeout
                           1);   // infinite retry timeout
}

int MocketEndPoint::receive (void *pBuf, uint32 ui32BufSize, int64 i64Timeout)
{
    if (_pMocket == NULL) {
        return -1;
    }
    return _pMocket->receive (pBuf, ui32BufSize, i64Timeout);
}

