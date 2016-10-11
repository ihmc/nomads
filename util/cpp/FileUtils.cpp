/*
 * FileUtils.cpp
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

#include "FileUtils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include  <limits.h>

#if defined (WIN32)
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
    #define _mkdir(x) mkdir(x, 0755)
    #if defined (OSX)
        #include <sys/syslimits.h>
    #endif
#else
    #error Must Define WIN32 or UNIX!
#endif

#include "DArray2.h"
#include "FileWriter.h"
#include "Logger.h"
#include "NLFLib.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg


using namespace NOMADSUtil;

bool FileUtils::createDirectory (const char *pszPath)
{
    char pszDirPath[PATH_MAX], pszSubDirName[PATH_MAX];
    char *pszStr, *pszTmp;
    strcpy (pszDirPath, pszPath);
    if (pszDirPath[strlen(pszPath)-1] == getPathSepChar()) {
        pszDirPath[strlen(pszPath)-1] = '\0';
    }

    pszStr = strtok_mt (pszDirPath, getPathSepCharAsString(), &pszTmp);

    #ifndef WIN32
        _mkdir (pszDirPath);
    #endif
    while ((pszStr = strtok_mt (NULL, getPathSepCharAsString(), &pszTmp)) != NULL) {
        strcpy (pszSubDirName, pszStr);
        strcat (pszDirPath, getPathSepCharAsString());
        strcat (pszDirPath, pszSubDirName);
        _mkdir (pszDirPath);
    }
        
    // Check it the directory has beed created successfully
    return directoryExists (pszDirPath);
}

bool FileUtils::deleteDirectory (const char *pszPath) 
{
#if defined (WIN32)

   HANDLE hFind;    // file handle
   WIN32_FIND_DATA FindFileData;
   TCHAR DirPath[PATH_MAX];
   TCHAR FileName[PATH_MAX];

   strcpy (DirPath, pszPath);
   strcat (DirPath, "\\*");    // searching all files
   strcpy (FileName, pszPath);
   strcat (FileName, "\\");

   // find the first file
   hFind = FindFirstFile (DirPath,&FindFileData);
   if (hFind == INVALID_HANDLE_VALUE) {
       return false;
   }
   strcpy (DirPath,FileName);

   bool bSearch = true;
   
    while (bSearch) {    // until we find an entry
        if (FindNextFile (hFind,&FindFileData)) {
            if ((strcmp (FindFileData.cFileName, ".") == 0) ||
                (strcmp (FindFileData.cFileName, "..") == 0)) {
                continue;
                }
            strcat (FileName,FindFileData.cFileName);
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // we have found a directory, recurse
                if (!deleteDirectory (FileName)) {
                    FindClose (hFind);
                    return false;    // directory couldn't be deleted
                }
                // remove the empty directory
                RemoveDirectory (FileName);
                strcpy (FileName,DirPath);
            }
            else {
                if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                    // change read-only file mode
                    _chmod(FileName, _S_IWRITE);
                    if (!DeleteFile (FileName)) {    // delete the file
                        FindClose(hFind);
                        return false;
                    }
                strcpy (FileName,DirPath);
            }
        }
        else {
             // no more files there
            if (GetLastError() == ERROR_NO_MORE_FILES)
            bSearch = false;
            else {
                // some error occurred; close the handle and return FALSE
                FindClose (hFind);
                return false;
            }
        }

    }
    FindClose (hFind);
    return (RemoveDirectory (pszPath) == TRUE);     // remove the empty directory

#elif defined (UNIX)
    
    String currentDirPath = String (pszPath); 
    struct dirent **eps;
    struct stat sb;
    int n = 0;
    bool bTrailingSlash = (pszPath[strlen (pszPath) - 1] == '/');
    
    #if !defined (ANDROID)
        if ((n = scandir ((const char*) currentDirPath, &eps, NULL, alphasort)) < 0) {
            return false;
        }
    #endif
    
    for (int i = 0; i < n; i++) {
        if ((strcmp (".", eps[i]->d_name) == 0) || (strcmp ("..", eps[i]->d_name) == 0)) {  
            continue;
        }
        String currentFilePath = currentDirPath;
        if (!bTrailingSlash) {
            currentFilePath += "/";
        }
        currentFilePath += eps[i]->d_name;
        
        if (lstat ((const char*) currentFilePath, &sb) != 0) {
            perror ((const char*) currentFilePath);
            continue;
        }  
        
        if (S_ISDIR (sb.st_mode)) {
            if (!deleteDirectory ((const char*) currentFilePath)) {
                for (int i = 0; i < n; i++) {
                    free (eps[i]);
                }
                if(n > 0) {
                    free (eps);
                }
                return false;
            }
        } 
        else if (S_ISREG (sb.st_mode)) {
            if (unlink ((const char*) currentFilePath) != 0) {
                return false;
            }
        }

        free (eps[i]);
    }

    free (eps);

    return (rmdir ((const char*) currentDirPath) == 0);

#endif
}

bool FileUtils::directoryExists (const char *pszPath)
{
    struct stat statinfo;
    if ((stat (pszPath, &statinfo) == -1) || ((statinfo.st_mode & S_IFMT) != S_IFDIR)) {
        return false;
    }

    return true;
}

char ** FileUtils::listFilesInDirectory (const char *pszPath, bool bIncludeDirs)
{
    if (pszPath == NULL) {
        return NULL;
    }

    DArray2<String> files;

#if defined (WIN32)
    WIN32_FIND_DATA ffd;
    DWORD dwError=0;

    TCHAR szDir[MAX_PATH];
    strcpy (szDir, pszPath);
    strcat (szDir, "\\*");    // searching all files

    HANDLE hFind = FindFirstFile (szDir, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    unsigned int idx = 0;
    do {
        if (bIncludeDirs || !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            files[idx] = ffd.cFileName;
            idx++;
        }
    }
    while (FindNextFile(hFind, &ffd) != 0);
 
    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES) {
        FindClose (hFind);
    }

    FindClose (hFind);

#elif defined (UNIX)
    DIR *pDir = opendir (pszPath);
    if (pDir == NULL) {
        return NULL;
    }
    
    struct dirent *pDe = readdir (pDir);
    for (unsigned int i = 0; pDe != NULL; pDe = readdir (pDir)) {
        String fullName (pszPath);
        if (!fullName.endsWith ("/")) {
            fullName += "/";
        }
        fullName += pDe->d_name;
        if (fileExists (fullName.c_str()) || (bIncludeDirs && directoryExists (fullName.c_str()))) {
            files[i] = fullName.c_str();
            i++;
        }
    }

    closedir (pDir);
#endif

    if (files.size() == 0) {
        return NULL;
    }

    char **ppFileList = (char **) calloc (files.size()+1, sizeof (char*));
    if (ppFileList == NULL) {
        return NULL;
    }

    for (unsigned int i = 0; i < files.size(); i++) {
        ppFileList[i] = files[i].r_str();
    }

    return ppFileList;
}
            
String * FileUtils::enumSubdirs (const char *pszPath)
{
#if defined (WIN32)

    HANDLE hFind;    // file handle
    WIN32_FIND_DATA FindFileData;

    TCHAR DirPath[PATH_MAX];
    strcpy (DirPath, pszPath);
    strcat (DirPath, "\\*");    // searching all files

    // find the first file
    hFind = FindFirstFile (DirPath, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    // checks how many subdirectories are present
    bool bSearch = true;
    int j = 0;
    while (bSearch) {    // until we find an entry
        if (FindNextFile (hFind, &FindFileData)) {
            if ((strcmp (FindFileData.cFileName, ".") == 0) ||
                (strcmp (FindFileData.cFileName, "..") == 0)) {
                continue;
            }
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                j++;    
            }
        }        
        else {
            // no more files there
            if (GetLastError() == ERROR_NO_MORE_FILES) {
                bSearch = NULL;
            }
            else {
                // some error occurred
                FindClose (hFind);
                return NULL;
            }
        }
    }

    // store the subdirectories names
    hFind = FindFirstFile (DirPath, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    String *pSubDirs = new String[j+1];
    bSearch = true;
    j = 0;
    while (bSearch) {    // until we find an entry
        if (FindNextFile (hFind, &FindFileData)) {
            if ((strcmp (FindFileData.cFileName, ".") == 0) ||
                (strcmp (FindFileData.cFileName, "..") == 0)) {
                continue;
            }
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                pSubDirs[j++] = FindFileData.cFileName;
            }
        }        
        else {
            // no more files there
            if (GetLastError() == ERROR_NO_MORE_FILES) {
                bSearch = false;
            }
            else {
                // some error occurred
                FindClose (hFind);
                return NULL;
            }
        }
    }

    pSubDirs[j] = "";

    FindClose (hFind);  // close the file handle           
    return pSubDirs;

#elif defined (UNIX)

    struct dirent **eps;
    struct stat sb;
    int n = 0;
    char pszCurrentFilePath[PATH_MAX];
    bool bTrailingSlash = (pszPath[strlen (pszPath) - 1] == '/');

    #if !defined (ANDROID)
        if ((n = scandir (pszPath, &eps, NULL, alphasort)) < 0) {
            return NULL;
        }
    #endif

    String *pSubDirs = new String[n];
    int j = 0;
    for (int i = 0; i < n; i++) {
        if ((strcmp (".", eps[i]->d_name) == 0) || 
            (strcmp ("..", eps[i]->d_name) == 0)) {
            continue;
        }
        strcpy (pszCurrentFilePath, pszPath);
        if (!bTrailingSlash) {
            strcat (pszCurrentFilePath, "/");
        }
        strcat (pszCurrentFilePath, eps[i]->d_name);

        if (lstat (pszCurrentFilePath, &sb) != 0) {
            perror (pszCurrentFilePath);
            continue;
        }

        if (S_ISDIR (sb.st_mode)) {
            pSubDirs[j++] = eps[i]->d_name;
        }

        free (eps[i]);
    }
    pSubDirs[j] = "";

    free (eps);

    return pSubDirs;

#endif
}

void FileUtils::transformPathSeparator (char *pszPath)
{
    #if defined (WIN32)
        char platSep = '\\';
        char otherPlatSep = '/';
    #elif defined (UNIX) 
        char platSep = '/';
        char otherPlatSep = '\\';
    #else
        #error 'Sorry. Must define WIN32 or UNIX'
    #endif

    int iLen = (int) strlen(pszPath);
    for (int i = 0; i < iLen; i++) {
        if (pszPath[i] == otherPlatSep) {
            pszPath[i] = platSep;
        }
    }
}

void FileUtils::createDirStructure (const char *pszPath)
{
    char pathSep = getPathSepChar();

    if (pszPath[strlen(pszPath) - 1] == pathSep) {
        //if it is a directory
        //printf("invoking createDirectory (1) with [%s]\n", pszPath);
        createDirectory (pszPath);
    }
    else {
        for (int i = (int) strlen(pszPath) - 2; i > 0; i--) {
            if (pszPath[i] == pathSep) {
                char* pszDirNameAux = (char*) malloc (i + 1);
                strncpy (pszDirNameAux, pszPath, i);
                pszDirNameAux[i] = 0;
                //printf("invoking createDirectory (2) with [%s]\n", pszDirNameAux);
                createDirectory (pszDirNameAux);
                free (pszDirNameAux);
                return;
            }
        }
    }
}

int FileUtils::deleteFile (const char *pszPath)
{
    if (pszPath == NULL) {
        return -1;
    }
    if (!fileExists (pszPath)) {
        return 1;
    }
    if (remove (pszPath) < 0) {
        return -2;
    }
    return 0;
}

int FileUtils::dumpBufferToFile (const void *pBuf, uint64 ui64BufLen, const char *pszPath)
{
    if (pszPath == NULL) {
        return -1;
    }
    FILE *pFile = fopen (pszPath, "wb");
    if (pFile == NULL) {
        return -2;
    }
    const unsigned char *ptmpbuf = static_cast<const unsigned char *>(pBuf);
    FileWriter fw (pFile, true);
    for (uint64 ui64Written = 0U; ui64Written < ui64BufLen;) {
        unsigned int uiBytesToWrite = static_cast<unsigned int>(NOMADSUtil::minimum ((uint64) UINT_MAX, ui64BufLen));
        if (fw.writeBytes (ptmpbuf, uiBytesToWrite) < 0) {
            return -3;
        }
        ptmpbuf += uiBytesToWrite;
        ui64BufLen -= uiBytesToWrite;
    }
    return 0;
}

bool FileUtils::fileExists (const char *pszPath)
{
    if (pszPath == NULL) {
        return false;
    }
    struct stat statinfo;
    if ((stat (pszPath, &statinfo) == -1) || ((statinfo.st_mode & S_IFMT) != S_IFREG)) {
        return false;
    }

    return true;
}

int64 FileUtils::fileSize (const char *pszPath)
{
    struct stat statinfo;
    if ((stat (pszPath, &statinfo) == -1) || ((statinfo.st_mode & S_IFMT) != S_IFREG)) {
        return -1;
    }

    return statinfo.st_size;
}

void * FileUtils::readFile (const char *pszPath, int64 *pi64FileSize)
{
    FILE *pFile = fopen (pszPath, "rb");
    if (pFile == NULL) {
        return NULL;
    }
    int64 i64FileSize = fileSize (pszPath);
    if (i64FileSize < 0) {
        fclose (pFile);
        return NULL;
    }
    void *pBuf = malloc (i64FileSize);
    if (pBuf == NULL) {
        fclose (pFile);
        return NULL;
    }
    if (i64FileSize != fread (pBuf, 1, i64FileSize, pFile)) {
        fclose (pFile);
        free (pBuf);
        return NULL;
    }
    if (pi64FileSize) {
        *pi64FileSize = i64FileSize;
    }
    fclose (pFile);
    return pBuf;
}
