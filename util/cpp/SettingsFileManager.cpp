/*
 * SettingsFileManager.cpp
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

#include "SettingsFileManager.h"

#include "Exceptions.h"
#include "FileReader.h"
#include "FileWriter.h"

#include <ctype.h>
#include <errno.h>

#if defined (UNIX)
    #define stricmp strcasecmp
#endif

using namespace NOMADSUtil;

void SettingsFileManager::readFile (StringHashtable<StringStringHashtable> *pshData, const char *pszFilePath, const char *pszKeyLabel)
{
    // Check parameters
    if (pshData == NULL) {
        throw ParamException ("psvpmData == NULL");
    }
    if (pszFilePath == NULL) {
        throw ParamException ("pszFilePath == NULL");
    }
    if (pszKeyLabel == NULL) {
        throw ParamException ("pszKeyLabel == NULL");
    }

    // Open input file
    FILE *fileInput;
    if (NULL == (fileInput = fopen (pszFilePath, "r"))) {
        char szBuf[1024];
        sprintf (szBuf, "failed to open file <%s>; error = %s", pszFilePath, strerror (errno));
        throw IOException(szBuf);
    }

    // Create reader
    FileReader fr (fileInput, true);
    PushbackLineOrientedReader pblor (&fr);

    // Read entries until EOF or other problem
    StringStringHashtable *pssh = NULL;
    while (1) {
        try {
            pssh = new StringStringHashtable (false);      // false -> case-insensitive comparison of keys
            readEntry (pssh, &pblor);
            if (NULL == pssh->get(pszKeyLabel)) {
                delete pssh;
                pssh = NULL;
                char szBuf[1024];
                sprintf (szBuf, "did not find key <%s> while reading file", (const char*) pszKeyLabel);
                throw FormatException (szBuf);
            }
            const char * pszKeyValue = pssh->get (pszKeyLabel);
            pshData->put (pszKeyValue, pssh);
        }
        catch (EOFException) {
            break;
        }
        catch (Exception e) {
            delete pssh;
            pssh = NULL;
            throw e;
        }
    }
    delete pssh;
    pssh = NULL;
}

void SettingsFileManager::writeFile (StringHashtable<StringStringHashtable> *pshData, const char *pszFilePath, const char *pszKeyLabel)
{
    // Check parameters
    if (pshData == NULL) {
        throw ParamException ("psvpmData == NULL");
    }
    if (pszFilePath == NULL) {
        throw ParamException ("pszFilePath == NULL");
    }
    if (pszKeyLabel == NULL) {
        throw ParamException ("pszKeyLabel == NULL");
    }

    FILE *fileOutput;
    if (NULL == (fileOutput = fopen (pszFilePath, "w"))) {
        char szBuf[1024];
        sprintf (szBuf, "failed to open file <%s>; error = %s", pszFilePath, strerror (errno));
        throw IOException(szBuf);
    }

    StringHashtable<StringStringHashtable>::Iterator it = pshData->getAllElements();
    while (!it.end()) {
        const char *pszKey = it.getKey();
        StringStringHashtable *pssh = it.getValue();
        fprintf (fileOutput, "[Entry]\n");
        fprintf (fileOutput, "%s=%s\n", pszKeyLabel, pszKey);

        StringStringHashtable::Iterator it2 = pssh->getAllElements();
        while (!it2.end()) {
            fprintf (fileOutput, "%s=%s\n", it2.getKey(), it2.getValue());
            it2.nextElement();
        }
        it.nextElement();
    }

    fclose (fileOutput);
}

void SettingsFileManager::readEntry (StringStringHashtable *psshEntry, PushbackLineOrientedReader *pReader)
{
    int rc;
    char szBuf[1024];

    // Read "[Entry]"
    while (1) {
        if ((rc = pReader->readLine(szBuf, sizeof(szBuf))) < 0) {
            throw EOFException();
        }
        else if (rc == 0) {
            // blank line - skip over
        }
        else {
            if (0 != stricmp (szBuf, "[Entry]")) {
                // Error - unexpected data read
                char szErrorBuf[2048];
                sprintf (szErrorBuf, "error - expecting <[Entry]>, got <%s>", szBuf);
                throw FormatException (szErrorBuf);
            }
            else {
                break;
            }
        }
    }

    // Read data until the next "[Entry]"
    while (1) {
        if ((rc = pReader->readLine(szBuf, sizeof(szBuf))) < 0) {
            // Since we started reading an entry - return with what we have
            break;
        }
        else if (rc == 0) {
            // blank line - skip over
        }
        else {
            if (0 == stricmp (szBuf, "[Entry]")) {
                // Encountered another [Entry] - push the line back and return the current entry
                pReader->pushbackLine (szBuf);
                break;
            }
            else {
                // Parse and add the line to the entry
                char *pszValue = strchr (szBuf, '=');
                if (pszValue == NULL) {
                    // Did not find the expected = sign
                    char szErrorBuf[2048];
                    sprintf (szErrorBuf, "error did not find = while parsing <%s>", szBuf);
                    throw FormatException (szErrorBuf);
                }
                *pszValue++ = '\0';
                psshEntry->put (szBuf, pszValue);
            }
        }
    }
}
