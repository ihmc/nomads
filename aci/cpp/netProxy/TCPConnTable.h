#ifndef INCL_TCP_CONN_TABLE_H
#define INCL_TCP_CONN_TABLE_H

/*
 * TCPConnTable.h
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
 * Data structure that maintains all the information about
 * the TCP connections that the NetProxy is remapping.
 */

#include <mutex>

#include "NPDArray2.h"

#include "Entry.h"
#include "Utilities.h"


namespace ACMNetProxy
{
    class TCPConnTable
    {
    public:
        TCPConnTable (void);
        explicit TCPConnTable (const TCPConnTable & rTCPCT) = delete;
        virtual ~TCPConnTable (void);

        // Methods to access entries
        Entry * const getEntry (uint16 ui16LocalID) const;
        Entry * const getEntry (uint16 ui16LocalID, uint16 ui16RemoteID) const;
        Entry * const getEntry (uint32 ui32LocalIP, uint16 ui16LocalPort, uint32 ui32RemoteIP,
                                uint16 ui16RemotePort, uint32 uint32AssignedPriority = 0);
        Entry * const getNextEntry (void);
        Entry * const getNextActiveLocalEntry (void);
        Entry * const getNextActiveRemoteEntry (void);
        Entry * const getNextClosedEntry (void);
        void resetGet (void);

        uint16 getActiveLocalConnectionsCount (void) const;
        unsigned int getEntriesNum (void) const;

        uint32 getHighestKnownPriority (void) const;
        uint32 getNewHighestPriority(void) const;
        void setHighestKnownPriority (uint32 ui32HighestKnownPriority);
        void setNewHighestPriority (uint32 ui32NewHighestPriority);

        void clearTable (void);
        void removeUnusedEntries (void);

        std::mutex & getMutexRef (void) const;


    private:
        uint32 _ui32FirstIndex;
        uint32 _ui32NextIndex;
        bool _bIsCounterReset;

        // Variables to manage the priority algorithm
        uint32 _ui32HighestKnownPriority;
        uint32 _ui32NewHighestPriority;

        NPDArray2<Entry> _entries;

        mutable std::mutex _mtx;
    };


    inline TCPConnTable::~TCPConnTable (void)
    {
        clearTable();
    }

    inline unsigned int TCPConnTable::getEntriesNum (void) const
    {
        return _entries.size();
    }

    inline uint32 TCPConnTable::getHighestKnownPriority (void) const
    {
        return _ui32HighestKnownPriority;
    }

    inline uint32 TCPConnTable::getNewHighestPriority (void) const
    {
        return _ui32NewHighestPriority;
    }

    inline void TCPConnTable::setHighestKnownPriority (uint32 ui32HighestKnownPriority)
    {
        _ui32HighestKnownPriority = ui32HighestKnownPriority;
    }

    inline void TCPConnTable::setNewHighestPriority (uint32 ui32NewHighestPriority)
    {
        _ui32NewHighestPriority = ui32NewHighestPriority;
    }

    inline void TCPConnTable::clearTable (void)
    {
        std::lock_guard<std::mutex> lg{_mtx};

        for (long i = 0; i <= _entries.getHighestIndex(); i++) {
            _entries.clear (i);
        }
    }

    inline TCPConnTable::TCPConnTable (void) :
        _ui32FirstIndex{0}, _ui32NextIndex{0}, _bIsCounterReset{true},
        _ui32HighestKnownPriority{0}, _ui32NewHighestPriority{0}
    { }

    inline std::mutex & TCPConnTable::getMutexRef (void) const
    {
        return _mtx;
    }
}

#endif   // #ifndef INCL_TCP_CONN_TABLE_H
