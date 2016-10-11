/*
 * FileReader.h
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

#ifndef INCL_FILE_READER_H
#define INCL_FILE_READER_H

#include <stdio.h>
#include "Reader.h"

namespace NOMADSUtil
{
    class FileReader : public Reader
    {
        public:
            FileReader (const char *pszFilePath, const char *pszMode);
            explicit FileReader (FILE *pfile, bool bCloseFileWhenDone = false);
            virtual ~FileReader (void);
            int read (void *pBuf, int iCount);
            int readBytes (void *pBuf, uint32 ui32Count);
            // set the file pointer at the ui64Pos-th position from the
            // beginning of the file
            int seek (long ui64Pos);

        protected:
            bool _bCloseFileWhenDone;
            FILE *_pfileInput;
    };
}

#endif   // #ifndef INCL_FILE_READER_H
