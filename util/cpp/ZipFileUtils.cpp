/*
 * ZipFileUtils.cpp
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

#include "ZipFileUtils.h"

#include "NLFLib.h"

#if defined (WIN32)
	#define NOMINMAX
	#include <winsock2.h>
    #include <windows.h>
    #include <io.h>
    #include <direct.h>
    #define PATH_MAX MAX_PATH
#elif defined (UNIX)
    #include <stdio.h>
    #include <stdlib.h>
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/errno.h>
    #include <string.h>
    #if defined (OSX)
        #include <sys/syslimits.h>
    #endif
#else
    #error Must Define WIN32 or UNIX!
#endif

#include "FileUtils.h"
#include "ZipFileReader.h"
#include "CompressedReader.h"
#include "BufferReader.h"

using namespace NOMADSUtil;

#include "Logger.h"
#define checkAndLogMsg if (pLogger) pLogger->logMsg

int ZipFileUtils::unzip (const char *pszPathToZipFile, const char *pszDestDir)
{
    const char *pszMethodName = "FileUtils::unzip";

    ZipFileReader zipReader;
    if (zipReader.init (pszPathToZipFile) != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "invalid zip file\n");
        return -1;
    }

	char referenceDestDir[PATH_MAX];
	strcpy(referenceDestDir, pszDestDir);

    // Create the destination directory
    if (!FileUtils::createDirectory (pszDestDir)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "error creating destination directory: %s\n",
                        pszDestDir);
        return -2;
    }

    // Unzip the contents in the services directory
    for (int i = 0; i < zipReader.getFileCount(); i++) {
        char pszFileName[PATH_MAX];
        if (i == 0) {
            strcpy (pszFileName, zipReader.getFirstFileName());
        }
        else {
            strcpy (pszFileName, zipReader.getNextFileName());
        }

        if (pszFileName[strlen(pszFileName)-1] == '/') {
            String dirName = referenceDestDir;
            dirName += getPathSepCharAsString();
            dirName += pszFileName;
            FileUtils::transformPathSeparator( (char*) dirName);

            if (!FileUtils::createDirectory (dirName)) {
                printf ("error creating directory: %s\n", (const char*) dirName);
                checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                "error creating directory: %s\n",
                                (const char*) dirName);
                return -3;
            }
        }
        else {
            String fName = pszDestDir;
            fName += getPathSepCharAsString();
            fName += pszFileName;
            FileUtils::transformPathSeparator( (char*) fName);

            FileUtils::createDirStructure (fName);

            ZipFileReader::Entry *pEntry = zipReader.getEntry (pszFileName);
            CompressedReader *pcr = new CompressedReader (new BufferReader (pEntry->pBuf, pEntry->ui32CompSize), true, true);
            void *pFileContents = malloc (pEntry->ui32UncompSize);
            if ((pcr->readBytes (pFileContents, pEntry->ui32UncompSize)) < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                "error uncompressing file: %s\n",
                                pszFileName);

                free (pFileContents);
                delete pcr;
                delete pEntry;
                return -4;
            }
            FILE *fd = fopen (fName, "wb");
            if (fd == NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                "error creating file: %s\n",
                                (const char*) fName);
            
                free (pFileContents);
                delete pcr;
                delete pEntry;
                return -5;
            }
            if ((pEntry->ui32UncompSize > 0) && (fwrite (pFileContents, 1, pEntry->ui32UncompSize, fd) == 0)) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                "error writing file: %s\n",
                                (const char*) fName);
                
                free (pFileContents);
                delete pcr;
                delete pEntry;
                fclose (fd);
                return -6;
            }

            free (pFileContents);
            delete pcr;
            delete pEntry;
            fclose (fd);
        }
    }

    return 0;
}
