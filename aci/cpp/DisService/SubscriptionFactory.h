/*
 * SubscriptionFactory.h
 *
 * This file is part of the IHMC DisService Library/Component
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 25, 2011, 3:35 PM
 */

#ifndef INCL_SUBSCRIPTION_FACTORY_H
#define	INCL_SUBSCRIPTION_FACTORY_H

#include "FTypes.h"

namespace IHMC_ACI
{
    class Subscription;

    class SubscriptionFactory
    {
        public:
            static Subscription * getSubsctiption (uint8 ui8Priority, bool bGrpReliable, bool bMsgReliable, bool bSequenced);
            static Subscription * getSubsctiption (uint16 ui16Tag, uint8 ui8Priority, bool bGrpReliable, bool bMsgReliable, bool bSequenced);
            static Subscription * getSubsctiption (const char *pszPredicate, uint8 ui8PredicateType, uint8 ui8Priority, bool bGrpReliable, bool bMsgReliable, bool bSequenced);
    };
}

#endif	// INCL_SUBSCRIPTION_FACTORY_H

