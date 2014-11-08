/*
 * HistoryFactory.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#include "HistoryFactory.h"

#include "History.h"

using namespace IHMC_ACI;

History * HistoryFactory::getHistory (uint16 ui16HistoryLength, int64 i64RequestTimeout)
{
    return new ShiftHistory (ui16HistoryLength, i64RequestTimeout);
}

History * HistoryFactory::getHistory (uint32 ui32StartSeqNo, uint32 ui32EndSeqNo, int64 i64RequestTimeout)
{
    return new DiscreteHistory (ui32StartSeqNo, ui32EndSeqNo, i64RequestTimeout);
}

History * HistoryFactory::getHistory (int64 i64PublishTimeStart, int64 i64PublishTimeEnd, int64 i64RequestTimeout)
{
    return new TimeHistory (i64PublishTimeStart, i64PublishTimeEnd, i64RequestTimeout);
}

