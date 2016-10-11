/* 
 * Fragmenter.cpp
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

#include "Fragmenter.h"

#include "NLFLib.h"

namespace NOMADSUtil
{
    NetworkMessage::ChunkType getChunkType (uint16 ui16Offset, uint16 ui16FragLen, uint16 ui16DataAndMetadataLen)
    {
        if (ui16Offset == 0) {
            return NetworkMessage::CT_DataMsgStart;
        }
        else if ((ui16Offset + ui16FragLen ) < ui16DataAndMetadataLen) {
            return NetworkMessage::CT_DataMsgInter;
        }
        else {
            return NetworkMessage::CT_DataMsgEnd;
        }
    }
}

using namespace NOMADSUtil;

Fragmenter::Fragmenter (uint16 ui16PayLoadLen,
                        const void *pMetadata, uint16 ui16MetadataLen,
                        const void *pData, uint16 ui16DataLen)
    : _ui16PayLoadLen (ui16PayLoadLen),
      _ui16Offset (0),
      _ui16MetadataLen (ui16MetadataLen),
      _ui16DataLen (ui16DataLen),
      _ui16DataAndMetadataLen (_ui16MetadataLen + _ui16DataLen),
      _pMetadata ((const char *) pMetadata), _pData ((const char *) pData)
{
}

Fragmenter::~Fragmenter (void)
{
}

const void * Fragmenter::getNext (uint16 &ui16MetadataLen, uint16 &ui16DataLen, NetworkMessage::ChunkType &chunkType)
{
    ui16MetadataLen = ui16DataLen = 0;
    for (; _ui16Offset < _ui16DataAndMetadataLen; ) {
        if (_ui16Offset < _ui16MetadataLen) {
            ui16MetadataLen = minimum (_ui16PayLoadLen, (uint16)(_ui16MetadataLen - _ui16Offset));
            chunkType = getChunkType (_ui16Offset, ui16MetadataLen, _ui16DataAndMetadataLen);
            const char *ptmp = _pMetadata + _ui16Offset;
            _ui16Offset += ui16MetadataLen;
            return ptmp;
        }
        else {
            const uint16 ui16DataOffset = (_ui16Offset - _ui16MetadataLen);
            ui16DataLen = minimum (_ui16PayLoadLen, (uint16)(_ui16DataLen - ui16DataOffset));
            chunkType = getChunkType (_ui16Offset, ui16DataLen, _ui16DataAndMetadataLen);
            const char *ptmp = _pData + ui16DataOffset;
            _ui16Offset += ui16DataLen;
            return ptmp;
        }
    }
    return NULL;
}

