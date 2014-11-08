/*
 * PushbackLineOrientedReader.h
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

#ifndef INCL_PUSHBACK_LINE_ORIENTED_READER_H
#define INCL_PUSHBACK_LINE_ORIENTED_READER_H

#include "LineOrientedReader.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class PushbackLineOrientedReader : public LineOrientedReader
    {
        public:
            PushbackLineOrientedReader (Reader *pr, bool bDeleteWhenDone = false);
            ~PushbackLineOrientedReader (void);
            int pushbackLine (const char *pszLine);
            int readLine (char *pszBuf, int iBufSize);
        protected:
            bool _bBufferEmpty;
            String _bufferedLine;
    };
}

#endif   // #ifndef INCL_PUSHBACK_LINE_ORIENTED_READER_H
