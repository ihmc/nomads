/* 
 * RegistryInterface.h
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
 * authors : Giacomo Benincasa   (gbenincasa@ihmc.us)
 * Created on February 16, 2015, 11:50 PM
 */

#ifndef INCL_REGISTRY_INTERFACE_H
#define	INCL_REGISTRY_INTERFACE_H

#include "FTypes.h"

#include "Callback.h"
#include "SimpleCommHelper2.h"

namespace NOMADSUtil
{
    template<class S> class Skeleton;

    template<class S>
    class RegistryInterface
    {
        public:
            virtual bool deregisterAllCallbacks (void) = 0;
            virtual bool deregisterCallback (uint16 ui16ApplicationId, CallbackExecutor *pCBackExec,
                                             SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error) = 0;
            virtual Skeleton<S> * getSkeleton (uint16 ui16ApplicationId) = 0;
            virtual S * getSvcInstace (void) = 0;
            virtual bool registerCallback (uint16 ui16ApplicationId, SimpleCommHelper2 *pCommHelper,
                                           SimpleCommHelper2 *pCallbackCommHelper, SimpleCommHelper2::Error &error) = 0;
            virtual uint16 registerSkeleton (uint16 ui16ApplicationId, Skeleton<S> *pAdapor) = 0;
            virtual void removedSkeleton (uint16 ui16ApplicationId) = 0;
    };
}

#endif	/* INCL_REGISTRY_INTERFACE_H */

