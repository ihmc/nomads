#ifndef INCL_COMM_INTERFACE_H
#define INCL_COMM_INTERFACE_H

/*
 * CommInterface.h
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
#include "InetAddr.h"


class CommInterface
{
    public:
        virtual ~CommInterface (void);

        virtual CommInterface * newInstance (void) = 0;

        virtual int bind (uint16 ui16Port) = 0;
        virtual int bind (NOMADSUtil::InetAddr *pLocalAddr) = 0;
        virtual NOMADSUtil::InetAddr getLocalAddr (void) = 0;
        virtual int getLocalPort (void) = 0;
        virtual int close (void) = 0;
        virtual int shutdown (bool bReadMode, bool bWriteMode) = 0;
        virtual int setReceiveTimeout (uint32 ui32TimeoutInMS) = 0;
        virtual int setReceiveBufferSize (uint32 ui32BufferSize) = 0;
        virtual int sendTo (NOMADSUtil::InetAddr *pRemoteAddr, const void *pBuf, int iBufSize, const char *pszHints = nullptr) = 0;
        virtual int receive (void *pBuf, int iBufSize, NOMADSUtil::InetAddr *pRemoteAddr) = 0;
        virtual int getLastError (void) = 0;
        virtual int isRecoverableSocketError (void) = 0;
};

#endif   // #ifndef INCL_COMM_INTERFACE_H
