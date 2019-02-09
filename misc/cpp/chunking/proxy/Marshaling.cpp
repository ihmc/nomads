
#include "Marshaling.h"

#include "ReassemblerArguments.h"

#include "SimpleCommHelper2.h"
#include "ChunkerArguments.h"
#include "MimeUtils.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

namespace CHUNKER_MARSHALLING
{
    int readBlock (Reader *pReader, void *&pBuf, uint32 &ui32BufLen)
    {
        ui32BufLen = 0U;
        if ((pReader->read32 (&ui32BufLen) < 0) || (ui32BufLen == 0U)) {
            return -1;
        }
        pBuf = malloc (ui32BufLen);
        if (pBuf == NULL) {
            return -2;
        }
        if (pReader->readBytes (pBuf, ui32BufLen)) {
            return -3;
        }
        return 0;
    }

    int readBlock (Reader *pReader, BufferReader &br)
    {
        uint32 ui32BufLen = 0U;
        void *pBuf = NULL;
        if (readBlock (pReader, pBuf, ui32BufLen) == 0) {
            br.init (pBuf, ui32BufLen);
            return 0;
        }
        return -1;
    }
}

ChunkerMarshaling::ChunkerMarshaling (NOMADSUtil::SimpleCommHelper2 *pCommHelper)
    : _pCommHelper (pCommHelper)
{
}

ChunkerMarshaling::~ChunkerMarshaling (void)
{
}

PtrLList<Chunker::Fragment> * ChunkerMarshaling::fragmentBuffer (const void *pBuf, uint32 ui32Len, const char *pszInputMimeType,
                                                                 uint8 ui8NoOfChunks, const char *pszOutputMimeType,
                                                                 uint8 ui8ChunkCompressionQuality)
{
    FragmentBufferArguments args (pBuf, ui32Len, pszInputMimeType, ui8NoOfChunks, pszOutputMimeType, ui8ChunkCompressionQuality);
    if (args.write (_pCommHelper->getWriterRef()) < 0) {
        return NULL;
    }

    Chunks chunks (false);
    if (chunks.read (_pCommHelper->getReaderRef()) < 0) {
        return NULL;
    }
    PtrLList<Chunker::Fragment> *pFragments = new PtrLList<Chunker::Fragment>();
    if (pFragments == NULL) {
        return NULL;
    }
    for (unsigned int uiChunkId = 1; uiChunkId < chunks.size(); uiChunkId++) {
        if (chunks.used (uiChunkId)) {
            Chunker::Fragment *pFrag = new Chunker::Fragment();
            if (pFrag != NULL) {
                uint32 ui32BufLen = chunks[uiChunkId].getBufferLength ();
                pFrag->pReader = new BufferReader (chunks[uiChunkId].getBuffer(), ui32BufLen, true);
                pFrag->ui64FragLen = ui32BufLen;
                pFrag->src_type = pszInputMimeType;
                pFrag->out_type = pszOutputMimeType;
                pFrag->ui8Part = uiChunkId;
                pFrag->ui8TotParts = ui8NoOfChunks;
                pFragments->append (pFrag);
            }
        }
    }
    return pFragments;
}

PtrLList<Chunker::Fragment> * ChunkerMarshaling::fragmentFile (const char *pszFileName, const char *pszInputMimeType,
                                                               uint8 ui8NoOfChunks, const char *pszOutputMimeType,
                                                               uint8 ui8ChunkCompressionQuality)
{
    // TODO: implement this
    return NULL;
}

Chunker::Fragment * ChunkerMarshaling::extractFromBuffer (const void *pBuf, uint32 ui32Len, const char *pszInputMimeType, const char *pszOutputMimeType,
                                                          uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals)
{
    ExtractFromBufferArguments args (pBuf, ui32Len, pszInputMimeType, pszOutputMimeType, ui8ChunkCompressionQuality, ppPortionIntervals);
    if (args.write (_pCommHelper->getWriterRef()) < 0) {
        return NULL;
    }

    Chunker::Fragment *pFrag = new Chunker::Fragment();
    if (pFrag == NULL) {
        return NULL;
    }

    BufferReader *pBR = new BufferReader();
    if (CHUNKER_MARSHALLING::readBlock (_pCommHelper->getReaderRef(), *pBR) < 0) {
        return NULL;
    }
    pFrag->pReader = pBR;
    pFrag->src_type = pszInputMimeType;
    pFrag->out_type = pszOutputMimeType;
    pFrag->ui8Part = 0;
    pFrag->ui8TotParts = 1;
    return pFrag;
}

Chunker::Fragment * ChunkerMarshaling::extractFromFile (const char *pszFileName, const char *pszInputMimeType, const char *pszOutputMimeType,
                                                        uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals)
{
    // TODO: implement this
    return NULL;
}

//---------------------------------------------------------

ChunkReassemblerMarshaling::ChunkReassemblerMarshaling (SimpleCommHelper2 *pCommHelper)
    : _pCommHelper (pCommHelper)
{
}

ChunkReassemblerMarshaling::~ChunkReassemblerMarshaling (void)
{
}

NOMADSUtil::BufferReader * ChunkReassemblerMarshaling::reassemble (DArray2<BufferReader> *pFragments, Annotations *pAnnotations,
                                                                   const char *pszChunkMimeType, uint8 ui8NoOfChunks, uint8 ui8CompressionQuality)
{
    const unsigned int uiSize = pFragments->size();
    Chunks chunks (false, uiSize);
    for (unsigned int i = 0; i < uiSize; i++) {
        if (pFragments->used (i)) {
            chunks[i].init ((*pFragments)[i].getBuffer(), (*pFragments)[i].getBufferLength(), false);
        }
    }

    ReassembleArguments args (&chunks, pAnnotations, pszChunkMimeType, ui8NoOfChunks, ui8CompressionQuality);
    if (args.write (_pCommHelper->getWriterRef()) < 0) {
        return NULL;
    }

    uint32 ui32BufLen = 0U;
    void *pBuf = NULL;
    if (CHUNKER_MARSHALLING::readBlock (_pCommHelper->getReaderRef(), pBuf, ui32BufLen) < 0) {
        return NULL;
    }

    return new BufferReader (pBuf, ui32BufLen, true);
}

