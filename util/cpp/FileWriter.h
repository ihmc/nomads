/*
 * FileWriter.h
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

#ifndef INCL_FILE_WRITER_H
#define INCL_FILE_WRITER_H

#include "Writer.h"

#include <stdio.h>

namespace NOMADSUtil
{
    class FileWriter : public Writer
    {
        public:
            FileWriter (FILE *file, bool bCloseWhenDone = false);
            FileWriter (const char *pszFilename, const char *pszMode);
            ~FileWriter (void);

            int writeBytes (const void *pBuf, unsigned long ulCount);

            // Flush any buffered data
            int flush (void);
            // Close the underlying 'stream'
            int close();

        protected:
            FILE *fileOutput;

        private:
            // if true, the FileWriter manages the open and close of file descriptor
            bool _bHandleFD;
    };
}

#endif   // #ifndef INCL_FILE_WRITER_H
