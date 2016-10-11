/*
 * TestDriver.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2016 IHMC.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "Chunker.h"
#include "ChunkingUtils.h"
#include "ChunkReassembler.h"
#include "MimeUtils.h"

#include "BufferReader.h"
#include "File.h"
#include "FileReader.h"
#include "FileWriter.h"
#include "ImageCodec.h"
#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"
#include "VideoCodec.h"

#include "BMPImage.h"

#if defined (WIN32)
  //  #include <Windows.h>
    #define PATH_MAX _MAX_PATH
#elif defined (UNIX)
    #if defined (OSX)
        #define PATH_MAX 1024
    #endif
    #define stricmp strcasecmp
#endif

#if defined (UNIX)
    #define stricmp strcasecmp
#endif

using namespace IHMC_MISC;
using namespace NOMADSUtil;

namespace IHMC_MISC_TEST
{
    enum GeneratedFileType
    {
        DECODED,
        CHUNK,
        REASSEMBLED,
        PARTIALLY_REASSEMBLED,
        EXTRACTED
    };

    class CircularBufferReader : public BufferReader
    {
        public:
            CircularBufferReader (const void *pBuf, uint32 ui32BufLen, bool bDeleteWhenDone = false);
            ~CircularBufferReader (void);
            int read (void *pBuf, int iCount);
            int readBytes (void *pBuf, uint32 ui32Count);
            int skipBytes (uint32 ui32Count);

        private:
            int loop (void);
    };

    CircularBufferReader::CircularBufferReader (const void *pBuf, uint32 ui32BufLen, bool bDeleteWhenDone)
        : BufferReader (pBuf, ui32BufLen, bDeleteWhenDone) {}
    CircularBufferReader::~CircularBufferReader (void) {}

    int CircularBufferReader::read (void *pBuf, int iCount)
    {
        int rc = BufferReader::read (pBuf, iCount);
        return (rc < 0) ? rc : loop();
    }

    int CircularBufferReader::readBytes (void *pBuf, uint32 ui32Count)
    {
        int rc = BufferReader::readBytes (pBuf, ui32Count);
        return (rc < 0) ? rc : loop();
    }

    int CircularBufferReader::skipBytes (uint32 ui32Count)
    {
        int rc = BufferReader::skipBytes (ui32Count);
        return (rc < 0) ? rc : loop();
    }

    int CircularBufferReader::loop (void)
    {
        _ui32NextPtr = _ui32NextPtr % _ui32BufLen;
        _ui32TotalBytesRead = _ui32TotalBytesRead % _ui32BufLen;
        return 0;
    }

    struct Annotation
    {
        Annotation (void)
            : ppIntervals (NULL),
              pFragment (NULL)
        {}
        Annotation (const Annotation &rhsAnn)
            : ppIntervals (rhsAnn.ppIntervals),
              pFragment (rhsAnn.pFragment)
        {}
        ~Annotation (void) {}

        Chunker::Interval **ppIntervals;
        Chunker::Fragment *pFragment;
    };

    void * toBuf (Reader &r, uint64 ui64Len)
    {
        void *pBuf = malloc (ui64Len);
        if (pBuf != NULL) {
            if (r.read (pBuf, ui64Len) < 0) {
                free (pBuf);
                pBuf = NULL;
            }
        }
        return pBuf;
    }

    void toCircularReader (Chunker::Fragment *pFragment)
    {
        if (pFragment != NULL) {
            void *pbuf = toBuf (*(pFragment->pReader), pFragment->ui64FragLen);
            delete (pFragment->pReader);
            pFragment->pReader = new CircularBufferReader (pbuf, pFragment->ui64FragLen, true); // BufferReader will deallocate pBuf
        }
    }

    const char * toString (GeneratedFileType type)
    {
        switch (type) {
            case DECODED: return "decoded";
            case CHUNK: return "chunk";
            case REASSEMBLED: return "reassembled";
            case PARTIALLY_REASSEMBLED: return "partially_reassembled";
            case EXTRACTED: return "extracted";
            default: return "unknown";
        }
    }

    void toFile (const File &file, BufferReader &br, Chunker::Type type, GeneratedFileType genereatFileType, bool bWithAnnotation, uint8 ui8ChunkId = 0)
    {
        const String basefilename (file.getName (true)); // (no extension)
        String outfilename ("GEN-");
        outfilename += basefilename;
        outfilename += '-';
        outfilename += toString (genereatFileType);
        if ((genereatFileType == CHUNK) || (genereatFileType == PARTIALLY_REASSEMBLED)) {
            outfilename += '_';
            outfilename += static_cast<uint32>(ui8ChunkId);
        }
        if (bWithAnnotation) {
            outfilename += "_annotated";
        }
        outfilename += '.';
        outfilename += MimeUtils::toExtesion (type);
        if (ChunkingUtils::dump (outfilename, &br) < 0) {
            fprintf (stderr, "could not write to file %s.\n", outfilename.c_str ());
        }
    }

    void toFile (const File &file, Reader &r, uint64 ui64Len, Chunker::Type type, GeneratedFileType genereatFileType, bool bWithAnnotation, uint8 ui8ChunkId = 0)
    {
        void *pBuf = toBuf (r, ui64Len);
        if (pBuf != NULL) {
            BufferReader br (pBuf, ui64Len, true); // BufferReader will deallocate pBuf
            toFile (file, br, type, genereatFileType, bWithAnnotation, ui8ChunkId);
        }
    }

    void toFile (const File &file, Chunker::Fragment *pFragment, GeneratedFileType genereatFileType, Chunker::Type type)
    {
        if (pFragment != NULL) {
            toFile (file, *pFragment->pReader, pFragment->ui64FragLen, type, genereatFileType, false, pFragment->ui8Part);
        }
    }

    BMPImage * decodeImage (const File &file, const void *pBuf, uint64 ui64FileSize, Chunker::Type srcType)
    {
        const String filename (file.getName (false));
        BMPImage *pBMPImg = ImageCodec::decode (pBuf, ui64FileSize, srcType);
        if (pBMPImg == NULL) {
            fprintf (stderr, "could not decode file: %s\n", filename.c_str ());
            return NULL;
        }
        // ChunkingUtils::toReader
        BufferReader *pReader = ImageCodec::encode (pBMPImg, Chunker::BMP, 100);
        if (pReader == NULL) {
            fprintf (stderr, "could not encode file: %s\n", filename.c_str ());
            return NULL;
        }
        toFile (file, *pReader, Chunker::BMP, DECODED, false);
        delete pReader;
        return pBMPImg;
    }

    Annotation extractImage (const File &file, BMPImage *pBMPImg, const void *pBuf, uint64 ui64FileSize, Chunker::Type srcType, Chunker::Interval **ppIntervals)
    {
        const float fWidth = pBMPImg->getWidth();
        const float fHeight = pBMPImg->getHeight();

        Annotation annotation;
        if ((ppIntervals == NULL) || (ppIntervals[0] == NULL)) {
            annotation.ppIntervals = static_cast<Chunker::Interval **>(calloc (3, sizeof (Chunker::Interval*)));
            annotation.ppIntervals[0] = new Chunker::Interval();
            annotation.ppIntervals[0]->dimension = Chunker::X;
            annotation.ppIntervals[0]->uiStart = fWidth / 4.0f;
            annotation.ppIntervals[0]->uiEnd = pBMPImg->getWidth() - annotation.ppIntervals[0]->uiStart;
            annotation.ppIntervals[1] = new Chunker::Interval ();
            annotation.ppIntervals[1]->dimension = Chunker::Y;
            annotation.ppIntervals[1]->uiStart = fHeight / 4.0f;
            annotation.ppIntervals[1]->uiEnd = pBMPImg->getHeight() - annotation.ppIntervals[1]->uiStart;
            annotation.ppIntervals[2] = NULL;
        }
        else {
            annotation.ppIntervals = ppIntervals;
        }

        pLogger->logMsg ("EXTRACT", Logger::L_Info, "%u %u %u %u",
                         annotation.ppIntervals[0]->uiStart, annotation.ppIntervals[0]->uiEnd,
                         annotation.ppIntervals[1]->uiStart, annotation.ppIntervals[1]->uiEnd);
        annotation.pFragment = Chunker::extractFromBuffer (pBuf, ui64FileSize, srcType, srcType, 100, annotation.ppIntervals);
        toCircularReader (annotation.pFragment);
        toFile (file, annotation.pFragment, EXTRACTED, srcType);
        return annotation;
    }

    Annotation extractVideo (const File &file, int64 i64Duration, const void *pBuf, uint64 ui64FileSize, Chunker::Type srcType, Chunker::Interval **ppIntervals)
    {
        Annotation annotation;
        if ((ppIntervals == NULL) || (ppIntervals[0] == NULL)) {
            annotation.ppIntervals = static_cast<Chunker::Interval **>(calloc (2, sizeof (Chunker::Interval*)));
            annotation.ppIntervals[0] = new Chunker::Interval ();
            annotation.ppIntervals[0]->dimension = Chunker::T;
            annotation.ppIntervals[0]->uiStart = i64Duration / 2.0f;
            annotation.ppIntervals[0]->uiEnd = i64Duration;
        }
        else {
            annotation.ppIntervals = ppIntervals;
        }

        pLogger->logMsg ("EXTRACT", Logger::L_Info, "%u %u",
                         annotation.ppIntervals[0]->uiStart, annotation.ppIntervals[0]->uiEnd);
        annotation.pFragment = Chunker::extractFromBuffer (pBuf, ui64FileSize, srcType, srcType, 100, annotation.ppIntervals);
        toCircularReader (annotation.pFragment);
        toFile (file, annotation.pFragment, EXTRACTED, srcType);
        return annotation;
    }

    int toFile (BufferReader *pReassembledObj, const File &file, const String &filename,
                Chunker::Type outType, const String &srcChecksum,
                unsigned short uiNChunks, unsigned short usTotalNChunks, bool bWithAnnotation)
    {
        if (pReassembledObj == NULL) {
            fprintf (stderr, "could not reasemble: %s\n", filename.c_str());
            return -1;
        }
        const GeneratedFileType type = (uiNChunks == usTotalNChunks ? REASSEMBLED : PARTIALLY_REASSEMBLED);
        toFile (file, *pReassembledObj, outType, type, bWithAnnotation, uiNChunks);
        pReassembledObj->setPosition (0U);
        if (type == REASSEMBLED) {
            const String dstChecksum (ChunkingUtils::getMD5Checksum (pReassembledObj));
            if (srcChecksum != dstChecksum) {
                fprintf (stdout, "source and reassembled object checksums differ (this may "
                         "highlight a problem if lossless compression was used): src <%s> "
                         "dst <%s>\n", srcChecksum.c_str(), dstChecksum.c_str());
            }
        }
        return 0;
    }

    int test (const File &file, unsigned short usNChunks, Chunker::Interval **ppIntervals)
    {
        const String filename (file.getName (false));

        if (!file.exists()) {
            fprintf (stderr, "input file not found: %s\n", filename.c_str());
            return -1;
        }
        const Chunker::Type srcType = MimeUtils::toType (file.getExtension());
        if (srcType == Chunker::UNSUPPORTED) {
            return -2;
        }
        const uint32 ui32SrcFileSize = static_cast<uint32>(file.getFileSize());
        FileReader fr (file.getPath(), "rb");
        void *pBuf = malloc (ui32SrcFileSize);
        if ((pBuf == NULL) || (fr.readBytes (pBuf, ui32SrcFileSize) < 0)) {
            return -3;
        } 
        const void *pSrcMediaBuf = pBuf;
        const String srcChecksum (ChunkingUtils::getMD5Checksum (pSrcMediaBuf, ui32SrcFileSize));

        // Get annotation
        Annotation annotation;
        ChunkReassembler::Type reassemblerType = ChunkReassembler::UNSUPPORTED;
        if (ImageCodec::supports (srcType)) {
            reassemblerType = ChunkReassembler::Image;
            // Type-specifi tests
            BMPImage *pBMPImage = decodeImage (file, pSrcMediaBuf, ui32SrcFileSize, srcType);
            if (pBMPImage == NULL) {
                return -4;
            }

            Annotation a (extractImage (file, pBMPImage, pSrcMediaBuf, ui32SrcFileSize, srcType, ppIntervals));
            annotation.pFragment = a.pFragment;
            annotation.ppIntervals = a.ppIntervals;
            pLogger->logMsg ("EXTRACT", Logger::L_Info, "%u %u %u %u",
                annotation.ppIntervals[0]->uiStart, annotation.ppIntervals[0]->uiEnd,
                annotation.ppIntervals[1]->uiStart, annotation.ppIntervals[1]->uiEnd);
            delete pBMPImage;
        }
        else if (VideoCodec::supports (srcType)) {
            int64 i64Duration = 10000;  // 10 seconds (fake)
            reassemblerType = ChunkReassembler::Video;
            Annotation a (extractVideo (file, i64Duration, pSrcMediaBuf, ui32SrcFileSize, srcType, ppIntervals));
            annotation.pFragment = a.pFragment;
            annotation.ppIntervals = a.ppIntervals;
            pLogger->logMsg ("EXTRACT", Logger::L_Info, "%u %u %u %u",
                annotation.ppIntervals[0]->uiStart, annotation.ppIntervals[0]->uiEnd);
        }
        const uint64 ui64AnnLen = (annotation.pFragment == NULL ? 0 : annotation.pFragment->ui64FragLen);
        void *pAnnBuf = (annotation.pFragment == NULL ? NULL : toBuf (*(annotation.pFragment->pReader), ui64AnnLen));

        // Chunk
        PtrLList<Chunker::Fragment> *pFragments = Chunker::fragmentBuffer (pSrcMediaBuf, ui32SrcFileSize, srcType, usNChunks, srcType, 100);
        if (pFragments == NULL) {
            fprintf (stderr, "could not chunk file: %s\n", filename.c_str ());
            return -6;
        }
        for (Chunker::Fragment *pFragment = pFragments->getFirst(); pFragment != NULL; pFragment = pFragments->getNext()) {
            // pFragment->pReader need to be read multiple times, convert it to BufferReader in order to reset it
            toCircularReader (pFragment);
            toFile (file, pFragment, CHUNK, srcType);
        }

        // Annotation blank image
        Chunker::Fragment *pFragment = pFragments->getFirst();
        const Chunker::Type outType = pFragment->out_type;
        ChunkReassembler reassembler;
        if (reassembler.init (reassemblerType, pFragment->ui8TotParts) < 0) {
            fprintf (stderr, "could not init the reassembler: %s\n", filename.c_str ());
            return -6;
        }
       /* reassembler.incorporateAnnotation (annotation.ppIntervals, pAnnBuf, ui64AnnLen, annotation.pFragment->out_type);
        BufferReader *pReassembledAndAnnotatedObj = reassembler.getReassembledObject (outType, 100);
        toFile (pReassembledAndAnnotatedObj, file, filename, outType, srcChecksum, 0, usNChunks, true);*/

        // Partial to complete reassembling, with and without annotation
        for (unsigned int uiNChunksToReassemble = 0; uiNChunksToReassemble <= usNChunks; uiNChunksToReassemble += 2) {
            pFragment = pFragments->getFirst();
            if (reassembler.init (reassemblerType, pFragment->ui8TotParts) < 0) {
                fprintf (stderr, "could not init the reassembler: %s\n", filename.c_str());
                return -6;
            }
            const unsigned int uiNChunksToReassembleTmp = (uiNChunksToReassemble == 0 ? 1 : uiNChunksToReassemble);
            for (unsigned int i = 0; (pFragment != NULL) && (i < uiNChunksToReassembleTmp); pFragment = pFragments->getNext (), i++) {
                void *pChunkBuf = toBuf (*pFragment->pReader, pFragment->ui64FragLen);
                if (reassembler.incorporateChunk (pChunkBuf, pFragment->ui64FragLen, pFragment->out_type, pFragment->ui8Part) < 0) {
                    fprintf (stderr, "could not reasemble: %s. Adding chunk %d failed.\n",
                             filename.c_str(), static_cast<int>(pFragment->ui8Part));
                    free (pChunkBuf);
                    return -7;
                }
                free (pChunkBuf);
            }
            BufferReader *pReassembledAndAnnotatedObj = reassembler.getReassembledObject (outType, 100);
            toFile (pReassembledAndAnnotatedObj, file, filename, outType, srcChecksum, uiNChunksToReassembleTmp, usNChunks, false);
            delete pReassembledAndAnnotatedObj;
            if (annotation.pFragment != NULL) {
                reassembler.incorporateAnnotation (annotation.ppIntervals, pAnnBuf, ui64AnnLen, annotation.pFragment->out_type);
                pReassembledAndAnnotatedObj = reassembler.getAnnotatedObject (outType, 100);
                toFile (pReassembledAndAnnotatedObj, file, filename, outType, srcChecksum, uiNChunksToReassembleTmp, usNChunks, true);
            }
        }

        free (pAnnBuf);
        delete annotation.pFragment;
        delete annotation.ppIntervals;
        return 0;
    }
}

using namespace IHMC_MISC_TEST;

int main (int argc, char *argv[])
{
    pLogger = new Logger();
    pLogger->enableScreenOutput();
    pLogger->setDebugLevel (Logger::L_LowDetailDebug);
    if (argc < 2) {
        fprintf (stderr, "usage: %s <file1> [<file2> <file3> ...]\n", argv[0]);
        fprintf (stderr, "    where fileX is the file to be chunked and reassembled\n");
        return -1;
    }

    unsigned short usNChunks = 4;
    unsigned int uiAnnotationIdx = 0;
    Chunker::Interval **ppIntervals = static_cast<Chunker::Interval **>(calloc (3, sizeof (Chunker::Interval*)));
    for (int i = 1; i < argc; i++) {
        const String arg (argv[i]);
        if ((arg == "-n") || (arg == "-nchunks")) {
            usNChunks = atoui32 (argv[++i]);
        }
        else if ((arg == "-e") || (arg == "-extract")) {
            ppIntervals[uiAnnotationIdx] = new Chunker::Interval();
            String dimension (argv[++i]);
            dimension.convertToUpperCase();
            char chDimension = dimension.c_str()[0];
            switch (chDimension) {
                case 'X':
                    ppIntervals[uiAnnotationIdx]->dimension = Chunker::X;
                    break;
                case 'Y':
                    ppIntervals[uiAnnotationIdx]->dimension = Chunker::Y;
                    break;
                case 'T':
                    ppIntervals[uiAnnotationIdx]->dimension = Chunker::T;
                    break;
                default:
                    fprintf (stderr, "unsupported dimension %s\n", dimension.c_str());
                    return -2;
            }
            ppIntervals[uiAnnotationIdx]->uiStart = atoui32 (argv[++i]);
            ppIntervals[uiAnnotationIdx]->uiEnd = atoui32 (argv[++i]);
            uiAnnotationIdx++;
        }
        else {
            const String filename (argv[i]);
            File file (filename);
            if (test (file, usNChunks, ppIntervals) < 0) {
                return -2;
            }
        }
    }
    return 0;
}

