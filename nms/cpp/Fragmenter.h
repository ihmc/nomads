/* 
 * Fragmenter.h
 *
 * This file is part of the IHMC Network Message Service Library
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 19, 2015, 3:32 AM
 */

#ifndef INCL_FRAGMENTER_H
#define	INCL_FRAGMENTER_H

#include "MessageFactory.h"

namespace NOMADSUtil
{
    class Fragmenter
    {
        public:
            Fragmenter (uint16 ui16PayLoadLen,
                        const void *pMetadata, uint16 ui16MetadataLen,
                        const void *pData, uint16 ui16DataLen);
            ~Fragmenter (void);

            const void * getNext (uint16 &ui16MetadataLen, uint16 &ui16DataLen, NetworkMessage::ChunkType &chunkType);

        private:
            const uint16 _ui16PayLoadLen;
            uint16 _ui16Offset;
            const uint16 _ui16MetadataLen;
            const uint16 _ui16DataLen;
            const uint16 _ui16DataAndMetadataLen;
            const char *_pMetadata;
            const char *_pData;
    };
}

#endif	/* INCL_FRAGMENTER_H */

