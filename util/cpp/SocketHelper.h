/*
* SocketHelper.h
*
* This file is part of the IHMC Util Library
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
* Author: Giacomo Benincasa     (gbenincasa@ihmc.us)
*/

#ifndef INCL_SOCKET_HELPER_H
#define INCL_SOCKET_HELPER_H

namespace NOMADSUtil
{
    class SocketHelper
    {
        public:
            static int getLastError (void);
            static const char * getLastErrorDescription (void);
    };
}

#endif  // INCL_SOCKET_HELPER_H

