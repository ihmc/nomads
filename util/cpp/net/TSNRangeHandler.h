/*
 * TSNRangeHandler.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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
 */

#ifndef INCL_UTIL_TSN_RANGE_HANDLER_H
#define	INCL_UTIL_TSN_RANGE_HANDLER_H

#include "RangeDLList.h"

namespace NOMADSUtil
{
    class BufferWriter;
    class Reader;

    class SAckTSNRangeHandler : public UInt16RangeDLList
    {
        public:
            SAckTSNRangeHandler (void);

            int setCumulativeTSN (uint16 ui16CumulativeTSN);
            uint16 getCumulativeTSN (void);
            bool hasTSN (uint16 ui16TSN);

            /**
             * Returns 0 in case of a duplicate TSN or 1 if the TSN was
             * successfully added
             */
            int addTSN (uint16 ui16TSN);

            virtual int read (NOMADSUtil::Reader *pReader, uint16 ui16MaxSize);
            virtual int write (NOMADSUtil::BufferWriter *pWriter, uint16 ui16MaxSize);

            void reset (void);

        protected:
            uint16 _ui16CumulativeTSN;
    };

    //--------------------------------------------------------------------------
    // SAckTSNRangeHandler
    //--------------------------------------------------------------------------
    inline SAckTSNRangeHandler::SAckTSNRangeHandler()
        : UInt16RangeDLList (true)
    {
        _ui16CumulativeTSN = 65535;
    }

    inline int SAckTSNRangeHandler::setCumulativeTSN (uint16 ui16CumulativeTSN)
    {
        _ui16CumulativeTSN = ui16CumulativeTSN;
        return 0;
    }

    inline uint16 SAckTSNRangeHandler::getCumulativeTSN (void)
    {
        return _ui16CumulativeTSN;
    }

    inline  bool SAckTSNRangeHandler::hasTSN (uint16 ui16TSN)
    {
        if (SequentialArithmetic::lessThanOrEqual (ui16TSN, _ui16CumulativeTSN)) {
            return true;
        }
        return UInt16RangeDLList::hasTSN (ui16TSN);
    }

    inline void SAckTSNRangeHandler::reset (void)
    {
        _ui16CumulativeTSN = 65535;
        UInt16RangeDLList::reset();
    }
}

#endif  // INCL_UTIL_TSN_RANGE_HANDLER_H
