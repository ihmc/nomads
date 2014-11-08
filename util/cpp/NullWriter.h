/* 
 * NullWriter.h
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
 *
 * Dummy writer that discards all data written to it but reports that the write
 * operation succeeded.
 * It can be useful to compute the number of bytes that would be written without
 * actually writing them.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 4, 2014, 5:39 PM
 */

#ifndef INCL_NULL_WRITER_H
#define	INCL_NULL_WRITER_H

#include "Writer.h"

namespace NOMADSUtil
{
    class NullWriter : public Writer
    {
        public:
            NullWriter (void);
            virtual ~NullWriter (void);

            int writeBytes (const void *pBuf, unsigned long ulCount);
    };
}

#endif	// INCL_NULL_WRITER_H

