#ifndef INCL_MOCKET_WRITER_H
#define INCL_MOCKET_WRITER_H

/*
 * MocketWriter.h
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

#include "Writer.h"


class Mocket;
class StreamMocket;

class MocketWriter : NOMADSUtil::Writer
{
    public:
        MocketWriter (StreamMocket *pMocket, bool bDeleteWhenDone = false);
        ~MocketWriter (void);

        // Returns 0 if successful or -1 in case of an error
        int writeBytes (const void *pBuf, unsigned long ulCount);

        // Flush any buffered data
        int flush (void);

        // Close the underlying 'stream'
        int close();

    private:
        StreamMocket * _pStreamMocket;
        bool _bDeleteWhenDone;
};

#endif //INCL_MOCKET_WRITER_H
