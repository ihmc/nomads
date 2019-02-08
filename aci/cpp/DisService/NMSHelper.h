/*
 * NMSHelper.h
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
 * Created on June 24, 2015, 4:30 PM
 */

#ifndef INCL_NMS_HELPER_H
#define	INCL_NMS_HELPER_H

#include "NetworkMessageService.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class NetworkMessageService;
    class NetworkMessageServiceProxy;
    class NetworkMessageServiceProxyServer;
}

namespace IHMC_ACI
{
    class NMSHelper
    {
        public:
            NMSHelper (void);
            ~NMSHelper (void);
            NOMADSUtil::NetworkMessageServiceInterface * getNetworkMessageService (NOMADSUtil::PROPAGATION_MODE mode, uint16 ui16NMSSvcPort, bool bAsyncDelivery,
                                                                                   bool bAsyncTransmission, uint8 ui8MessageVersion, bool bReplyViaUnicast,
                                                                                   const char **ppszBindingInterfaces, const char **ppszIgnoredInterfaces,
                                                                                   const char **ppszAddedInterfaces, const char *pszDestAddr, uint8 ui8McastTTL,
                                                                                   const char *pszSessionKey, bool bPassphraseEncryption,
                                                                                   const char *pszGroupKeyFileName, NOMADSUtil::ConfigManager *pCfgMgr);

            bool isConnected (void);
            int start (void);
            int stop (void);

            int startAdaptors (void);
            int stopAdaptors (void);

            void requestTerminationAndWait (void);

        private:
            NOMADSUtil::NetworkMessageService *_pNMS;
            NOMADSUtil::NetworkMessageServiceProxy *_pProxClient;
            NOMADSUtil::NetworkMessageServiceProxyServer *_pProxySrv;
    };
}

#endif	/* INCL_NMS_HELPER_H */

