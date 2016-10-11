/*
 * SerialReader.h
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

#ifndef INCL_SERIAL_READER_H
#define INCL_SERIAL_READER_H

#include "FTypes.h"
#include "Reader.h"

namespace NOMADSUtil
{
    class Serial;

    class SerialReader : public Reader
    {
        public:
            SerialReader (Serial *pSerial, bool bDeleteWhenDone = false);

            ~SerialReader (void);

            // Read upto iCount bytes; returns number of bytes read or
            //     a negative number in case of error
            int read (void *pBuf, int iCount);

            // Returns the number of bytes available
            uint32 getBytesAvailable (void);

        private:
            Serial *_pSerial;
            bool _bDeleteWhenDone;

    };
}

#endif   // #ifndef INCL_SERIAL_READER_H
