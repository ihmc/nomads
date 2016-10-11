#ifndef INCL_TCP_CONN_TABLE_H
#define INCL_TCP_CONN_TABLE_H

/*
 * TCPConnTable.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
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

#include "NPDArray2.h"
#include "Mutex.h"

#include "Utilities.h"
#include "Entry.h"


namespace ACMNetProxy
{
    class TCPConnTable
    {
    public:
        virtual ~TCPConnTable (void);

        static TCPConnTable * const getTCPConnTable (void);

			
        // Methods to iterate over the entries
        Entry * const getEntry (uint16 ui16LocalID) const;
        Entry * const getEntry (uint16 ui16LocalID, uint16 ui16RemoteID) const;
		Entry * const getEntry(uint32 ui32LocalIP, uint16 ui16LocalPort, uint32 ui32RemoteIP, uint16 ui16RemotePort, uint32 uint32AssignedPriority = 0);
        Entry * const getNextEntry (void);
        Entry * const getNextActiveLocalEntry (void);
        Entry * const getNextActiveRemoteEntry (void);
        Entry * const getNextClosedLocalEntry (void);
        void resetGet (void);

        uint16 getActiveLocalConnectionsCount (void) const;
        uint16 getActiveLocalConnectionsCount (uint32 ui32RemoteIP, ConnectorType connectorType) const;
        unsigned int getEntriesNum (void) const;

        void clearTable (void);
        void cleanMemory (void);

        void lock (void);
        void unlock (void);

        static const uint32 STANDARD_MSL;                       // Standard Maximum Segment Lifetime of 2 minutes (RFC 793)
        static const uint16 LB_RTO;                             // Retransmission TimeOut Lower Bound of 100 milliseconds (in RFC 793 is 1 second)
        static const uint16 UB_RTO;                             // Retransmission TimeOut Upper Bound of 60 seconds (RFC 793)
        static const uint16 RTO_RECALCULATION_TIME;             // Time that has to pass before recalculating RTO (RFC 793)
        static const double ALPHA_RTO;                          // Alpha constant for RTO calculation (RFC 793)
        static const double BETA_RTO;                           // Beta constant for RTO calculation (RFC 793)

		uint32 highestKnownPriority;
		uint32 newHighestPriority;
    private:
        TCPConnTable (void);
        explicit TCPConnTable (const TCPConnTable &rTCPConnTable);

        uint32 _ui32FirstIndex;
        uint32 _ui32NextIndex;
        bool _bIsCounterReset;

        NPDArray2<Entry> _entries;

        mutable NOMADSUtil::Mutex _m;
    };


    inline TCPConnTable::~TCPConnTable (void)
    {
        clearTable();
    }

    inline TCPConnTable * const TCPConnTable::getTCPConnTable (void)
    {
        static TCPConnTable tcpConnTable;

        return &tcpConnTable;
    }

    inline unsigned int TCPConnTable::getEntriesNum (void) const
    {
        return _entries.size();
    }
	
    inline void TCPConnTable::clearTable (void)
    {
        _m.lock();

        for (long i = 0; i <= _entries.getHighestIndex(); i++) {
            _entries.clear (i);
        }

        _m.unlock();
    }

    inline void TCPConnTable::lock (void)
    {
        _m.lock();
    }

    inline void TCPConnTable::unlock (void)
    {
        _m.unlock();
    }

    inline TCPConnTable::TCPConnTable (void) :
		_ui32FirstIndex(0), _ui32NextIndex(0), _bIsCounterReset(true), highestKnownPriority(0), newHighestPriority(0) {}
}

#endif   // #ifndef INCL_TCP_CONN_TABLE_H
