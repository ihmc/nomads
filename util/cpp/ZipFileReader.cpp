/*
 * ZipFileReader.cpp
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

#include "ZipFileReader.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

using namespace NOMADSUtil;

ZipFileReader::ZipFileReader (bool bCloseFile)
    : _bCloseFile (bCloseFile)
{
    _pFileInput = NULL;
    lLastFileHeaderSeekPos = 0;
    pEntries = NULL;
}

ZipFileReader::~ZipFileReader (void)
{
    if (_pFileInput && _bCloseFile) {
        fclose (_pFileInput);
        _pFileInput = NULL;
    }
    if (pEntries) {
        delete[] pEntries;
        pEntries = NULL;
    }
}

int ZipFileReader::init (const char *pszFilename, bool bCacheDir)
{
    FILE *pInput = fopen (pszFilename, "rb");
    if (NULL == pInput) {
        return RC_FileAccessError;
    }
    return init (pInput, bCacheDir);
}

int ZipFileReader::init (FILE *Input, bool bCacheDir)
{
    if (Input == NULL) {
        return RC_FileAccessError;
    }
    _pFileInput = Input;
    u4 u4Count = 0;
    u4 u4Index = 0;
    bool bDone = false;
    while (!bDone) {
        switch (readSignature (_pFileInput)) {
            case T_LocalFileHeader:
                //if (lfh.skip (fileInput)) {
                if (lfh.read (_pFileInput)) {
                    return RC_FileFormatError;
                }
                if (fseek (_pFileInput, lfh.u4BytesToSkipToNextHeader, SEEK_CUR)) {
                    return RC_FileFormatError;
                }
                u4Count++;
                break;

            case T_FileHeader:
                if (bCacheDir) {
                    if (u4Index >= u4Count) {
                        return RC_FileFormatError;
                    }
                    iEntryArraySize = u4Count;
                    if (pEntries == NULL) {
                        if (NULL == (pEntries = new FileEntry[u4Count])) {
                            return RC_FileFormatError;     // Should not be running out of memory
                        }
                    }
                    if (fh.read (_pFileInput)) {
                        return RC_FileFormatError;
                    }
                    if (pEntries[u4Index].init (fh)) {    // For efficiency, FileEntry::init steals the
                                                          //     name, comment, and extra fields that are
                                                          //     allocated in fh
                                                          // fh is automatically modified so that it will
                                                          //     not delete the fields
                        return RC_FileFormatError;
                    }
                    u4Index++;
                }
                else {
                    if (fh.skip (_pFileInput)) {
                        return RC_FileFormatError;
                    }
                }
                break;

            case T_EndOfCentralDir:
                if (ecd.read (_pFileInput)) {
                    return RC_FileFormatError;
                }
                if (bCacheDir) {
                    if (u4Index < u4Count) {
                        return RC_FileFormatError;
                    }
                }
                bDone = true;
                break;

            default:
                return RC_FileFormatError;
        }
    }
    if (bCacheDir) {
        qsort (pEntries, u4Index, sizeof (FileEntry), FileEntry::compare);
    }
    return RC_Ok;
}

int ZipFileReader::getFileCount (void)
{
    return ecd.u2CentralDirCount;
}

const char * ZipFileReader::getFirstFileName (void)
{
    if (fseek (_pFileInput, ecd.u4CentralDirOffset, SEEK_SET)) {
        return NULL;
    }
    if (T_FileHeader != readSignature (_pFileInput)) {
        return NULL;
    }
    if (fh.read (_pFileInput)) {
        return NULL;
    }
    lLastFileHeaderSeekPos = ftell (_pFileInput);
    return fh.pszFileName;
}

const char * ZipFileReader::getNextFileName (void)
{
    if (fseek (_pFileInput, lLastFileHeaderSeekPos, SEEK_SET)) {
        return NULL;
    }
    if (T_FileHeader != readSignature (_pFileInput)) {
        return NULL;
    }
    if (fh.read (_pFileInput)) {
        return NULL;
    }
    lLastFileHeaderSeekPos = ftell (_pFileInput);
    return fh.pszFileName;
}

bool ZipFileReader::checkForFile (const char *pszFileName)
{
    if (pEntries) {
        if (NULL == findFileEntry (pszFileName)) {
            return false;
        }
    }
    else {
        if (findFileHeader (pszFileName, _pFileInput)) {
            return false;
        }
    }
    return true;
}

ZipFileReader::Entry * ZipFileReader::getEntry (const char *pszName) const
{
    if (pEntries) {
        // We have a sorted list of directory entries - call fileFileEntry
        FileEntry *pfe;
        if (NULL == (pfe = ((ZipFileReader*)this)->findFileEntry (pszName))) {
            return NULL;
        }
        return ((ZipFileReader*)this)->getEntry (pfe, _pFileInput);
    }
    if (((ZipFileReader*)this)->findFileHeader (pszName, _pFileInput)) {
        return NULL;
    }
    return ((ZipFileReader*)this)->getEntry (&((ZipFileReader*)this)->fh, _pFileInput);
}

ZipFileReader::Entry * ZipFileReader::getEntry (int iIndex) const
{
    if ((iIndex < 0) || (iIndex >= ((ZipFileReader*)this)->getFileCount())) {
        return NULL;
    }
    if (pEntries) {
        // We have a sorted list of directory entries - call getFileEntry
        FileEntry *pfe;
        if (NULL == (pfe = ((ZipFileReader*)this)->getFileEntry (iIndex))) {
            return NULL;
        }
        return ((ZipFileReader*)this)->getEntry (pfe, _pFileInput);
    }
    if (((ZipFileReader*)this)->gotoFileHeader (iIndex, _pFileInput)) {
        return NULL;
    }
    return ((ZipFileReader*)this)->getEntry (&((ZipFileReader*)this)->fh, _pFileInput);
}

int ZipFileReader::readSignature (FILE *fileInput)
{
    u4 u4Sig;
    if (1 != fread (&u4Sig, sizeof (u4), 1, fileInput)) {
        return -1;
    }
    u4Sig = ZipFileReader::swapBytes (u4Sig);
    if (u4Sig == 0x04034b50) {
        return T_LocalFileHeader;
    }
    else if (u4Sig == 0x02014b50) {
        return T_FileHeader;
    }
    else if (u4Sig == 0x06054b50) {
        return T_EndOfCentralDir;
    }
    return -2;
}

int ZipFileReader::findFileHeader (const char *pszFile, FILE *fileInput)
{
    if (fseek (fileInput, ecd.u4CentralDirOffset, SEEK_SET)) {
        return -1;
    }
    while (1) {
        if (T_FileHeader != readSignature (fileInput)) {
            return -2;
        }
        if (fh.read (fileInput)) {
            return -3;
        }
        if (0 == strcmp (pszFile, fh.pszFileName)) {
            return 0;
        }
    }
}

int ZipFileReader::gotoFileHeader (int iIndex, FILE *fileInput)
{
    if ((iIndex < 0) || (iIndex >= getFileCount())) {
        return -1;
    }
    if (fseek (fileInput, ecd.u4CentralDirOffset, SEEK_SET)) {
        return -2;
    }
    for (int i = 0; i < iIndex; i++) {
        if (T_FileHeader != readSignature (fileInput)) {
            return -3;
        }
        if (fh.skip (fileInput)) {
            return -4;
        }
    }
    if (T_FileHeader != readSignature (fileInput)) {
        return -5;
    }
    if (fh.read (fileInput)) {
        return -6;
    }
    return 0;
}

ZipFileReader::FileEntry * ZipFileReader::binarySearch (const char *pszFile, int iBegin, int iEnd)
{
    if (iBegin > iEnd) {
        return NULL;
    }
    int iCompResult = 0;
    int iIndex = (iBegin + iEnd) / 2;
    iCompResult = strcmp (pszFile, pEntries[iIndex].pszName);
    if (iCompResult == 0) {
        return &pEntries[iIndex];
    }
    else if (iCompResult < 0) {
        return binarySearch (pszFile, iBegin, iIndex - 1);
    }
    else { // iCompResult > 0
        return binarySearch (pszFile, iIndex+1, iEnd);
    }
    return NULL;
}

ZipFileReader::FileEntry * ZipFileReader::findFileEntry (const char *pszFileName)
{
    return binarySearch (pszFileName, 0, iEntryArraySize-1);
}

ZipFileReader::FileEntry * ZipFileReader::getFileEntry (int iIndex)
{
    if ((iIndex < 0) || (iIndex >= iEntryArraySize)) {
        return NULL;
    }
    return &pEntries[iIndex];
}

ZipFileReader::Entry * ZipFileReader::getEntry (FileEntry *pfe, FILE *fileInput)
{
    // Seek to the start of the LocalFileHeader
    if (fseek (fileInput, pfe->u4Offset, SEEK_SET)) {
        return NULL;
    }

    // Read the local file header
    if (T_LocalFileHeader != ((ZipFileReader*)this)->readSignature (fileInput)) {
        return NULL;
    }
    if (((ZipFileReader*)this)->lfh.read (fileInput)) {
        return NULL;
    }

    // Create a new entry object
    Entry *pEntry = new Entry;

    // Fill in the meta information about the entry
    pEntry->ui32Offset = pfe->u4Offset;
    pEntry->ui16VersionMadeBy = pfe->u2VersionMadeBy;
    pEntry->ui16VersionNeeded = pfe->u2VersionNeeded;
    pEntry->ui16Flags = pfe->u2Flags;
    pEntry->compType = (Entry::CompressionType) pfe->u2CompMethod;
    pEntry->ui16LastModTime = pfe->u2LastModFileTime;
    pEntry->ui16LastModDate = pfe->u2LastModFileDate;
    pEntry->ui32CRC32 = pfe->u4CRC32;
    pEntry->ui32CompSize = pfe->u4CompSize;
    pEntry->ui32UncompSize = pfe->u4UncompSize;
    pEntry->ui16ExtraFieldLen = pfe->u2ExtraFieldLen;
    pEntry->pszName = new char [strlen(pfe->pszName)+1];
    strcpy (pEntry->pszName, pfe->pszName);
    if (pfe->u2ExtraFieldLen > 0) {
        pEntry->pExtraField = new char [pfe->u2ExtraFieldLen];
        memcpy (pEntry->pExtraField, pfe->pExtraField, pfe->u2ExtraFieldLen);
    }
    if (pfe->pszComment) {
        pEntry->pszComment = new char [strlen(pfe->pszComment)+1];
        strcpy (pEntry->pszComment, pfe->pszComment);
    }

    if (pEntry->ui32CompSize > 0) {
        // Read the data for the entry
        if (NULL == (pEntry->pBuf = new char [lfh.u4CompSize+1])) {      // Note that the allocated buffer size
            delete pEntry;                                               //     is one more than the actual size
            return NULL;                                                 // See the .h file for details
        }
        if (1 != fread (pEntry->pBuf, lfh.u4CompSize, 1, fileInput)) {
            delete pEntry;
            return NULL;
        }
    }
    else {
        pEntry->pBuf = NULL;
    }

    return pEntry;
}

ZipFileReader::Entry * ZipFileReader::getEntry (FileHeader *pfh, FILE *fileInput)
{
    // Seek to the start of the LocalFileHeader
    if (fseek (fileInput, pfh->i4LocalHdrRelOffset, SEEK_SET)) {
        return NULL;
    }

    // Read the local file header
    if (T_LocalFileHeader != ((ZipFileReader*)this)->readSignature (fileInput)) {
        return NULL;
    }
    if (((ZipFileReader*)this)->lfh.read (fileInput)) {
        return NULL;
    }

    // Create a new entry object
    Entry *pEntry = new Entry;

    // Fill in the meta information about the entry
    pEntry->ui32Offset = pfh->i4LocalHdrRelOffset;
    pEntry->ui16VersionMadeBy = pfh->u2VersionMadeBy;
    pEntry->ui16VersionNeeded = pfh->u2VersionNeeded;
    pEntry->ui16Flags = pfh->u2Flags;
    pEntry->compType = (Entry::CompressionType) pfh->u2CompMethod;
    pEntry->ui16LastModTime = pfh->u2LastModFileTime;
    pEntry->ui16LastModDate = pfh->u2LastModFileDate;
    pEntry->ui32CRC32 = pfh->u4CRC32;
    pEntry->ui32CompSize = pfh->u4CompSize;
    pEntry->ui32UncompSize = pfh->u4UncompSize;
    pEntry->ui16ExtraFieldLen = pfh->u2ExtraFieldLen;
    pEntry->pszName = pfh->pszFileName;
    pfh->pszFileName = NULL;
    pEntry->pExtraField = pfh->pExtraField;
    pfh->pExtraField = NULL;
    pEntry->pszComment = pfh->pszFileComment;
    pfh->pszFileComment = NULL;

    if (pEntry->ui32CompSize > 0) {
        // Read the data for the entry
        if (NULL == (pEntry->pBuf = new char [lfh.u4CompSize+1])) {      // Note that the allocated buffer size
            delete pEntry;                                               //     is one more than the actual size
            return NULL;                                                 // See the .h file for details
        }
        if (1 != fread (pEntry->pBuf, lfh.u4CompSize, 1, fileInput)) {
            delete pEntry;
            return NULL;
        }
    }
    else {
        pEntry->pBuf = NULL;
    }

    return pEntry;
}

int ZipFileReader::skipToEndOfSignature (u4 u4Signature, FILE *fileInput)
{
    u4Signature = swapBytes (u4Signature);
    unsigned char uchByte;
    int iMatchPos = 0;
    int iBytesSkipped = 0;
    while (1 == fread (&uchByte, 1, 1, fileInput)) {
        iBytesSkipped++;
        if (uchByte == ((unsigned char*) &u4Signature) [iMatchPos]) {
            if (iMatchPos == 3) {
                return iBytesSkipped;
            }
            else {
                iMatchPos++;
            }
        }
        else {
            iMatchPos = 0;
        }
    }
    return -1;
}

ZipFileReader::Entry::Entry (void)
{
    ui32Offset = 0;
    ui16VersionMadeBy = 0;
    ui16VersionNeeded = 0;
    ui16Flags = 0;
    compType = CT_None;
    ui16LastModTime = 0;
    ui16LastModDate = 0;
    ui32CRC32 = 0;
    ui32CompSize = 0;
    ui32UncompSize = 0;
    ui16ExtraFieldLen = 0;
    pszName = NULL;
    pExtraField = NULL;
    pszComment = NULL;
    pBuf = NULL;
}

ZipFileReader::Entry::~Entry (void)
{
    delete[] pszName;
    pszName = NULL;
    delete[] pExtraField;
    pExtraField = NULL;
    delete[] pszComment;
    pszComment = NULL;
    delete[] pBuf;
    pBuf = NULL;
}

ZipFileReader::LocalFileHeader::LocalFileHeader (void)
{
    u4Sig = 0;
    u2Version = 0;
    u2Flags = 0;
    u2CompMethod = 0;
    u2LastModFileTime = 0;
    u2LastModFileDate = 0;
    u4CRC32 = 0;
    u4CompSize = 0;
    u4UncompSize = 0;
    u2FileNameLen = 0;
    u2ExtraFieldLen = 0;
    pszFileName = NULL;
    pExtraField = NULL;
    u4BytesToSkipToNextHeader = 0;
}

ZipFileReader::LocalFileHeader::~LocalFileHeader (void)
{
    delete[] pszFileName;
    pszFileName = NULL;
    delete[] pExtraField;
    pExtraField = NULL;
}

int ZipFileReader::LocalFileHeader::read (FILE *fileInput)
{
    // Cleanup fields if necessary
    delete[] pszFileName;
    pszFileName = NULL;
    delete[] pExtraField;
    pExtraField = NULL;

    // Assume that the signature has already been read
    if (1 != fread (&u2Version, 26, 1, fileInput)) {
        return -1;
    }
    u2Version = ZipFileReader::swapBytes (u2Version);
    u2Flags = ZipFileReader::swapBytes (u2Flags);
    u2CompMethod = ZipFileReader::swapBytes (u2CompMethod);
    u2LastModFileTime = ZipFileReader::swapBytes (u2LastModFileTime);
    u2LastModFileDate = ZipFileReader::swapBytes (u2LastModFileDate);
    u4CRC32 = ZipFileReader::swapBytes (u4CRC32);
    u4CompSize = ZipFileReader::swapBytes (u4CompSize);
    u4UncompSize = ZipFileReader::swapBytes (u4UncompSize);
    u2FileNameLen = ZipFileReader::swapBytes (u2FileNameLen);
    u2ExtraFieldLen = ZipFileReader::swapBytes (u2ExtraFieldLen);
    if (u2FileNameLen > 0) {
        if (NULL == (pszFileName = new char [u2FileNameLen+1])) {
            return -2;
        }
        if (1 != fread (pszFileName, u2FileNameLen, 1, fileInput)) {
            return -3;
        }
        pszFileName[u2FileNameLen] = '\0';
    }
    if (u2ExtraFieldLen > 0) {
        if (NULL == (pExtraField = new char [u2ExtraFieldLen])) {
            return -4;
        }
        if (1 != fread (pExtraField, u2ExtraFieldLen, 1, fileInput)) {
            return -5;
        }
    }
    if (u2Flags & 0x08) {
        // The values for u4CRC32, u4CompSize, and u4UncompSize are 0 and the
        //     actual values for these fields follows the data
        // The data descriptor must be located by reading ahead from the current position
        //     but we must seek back to the current position before returning
        // NOTE: It appears that the data descriptor begins with the signature
        //       0x08074b50 but has not been verified by looking at a specification

        long lCurrPos = ftell (fileInput);
        if (skipToEndOfSignature (0x08074b50, fileInput) < 0) {
            // Did not find the signature for the start of the data descriptor
            return -6;
        }

        // Now read the data descriptor
        if (1 != fread (&u4CRC32, 4, 1, fileInput)) {
            return -7;
        }
        if (1 != fread (&u4CompSize, 4, 1, fileInput)) {
            return -8;
        }
        if (1 != fread (&u4UncompSize, 4, 1, fileInput)) {
            return -9;
        }
        u4CRC32 = ZipFileReader::swapBytes (u4CRC32);
        u4CompSize = ZipFileReader::swapBytes (u4CompSize);
        u4UncompSize = ZipFileReader::swapBytes (u4UncompSize);

        fseek (fileInput, lCurrPos, SEEK_SET);
        u4BytesToSkipToNextHeader = u4CompSize + 16;   // +16 for the data descriptor
    }
    else {
        u4BytesToSkipToNextHeader = u4CompSize;
    }
    return 0;
}

int ZipFileReader::LocalFileHeader::skip (FILE *fileInput)
{
    // Assume that the signature has already been read
    if (1 != fread (&u2Version, 26, 1, fileInput)) {
        return -1;
    }
    u2Version = ZipFileReader::swapBytes (u2Version);
    u2Flags = ZipFileReader::swapBytes (u2Flags);
    u2CompMethod = ZipFileReader::swapBytes (u2CompMethod);
    u2LastModFileTime = ZipFileReader::swapBytes (u2LastModFileTime);
    u2LastModFileDate = ZipFileReader::swapBytes (u2LastModFileDate);
    u4CRC32 = ZipFileReader::swapBytes (u4CRC32);
    u4CompSize = ZipFileReader::swapBytes (u4CompSize);
    u4UncompSize = ZipFileReader::swapBytes (u4UncompSize);
    u2FileNameLen = ZipFileReader::swapBytes (u2FileNameLen);
    u2ExtraFieldLen = ZipFileReader::swapBytes (u2ExtraFieldLen);
    if (fseek (fileInput, u2FileNameLen+u2ExtraFieldLen, SEEK_CUR)) {
        return -2;
    }
    if (u2Flags & 0x08) {
        // There is a data descriptor - see comments in the read() function
        long lCurrPos = ftell (fileInput);
        if (skipToEndOfSignature (0x08074b50, fileInput) < 0) {
            // Did not find the signature for the start of the data descriptor
            return -3;
        }

        // Now read the data descriptor
        if (1 != fread (&u4CRC32, 4, 1, fileInput)) {
            return -4;
        }
        if (1 != fread (&u4CompSize, 4, 1, fileInput)) {
            return -5;
        }
        if (1 != fread (&u4UncompSize, 4, 1, fileInput)) {
            return -6;
        }
        u4CRC32 = ZipFileReader::swapBytes (u4CRC32);
        u4CompSize = ZipFileReader::swapBytes (u4CompSize);
        u4UncompSize = ZipFileReader::swapBytes (u4UncompSize);

        fseek (fileInput, lCurrPos, SEEK_SET);
        u4BytesToSkipToNextHeader = u4CompSize + 16;   // +16 for the data descriptor
    }
    else {
        u4BytesToSkipToNextHeader = u4CompSize;
    }

    return 0;
}

ZipFileReader::DataDescriptor::DataDescriptor (void)
{
    u4CRC32 = 0;
    u4CompSize = 0;
    u4UncompSize = 0;
}

ZipFileReader::DataDescriptor::~DataDescriptor (void)
{
}

int ZipFileReader::DataDescriptor::read (FILE *fileInput)
{
    if (1 != (fread (&u4CRC32, 12, 1, fileInput))) {
        return -1;
    }
    u4CRC32 = ZipFileReader::swapBytes (u4CRC32);
    u4CompSize = ZipFileReader::swapBytes (u4CompSize);
    u4UncompSize = ZipFileReader::swapBytes (u4UncompSize);
    return 0;
}

ZipFileReader::FileHeader::FileHeader (void)
{
    u4Sig = 0;
    u2VersionMadeBy = 0;
    u2VersionNeeded = 0;
    u2Flags = 0;
    u2CompMethod = 0;
    u2LastModFileTime = 0;
    u2LastModFileDate = 0;
    u4CRC32 = 0;
    u4CompSize = 0;
    u4UncompSize = 0;
    u2FileNameLen = 0;
    u2ExtraFieldLen = 0;
    u2FileCommentLen = 0;
    u2DiskNumberStart = 0;
    u2IntFileAttr = 0;
    u4ExtFileAttr = 0;
    i4LocalHdrRelOffset = 0;
    pszFileName = NULL;
    pExtraField = NULL;
    pszFileComment = NULL;
}

ZipFileReader::FileHeader::~FileHeader (void)
{
    delete[] pszFileName;
    pszFileName = NULL;
    delete[] pExtraField;
    pExtraField = NULL;
    delete[] pszFileComment;
    pszFileComment = NULL;
}

int ZipFileReader::FileHeader::read (FILE *fileInput)
{
    // Cleanup fields if necessary
    delete[] pszFileName;
    pszFileName = NULL;
    delete[] pExtraField;
    pExtraField = NULL;
    delete[] pszFileComment;
    pszFileComment = NULL;

    // Assume that the signature has already been read
    if (1 != fread (&u2VersionMadeBy, 42, 1, fileInput)) {
        return -1;
    }
    u2VersionMadeBy = ZipFileReader::swapBytes (u2VersionMadeBy);
    u2VersionNeeded = ZipFileReader::swapBytes (u2VersionNeeded);
    u2Flags = ZipFileReader::swapBytes (u2Flags);
    u2CompMethod = ZipFileReader::swapBytes (u2CompMethod);
    u2LastModFileTime = ZipFileReader::swapBytes (u2LastModFileTime);
    u2LastModFileDate = ZipFileReader::swapBytes (u2LastModFileDate);
    u4CRC32 = ZipFileReader::swapBytes (u4CRC32);
    u4CompSize = ZipFileReader::swapBytes (u4CompSize);
    u4UncompSize = ZipFileReader::swapBytes (u4UncompSize);
    u2FileNameLen = ZipFileReader::swapBytes (u2FileNameLen);
    u2ExtraFieldLen = ZipFileReader::swapBytes (u2ExtraFieldLen);
    u2FileCommentLen = ZipFileReader::swapBytes (u2FileCommentLen);
    u2DiskNumberStart = ZipFileReader::swapBytes (u2DiskNumberStart);
    u2IntFileAttr = ZipFileReader::swapBytes (u2IntFileAttr);
    u4ExtFileAttr = ZipFileReader::swapBytes (u4ExtFileAttr);
    i4LocalHdrRelOffset = ZipFileReader::swapBytes (i4LocalHdrRelOffset);

    if (u2FileNameLen > 0) {
        pszFileName = new char [u2FileNameLen+1];
        if (1 != fread (pszFileName, u2FileNameLen, 1, fileInput)) {
            return -2;
        }
        pszFileName[u2FileNameLen] = '\0';
    }
    if (u2ExtraFieldLen > 0) {
        pExtraField = new char [u2ExtraFieldLen];
        if (1 != fread (pExtraField, u2ExtraFieldLen, 1, fileInput)) {
            return -3;
        }
    }
    if (u2FileCommentLen > 0) {
        pszFileComment = new char [u2FileCommentLen+1];
        if (1 != fread (pszFileComment, u2FileCommentLen, 1, fileInput)) {
            return -4;
        }
        pszFileComment[u2FileCommentLen] = '\0';
    }
    return 0;
}

int ZipFileReader::FileHeader::skip (FILE *fileInput)
{
    // Assume that the signature has already been read
    if (1 != fread (&u2VersionMadeBy, 42, 1, fileInput)) {
        return -1;
    }
    u2VersionMadeBy = ZipFileReader::swapBytes (u2VersionMadeBy);
    u2VersionNeeded = ZipFileReader::swapBytes (u2VersionNeeded);
    u2Flags = ZipFileReader::swapBytes (u2Flags);
    u2CompMethod = ZipFileReader::swapBytes (u2CompMethod);
    u2LastModFileTime = ZipFileReader::swapBytes (u2LastModFileTime);
    u2LastModFileDate = ZipFileReader::swapBytes (u2LastModFileDate);
    u4CRC32 = ZipFileReader::swapBytes (u4CRC32);
    u4CompSize = ZipFileReader::swapBytes (u4CompSize);
    u4UncompSize = ZipFileReader::swapBytes (u4UncompSize);
    u2FileNameLen = ZipFileReader::swapBytes (u2FileNameLen);
    u2ExtraFieldLen = ZipFileReader::swapBytes (u2ExtraFieldLen);
    u2FileCommentLen = ZipFileReader::swapBytes (u2FileCommentLen);
    u2DiskNumberStart = ZipFileReader::swapBytes (u2DiskNumberStart);
    u2IntFileAttr = ZipFileReader::swapBytes (u2IntFileAttr);
    u4ExtFileAttr = ZipFileReader::swapBytes (u4ExtFileAttr);
    i4LocalHdrRelOffset = ZipFileReader::swapBytes (i4LocalHdrRelOffset);

    if (fseek (fileInput, u2FileNameLen+u2ExtraFieldLen+u2FileCommentLen, SEEK_CUR)) {
        return -2;
    }
    return 0;
}

ZipFileReader::EndOfCentralDir::EndOfCentralDir (void)
{
    u4Sig = 0;
    u2DiskNum = 0;
    u2CentralDirDiskNum = 0;
    u2LocalDiskCentralDirCount = 0;
    u2CentralDirCount = 0;
    u4CentralDirSize = 0;
    u4CentralDirOffset = 0;
    u2ZipFileCommentLen = 0;
    pszZipFileComment = NULL;
}

ZipFileReader::EndOfCentralDir::~EndOfCentralDir (void)
{
    delete[] pszZipFileComment;
    pszZipFileComment = NULL;
}

int ZipFileReader::EndOfCentralDir::read (FILE *fileInput)
{
    // Cleanup fields if necessary
    delete[] pszZipFileComment;
    pszZipFileComment = NULL;

    // Assume that the signature has already been read
    if (1 != fread (&u2DiskNum, 18, 1, fileInput)) {
        return -1;
    }
    u2DiskNum = ZipFileReader::swapBytes (u2DiskNum);
    u2CentralDirDiskNum = ZipFileReader::swapBytes (u2CentralDirDiskNum);
    u2LocalDiskCentralDirCount = ZipFileReader::swapBytes (u2LocalDiskCentralDirCount);
    u2CentralDirCount = ZipFileReader::swapBytes (u2CentralDirCount);
    u4CentralDirSize = ZipFileReader::swapBytes (u4CentralDirSize);
    u4CentralDirOffset = ZipFileReader::swapBytes (u4CentralDirOffset);
    u2ZipFileCommentLen = ZipFileReader::swapBytes (u2ZipFileCommentLen);
    if (u2ZipFileCommentLen > 0) {
        pszZipFileComment = new char [u2ZipFileCommentLen+1];
        if (1 != fread (pszZipFileComment, u2ZipFileCommentLen, 1, fileInput)) {
            return -2;
        }
        pszZipFileComment[u2ZipFileCommentLen] = '\0';
    }
    return 0;
}

ZipFileReader::FileEntry::FileEntry (void)
{
    u4Offset = 0;
    u2VersionMadeBy = 0;
    u2VersionNeeded = 0;
    u2Flags = 0;
    u2CompMethod = 0;
    u2LastModFileTime = 0;
    u2LastModFileDate = 0;
    u4CRC32 = 0;
    u4CompSize = 0;
    u4UncompSize = 0;
    u2ExtraFieldLen = 0;
    pszName = NULL;
    pExtraField = NULL;
    pszComment = NULL;
}

ZipFileReader::FileEntry::~FileEntry (void)
{
    delete[] pszName;
    pszName = NULL;
    delete[] pExtraField;
    pExtraField = NULL;
    delete[] pszComment;
    pszComment = NULL;
}

int ZipFileReader::FileEntry::init (FileHeader &fh)
{
    u4Offset = fh.i4LocalHdrRelOffset;
    u2VersionMadeBy = fh.u2VersionMadeBy;
    u2VersionNeeded = fh.u2VersionNeeded;
    u2Flags = fh.u2Flags;
    u2CompMethod = fh.u2CompMethod;
    u2LastModFileTime = fh.u2LastModFileTime;
    u2LastModFileDate = fh.u2LastModFileDate;
    u4CRC32 = fh.u4CRC32;
    u4CompSize = fh.u4CompSize;
    u4UncompSize = fh.u4UncompSize;
    u2ExtraFieldLen = fh.u2ExtraFieldLen;

    // Steal the allocated memory in FileHeader
    pszName = fh.pszFileName;
    pExtraField = fh.pExtraField;
    pszComment = fh.pszFileComment;

    // Set the pointers in FileHeader to NULL so that it won't be deleted
    fh.pszFileName = NULL;
    fh.pExtraField = NULL;
    fh.pszFileComment = NULL;
    return 0;
}

int ZipFileReader::FileEntry::compare (const void *pElem1, const void *pElem2)
{
    return strcmp (((FileEntry*)pElem1)->pszName,
                   ((FileEntry*)pElem2)->pszName);
}


