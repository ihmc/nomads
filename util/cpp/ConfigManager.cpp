/*
 * ConfigManager.cpp
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

#include "ConfigManager.h"
#include "Exceptions.h"

#include "NLFLib.h"
#include "Writer.h"
#include "FileWriter.h"
#include "LineOrientedReader.h"
#include "DArray2.h"
#include "StrClass.h"
#include "FileReader.h"
#include "FileUtils.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#if defined (UNIX)
    #define stricmp strcasecmp
    #include <limits.h>
#endif

using namespace NOMADSUtil;

ConfigManager::ConfigManager (void)
    : _iMaxLineLen (0),
      // By default bCaseSensitiveKeys, bCloneKeys, bDeleteKeys, bCloneValues and
       // bDeleteValues are all set to "true"
      _pSettings (new StringStringHashtable()),
      _pszConfigFile (NULL)
{
}

ConfigManager::~ConfigManager (void)
{
    delete _pSettings;
    _pSettings = NULL;
    if (_pszConfigFile != NULL) {
        free (_pszConfigFile);
        _pszConfigFile = NULL;
    }
}

int ConfigManager::init (int iMaxLineLen)
{
    _iMaxLineLen = iMaxLineLen;
    return 0;
}

StringStringHashtable * ConfigManager::getProperties (void)
{
    return _pSettings;
}

void ConfigManager::display (void)
{
    printf ("ConfigManager:\n");
    if (_pSettings == NULL) {
        return;
    }
    StringStringHashtable::Iterator iter = _pSettings->getAllElements();
    while (!iter.end()) {
        const char *pszKey = iter.getKey();
        if (pszKey == NULL) {
            pszKey = "NULL";
        }
        const char *pszValue = iter.getValue();
        if (pszValue == NULL) {
            pszValue = "NULL";
        }
        printf ("%s=%s\n", pszKey, pszValue);
        iter.nextElement();
    }
}

int ConfigManager::write (Writer *pWriter)
{
    static char separator = '=';
    static char endOfLine = '\n';
    int rc = 0;
    StringStringHashtable::Iterator it = _pSettings->getAllElements();
    while (!it.end()) {
        const char *pszVal = it.getKey();
        if ((rc = pWriter->writeBytes (pszVal, (unsigned long)strlen (pszVal))) < 0) {
            break;
        }
        if ((rc = pWriter->writeBytes (&separator, 1)) < 0) {
            break;
        }
        pszVal = it.getValue();
        if ((rc = pWriter->writeBytes (pszVal, (unsigned long)strlen (pszVal))) < 0) {
            break;
        }
        if ((rc = pWriter->writeBytes (&endOfLine, 1)) < 0) {
            break;
        }
        it.nextElement();
    }
    return rc;
}

void ConfigManager::writeConfigFile (const char *pszFilePath)
{
    if (pszFilePath == NULL) {
    	throw ParamException ("pszFilePath == NULL");
    }

    FILE *pFileOutput;
    if (NULL == (pFileOutput = fopen (pszFilePath, "w"))) {
        char szBuf[1024];
        sprintf (szBuf, "failed to open file <%s>; error = %s", pszFilePath, strerror (errno));
        throw IOException (szBuf);
    }

    FileWriter fw (pFileOutput);
    write (&fw);
    fw.flush();
}

void ConfigManager::writeConfigFile (void)
{
    char szBuf[PATH_MAX];

    if (_pszConfigFile == NULL) {
        sprintf (szBuf, "failed to write Config file as no filename set in _pszConfigFile");
       	throw IOException (szBuf);
    }
    writeConfigFile (_pszConfigFile);
}

int ConfigManager::addProperties (ConfigManager &cfgMgr)
{
    for (StringStringHashtable::Iterator iter = cfgMgr.getAllElements(); !iter.end(); iter.nextElement()) {
        if (hasValue (iter.getKey())) {
            const char *pszCurrValue = getValue (iter.getKey());
            const char *pszNewValue = iter.getValue();
            if (pszCurrValue == NULL) {
                if (pszNewValue != NULL) {
                    return -1;
                }
            }
            else if (pszNewValue == NULL) {
                return -2;
            }
            else if (stricmp (pszCurrValue, pszNewValue) != 0) {
                return -3;
            }
            // no need to add it again
        }
        else {
            setValue (iter.getKey(), iter.getValue());
        }
    }
    return 0;
}

int ConfigManager::read (Reader *pReader, uint32 ui32Len, bool bBeTolerant)
{
    if (_iMaxLineLen == 0) {
        // init() has not been called
        return -1;
    }
    char *pszBuf = new char [_iMaxLineLen];
    if (pszBuf == NULL) {
        // memory exhausted
        return -2;
    }

    uint32 ui32BytesRead = 0;
    int iLen = 0;
    int intLineCounter = -1;
    LineOrientedReader lr (pReader, false);
    while (ui32BytesRead < ui32Len) {
        int iLen = lr.readLine (pszBuf, _iMaxLineLen);
        if (iLen < 0) {
            break;
        }
        ++intLineCounter;
        ui32BytesRead += iLen;

        if (iLen == 0) {
            // Empty line
            if (bBeTolerant) {
                continue;
            }
            else {
                delete[] pszBuf;
                return -4;
            }
        }

        if (pszBuf[0] == '#') {
            continue;
        }

        if (iLen == 1) {
            delete[] pszBuf;
            return -5;
        }

        if (pszBuf[iLen-1] != '\n') {
            if (((uint32)iLen) > ui32Len) {
                // Did not find \n so the buffer was not big enough!
                delete[] pszBuf;
                return -6;
            }
        }

        char *pszSeparator;
        if (NULL == (pszSeparator = strchr (pszBuf, '='))) {
            if (bBeTolerant) {
                continue;
            }
            else {
                delete[] pszBuf;
                return -7;
            }
        }
        char *pszKey = pszBuf;
        while ((*pszKey == ' ') || (*pszKey == '\t')) {
            *pszKey++;
        }
        if (*pszKey == '=') {
            // Key field was empty!
            if (bBeTolerant) {
                continue;
            }
            else {
                delete[] pszBuf;
                return -8;
            }
        }
        *pszSeparator = '\0';
        char *pszKeyEnd = pszSeparator-1;
        //skip leading spaces or tabs
        while ((*pszKeyEnd == ' ') || (*pszKeyEnd == '\t')) {
            *pszKeyEnd-- = '\0';
        }
        // Key is in pszKey

        char *pszValue = pszSeparator+1;
        while ((*pszValue == ' ') || (*pszValue == '\t')) {
            pszValue++;
        }
        if (*pszValue == '\0') {
            // Value field was empty!
        }
        char *pszValueEnd = pszValue + strlen (pszValue) - 1;
        //skip trailing spaces or tabs
        while ((*pszValueEnd == ' ') || (*pszValueEnd == '\t')) {
            *pszValueEnd-- = '\0';
        }

        // Value is in pszValue
        _pSettings->put (pszKey, pszValue);
    }

    if (iLen >= 0) {
        return 0;
    }
    // Return the error code
    return iLen;
}

int ConfigManager::readConfigFile (const char *pszFile, bool bBeTolerant)
{
    if (_iMaxLineLen == 0) {
        // init() has not been called
        return -1;
    }
    if (pszFile == NULL) {
        return -2;
    }
    const int64 i64FileSize = FileUtils::fileSize (pszFile);
    if (i64FileSize < 0) {
        return -3;
    }
    if (i64FileSize > 0xFFFFFFFF) {
        return -4;
    }
    FileReader reader (pszFile, "r");
    int rc = read (&reader, (uint32) i64FileSize, bBeTolerant);
    if (rc >= 0) {
        if (_pszConfigFile) {
            free (_pszConfigFile);
            _pszConfigFile = NULL;
        }
        _pszConfigFile = strDup (pszFile);
    }
    return rc;
}

int ConfigManager::performVariableSubstitution (void)
{
    StringStringHashtable *pOriginalTable = _pSettings;
    _pSettings = new StringStringHashtable();
    for (StringStringHashtable::Iterator i = pOriginalTable->getAllElements(); !i.end(); i.nextElement()) {
        const char *pszKey = i.getKey();
        const char *pszValue = i.getValue();
        if (pszValue[0] == '$') {
            const char *pszNewValue = pOriginalTable->get (pszValue+1);
            if (pszNewValue != NULL) {
                _pSettings->put (pszKey, (char*)pszNewValue);   // Value will be cloned anyway - no problem with (char*) cast
            }
            else {
                _pSettings->put (pszKey, (char*)pszValue);      // Value will be cloned anyway - no problem with (char*) cast
            }
        }
        else {
            _pSettings->put (pszKey, (char*)pszValue);          // Value will be cloned anyway - no problem with (char*) cast
        }
    }
    delete pOriginalTable;
    return 0;
}

void ConfigManager::setValue (const char *pszKey, const char *pszValue)
{
    _pSettings->put (pszKey, (char *) pszValue);
}

void ConfigManager::setValue (const char *pszKey, int iValue)
{
    char szBuf[20];
    sprintf (szBuf, "%d", iValue);
    _pSettings->put (pszKey, szBuf);
}

const char * ConfigManager::removeValue (const char *pszKey) const
{
    return _pSettings->remove (pszKey);
}

const char * ConfigManager::getValue (const char *pszKey) const
{
    return _pSettings->get (pszKey);
}

int ConfigManager::getValueAsInt (const char *pszKey) const
{
    const char *pszValue = getValue (pszKey);
    if (pszValue) {
        return atoi (pszValue);
    }
    return 0;
}

int64 ConfigManager::getValueAsInt64 (const char *pszKey) const
{
    const char *pszValue = getValue (pszKey);
    if (pszValue) {
        return atoi64 (pszValue);
    }
    return 0;
}

uint32 ConfigManager::getValueAsUInt32 (const char *pszKey) const
{
    const char *pszValue = getValue (pszKey);
    if (pszValue) {
        return atoui32 (pszValue);
    }
    return 0;
}

uint64 ConfigManager::getValueAsUInt64 (const char *pszKey) const
{
    const char *pszValue = getValue (pszKey);
    if (pszValue) {
        return atoui64 (pszValue);
    }
    return 0;
}

double ConfigManager::getValueAsDouble (const char *pszKey) const
{
    const char *pszValue = getValue (pszKey);
    if (pszValue) {
        return atof (pszValue);
    }
    return 0.0f;
}

float ConfigManager::getValueAsFloat (const char *pszKey) const
{
    const char *pszValue = getValue (pszKey);
    if (pszValue) {
        return (float) atof (pszValue);
    }
    return 0.0f;
}

const char * ConfigManager::getValue (const char *pszKey, const char *pszDefault) const
{
    if (hasValue (pszKey)) {
        return getValue (pszKey);
    }
    else {
        _pSettings->put (pszKey, (char *) pszDefault);
        return pszDefault;
    }
}

int ConfigManager::getValueAsInt (const char *pszKey, int iDefault) const
{
    if (hasValue (pszKey)) {
        return getValueAsInt(pszKey);
    }
    else {
        char szBuf[20];
        sprintf (szBuf, "%d", iDefault);
        _pSettings->put (pszKey, szBuf);
        return iDefault;
    }
}

uint32 ConfigManager::getValueAsUInt32 (const char *pszKey, uint32 ui32Default) const
{
    if (hasValue (pszKey)) {
        return getValueAsUInt32 (pszKey);
    }
    else {
        char szBuf[22];
        i64toa (szBuf, (int64) ui32Default);
        _pSettings->put (pszKey, szBuf);
        return ui32Default;
    }
}

int64 ConfigManager::getValueAsInt64 (const char *pszKey, int64 i64Default) const
{
    if (hasValue (pszKey)) {
        return getValueAsInt64 (pszKey);
    }
    else {
        char szBuf[22];
        i64toa (szBuf, i64Default);
        _pSettings->put (pszKey, szBuf);
        return i64Default;
    }
}

bool ConfigManager::getValueAsBool (const char *pszKey, bool bDefault) const
{
    if (hasValue (pszKey)) {
        return getValueAsBool (pszKey);
    }
    else {
        _pSettings->put (pszKey, (bDefault ? (char *) "true" : (char *) "false"));
        return bDefault;
    }
}

bool ConfigManager::hasValue (const char *pszKey) const
{
    return (_pSettings->get (pszKey) != NULL);
}

bool ConfigManager::getValueAsBool (const char *pszKey) const
{
    return parseBool (getValue (pszKey));
}

StringStringHashtable::Iterator ConfigManager::getAllElements()
{
    return _pSettings->getAllElements();
}

char * ConfigManager::getDefaultConfigFileDirectory (const char *pszHomeDir)
{
    // Fist, check in directories placed relatively to the location of the executable file
    if (pszHomeDir != NULL) {
        String configFilePath (pszHomeDir);
        configFilePath += getPathSepCharAsString();
        configFilePath += "conf";

        if (FileUtils::directoryExists (configFilePath)) {
            return configFilePath.r_str();
        }
    }

    // Second, check absolute paths
    DArray2<String> defaultPaths;

    #ifdef ANDROID
        defaultPaths[0] = getPathSepCharAsString();
        defaultPaths[0] += "sdcard";
        defaultPaths[0] += getPathSepCharAsString();
        defaultPaths[0] += "ihmc";
        defaultPaths[0] += getPathSepCharAsString();
        defaultPaths[0] += "conf";
        defaultPaths[0] += getPathSepCharAsString();

        defaultPaths[1] = getPathSepCharAsString();
        defaultPaths[1] += "sdcard";
        defaultPaths[1] += getPathSepCharAsString();
        defaultPaths[1] += "external_sd";
        defaultPaths[1] += getPathSepCharAsString();
        defaultPaths[1] += "ihmc";
        defaultPaths[1] += getPathSepCharAsString();
        defaultPaths[1] += "conf";
        defaultPaths[1] += getPathSepCharAsString();
    #elif defined WIN32
        defaultPaths[0] += "C:\\ihmc";
        defaultPaths[0] += getPathSepCharAsString();
        defaultPaths[0] += "conf";
        defaultPaths[0] += getPathSepCharAsString();
    #else
        // Linux or OSX
        defaultPaths[0] = getenv ("HOME");
        defaultPaths[0] += getPathSepCharAsString();
        defaultPaths[0] += "ihmc";
        defaultPaths[0] += getPathSepCharAsString();
        defaultPaths[0] += "conf";
        defaultPaths[0] += getPathSepCharAsString();
    #endif

    for (unsigned int i = 0; i < defaultPaths.size(); i++) {
        if (defaultPaths.used (i)) {
            if (FileUtils::directoryExists (defaultPaths[i].c_str())) {
                return defaultPaths[i].r_str();
            }
        }
    }
    return NULL;
}

char * ConfigManager::getDefaultConfigFilePath (const char *pszHomeDir, const char *pszConfigFileName)
{
    if (pszConfigFileName == NULL) {
        return NULL;
    }

    char *pszDir = getDefaultConfigFileDirectory (pszHomeDir);
    if (pszDir == NULL) {
        return NULL;
    }
    String pathToFile (pszDir);
    free (pszDir);
    if (!pathToFile.endsWith (getPathSepCharAsString())) {
        pathToFile += getPathSepCharAsString();
    }
    pszDir = NULL;
    pathToFile += pszConfigFileName;
    if (FileUtils::fileExists (pathToFile.c_str())) {
        return pathToFile.r_str();
    }

    return NULL;
}

bool ConfigManager::parseBool (const char *pszValue)
{
    if (pszValue) {
        if (0 == stricmp (pszValue, "yes")) {
            return true;
        }
        else if (0 == stricmp (pszValue, "true")) {
            return true;
        }
        else if (0 == stricmp (pszValue, "on")) {
            return true;
        }
        else if (0 == strcmp (pszValue, "1")) {
            return true;
        }
    }
    return false;
}

