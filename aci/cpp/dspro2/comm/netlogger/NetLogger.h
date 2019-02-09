/*
 * NetLogger.h
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Created on February 20, 2017, 9:45 PM
 */

#ifndef INCL_NET_LOGGER_H
#define INCL_NET_LOGGER_H

#include "BufferReader.h"

#include <string>

#define checkAndNotify if (pNetLogger != nullptr) pNetLogger->notify

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class CommAdaptorManager;

    class NetLogger
    {
        public:
            NetLogger (const char *pszNodeId);
            ~NetLogger (void);

            int init (CommAdaptorManager *pCommAdptMgr);

            class Measure
            {
                public:
                    enum Subject {
                        CONF    = 0x00,
                        LOG     = 0x01,
                        MATCH   = 0x02,
                        PUBS    = 0x03,
                        NET     = 0x04,
                    };

                    explicit Measure (const char *pszTopic, Subject subject);
                    virtual ~Measure (void);

                    virtual void addUI32 (const char *pszKey, uint32 ui32Value) = 0;
                    virtual void addUI64 (const char *pszKey, uint64 ui64Value) = 0;
                    virtual void addString (const char *pszKey, const char *pszValue) = 0;
                    virtual int fillByteArray (NOMADSUtil::BufferReader &br) = 0;

                private:
                    friend class NetLogger;
                    virtual void addHeader (std::string nodeId) = 0;

                public:
                    Subject _subject;
                    const std::string _topic;
            };

            int notify (NOMADSUtil::ConfigManager *pCfgMgr);
            int notify (const char *pszMessage);
            int notify (Measure *pMeasure);

            static const char * ENABLE_PROPERTY;

        private:
            const std::string _nodeId;
            CommAdaptorManager *_pCommAdptMgr;
    };

    extern NetLogger *pNetLogger;

    class MeasureFactory
    {
        public:
            static NetLogger::Measure * createMeasure (const char *pszTopic, NetLogger::Measure::Subject subject);
    };
}

#endif  /* INCL_NET_LOGGER_H */
