/* 
 * WakeOnLAN.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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
 * Author: mbreedy
 * Created on February, 2010.
 */

#ifndef INCL_WAKE_ON_LAN_H
#define INCL_WAKE_ON_LAN_H

#include "FTypes.h"
#include "UDPDatagramSocket.h"

namespace NOMADSUtil
{

    class WakeOnLAN
    {
        public:
            WakeOnLAN (void);
            virtual ~WakeOnLAN (void);
            int init (void);

            int wakeUp (const char *pszMACAddr);
            int wakeUp (uint8 *pui8MACAddr);

        public:
            static const uint16 MAC_ADDR_LEN = 6;    /*!!*/ // NOTE: See dependency in wakeUp() method if this is being changed

        private:
            UDPDatagramSocket _dgSocket;
    };

}

#endif    // #ifndef INCL_WAKE_ON_LAN_H
