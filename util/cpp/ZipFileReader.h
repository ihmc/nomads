/*
 * ZipFileReader.h
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
 *
 * This class reads files of type .zip or .jar
 *
 * See comments for the getEntry() functions below regarding using this class is to extract
 *     compressed (deflated) entries
 *
 * NOTE: Objects of this class are not thread-safe. In particular, Due to some state variables
 *       and seek operations performed on the input file, multiple threads should not
 *       simultaneously call functions.
 */

#ifndef INCL_ZIP_FILE_READER_H
#define INCL_ZIP_FILE_READER_H

#include <stdio.h>

#include "FTypes.h"

namespace NOMADSUtil
{

    typedef int i4;
    typedef unsigned long u4;
    typedef unsigned short u2;

    class ZipFileReader
    {
        public:
            ZipFileReader (void);
            ~ZipFileReader (void);
            enum ReturnCodes {
                RC_Ok,
                RC_FileAccessError = 1,
                RC_FileFormatError = 2
            };
            struct Entry {
                Entry (void);
                ~Entry (void);
                enum CompressionType {
                    CT_None = 0,
                    CT_Deflate = 8
                };
                uint32 ui32Offset;
                uint16 ui16VersionMadeBy;
                uint16 ui16VersionNeeded;
                uint16 ui16Flags;
                CompressionType compType;
                uint16 ui16LastModTime;
                uint16 ui16LastModDate;
                uint32 ui32CRC32;
                uint32 ui32CompSize;
                uint32 ui32UncompSize;
                uint16 ui16ExtraFieldLen;
                char *pszName;
                char *pExtraField;
                char *pszComment;
                char *pBuf;
            };
            int init (const char *pszFile, bool bCacheDir = false);
            int getFileCount (void);
            const char * getFirstFileName (void);
            const char * getNextFileName (void);
            bool checkForFile (const char *pszFileName);

            // Returns an entry for the specified name or NULL if the name is not found
            //
            // NOTES: The entry object that is returned must be deleted by the caller
            //
            //        The buffer allocated for the data (pBuf) will be one byte larger than
            //            the size of the entry (ui32CompSize). This is simply as a matter
            //            of convenience. If this is a deflated entry and the zlib library
            //            is used to decompress, then the "nowrap" option of the zlib library
            //            will need to be used (see inflate.c in zlib). However, the "nowrap"
            //            option requires an extra padded byte for the input buffer.
            //
            //            NOTE to the above note: Although an extra byte is allocated for the
            //                buffer, the size reported is still the original size.
            //
            //            Example: If the CompressedReader/BufferReader classes are being used,
            //                here is one way to handle decompressing entries:
            //
            //                CompressedReader *pcr = new CompressedReader (new BufferReader (pEntry->pBuf,
            //                                                                                pEntry->ui32CompSize+1),
            //                                                              true,
            //                                                              true);
            //
            //                and then read pEntry->ui32UncompSize number of bytes from the CompressedReader
            Entry * getEntry (const char *pszName) const;

            // Returns entry at the specified index
            // Index may range from 0 to getFileCount()-1
            //
            // NOTE: If bCacheDir is true in init(), the ZipFileReader currently sorts the
            //           entries upon loading the directory of the zip file. Therefore, the
            //           order will be different from that in the original zip file.
            //
            //       Also - see notes above for getEntry (const char *pszName)
            Entry * getEntry (int iIndex) const;

            static i4 swapBytes (i4 i4Input);
            static u2 swapBytes (u2 u2Input);
            static u4 swapBytes (u4 u4Input);

            // Read and discard bytes from the specified input file until the specified
            //     signature is encountered
            // The file pointer is positioned at the end of the signature
            // Returns the number of bytes that were skipped or -1 in case of error
            static int skipToEndOfSignature (u4 u4Signature, FILE *fileInput);

        protected:
            enum Type {
                T_LocalFileHeader,
                T_FileHeader,
                T_EndOfCentralDir
            };
            #pragma pack (1)
            struct LocalFileHeader {
                LocalFileHeader (void);
                ~LocalFileHeader (void);
                int read (FILE *fileInput);
                int skip (FILE *fileInput);
                u4 u4Sig;
                u2 u2Version;
                u2 u2Flags;
                u2 u2CompMethod;
                u2 u2LastModFileTime;
                u2 u2LastModFileDate;
                u4 u4CRC32;
                u4 u4CompSize;
                u4 u4UncompSize;
                u2 u2FileNameLen;
                u2 u2ExtraFieldLen;
                char *pszFileName;
                char *pExtraField;
                u4 u4BytesToSkipToNextHeader;   // The number of bytes to skip to reach the start of
                                                //     the next header
                                                // This will include the bytes in u4CompSize plus the
                                                //     bytes needed for data descriptor that could optionally
                                                //     follow the data bytes
            };
            struct DataDescriptor {
                DataDescriptor (void);
                ~DataDescriptor (void);
                int read (FILE *fileInput);
                u4 u4CRC32;
                u4 u4CompSize;
                u4 u4UncompSize;
            };
            struct FileHeader {
                FileHeader (void);
                ~FileHeader (void);
                int read (FILE *fileInput);
                int skip (FILE *fileInput);
                u4 u4Sig;
                u2 u2VersionMadeBy;
                u2 u2VersionNeeded;
                u2 u2Flags;
                u2 u2CompMethod;
                u2 u2LastModFileTime;
                u2 u2LastModFileDate;
                u4 u4CRC32;
                u4 u4CompSize;
                u4 u4UncompSize;
                u2 u2FileNameLen;
                u2 u2ExtraFieldLen;
                u2 u2FileCommentLen;
                u2 u2DiskNumberStart;
                u2 u2IntFileAttr;
                u4 u4ExtFileAttr;
                i4 i4LocalHdrRelOffset;
                char *pszFileName;
                char *pExtraField;
                char *pszFileComment;
            };
            struct EndOfCentralDir {
                EndOfCentralDir (void);
                ~EndOfCentralDir (void);
                int read (FILE *fileInput);
                u4 u4Sig;
                u2 u2DiskNum;
                u2 u2CentralDirDiskNum;
                u2 u2LocalDiskCentralDirCount;
                u2 u2CentralDirCount;
                u4 u4CentralDirSize;
                u4 u4CentralDirOffset;
                u2 u2ZipFileCommentLen;
                char *pszZipFileComment;
            };
            #pragma pack()
            struct FileEntry {             /*!!*/ // There is really no need for this struct anymore.
                FileEntry (void);                 // Change the code to use FileHeader instead
                ~FileEntry (void);                // However, need to watch out for getEntry() which steals
                int init (FileHeader &fh);        //     memory pointers when using FileHeader
                static int compare (const void *pElem1, const void *pElem2);
                u4 u4Offset;
                u2 u2VersionMadeBy;
                u2 u2VersionNeeded;
                u2 u2Flags;
                u2 u2CompMethod;
                u2 u2LastModFileTime;
                u2 u2LastModFileDate;
                u4 u4CRC32;
                u4 u4CompSize;
                u4 u4UncompSize;
                u2 u2ExtraFieldLen;
                char *pszName;
                char *pExtraField;
                char *pszComment;
            };
        protected:
            int readSignature (FILE *fileInput);

            // Search the central directory linearly to find a matching file header
            //     The file header is read into fh and the file pointer is left
            //     at the end of the file header
            int findFileHeader (const char *pszFile, FILE *fileInput);

            // Step through the central directory linearly until the entry specified
            //     by iIndex is found
            //     The file header is read into fh and the file pointer is left
            //     at the end of the file header
            int gotoFileHeader (int iIndex, FILE *fileInput);

            FileEntry * binarySearch (const char *pszFile, int iBegin, int iEnd);

            // Search the FileEntry array to locate the corresponding entry
            //     Returns NULL if the directory information is not cached in pEntries
            //     or if no matching entry was found
            FileEntry * findFileEntry (const char *pszFile);

            // Returns the FileEntry at the specified index in the FileEntry array
            // Returns NULL if the directory information is not cached in pEntries
            //     or if the specified index was not valid
            FileEntry * getFileEntry (int iIndex);

            // Returns an entry given a FileEntry object and the input file
            Entry * getEntry (FileEntry *pfe, FILE *fileInput);

            // Returns an entry given a FileHeader object and the input file
            // NOTE: The pointers in FileHeader are "stolen" by getEntry and placed into
            //       Entry for efficiency reasons. After the call, pszFileName, pszFileComment,
            //       and pExtraBytes will be NULL in pfh
            Entry * getEntry (FileHeader *pfh, FILE *fileInput);

        protected:
            FILE *fileInput;
            long lLastFileHeaderSeekPos;
            LocalFileHeader lfh;
            FileHeader fh;
            EndOfCentralDir ecd;
            FileEntry *pEntries;
            int iEntryArraySize;
    };

    inline i4 ZipFileReader::swapBytes (i4 i4Input)
    {
        #if defined (BIG_ENDIAN_SYSTEM)
            i4 i4Output;
            char *pInput = (char*) &i4Input;
            char *pOutput = (char*) &i4Output;
            pOutput[0] = pInput[3];
            pOutput[1] = pInput[2];
            pOutput[2] = pInput[1];
            pOutput[3] = pInput[0];
            return i4Output;
        #elif defined (LITTLE_ENDIAN_SYSTEM)
            return i4Input;
        #endif
    }

    inline u2 ZipFileReader::swapBytes (u2 u2Input)
    {
        #if defined (BIG_ENDIAN_SYSTEM)
            u2 u2Output;
            char *pInput = (char*) &u2Input;
            char *pOutput = (char*) &u2Output;
            pOutput[0] = pInput[1];
            pOutput[1] = pInput[0];
            return u2Output;
        #elif defined (LITTLE_ENDIAN_SYSTEM)
            return u2Input;
        #endif
    }

    inline u4 ZipFileReader::swapBytes (u4 u4Input)
    {
        #if defined (BIG_ENDIAN_SYSTEM)
            u4 u4Output;
            char *pInput = (char*) &u4Input;
            char *pOutput = (char*) &u4Output;
            pOutput[0] = pInput[3];
            pOutput[1] = pInput[2];
            pOutput[2] = pInput[1];
            pOutput[3] = pInput[0];
            return u4Output;
        #elif defined (LITTLE_ENDIAN_SYSTEM)
            return u4Input;
        #endif
    }

}

#endif   // #ifndef INCL_ZIP_FILE_READER_H
