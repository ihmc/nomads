/*
 * TestDriver.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2014 IHMC.
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
 * Generic Test Driver for multiple test cases
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "BufferReader.h"
#include "FileReader.h"
#include "FileWriter.h"
#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"

#include "media/BMPImage.h"

#include "BMPHandler.h"
#include "JPEGLibWrapper.h"
#include "PNGLibWrapper.h"

#if defined (WIN32)
    #include <Windows.h>
    #define PATH_MAX _MAX_PATH
#elif defined (UNIX)
    #if defined (OSX)
        #define PATH_MAX 1024
    #endif
    #define stricmp strcasecmp
#endif

using namespace IHMC_MISC;
using namespace NOMADSUtil;

int testBMPChunker (int argc, char *argv[]);
int testJPEGLibWrapper (int argc, char *argv[]);
int testPNGLibWrapper (int argc, char *argv[]);

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stat _stat32
    #define stricmp _stricmp
#endif

int main (int argc, char *argv[])
{
    pLogger = new Logger();
    pLogger->enableScreenOutput();
    pLogger->setDebugLevel (Logger::L_LowDetailDebug);
    if (argc < 2) {
        fprintf (stderr, "usage: %s <Test> [<arg1> <arg2> ...]\n", argv[0]);
        fprintf (stderr, "    where Test can be JPEGLib, PNGLib, BMPChunker\n");
        return -1;
    }
    if (0 == stricmp (argv[1], "BMPChunker")) {
        return testBMPChunker (argc-2, &argv[2]);
    }
    else if (0 == stricmp (argv[1], "JPEGLib")) {
        return testJPEGLibWrapper (argc-2, &argv[2]);
    }
    else if (0 == stricmp (argv[1], "PNGLib")) {
        return testPNGLibWrapper (argc-2, &argv[2]);
    }
    else {
        fprintf (stderr, "unknown test <%s>\n", argv[1]);
        return -2;
    }
}

int testBMPChunker (int argc, char *argv[])
{
    int rc;
    if (argc < 3) {
        fprintf (stderr, "usage: BMPChunker chunk <numberOfChunks> <inputFile> *or*\n"
                         "       BMPChunker reassemble <numberOfChunks> <outputFile> <inputFile> [<inputFile>...]\n");
        return -1;
    }
    if (0 == stricmp (argv[0], "chunk")) {
        uint8 ui8NumberOfChunks = atoi (argv[1]);
        char *pszInputFile = argv[2];
        char szOutputFileNameBase[PATH_MAX];
        strcpy (szOutputFileNameBase, pszInputFile);
        char *pszExtension = strrchr (szOutputFileNameBase, '.');
        if ((pszExtension == NULL) || (0 != stricmp (pszExtension, ".bmp"))) {
            fprintf (stderr, "BMPChunker: input file <%s> does not have an extension of .bmp\n", pszInputFile);
            return -2;
        }
        *pszExtension = '\0';
        FileReader fr (pszInputFile, "rb");
        BMPImage inputImage;
        if (0 != (rc = inputImage.readHeaderAndImage (&fr))) {
            fprintf (stderr, "BMPChunker: failed to read input file <%s>; rc = %d\n", pszInputFile, rc);
            return -3;
        }
        for (uint8 ui8ChunkId = 1; ui8ChunkId <= ui8NumberOfChunks; ui8ChunkId++) {
            BMPImage *pChunk = BMPChunker::fragmentBMP (&inputImage, ui8ChunkId, ui8NumberOfChunks);
            if (pChunk == NULL) {
                fprintf (stderr, "BMPChunker: failed to obtain chunk %d of %d\n", (int) ui8ChunkId, (int) ui8NumberOfChunks);
                return -4;
            }
            char szChunkFileName[PATH_MAX];
            sprintf (szChunkFileName, "%s.%02d_%02d.bmp", szOutputFileNameBase, (int) ui8ChunkId, (int) ui8NumberOfChunks);
            FileWriter fw (szChunkFileName, "wb");
            if (0 != (rc = pChunk->writeHeaderAndImage (&fw))) {
                fprintf (stderr, "BMPChunker: failed to write chunk %d to output file <%s>; rc = %d\n",
                         (int) ui8ChunkId, szChunkFileName, rc);
                return -5;
            }
            delete pChunk;
        }
    }
    else if (0 == stricmp (argv[0], "reassemble")) {
        if (argc < 4) {
            fprintf (stderr, "BMPChunker: must provide at least one input file when doing reassembly\n");
            return -6;
        }
        uint8 ui8NumberOfChunks = atoi (argv[1]);
        char *pszOutputFile = argv[2];
        BMPReassembler bmpr;
        if (bmpr.init (ui8NumberOfChunks)) {
            fprintf (stderr, "BMPChecker: could not initialize reassebler for %d chunks\n", ui8NumberOfChunks);
            return -7;
        }
        for (int i = 3; i < argc; i++) {
            char *pszInputFile = argv[i];
            BMPImage chunk;
            FileReader fr (pszInputFile, "rb");
            if (chunk.readHeaderAndImage (&fr)) {
                fprintf (stderr, "BMPChecker: failed to read input file <%s>\n", pszInputFile);
                return -8;
            }
            if (0 != (rc= bmpr.incorporateChunk (&chunk, (uint8) (i-2)))) {
                fprintf (stderr, "BMPChecker:: failed to incorporate chunk %d from file <%s>; rc = %d\n", i-2, pszInputFile, rc);
                return -9;
            }
        }
        const BMPImage *pReassembledImage = bmpr.getReassembledImage();
        if (pReassembledImage == NULL) {
            fprintf (stderr, "BMPChecker: failed to get reassembled image\n");
            return -10;
        }
        FileWriter fw (pszOutputFile, "wb");
        if (0 != (rc = ((BMPImage*)pReassembledImage)->writeHeaderAndImage (&fw))) {
            fprintf (stderr, "BMPChecker: failed to write reassembled image to file <%s>; rc = %d\n", pszOutputFile, rc);
            return -11;
        }
    }
    else {
        fprintf (stderr, "do not understand operation <%s> - must be either chunk or reassemble\n", argv[0]);
        return -12;
    }
}

int testJPEGLibWrapper (int argc, char *argv[])
{
    int rc;
    if (argc < 3) {
        fprintf (stderr, "usage: JPEGLib <j2b|b2j> <inputfile> <outputfile>\n");
        return -1;
    }
    /*struct stat sb;
    if (0 != (stat (argv[1], &sb))) {
        fprintf (stderr, "could not access file <%s>\n");
        return -2;
    }
    if (sb.st_size <= 0) {
        fprintf (stderr, "size of file <%s> is %d - invalid\n", argv[1], sb.st_size);
        return -3;
    }
    void *pSrcData = malloc (sb.st_size);
    if (pSrcData == NULL) {
        fprintf (stderr, "could not allocate an array to read in file <%s> of size %lu\n", argv[1], sb.st_size);
        return -4;
    }
    FILE *fileInput = fopen (argv[1], "rb");
    if (fileInput == NULL) {
        fprintf (stderr, "could not open file <%s> to read\n", argv[1]);
        free (pSrcData);
        return -5;
    }
    if (sb.st_size != fread (pSrcData, 1, sb.st_size, fileInput)) {
        fprintf (stderr, "could not read %lu bytes from file <%s>\n", sb.st_size, argv[1]);
        fclose (fileInput);
        free (pSrcData);
        return -6;
    }
    fclose (fileInput); */
    FILE *fileOutput = fopen (argv[2], "wb");
    if (fileOutput == NULL) {
        fprintf (stderr, "could not open file <%s> to write\n", argv[2]);
        //free (pSrcData);
        return -7;
    }
    FileWriter fw (fileOutput, true);
    if (0 == stricmp (argv[0], "j2b")) {
        // Convert from JPEG to BMP
        //BMPImage *pBMPImage = JPEGLibWrapper::convertJPEGToBMP (pSrcData, sb.st_size);
        //free (pSrcData);
        BMPImage *pBMPImage = JPEGLibWrapper::convertJPEGToBMP (argv[1]);
        if (pBMPImage == NULL) {
            fprintf (stderr, "failed to convert <%s> to BMP\n", argv[1]);
            return -8;
        }
        else if (0 != (rc = pBMPImage->writeHeaderAndImage (&fw))) {
            fprintf (stderr, "failed to write BMP image to <%s>; rc = %d\n", argv[2], rc);
            return -9;
        }
        fw.close();
        return 0;
    }
    else if (0 == stricmp (argv[0], "b2j")) {
        // Convert from BMP to JPEG
        FILE *fileInput = fopen (argv[1], "rb");
        if (fileInput == NULL) {
            fprintf (stderr, "could not open file <%s> to read\n", argv[1]);
            return -10;
        }
        FileReader fr (fileInput, true);
        BMPImage sourceImage;
        if (0 != (rc = sourceImage.readHeaderAndImage (&fr))) {
            fprintf (stderr, "failed to read BMP file <%s>; rc = %d\n", argv[1], rc);
            return -11;
        }
        BufferReader *pBR = JPEGLibWrapper::convertBMPToJPEG (&sourceImage, 90);
        if (pBR == NULL) {
            fprintf (stderr, "failed to convert BMP file <%s> to JPEG\n", argv[1]);
        }
        FILE *fileOutput = fopen (argv[2], "wb");
        if (fileOutput == NULL) {
            fprintf (stderr, "could not open file <%s> to write\n", argv[2]);
            return -7;
        }
        FileWriter fw (fileOutput, true);
        if (0 != (rc = fw.writeBytes (pBR->getBuffer(), pBR->getBufferLength()))) {
            fprintf (stderr, "could not write %lu bytes to file <%s>; rc = %d\n",
                     pBR->getBufferLength(), argv[2], rc);
            return -8;
        }
        delete pBR;
        fw.close();
    }
    else {
        fprintf (stderr, "do not understand operation <%s> - must be either j2b or b2j\n", argv[0]);
        //free (pSrcData);
        return -8;
    }
    return 0;
}

int testPNGLibWrapper (int argc, char *argv[])
{
    int rc;
    if (argc < 2) {
        fprintf (stderr, "usage: PNGLib <input png> <output bmp>\n");
        return -1;
    }
    struct stat sb;
    if (0 != (stat (argv[0], &sb))) {
        fprintf (stderr, "could not access file <%s>\n", argv[0]);
        return -2;
    }
    if (sb.st_size <= 0) {
        fprintf (stderr, "size of file <%s> is %d - invalid\n", argv[0], sb.st_size);
        return -3;
    }
    void *pSrcData = malloc (sb.st_size);
    if (pSrcData == NULL) {
        fprintf (stderr, "could not allocate an array to read in file <%s> of size %lu\n", argv[0], sb.st_size);
        return -4;
    }
    FILE *fileInput = fopen (argv[0], "rb");
    if (fileInput == NULL) {
        fprintf (stderr, "could not open file <%s> to read\n", argv[0]);
        free (pSrcData);
        return -5;
    }
    if (sb.st_size != fread (pSrcData, 1, sb.st_size, fileInput)) {
        fprintf (stderr, "could not read %lu bytes from file <%s>\n", sb.st_size, argv[1]);
        fclose (fileInput);
        free (pSrcData);
        return -6;
    }
    fclose (fileInput);
    BMPImage *pBMPImage = PNGLibWrapper::convertPNGToBMP (pSrcData, sb.st_size);
    if (pBMPImage == NULL) {
        fprintf (stderr, "PNGLibWrapper::convertPNGToBMP returned NULL\n");
        free (pSrcData);
        return -7;
    }
    free (pSrcData);
    FILE *fileOutput = fopen (argv[1], "wb");
    if (fileOutput == NULL) {
        fprintf (stderr, "could not open file <%s> to write\n", argv[1]);
        delete pBMPImage;
        return -8;
    }
    FileWriter fw (fileOutput, true);
    if (0 != (rc = pBMPImage->writeHeaderAndImage (&fw))) {
        fprintf (stderr, "failed to write BMP image to <%s>; rc = %d\n", argv[2], rc);
        return -9;
    }
    fw.close();

    BufferReader *pBR = PNGLibWrapper::convertBMPtoPNG (pBMPImage);
    if (pBR == NULL) {
        fprintf (stderr, "failed to convert BMP file <%s> to PNG\n", argv[1]);
    }
    String outputfile (argv[0]);
    outputfile += ".png";
    FileWriter pngf (outputfile, "w");
    if (0 != (rc = pngf.writeBytes (pBR->getBuffer(), pBR->getBufferLength()))) {
        fprintf (stderr, "could not write %lu bytes to file <%s>; rc = %d\n",
                 pBR->getBufferLength(), argv[2], rc);
        return -8;
    }
    pngf.close();
    delete pBR;
    
    return 0;
}
