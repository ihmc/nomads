/* 
 * Unmarshaller.h
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
 * Created on February 27, 2015, 3:38 PM
 */

#ifndef INCL_UNMARSHALLER_H
#define	INCL_UNMARSHALLER_H

#include "FTypes.h"
#include "SimpleCommHelper2.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class Stub;

    typedef bool (*UnmarshalFnPtr) (uint16 uiApplicationId, const String &methoName,
                                    void *pSvc, SimpleCommHelper2 *pCommHelper,
                                    SimpleCommHelper2::Error &error);

    typedef bool (*StubUnmarshalFnPtr) (uint16 uiApplicationId, const String &methoName,
                                        Stub *pStub, SimpleCommHelper2 *pCommHelper);
}


#endif	/* INCL_UNMARSHALLER_H */

