/*
 * DataRelayer.h
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

#ifndef INCL_DATA_RELAYER_H
#define INCL_DATA_RELAYER_H

#include "ConditionVariable.h"
#include "Mutex.h"
#include "Reader.h"
#include "Thread.h"
#include "Writer.h"

namespace NOMADSUtil
{

    class DataRelayer : public Thread
    {
        public:
            DataRelayer (Reader *pReader, Writer *pWriter, bool bDeleteWhenDone = false);
            ~DataRelayer (void);

            void run (void);
            bool isDone (void);
            void blockUntilDone (void);

        private:
            Reader *_pReader;
            Writer *_pWriter;
            bool _bDeleteWhenDone;
            bool _bDone;
            Mutex _m;
            ConditionVariable _cv;
    };

    inline bool DataRelayer::isDone (void)
    {
        return _bDone;
    }

}

#endif   // #ifndef INCL_DATA_RELAYER_H
