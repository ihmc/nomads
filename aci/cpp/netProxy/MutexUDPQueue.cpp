/*
 * MutexUDPQueue.cpp
 *
 * This file is part of the IHMC NetProxy Library/Component
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

#include "SequentialArithmetic.h"

#include "MutexUDPQueue.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    UDPDatagramPacket * MutexUDPQueue::findPacketFromIPHeader (const IPHeader * const pIPHeader)
    {
        if (!pIPHeader) {
            return NULL;
        }

        resetGet();
        UDPDatagramPacket *pUDPDatagramPacket = NULL;
        while ((pUDPDatagramPacket = getNext())) {
            if (pUDPDatagramPacket->matchesIPPacket (pIPHeader)) {
                return pUDPDatagramPacket;
            }
        }

        return NULL;
    }

    int MutexUDPQueue::enqueue (UDPDatagramPacket * const pUDPDatagramPacket)
    {
        if (!pUDPDatagramPacket) {
            return ENQUEUING_NULL;
        }

        if (_ui32EnqueuedBytes >= _ui32MaxEnqueuableBytes) {
            // Enqueuing is forbidden only if limit has already been reached
            return ENQUEUING_BUFFER_FULL;
        }

        if (0 != PtrQueue<UDPDatagramPacket>::enqueue (pUDPDatagramPacket)) {
            return ENQUEUING_ERROR;
        }
        // If packet is not yet complete, the amount of currently enqueued bytes is not increased
        _ui32EnqueuedBytes += pUDPDatagramPacket->getPacketLen();

        return ENQUEUING_SUCCEDED;
    }

    int MutexUDPQueue::reassembleUDPDatagram (const IPHeader * const pIPHeader, const UDPHeader * const pUDPHeader)
    {
        if (!pIPHeader || !pUDPHeader) {
            return REASSEMBLING_NULL;
        }

        if ((pIPHeader->ui16FlagsAndFragOff & IP_OFFSET_FILTER) != 0) {
            // Fragment received --> looking for relative incomplete UDP datagram
            UDPDatagramPacket *pUDPDatagramPacket;
            resetGet();
            while ((pUDPDatagramPacket = getNext())) {
                if (!pUDPDatagramPacket->isDatagramComplete() &&
                    SequentialArithmetic::lessThan (pUDPDatagramPacket->getIPIdentification(), pIPHeader->ui16Ident)) {
                    // Old incomplete fragments can be deleted from the queue
                    delete remove (pUDPDatagramPacket);
                }
                else if (pUDPDatagramPacket->isMissingFragment (pIPHeader)) {
                    // Previous fragment found --> reassembling and returning result
                    int rc = pUDPDatagramPacket->reassembleFragment (pIPHeader, (uint8*) pUDPHeader);
                    if (rc < 0) {
                        return REASSEMBLING_ERROR;
                    }
                    if (rc == 0) {
                        // rc is 0 if fragment is not yet complete --> return REASSEMBLING_INCOMPLETE
                        return REASSEMBLING_INCOMPLETE;
                    }
                    // Fragment is now complete --> incrementing _ui32EnqueuedBytes and returning REASSEMBLING_COMPLETE
                    _ui32EnqueuedBytes += rc;

                    return REASSEMBLING_COMPLETE;
                }
            }
        }

        return REASSEMBLING_IMPOSSIBLE;
    }
}
