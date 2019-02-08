/*
 * MocketsEndPoint.h
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

#ifndef INCL_MOCKETS_END_POINT_H
#define INCL_MOCKETS_END_POINT_H

#include "ConnEndPoint.h"

class Mocket;

namespace IHMC_ACI
{
    class MocketEndPoint : public ConnEndPoint
    {
        public:
            MocketEndPoint (Mocket *pMocket, int64 i64Timeout);
            virtual ~MocketEndPoint (void);

            int connect (const char *pszRemoteHost, uint16 ui16RemotePort);
            void close (void);
            NOMADSUtil::String getRemoteAddress (void);
            int send (const void *pBuf, int iSize);
            int receive (void *pBuf, uint32 ui32BufSize, int64 i64Timeout);

        private:
            friend class MocketConnListener;

            Mocket *_pMocket;
            const int64 _i64Timeout;
    };
}

#endif    /* INCL_MOCKETS_END_POINT_H */
