/*
 * NatsWrapper.cpp
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#include "NatsWrapper.h"

#include "nats.h"

#include "ConcurrentQueue.h"
#include "ConfigManager.h"
#include "Logger.h"

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <sstream>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_MISC_NATS;
using namespace NOMADSUtil;
using std::function;
using std::lock_guard;
using std::multimap;
using std::pair;
using std::string;
using std::mutex;
using std::thread;

namespace NATS_WRAPPER_IMPL
{
    template <typename T>
    string toString(T value)
    {
        std::ostringstream os ;
        os << value ;
        return os.str() ;
    }

    string toURL (const string addr, int iPort) {
        if (iPort < 0) {
            return string ();
        }
        string url ("nats://");
        url += addr;
        url += ':';
        url += toString (iPort);
        return url;
    }

    string errorCodeToString (int rc) {
        switch (rc) {
            case NATS_OK: return string ("OK");
            case NATS_PROTOCOL_ERROR: return string ("Protocol Error");
            case NATS_IO_ERROR: return string ("IO Error");
            case NATS_CONNECTION_CLOSED: return string ("Connection Closed");
            case NATS_NO_SERVER: return string ("No Server");
            case NATS_STALE_CONNECTION: return string ("Stale Connection");
            case NATS_SECURE_CONNECTION_WANTED: return string ("Secure Connection Wanted");
            case NATS_SECURE_CONNECTION_REQUIRED: return string ("Secure Connection Required");
            case NATS_CONNECTION_AUTH_FAILED: return string ("Authentication Failed");
            case NATS_ADDRESS_MISSING: return string ("Incorrect URL");
            case NATS_TIMEOUT: return string ("An operation times out");
            case NATS_SSL_ERROR: return string ("SSL Error");
            default: {
                string msg ("Error.  Return code: ");
                msg = msg + toString (rc);
                return msg;
            }
        }
    }

    void logrc (const char *pszMethodName, int rc)
    {
        switch (rc) {
            case NATS_OK: break;
            default:
                string err (errorCodeToString (rc));
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "NATS operation failed: %s.\n", err.c_str ());
        }
    }

    // ------------------------------------------------------------------------

    typedef multimap<string, NatsWrapper::Listener *> MultiMap;
    typedef pair<string, NatsWrapper::Listener *> MultiMapPair;

    struct MessageArrivedCb
    {
        MessageArrivedCb (const char *pszTopic, const void *pMsg, int iLen);
        ~MessageArrivedCb (void);

        const string topic;
        void *pMsgBuf;
        const int iMsgBufLen;
    };

    class Notifier
    {
        public:
            Notifier (bool bAsynchronous);
            ~Notifier (void);

            void init (void);
            bool contains (const char *pszTopic) const;
            void addListener (const char *pszTopic, NatsWrapper::Listener *pListener);
            void messageArrived (MessageArrivedCb *pCback);

        private:
            void notify (const MessageArrivedCb *pCback) const;
            void popAndNotify (void);

        private:
            const bool _bAsynchronous;
            mutable mutex _mListenerByTopic;
            MultiMap _listenerByTopic;
            ConcurrentQueue<MessageArrivedCb*> _queue;
    };

    struct Nats
    {
        Nats (bool bAsynchronous);
        ~Nats (void);

        int init (void);
        int connect (string url);
        void destroy (void);
        bool isInitialized (void);

        int publish (const char *pszTopic, const void *pMsg, int iLen);
        int subscribe (const char *pszTopic, NatsWrapper::Listener *pListener);

        private:
            mutex m;
            bool initialized;
            natsConnection *pConnection;
            natsSubscription *pSub;
            Notifier notifier;
    };
}

using namespace NATS_WRAPPER_IMPL;

int NatsWrapper::DEFAULT_PORT = 4222;

NatsWrapper::NatsWrapper (bool bAsynchronous)
    : _pNats (new Nats (bAsynchronous))
{
}

NatsWrapper::~NatsWrapper (void)
{
    delete _pNats;
}

int NatsWrapper::init (ConfigManager *pCfgMgr)
{
    auto pszMethodName = "NatsWrapper::init (1)";
    if (pCfgMgr == nullptr) {
        return -1;
    }
    const string addr (pCfgMgr->getValue ("nats.broker.addr"));
    if (addr.length() <= 0) {
        // Nothing to do, not using NATS
        return 0;
    }
    auto iPort = pCfgMgr->getValueAsInt ("nats.broker.port", 4222);
    return init (addr.c_str(), iPort);
}

int NatsWrapper::init (const char *pszBrokerHost, int iPort)
{
    auto pszMethodName = "NatsWrapper::init (2)";
    const string addr (pszBrokerHost);
    const string url (toURL (addr, iPort));
    auto rc = _pNats->connect (url.c_str());
    switch (rc) {
    case NATS_OK: {
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "NATS connected connected at URL %s.\n", url.c_str());
        _pNats->init();
        break;
    }
    default:
        _pNats->destroy();
        logrc (pszMethodName, rc);
        return -2;
    }
    return 0;
}

int NatsWrapper::publish (const char *pszTopic, const void *pMsg, int iLen)
{
    auto rc =_pNats->publish (pszTopic, pMsg, iLen);
    logrc ("NatsWrapper::publish", rc);
    return (rc == NATS_OK ? 0 : -1);
}

int NatsWrapper::subscribe (const char *pszTopic, Listener *pListener)
{
    auto rc = _pNats->subscribe (pszTopic, pListener);
    logrc ("NatsWrapper::subscribe", rc);
    return (rc == NATS_OK ? 0 : -1);
}

//-----------------------------------------------------------------------------

Nats::Nats (bool bAsynchronous)
    : initialized (false),
      pConnection (nullptr), pSub (nullptr),
      notifier (bAsynchronous)
{}

Nats::~Nats (void)
{
    destroy();
}

int Nats::init (void)
{
    notifier.init();
    return 0;
}

int Nats::connect (string url)
{
    lock_guard<mutex> synchronized (m);
    auto rc = natsConnection_ConnectTo (&pConnection, url.c_str());
    if (rc == NATS_OK) {
        initialized = true;
    }
    return (int)rc;
}

void Nats::destroy (void)
{
    lock_guard<mutex> synchronized (m);
    if (pConnection != nullptr) {
        natsConnection_Destroy (pConnection);
        initialized = false;
    }
}

bool Nats::isInitialized (void)
{
    return initialized;
}

int Nats::publish (const char *pszTopic, const void *pMsg, int iLen)
{
    lock_guard<mutex> synchronized (m);
    if (initialized) {
        auto rc = natsConnection_Publish (pConnection, pszTopic, pMsg, iLen);
        return rc;
    }
    return NATS_ILLEGAL_STATE;
}


int Nats::subscribe (const char *pszTopic, NatsWrapper::Listener *pListener)
{
    lock_guard<mutex> synchronized (m);
    if (!initialized) {
        return NATS_ILLEGAL_STATE;
    }
    if (notifier.contains (pszTopic)) {
        notifier.addListener (pszTopic, pListener);
        return NATS_OK;
    }
    auto rc = natsConnection_Subscribe (&pSub, pConnection, pszTopic, [](natsConnection *, natsSubscription *pSub, natsMsg *pMsg, void *pClosure) {
        const int iLen = natsMsg_GetDataLength (pMsg);
        const void *pMsgBuf = natsMsg_GetData (pMsg);
        if ((pMsgBuf != nullptr) && (iLen > 0)) {
            ((Notifier *)pClosure)->messageArrived (new MessageArrivedCb (
                natsMsg_GetSubject (pMsg), pMsgBuf, iLen));
        }
        natsMsg_Destroy (pMsg); // MessageArrivedCb made a copy of everything, can destroy
    }, &notifier);
    notifier.addListener (pszTopic, pListener);
    return rc;
}

//-----------------------------------------------------------------------------

MessageArrivedCb::MessageArrivedCb (const char *pszTopic, const void *pMsg, int iLen)
    : topic (pszTopic), pMsgBuf (malloc (iLen)), iMsgBufLen (pMsgBuf == nullptr ? 0 : iLen)
{
    if (pMsgBuf != nullptr) {
        memcpy (pMsgBuf, pMsg, iMsgBufLen);
    }
}

MessageArrivedCb::~MessageArrivedCb (void)
{
    if (pMsgBuf != nullptr) free (pMsgBuf);
}

//-----------------------------------------------------------------------------

Notifier::Notifier (bool bAsynchronous)
    : _bAsynchronous (bAsynchronous)
{}

Notifier::~Notifier (void) {}

void Notifier::init (void)
{
    if (_bAsynchronous) {
        thread thNotifier (std::bind (&Notifier::popAndNotify, this));
        thNotifier.detach();
    }
}

bool Notifier::contains (const char *pszTopic) const
{
    const MultiMap::const_iterator iter = _listenerByTopic.find (string (pszTopic));
    return iter != _listenerByTopic.end();
}

void Notifier::addListener (const char *pszTopic, NatsWrapper::Listener *pListener)
{
    lock_guard<mutex> synchronized (_mListenerByTopic);
    _listenerByTopic.insert (MultiMapPair (string (pszTopic), pListener));
}

void Notifier::messageArrived (MessageArrivedCb *pCback)
{
    if (pCback == nullptr) return;
    if (_bAsynchronous) {
        _queue.push (pCback);
    }
    else {
        notify (pCback);
    }
}

void Notifier::notify (const MessageArrivedCb *pCback) const
{
    if (pCback == nullptr) return;
    lock_guard<mutex> synchronized (_mListenerByTopic);
    pair<MultiMap::const_iterator, MultiMap::const_iterator> range = _listenerByTopic.equal_range (pCback->topic);
    for (MultiMap::const_iterator it = range.first; it != range.second; ++it) {
        it->second->messageArrived (pCback->topic.c_str(), pCback->pMsgBuf, pCback->iMsgBufLen);
    }
}

void Notifier::popAndNotify (void)
{
    MessageArrivedCb *pMsgCb = nullptr;
    for (_queue.pop (pMsgCb); pMsgCb != nullptr; _queue.pop (pMsgCb)) {
        notify (pMsgCb);
        delete pMsgCb;
    }
}

