/* 
 * Protocol.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 29, 2015, 4:43 PM
 */

#ifndef INCL_PROTOCOL_H
#define	INCL_PROTOCOL_H

#include "StrClass.h"
#include "SimpleCommHelper2.h"

namespace NOMADSUtil
{
    class Protocol
    {
        public:
            static const String REGISTER_PROXY;
            static const String REGISTER_PROXY_CALLBACK;

            static SimpleCommHelper2::Error doHandshake (SimpleCommHelper2 *pCommHelper, const String &service, const String &version);
    };
}

#endif	/* INCL_PROTOCOL_H */

