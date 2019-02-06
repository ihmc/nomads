/*
 * InstrumentedWriter.h
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
 */

#ifndef INCL_INSTRUMENTED_WRITER_H
#define INCL_INSTRUMENTED_WRITER_H

#include "Writer.h"

namespace NOMADSUtil
{
    class InstrumentedWriter : public Writer
    {
        public:
            InstrumentedWriter (Writer *pw, bool bDeleteWhenDone = false);
            ~InstrumentedWriter (void);

            void resetBytesWrittenCounter (void);
            uint32 getBytesWritten (void) const;

            int writeBytes (const void *pBuf, unsigned long ulCount);

        private:
            Writer *_pWriter;
            bool _bDeleteWhenDone;
            uint32 _ui32BytesWritten;
    };

    inline InstrumentedWriter::InstrumentedWriter (Writer *pw, bool bDeleteWhenDone)
    {
        _pWriter = pw;
        _bDeleteWhenDone = bDeleteWhenDone;
        _ui32BytesWritten = 0;
    }

    inline InstrumentedWriter::~InstrumentedWriter(void)
    {
        if (_bDeleteWhenDone) {
            delete _pWriter;
        }
        _pWriter = NULL;
    }

    inline void InstrumentedWriter::resetBytesWrittenCounter (void)
    {
        _ui32BytesWritten = 0;
    }

    inline uint32 InstrumentedWriter::getBytesWritten (void) const
    {
        return _ui32BytesWritten;
    }

    inline int InstrumentedWriter::writeBytes (const void *pBuf, unsigned long ulCount)
    {
        int rc = _pWriter->writeBytes (pBuf, ulCount);
        if (rc == 0) {
            _ui32BytesWritten += ulCount;
        }
        return rc;
    }
}

#endif   // #ifndef INCL_INSTRUMENTED_WRITER_H
