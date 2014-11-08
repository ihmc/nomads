/*
 * Pipe.h
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

#ifndef INCL_PIPE_H
#define INCL_PIPE_H

#include <sys/stat.h>

#if defined (WIN32)
    #include <fcntl.h>
    #include <io.h>
#elif defined (UNIX)
    #include <unistd.h>
#endif

namespace NOMADSUtil
{

    class Pipe
    {
        public:
            Pipe (void);
            ~Pipe (void);
            int init (void);

            // Get the file descriptor which can be used with read()
            int getReadFileDescriptor (void);

            // Get the file descriptor which can be used with write()
            int getWriteFileDescriptor (void);

            int sendBytes (void *pData, unsigned long ulSize);
            int receive (void *pBuf, int iSize);
            int receiveBytes (void *pBuf, unsigned long ulSize);
            unsigned long bytesAvail (void);

        protected:
            Pipe (int fdRead, int fdWrite);

        protected:
            int _fdRead;
            int _fdWrite;
            struct stat statbuf;
    };

    inline int Pipe::getReadFileDescriptor (void)
    {
        return _fdRead;
    }

    inline int Pipe::getWriteFileDescriptor (void)
    {
        return _fdWrite;
    }

}

#endif  // #ifndef INCL_PIPE_H
