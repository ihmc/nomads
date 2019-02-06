/*
 * TCPSegment.cpp
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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
 * Method definitions for the TCPSegment and the ReceivedData classes.
 */

#include <cstdlib>
#include <cstring>
#include <utility>

#include "TCPSegment.h"


namespace ACMNetProxy
{
    int ReceivedData::appendDataToBuffer (const uint8 * const pui8Data, uint32 ui32DataLen)
    {
        if (!pui8Data || (ui32DataLen == 0)) {
            return 0;
        }
        if (!canAppendData()) {
            // Cannot append data in this case
            return -1;
        }

        uint32 newDataLen = _uiItemLength + ui32DataLen;
        if (newDataLen <= _uiItemLength) {
            // Overflow --> cannot add so much data to buffer
            return -2;
        }

        if (_uiBufSize < newDataLen) {
            _pData = (unsigned char *) realloc (_pData, newDataLen);
            _uiBufSize = newDataLen;
        }
        memcpy (const_cast<unsigned char *> (getData()) + _uiItemLength, pui8Data, ui32DataLen);
        _uiItemLength = newDataLen;

        return _uiItemLength;
    }

    int ReceivedData::peekOctet (uint32 uiSequenceNumber) const
    {
        if (NOMADSUtil::SequentialArithmetic::lessThan (uiSequenceNumber, _uiSequenceNumber)) {
            return -1;
        }

        uint32 ui32Offset = NOMADSUtil::SequentialArithmetic::delta (uiSequenceNumber, _uiBeginningSequenceNumber);
        if (ui32Offset >= _uiBufSize) {
            return -2;
        }

        return _pData[ui32Offset];
    }

    int ReceivedData::dequeueBytes (uint8 *pBuf, unsigned int uiLen)
    {
        if (!pBuf && (uiLen > 0)) {
            return -1;
        }

        if (uiLen == 0) {
            return 0;
        }
        if (uiLen > _uiItemLength) {
            uiLen = _uiItemLength;
        }

        memcpy (pBuf, getData(), uiLen);
        incrementValues (uiLen);

        return uiLen;
    }

}
