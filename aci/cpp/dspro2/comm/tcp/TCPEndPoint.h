/*
 * TCPEndPoint.h
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
 * Created on April 15, 2014, 12:40 AM
 */

#ifndef INCL_TCP_END_POINT_H
#define INCL_TCP_END_POINT_H

#include "ConnEndPoint.h"

namespace NOMADSUtil
{
    class TCPSocket;
}

namespace IHMC_ACI
{
    class TCPEndPoint : public ConnEndPoint
    {
        public:
            TCPEndPoint (NOMADSUtil::TCPSocket *pSocket, int64 i64Timeout);
            virtual ~TCPEndPoint (void);

            int connect (const char *pszRemoteHost, uint16 ui16RemotePort);
            void close (void);
            NOMADSUtil::String getRemoteAddress (void);
            int send (const void *pBuf, int iSize);
            int receive (void *pBuf, uint32 ui32BufSize, int64 i64Timeout);

        private:
            friend class TCPConnListener;

            NOMADSUtil::TCPSocket *_pSocket;
            const int64 _i64Timeout;
    };
}

#endif    /* INCL_TCP_END_POINT_H */
