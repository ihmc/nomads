/*
 * ControllerFactory.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on January 28, 2009, 3:38 PM
 */

#ifndef INCL_CONTROLLER_FACTORY_H
#define	INCL_CONTROLLER_FACTORY_H

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class AckController;
    class DataCacheExpirationController;
    class DataCacheReplicationController;
    class DisseminationService;
    class ForwardingController;
    class SubscriptionForwardingController;

    class ControllerFactory
    {
        public:
            static void init (DisseminationService *pDisService, NOMADSUtil::ConfigManager *pConfigManager);

            static AckController * getAckControllerAndRegisterListeners (void);
            static DataCacheExpirationController * getExpControllerAndRegisterListeners (void);
            static DataCacheReplicationController * getRepControllerAndRegisterListeners (void);
            static ForwardingController * getForwControllerAndRegisterListeners (void);
            static SubscriptionForwardingController * getSubForwControllerAndRegisterListeners (void);
    };
}

#endif  // INCL_CONTROLLER_FACTORY_H
