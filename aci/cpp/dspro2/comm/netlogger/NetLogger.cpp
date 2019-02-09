/*
 * NetLogger.cpp
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

#include "NetLogger.h"

#include "BufferReader.h"
#include "CommAdaptorManager.h"
#include "NLFLib.h"

#include "measure.pb.h"

#include "ConfigManager.h"

#include <time.h>

#if defined (WIN32)
    #include <sys/timeb.h>
#elif defined (LINUX) || defined (MACOSX)
    #include <sys/time.h>
#endif

namespace IHMC_ACI
{
    NetLogger *pNetLogger;
}

using namespace NOMADSUtil;
using namespace IHMC_ACI;

namespace NET_LOGGER
{
    struct SubjectInfo
    {
        measure::Subject subject;
        IHMC_ACI::AdaptorType adaptorType;
    };

    static SubjectInfo SUBJECT_INFOS[5] = {
        { measure::dspro_log, IHMC_ACI::AdaptorType::NATS },
        { measure::dspro_log, IHMC_ACI::AdaptorType::NATS },
        { measure::dspro_publications, IHMC_ACI::AdaptorType::NATS },
        { measure::dspro_matches, IHMC_ACI::AdaptorType::NATS },
        { measure::disservice, IHMC_ACI::AdaptorType::UDP }
    };

    void fillTimestamp (google::protobuf::Timestamp &timestamp)
    {
    #if defined (WIN32)
        FILETIME ft;
        GetSystemTimeAsFileTime (&ft);
        UINT64 ticks = (((UINT64)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;

        timestamp.set_seconds ((INT64)((ticks / 10000000) - 11644473600LL));
        timestamp.set_nanos ((INT32)((ticks % 10000000) * 100));
    #else
        struct timeval tv;
        gettimeofday (&tv, nullptr);
        timestamp.set_seconds (tv.tv_sec);
        timestamp.set_nanos (tv.tv_usec * 1000);
    #endif
    }

    int toByteArray (const measure::Measure &measure, BufferReader &br)
    {
        const int iLen = measure.ByteSize();
        if (iLen <= 0) {
            return -1;
        }
        char *pBuf = reinterpret_cast<char *> (malloc (iLen));
        if (pBuf == nullptr) {
            return -2;
        }
        if (!measure.SerializeToArray (pBuf, iLen)) {
            free (pBuf);
            return -3;
        }
        br.init (pBuf, iLen, true);
        return 0;
    }

    int sendMeasure (CommAdaptorManager *pCommAdptMgr, NetLogger::Measure *pMeasure)
    {
        if ((pCommAdptMgr == nullptr) || (pMeasure == nullptr)) {
            return -1;
        }

        BufferReader br;
        if (pMeasure->fillByteArray (br) < 0) {
            return -2;
        }

        // Select proper adaptor
        if (pMeasure->_subject >= sizeof (SUBJECT_INFOS)) {
            return -3;
        }
        const IHMC_ACI::AdaptorType type = SUBJECT_INFOS[pMeasure->_subject].adaptorType;
        CommAdaptor *pAdaptor = pCommAdptMgr->getAdaptorByType (type);
        if (pAdaptor == nullptr) {
            return -4;
        }

        // Send measure
        if (pAdaptor->notifyEvent (br.getBuffer(), br.getBufferLength(), nullptr,
                                   pMeasure->_topic.c_str(), nullptr) < 0) {
            return -5;
        }

        return 0;
    }

    class MeasureImpl : public NetLogger::Measure
    {
        public:
            explicit MeasureImpl (const char *pszTopic, Subject subject)
                : Measure (pszTopic, subject),
                  _pTimestamp (new google::protobuf::Timestamp())
            {
                _pUint32Map = _measure.mutable_integers();
                _pStrMap = _measure.mutable_strings();
            }

            ~MeasureImpl (void) {}

            void addUI32 (const char *pszKey, uint32 ui32Value)
            {
                if (pszKey != nullptr) {
                    (*_pUint32Map)[pszKey] = ui32Value;
                }
            }

            void addUI64 (const char *pszKey, uint64 ui64Value)
            {
                if (pszKey != nullptr) {
                    (*_pUint32Map)[pszKey] = ui64Value;
                }
            }

            void addString (const char *pszKey, const char*pszValue)
            {
                if (pszKey != nullptr) {
                    std::string value (pszValue);
                    (*_pStrMap)[pszKey] = value;
                }
            }

            void addHeader (std::string nodeId)
            {
                (*_pStrMap)["node_id"] = nodeId;

                fillTimestamp (*_pTimestamp);
                _measure.set_allocated_timestamp (_pTimestamp);
                if (_subject < sizeof (SUBJECT_INFOS)) {
                    _measure.set_subject (SUBJECT_INFOS[_subject].subject);
                }
            }

            int fillByteArray (BufferReader &br)
            {
                if (toByteArray (_measure, br) < 0) {
                    return -1;
                }
                return 0;
            }

        private:
            google::protobuf::Timestamp *_pTimestamp;
            google::protobuf::Map<std::string, google::protobuf::int64> *_pUint32Map;
            google::protobuf::Map<std::string, std::string> *_pStrMap;
            measure::Measure _measure;
    };

    const char * TOPIC = "dspro.log";
    const char * CONFIG = "dspro.config";
}

using namespace NET_LOGGER;

const char * NetLogger::ENABLE_PROPERTY = "netlogger.enable";

NetLogger::NetLogger (const char *pszNodeId)
    : _nodeId (pszNodeId), _pCommAdptMgr (nullptr)
{
}

NetLogger::~NetLogger (void)
{
}

int NetLogger::init (CommAdaptorManager *pCommAdptMgr)
{
    if (pCommAdptMgr == nullptr) {
        return -1;
    }
    _pCommAdptMgr = pCommAdptMgr;
    return 0;
}

int NetLogger::notify (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == nullptr) {
        return -1;
    }

    MeasureImpl measure (CONFIG, Measure::CONF);
    StringStringHashtable::Iterator iter = pCfgMgr->getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        measure.addString (iter.getKey(), iter.getValue());
    }

    return notify (&measure);
}

int NetLogger::notify (const char *pszMessage)
{
    if (pszMessage == nullptr) {
        return -1;
    }

    MeasureImpl measure (TOPIC, Measure::LOG);
    measure.addString ("message", pszMessage);

    return notify (&measure);
}

int NetLogger::notify (Measure *pMeasure)
{
    if (pMeasure == nullptr) {
        return -1;
    }
    pMeasure->addHeader (_nodeId);
    if (sendMeasure (_pCommAdptMgr, pMeasure)) {
        return -2;
    }
    return 0;
}

NetLogger::Measure::Measure (const char *pszTopic, Subject subject)
    : _subject (subject), _topic (pszTopic)
{
}

NetLogger::Measure::~Measure (void)
{
}

NetLogger::Measure * MeasureFactory::createMeasure (const char *pszTopic, NetLogger::Measure::Subject subject)
{
    return new MeasureImpl (pszTopic, subject);
}
