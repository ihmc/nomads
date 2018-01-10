#ifndef INCL_LZMA_CONNECTOR_WRITER_H
#define INCL_LZMA_CONNECTOR_WRITER_H

/*
 * LzmaConnectorWriter.h
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
 * The LzmaConnectorWriter class compresses data using the LZMA library.
 */

#ifndef LZMA_API_STATIC
#define LZMA_API_STATIC
#endif

#include "lzma.h"

#include "Mutex.h"
#include "ConnectorWriter.h"

#define DEFAULT_OUT_BUF_SIZE 2048U
#define COMPRESSION_CHECK LZMA_CHECK_NONE


namespace ACMNetProxy
{
    class LzmaConnectorWriter : public ConnectorWriter
    {
    public:
        LzmaConnectorWriter (const CompressionSetting * const pCompressionSetting, unsigned long ulOutBufSize = DEFAULT_OUT_BUF_SIZE);
        virtual ~LzmaConnectorWriter (void);

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

        unsigned char *_pOutputBuffer;
        unsigned long _ulOutBufSize;
        lzma_stream _lzmaCompStream;

        NOMADSUtil::Mutex _m;
    };


    inline const ProxyMessage::CompressionType LzmaConnectorWriter::getCompressionFlag (void) const
    {
        return ProxyMessage::PMC_LZMACompressedData;
    }

    inline int LzmaConnectorWriter::lockConnectorWriter (void)
    {
        return _m.lock();
    }

    inline int LzmaConnectorWriter::unlockConnectorWriter (void)
    {
        return _m.unlock();
    }

}

#endif //INCL_LZMA_CONNECTOR_WRITER_H
