/* 
 * InstrumentedReader.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 * Created on August 3, 2012, 2:52 PM
 */

#ifndef INCL_INSTRUMENTED_READER_H
#define	INCL_INSTRUMENTED_READER_H

#include "Reader.h"

namespace NOMADSUtil
{
    class InstrumentedReader : public Reader
    {
        public:
            InstrumentedReader (Reader *pr = NULL, bool bDeleteWhenDone = false);
            virtual ~InstrumentedReader (void);

            // Instruments the reader pointed by pr.
            // If a reader had been previously set, and bDeleteWhenDone
            // was set on true, the old reader will be deleted.
            // It does NOT re-set the counter, use resetBytesReadCounter()
            // if needed.
            int init (Reader *pr, bool bDeleteWhenDone = false);

            void resetBytesReadCounter (void);
            uint32 getBytesRead (void);

            // Read upto iCount bytes; returns number of bytes read or
            //     a negative number in case of error
            int read (void *pBuf, int iCount);

        private:
            Reader *_pReader;
            bool _bDeleteWhenDone;
            uint32 _ui32BytesRead;
    };

    inline InstrumentedReader::InstrumentedReader (Reader *pr, bool bDeleteWhenDone)
        : _pReader (pr),
          _bDeleteWhenDone (bDeleteWhenDone),
          _ui32BytesRead (0U)
    {
    }

    inline InstrumentedReader::~InstrumentedReader (void)
    {
        if (_bDeleteWhenDone) {
            delete _pReader;
        }
        _pReader = NULL;
    }

    inline int InstrumentedReader::init (Reader *pr, bool bDeleteWhenDone)
    {
        if (pr == NULL) {
            return -1;
        }
        if (_bDeleteWhenDone && (_pReader != NULL)) {
            delete _pReader;
        }
        _pReader = pr;
        _bDeleteWhenDone = bDeleteWhenDone;
        return 0;
    }

    inline void InstrumentedReader::resetBytesReadCounter (void)
    {
        _ui32BytesRead = 0U;
    }

    inline uint32 InstrumentedReader::getBytesRead (void)
    {
        return _ui32BytesRead;
    }

    inline int InstrumentedReader::read (void *pBuf, int iCount)
    {
        if (iCount < 0) {
            return -1;
        }
        int rc = _pReader->read (pBuf, iCount);
        if (rc > 0) {
            _ui32BytesRead += iCount;
        }
        return rc;
    }
}

#endif  // #ifndef INCL_INSTRUMENTED_READER_H

