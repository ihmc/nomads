/*
 * ResourceLocker.h
 *
 *This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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
 * Author: Giacomo Benincasa	(gbenincasa@ihmc.us)
 * Created on March 3, 2010, 5:02 PM
 */

#ifndef INCL_RESURCE_LOCK_H
#define	INCL_RESURCE_LOCK_H

#include "FTypes.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{
    class ResourceLocker
    {
        public:
            ResourceLocker (void);
            virtual ~ResourceLocker (void);

            /*
             * These methods only increment/decrement the counter of the
             * references to the cached element that is being/has been locked.
             *
             * In case the counter does not exist it is created. The value of
             * the key is not copied. Different implementation of lock may pass
             * a copy of the key.
             *
             * In case the counter reaches 0, the value, and only the value, is
             * deleted. Different implementation of relese may delete the key as
             * well.
             */
            int lockResource (const char * pszResourceId);
            int unlockResource (const char * pszResourceId);
            bool isResourceLocked (const char * pszResourceId);

        private:
            int removeLock (const char * pszResourceId);

            typedef uint16 LockCounter;
            NOMADSUtil::StringHashtable<LockCounter> _lockedResources;
    };
}

#endif	// INCL_RESURCE_LOCK_H
