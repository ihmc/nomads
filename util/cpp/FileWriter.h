/*
 * FileWriter.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
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
