/*
 * SocketReader.h
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

#ifndef INCL_SOCKET_READER_H
#define INCL_SOCKET_READER_H

#include "Reader.h"

namespace NOMADSUtil
{

    class Socket;

    class SocketReader : public Reader
    {
        public:
            SocketReader (Socket *pSocket, bool bDeleteWhenDone = false);
            ~SocketReader (void);
            int read (void *pBuf, int iCount);
            int readBytes (void *pBuf, uint32 ui32Count);
            int close();
            Socket * getSocket (void);
        protected:
            Socket *_pSocket;
            bool _bDeleteSocketWhenDone;
    };

    inline Socket * SocketReader::getSocket (void)
    {
        return _pSocket;
    }

}

#endif   // #ifndef INCL_SOCKET_READER_H
