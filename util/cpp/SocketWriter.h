/*
 * SocketWriter.h
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

#ifndef INCL_SOCKET_WRITER_H
#define INCL_SOCKET_WRITER_H

#include "Writer.h"

namespace NOMADSUtil
{

    class Socket;

    class SocketWriter : public Writer
    {
        public:
            SocketWriter (Socket *pSocket, bool bDeleteWhenDone = false);
            ~SocketWriter (void);
            int writeBytes (const void *pBuf, unsigned long ulCount);
            unsigned long getTotalBytesWritten (void);
            int close();
            Socket * getSocket (void);

        protected:
            Socket *_pSocket;
            bool _bDeleteSocketWhenDone;
            unsigned long _ulBytesWritten;
    };

    inline unsigned long SocketWriter::getTotalBytesWritten (void)
    {
        return _ulBytesWritten;
    }

    inline Socket * SocketWriter::getSocket (void)
    {
        return _pSocket;
    }

}

#endif   // #ifndef INCL_SOCKET_WRITER_H
