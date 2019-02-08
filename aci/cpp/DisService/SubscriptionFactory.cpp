/*
 * SubscriptionFactory.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#include "SubscriptionFactory.h"

#include "Subscription.h"

using namespace IHMC_ACI;

Subscription * SubscriptionFactory::getSubsctiption (uint8 ui8Priority, bool bGrpReliable, bool bMsgReliable, bool bSequenced)
{
    return new GroupSubscription (ui8Priority, bGrpReliable, bMsgReliable, bSequenced);
}

Subscription * SubscriptionFactory::getSubsctiption (uint16 ui16Tag, uint8 ui8Priority,
                                                     bool bGrpReliable, bool bMsgReliable, bool bSequenced)
{
    if (ui16Tag == Subscription::DUMMY_TAG) {
        return new GroupSubscription (ui8Priority, bGrpReliable, bMsgReliable, bSequenced);
    }
    else {
         return new GroupTagSubscription (ui16Tag, ui8Priority, bGrpReliable, bMsgReliable, bSequenced);
    }
}

Subscription * SubscriptionFactory::getSubsctiption (const char *pszPredicate, uint8 ui8PredicateType,
                                                     uint8 ui8Priority, bool bGrpReliable, bool bMsgReliable, bool bSequenced)
{
    return new GroupPredicateSubscription (pszPredicate, ui8PredicateType,
                                           ui8Priority, bGrpReliable, bMsgReliable, bSequenced);
}

