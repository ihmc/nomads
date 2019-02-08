/*
 * BMPHandler.h
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
 */

#ifndef INCL_BMP_HANDLER_H
#define INCL_BMP_HANDLER_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class BMPImage;
    class BufferReader;
}

namespace IHMC_MISC
{
    class BMPChunker
    {
        public:
            static NOMADSUtil::BMPImage * fragmentBMP (NOMADSUtil::BMPImage *pSourceImage, uint8 ui8DesiredChunkId, uint8 ui8TotalNoOfChunks);
            static NOMADSUtil::BMPImage * extractFromBMP (NOMADSUtil::BMPImage *pSourceImage, uint32 ui32StartX, uint32 ui32EndX, uint32 ui32StartY, uint32 ui32EndY);
    };

    class BMPReassembler
    {
        public:
            BMPReassembler (void);
            ~BMPReassembler (void);

            int init (uint8 ui8TotalNoOfChunks);
            int incorporateChunk (NOMADSUtil::BMPImage *pChunk, uint8 ui8ChunkId);

            // NOTE: if new chunks are added, (part of) the changes to the image added
            // be the annotation may be overrridden. Annotations should be added to the
            // image after all chunks have been added
            // NOTE: the caller should implement the logic to avoid the addition of
            // overriding annotations
            int incorporateChunk (NOMADSUtil::BMPImage *pChunk, uint32 ui32StartX,
                                  uint32 ui32EndX, uint32 ui32StartY, uint32 ui32EndY);

            // Returns the BMPImage that is a result of all of the chunks incorporated
            // in the added annotations
            // NOTE: Caller must not modify this image
            // NOTE: The image wil be deleted when the instance of BMPReassembler is deleted
            const NOMADSUtil::BMPImage * getAnnotatedImage (void);

            // Returns the BMPImage that is a result of all of the chunks incorporated
            // NOTE: This method is not a simple accessor - it does interpolation of the
            //       pixel values for the missing chunks and is hence expensive
            // NOTE: Caller must not modify this image
            // NOTE: The image wil be deleted when the instance of BMPReassembler is deleted
            const NOMADSUtil::BMPImage * getReassembledImage (void);

        private:
            int interpolate (void);

        private:
            bool _addedAnnotations;
            uint8 _ui8TotalNoOfChunks;
            uint8 _ui8XIncr;
            uint8 _ui8YIncr;
            bool _abAddedChunks[256];
            NOMADSUtil::BMPImage *_pResultingImage;
    };
}

#endif   // #ifndef INCL_BMP_HANDLER_H
