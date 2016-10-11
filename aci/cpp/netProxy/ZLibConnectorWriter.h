#ifndef INCL_ZLIB_CONNECTOR_WRITER_H
#define INCL_ZLIB_CONNECTOR_WRITER_H

/*
 * ZlibConnectorWriter.h
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

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * ConnectorWriter compresses data using the ZLib library
 */

#include "zlib.h"

#include "Mutex.h"
#include "ConnectorWriter.h"

namespace ACMNetProxy
{
    class ZLibConnectorWriter : public ConnectorWriter
    {
    public:
        ZLibConnectorWriter (const CompressionSetting * const pCompressionSetting , unsigned long ulOutBufSize = 2048);
        virtual ~ZLibConnectorWriter (void);

        virtual const ProxyMessage::CompressionType getCompressionFlag (void) const;
        using ConnectorWriter::getCompressionLevel;
        using ConnectorWriter::getCompressionName;
        using ConnectorWriter::getCompressionSetting;
        using ConnectorWriter::isFlushed;

        virtual int lockConnectorWriter (void);
        virtual int unlockConnectorWriter (void);
        virtual int flush (unsigned char **pDest, unsigned int &uiDestLen);
        virtual int writeData (const unsigned char *pSrc, unsigned int uiSrcLen, unsigned char **pDest, unsigned int &uiDestLen, bool bFlushLocal = true);
        virtual int writeDataAndResetWriter (const unsigned char *pSrc, unsigned int uiSrcLen, unsigned char **pDest, unsigned int &uiDestLen);


    private:
        void resetCompStream (void);
        static void *alloc_mem (void *userdata, uInt items, uInt size);
        static void free_mem (void *userdata, void* data);

        unsigned char *_pOutputBuffer;
        unsigned long _ulOutBufSize;
        z_stream _zsCompStream;

        NOMADSUtil::Mutex _m;
    };


    inline const ProxyMessage::CompressionType ZLibConnectorWriter::getCompressionFlag (void) const
    {
        return ProxyMessage::PMC_ZLibCompressedData;
    }

    inline int ZLibConnectorWriter::lockConnectorWriter (void)
    {
        return _m.lock();
    }

    inline int ZLibConnectorWriter::unlockConnectorWriter (void)
    {
        return _m.unlock();
    }

}

#endif // INCL_ZLIB_CONNECTOR_WRITER_H
