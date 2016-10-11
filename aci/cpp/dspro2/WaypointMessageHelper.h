/* 
 * WaypointMessageHelper.h
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Created on August 10, 2012, 12:41 PM
 */

#ifndef INCL_WAYPOINT_MESSAGE_HELPER_H
#define	INCL_WAYPOINT_MESSAGE_HELPER_H

#include "Defs.h"
#include "PreviousMessageIds.h"

namespace IHMC_ACI
{
    class WaypointMessageHelper
    {
        public:
            static int readWaypointMessageForTarget (const void *pWaypointMsgPayload, uint32 ui32Offset, uint32 ui32TotalLen,
                                                     PreviousMessageIds &previouMessagesSentToTargets, uint32 &ui32WaypointMsgPayloadLen);

            static void * writeWaypointMessageForTarget (PreviousMessageIds &previouMessagesSentToTargets,
                                                         const void *pWaypointMsgPayload, uint32 ui32WaypointMsgPayloadLen,
                                                         uint32 &ui32TotalLen);
    };
}

#endif	/* INCL_WAYPOINT_MESSAGE_HELPER_H */

