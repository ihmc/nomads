/*
 * LineOrientedReader.h
 *
 *  This file is part of the IHMC Util Library
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

#ifndef INCL_LINE_ORIENTED_READER_H
#define INCL_LINE_ORIENTED_READER_H

#include "Reader.h"

namespace NOMADSUtil
{

    class LineOrientedReader : public Reader
    {
        public:
            LineOrientedReader (Reader *pr, bool bDeleteWhenDone = false);
            ~LineOrientedReader (void);
            int read (void *pBuf, int iCount);
            int readBytes (void *pBuf, uint32 ui32Count);

            // Reads a line of text terminated by "\r", "\n", or "\r\n" and strips the
            //     line termination before copying the line into the specified buffer
            // Returns a count of the number of characters in the line (could be 0 for an
            //     empty line), -1 in case of an error in reading, or -2 in case the specified
            //     buffer was too small to hold a complete line
            virtual int readLine (char *pszBuf, int iBufSize);

            void setDeleteReaderWhenDone (bool bDelete);

        protected:
            int readMoreData (void);
        protected:
            Reader *_pReader;
            bool _bDeleteReaderWhenDone;
            char _chBuffer;
            bool _bByteInBuffer;
    };

    inline void LineOrientedReader::setDeleteReaderWhenDone (bool bDelete)
    {
        _bDeleteReaderWhenDone = bDelete;
    }

}

#endif   // #ifndef INCL_LINE_ORIENTED_READER_H
