#ifndef INCL_PACKET_RECEIVER_H
#define INCL_PACKET_RECEIVER_H

/*
* PacketReceiver.h
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
* Thread that listens for new incoming packets on a newtork interface.
* Each received packet is dispatched to a handling function.
* The references to the network interface and handling function are
* passed in as parameters to the constructor.
*/

#include <string>
#include <atomic>
#include <chrono>
#include <mutex>
#include <memory>
#include <thread>
#include <functional>

#include "FTypes.h"

#include "Utilities.h"


namespace ACMNetProxy
{
    class NetworkInterface;


    class PacketReceiver
    {
    public:
        PacketReceiver (void);
        PacketReceiver (const std::string & sThreadName, std::shared_ptr<NetworkInterface> spNetworkInterface,
                        const std::function<int(uint8 * const, uint16, NetworkInterface * const)> & fPacketHandler);
        PacketReceiver (PacketReceiver && packetReceiver);

        PacketReceiver & operator = (PacketReceiver && packetReceiver);

        const std::string & getThreadName (void) const;
        bool isRunning (void) const;
        bool terminationRequested (void) const;

        int start (void);
        void requestTermination (void);
        int join (void) const;


    private:
        void run (void);


        std::atomic<bool> _bRunning;
        std::atomic<bool> _bTerminationRequested;
        std::unique_ptr<std::thread> _pThread;
        std::string _sThreadName;
        std::shared_ptr<NetworkInterface> _spNetworkInterface;
        std::function<int(uint8 * const, uint16, NetworkInterface * const)> _fPacketHandler;

        mutable bool _bJoined;
        mutable std::mutex _mtx;
    };


    inline PacketReceiver::PacketReceiver (void) :
        _bRunning{false}, _bTerminationRequested{false}, _pThread{nullptr}, _sThreadName{""},
        _spNetworkInterface{nullptr}, _fPacketHandler{}, _bJoined{false}
    { }

    inline PacketReceiver::PacketReceiver (const std::string & sThreadName, std::shared_ptr<NetworkInterface> spNetworkInterface,
                                           const std::function<int(uint8 * const, uint16, NetworkInterface * const)> & fPacketHandler) :
        _bRunning{false}, _bTerminationRequested{false}, _pThread{nullptr}, _sThreadName{sThreadName},
        _spNetworkInterface{spNetworkInterface}, _fPacketHandler{fPacketHandler}, _bJoined{false}
    {
        if (_spNetworkInterface == nullptr) {
            throw new std::invalid_argument {"pNetworkInteface is NULL"};
        }
    }

    inline PacketReceiver::PacketReceiver (PacketReceiver && packetReceiver) :
        _bRunning{false}, _bTerminationRequested{false}, _pThread{std::move (packetReceiver._pThread)}, _sThreadName{""},
        _spNetworkInterface{nullptr}, _fPacketHandler{nullptr}, _bJoined{packetReceiver._bJoined}
    {
        _bRunning.exchange (packetReceiver._bRunning);
        std::swap (_sThreadName, packetReceiver._sThreadName);
        std::swap (_spNetworkInterface, packetReceiver._spNetworkInterface);
        std::swap (_fPacketHandler, packetReceiver._fPacketHandler);
    }

    inline const std::string & PacketReceiver::getThreadName (void) const
    {
        return _sThreadName;
    }

    inline bool PacketReceiver::isRunning(void) const
    {
        return _bRunning;
    }

    inline bool PacketReceiver::terminationRequested (void) const
    {
        return _bTerminationRequested;
    }

    inline void PacketReceiver::requestTermination (void)
    {
        _bTerminationRequested = true;
    }
}

#endif  // INCL_PACKET_RECEIVER_H
