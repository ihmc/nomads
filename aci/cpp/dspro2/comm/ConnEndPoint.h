/*
 * EndPoint.h
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
 * Created on April 14, 2014, 12:21 PM
 */

#ifndef INCL_CONNECTION_END_POINT_H
#define INCL_CONNECTION_END_POINT_H

#include "FTypes.h"
#include "StrClass.h"

namespace IHMC_ACI
{
    class ConnEndPoint
    {
        public:
            static const int64 DEFAULT_TIMEOUT = 1000 *  // 1 second
                                                   30;   // 30 seconds

            ConnEndPoint (void);
            virtual ~ConnEndPoint (void);

            virtual int connect (const char *pszRemoteHost, uint16 ui16RemotePort) = 0;
            virtual void close (void) = 0;

            virtual NOMADSUtil::String getRemoteAddress (void) = 0;

            virtual int send (const void *pBuf, int iSize) = 0;

            // Retrieves the data from next message that is ready to be delivered to the application
            // At most ui32BufSize bytes are copied into the specified buffer
            // Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
            //     whereas a timeout of -1 implies wait indefinitely
            // NOTE: Any additional data in the packet that will not fit in the buffer is discarded
            // Returns the number of bytes that were copied into the buffer, -1 in case of the connection
            //     being closed, and 0 in case no data is available within the specified timeout
            virtual int receive (void *pBuf, uint32 ui32BufSize, int64 i64Timeout = 0) = 0;

        private:
           // const String _remotePeerAddr;
           // const uint16 ui16Port;
    };

    inline ConnEndPoint::ConnEndPoint (void)
    {
    }

    inline ConnEndPoint::~ConnEndPoint (void)
    {
    }
}

#endif    /* INCL_CONNECTION_END_POINT_H */
