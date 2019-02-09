/*
 * SessionId.h
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
 * Created on June 21, 2018, 1:02 PM
 */

#ifndef INCL_SESSION_ID_H
#define	INCL_SESSION_ID_H

#include <vector>
#include <mutex>

#include "StrClass.h"
#include "ConfigManager.h"

namespace IHMC_ACI
{
    class SessionIdListener
    {
        public:
            virtual void sessionIdChanged (void) = 0;
    };

    class SessionId
    {
        public:
            static const NOMADSUtil::String MIME_TYPE;

            ~SessionId (void);

            NOMADSUtil::String getSessionId (void) const;
            NOMADSUtil::String getSessionIdAndTimestamp (int64 &i64Timestamp) const;
            void setSessionId (const char *pszSessionId, int64 i64Timestamp);

            void registerSessionIdListener (SessionIdListener *pListener);

            SessionId (SessionId const&) = delete;
            void operator=(SessionId const&) = delete;

            static SessionId * getInstance (void);

            static NOMADSUtil::String getSessionId (NOMADSUtil::ConfigManager *pCfgMgr);

        private:
            SessionId (void);

        private:
            mutable std::mutex _m;
            int64 _i64Timestamp;
            NOMADSUtil::String _sessionId;
            std::vector<SessionIdListener*> _listeners;

            static SessionId *INSTANCE;
    };

    inline NOMADSUtil::String SessionId::getSessionId (void) const
    {
        _m.lock();
        NOMADSUtil::String cpy (INSTANCE->_sessionId);
        _m.unlock();
        return cpy;
    }

    inline NOMADSUtil::String SessionId::getSessionIdAndTimestamp (int64 &i64Timestamp) const
    {
        _m.lock();
        NOMADSUtil::String cpy (INSTANCE->_sessionId);
        i64Timestamp = INSTANCE->_i64Timestamp;
        _m.unlock();
        return cpy;
    }

    inline SessionId * SessionId::getInstance (void)
    {
        return INSTANCE;
    }
}

#endif  /* INCL_SESSION_ID_H */

