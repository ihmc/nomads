#ifndef INCL_MOCKET_READER_H
#define INCL_MOCKET_READER_H

/*
 * MocketReader.h
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

#include "Reader.h"


class Mocket;
class StreamMocket;

class MocketReader : public NOMADSUtil::Reader
{
    public:
        MocketReader (StreamMocket *pMocket, bool bDeleteWhenDone = false);
        ~MocketReader (void);

        // Read upto iCount bytes; returns number of bytes read or
        // a negative number in case of error
        int read (void *pBuf, int iCount);

        // Read ulCount bytes repeating calls to read if necessary
        // Returns 0 if successful or a negative number in case of error
        int readBytes (void *pBuf, unsigned long ulCount);

        // Returns the number of bytes available
        uint32 getBytesAvailable (void);

        // Returns the total bytes read so far
        uint32 getTotalBytesRead (void);

        // Close the underlying 'stream'
        int close();

        void setReadTimeout (unsigned long ulReadTimeout);
        unsigned long getReadTimeout();

    private:
        StreamMocket *_pStreamMocket;
        unsigned long _ulReadTimeout;
        bool _bDeleteWhenDone;
}; // class MocketReader


inline void MocketReader::setReadTimeout (unsigned long ulReadTimeout)
{
    _ulReadTimeout = ulReadTimeout;
}

inline unsigned long MocketReader::getReadTimeout()
{
    return _ulReadTimeout;
}

#endif //INCL_MOCKET_READER_H
