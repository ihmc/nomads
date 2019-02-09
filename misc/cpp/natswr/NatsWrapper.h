/*
 * NatsWrapper.h
 *
 * This file is part of the IHMC NetSensor Library/Component
 * Copyright (c) 2010-2018 IHMC.
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
 * This object wraps NATS and provide publish/subscribe API
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#ifndef INCL_NATS_WRAPPER
#define INCL_NATS_WRAPPER

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace NATS_WRAPPER_IMPL
{
    struct Nats;
}

namespace IHMC_MISC_NATS
{
    class NatsWrapper
    {
        public:
            class Listener
            {
                public:
                    virtual void messageArrived (const char *pszTopic, const void *pMsg, int iLen) = 0;
            };

        public:
            NatsWrapper (bool bAsynchronous);
            ~NatsWrapper (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);
            int init (const char *pszBrokerHost, int iPort);

            int publish (const char *pszTopic, const void *pMsg, int iLen);
            int subscribe (const char *pszTopic, Listener *pListener);

            static int DEFAULT_PORT;

        private:
            NATS_WRAPPER_IMPL::Nats *_pNats;
    };
}

#endif  /* INCL_NATS_WRAPPER */

