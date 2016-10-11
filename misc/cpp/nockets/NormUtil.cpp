/*
 * NormUtil.cpp
 *
 * This file is part of the IHMC NORM Socket Library.
 * Copyright (c) 2016 IHMC.
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#include "NormUtil.h"

#include <assert.h>

using namespace IHMC_MISC;

const char * IHMC_MISC::getNormEventAsString (NormEventType e)
{
    switch (e) {
        case NORM_EVENT_INVALID: return "NORM_EVENT_INVALID";
        case NORM_TX_QUEUE_VACANCY: return "NORM_TX_QUEUE_VACANCY";
        case NORM_TX_QUEUE_EMPTY: return "NORM_TX_QUEUE_EMPTY";
        case NORM_TX_FLUSH_COMPLETED: return "NORM_TX_FLUSH_COMPLETED";
        case NORM_TX_WATERMARK_COMPLETED: return "NORM_TX_WATERMARK_COMPLETED";
        case NORM_TX_CMD_SENT: return "NORM_TX_CMD_SENT";
        case NORM_TX_OBJECT_SENT: return "NORM_TX_OBJECT_SENT";
        case NORM_TX_OBJECT_PURGED: return "NORM_TX_OBJECT_PURGED";
        case NORM_TX_RATE_CHANGED: return "NORM_TX_RATE_CHANGED";
        case NORM_LOCAL_SENDER_CLOSED: return "NORM_LOCAL_SENDER_CLOSED";
        case NORM_REMOTE_SENDER_NEW: return "NORM_REMOTE_SENDER_NEW";
        case NORM_REMOTE_SENDER_RESET: return "NORM_REMOTE_SENDER_RESET";
        case NORM_REMOTE_SENDER_ADDRESS: return "NORM_REMOTE_SENDER_ADDRESS";
        case NORM_REMOTE_SENDER_ACTIVE: return "NORM_REMOTE_SENDER_ACTIVE";
        case NORM_REMOTE_SENDER_INACTIVE: return "NORM_REMOTE_SENDER_INACTIVE";
        case NORM_REMOTE_SENDER_PURGED: return "NORM_REMOTE_SENDER_PURGED";
        case NORM_RX_CMD_NEW: return "NORM_RX_CMD_NEW";
        case NORM_RX_OBJECT_NEW: return "NORM_RX_OBJECT_NEW";
        case NORM_RX_OBJECT_INFO: return "NORM_RX_OBJECT_INFO";
        case NORM_RX_OBJECT_UPDATED: return "NORM_RX_OBJECT_UPDATED";
        case NORM_RX_OBJECT_COMPLETED: return "NORM_RX_OBJECT_COMPLETED";
        case NORM_RX_OBJECT_ABORTED: return "NORM_RX_OBJECT_ABORTED";
        case NORM_GRTT_UPDATED: return "NORM_GRTT_UPDATED";
        case NORM_CC_ACTIVE: return "NORM_CC_ACTIVE";
        case NORM_CC_INACTIVE: return "NORM_CC_INACTIVE";
        case NORM_ACKING_NODE_NEW: return "NORM_ACKING_NODE_NEW";
        case NORM_SEND_ERROR: return "NORM_SEND_ERROR";
        case NORM_USER_TIMEOUT: return "NORM_USER_TIMEOUT";
        default:
            assert (false);
            return "UNKNOWN";
    }
}

