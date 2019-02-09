#ifndef INCL_CHUNKING_API_MARSHALINGS_H
#define	INCL_CHUNKING_API_MARSHALINGS_H

#include "ChunkingManager.h"

namespace NOMADSUtil
{
    class SimpleCommHelper2;
}

namespace IHMC_MISC
{
    class ChunkerMarshaling : public ChunkerInterface
    {
        public:
            explicit ChunkerMarshaling (NOMADSUtil::SimpleCommHelper2 *pCommHelper);
            ~ChunkerMarshaling (void);

            NOMADSUtil::PtrLList<Chunker::Fragment> * fragmentBuffer (const void *pBuf, uint32 ui32Len, const char *pszInputMimeType,
                                                                      uint8 ui8NoOfChunks, const char *pszOutputMimeType,
                                                                      uint8 ui8ChunkCompressionQuality);
            NOMADSUtil::PtrLList<Chunker::Fragment> * fragmentFile (const char *pszFileName, const char *pszInputMimeType,
                                                                    uint8 ui8NoOfChunks, const char *pszOutputMimeType,
                                                                    uint8 ui8ChunkCompressionQuality);
            Chunker::Fragment * extractFromBuffer (const void *pBuf, uint32 ui32Len, const char *pszInputMimeType, const char *pszOutputMimeType,
                                                   uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals);
            Chunker::Fragment * extractFromFile (const char *pszFileName, const char *pszInputMimeType, const char *pszOutputMimeType,
                                                 uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals);

        private:
            NOMADSUtil::SimpleCommHelper2 *_pCommHelper;
    };

    class ChunkReassemblerMarshaling : public ChunkReassemblerInterface
    {
        public:
            explicit ChunkReassemblerMarshaling (NOMADSUtil::SimpleCommHelper2 *pCommHelper);
            ~ChunkReassemblerMarshaling (void);

            NOMADSUtil::BufferReader * reassemble (NOMADSUtil::DArray2<NOMADSUtil::BufferReader> *pFragments,
                                                   Annotations *pAnnotations, const char *pszChunkMimeType,
                                                   uint8 ui8NoOfChunks, uint8 ui8CompressionQuality);

        private:
            NOMADSUtil::SimpleCommHelper2 *_pCommHelper;
    };
}


#endif  /* INCL_CHUNKING_API_MARSHALINGS_H */

