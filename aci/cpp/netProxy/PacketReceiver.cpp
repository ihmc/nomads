/*
* PacketReceiver.cpp
*
* This file is part of the IHMC NetProxy Library/Component
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
*/

#include "Logger.h"

#include "PacketReceiver.h"
#include "NetworkInterface.h"
#include "PacketRouter.h"

#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    PacketReceiver & PacketReceiver::operator = (PacketReceiver && packetReceiver)
    {
        _bJoined = packetReceiver._bJoined;
        _bRunning.exchange (packetReceiver._bRunning);
        std::swap (_pThread, packetReceiver._pThread);
        std::swap (_sThreadName, packetReceiver._sThreadName);
        std::swap (_spNetworkInterface, packetReceiver._spNetworkInterface);
        std::swap (_fPacketHandler, packetReceiver._fPacketHandler);

        return *this;
    }

    int PacketReceiver::start (void)
    {
        std::lock_guard<std::mutex> lg (_mtx);
        if (_spNetworkInterface == nullptr) {
            return -1;
        }

        if (_pThread == nullptr) {
            _pThread = make_unique<std::thread> (&PacketReceiver::run, this);
            return 0;
        }

        return -2;
    }

    int PacketReceiver::join (void) const
    {
        std::lock_guard<std::mutex> lg (_mtx);
        if (_pThread == nullptr) {
            return -1;
        }
        if (_bJoined) {
            // Already joined
            return -2;
        }

        if (_pThread) {
            _pThread->join();
        }
        _bJoined = true;

        return 0;
    }

    void PacketReceiver::run (void)
    {
        int rc = 0;
        uint16 ui16PacketLen = 0;
        const uint8 * pui8Buf = nullptr;

        _bRunning = true;
        while (!terminationRequested()) {
            if ((rc = _spNetworkInterface->readPacket (&pui8Buf, ui16PacketLen)) != 0) {
                checkAndLogMsg ("PacketReceiver::run", NOMADSUtil::Logger::L_MildError,
                                "readPacket() on thread %s failed with rc = %d\n", _sThreadName.c_str(), rc);
            }
            else if (pui8Buf && (ui16PacketLen > 0)) {
                if ((rc = _fPacketHandler (const_cast<uint8 *> (pui8Buf), ui16PacketLen, _spNetworkInterface.get())) < 0) {
                    checkAndLogMsg ("PacketReceiver::run", NOMADSUtil::Logger::L_Warning,
                                    "packet handler function failed when processing a "
                                    "packet of %hu bytes; rc = %d\n", ui16PacketLen, rc);
                }
            }
        }
        _bRunning = false;
    }
}