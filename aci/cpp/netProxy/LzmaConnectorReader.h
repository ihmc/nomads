#ifndef INCL_LZMA_CONNECTOR_READER_H
#define INCL_LZMA_CONNECTOR_READER_H

/*
 * LzmaConnectorReader.h
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
 * The LzmaConnectorReader class decompresses data compressed using the LZMA library.
 */

#ifndef LZMA_API_STATIC
#define LZMA_API_STATIC
#endif

#include "lzma.h"

#include "Mutex.h"
#include "ConnectorReader.h"

#define DEFAULT_IN_BUF_SIZE 1024U
#define DEFAULT_OUT_BUF_SIZE 4096U
#define DECODER_FLAGS LZMA_TELL_UNSUPPORTED_CHECK


namespace ACMNetProxy
{
    class LzmaConnectorReader : public ConnectorReader
    {
    public:
        virtual const ProxyMessage::CompressionType getCompressionFlag (void) const;
        using ConnectorReader::getCompressionLevel;
        using ConnectorReader::getCompressionName;

        virtual int receiveTCPDataProxyMessage (const uint8 *const ui8SrcData, uint16 ui16SrcLen, uint8 **pDest, uint32 &ui32DestLen);
        virtual int resetAndUnlockConnectorReader (void);


    private:
        friend class ConnectorReader;

        LzmaConnectorReader (const CompressionSetting * const pCompressionSetting, unsigned int ulInBufSize = DEFAULT_IN_BUF_SIZE,
                             unsigned int ulOutBufSize = DEFAULT_OUT_BUF_SIZE);
        virtual ~LzmaConnectorReader (void);

        virtual int lockConnectorReader (void);
        virtual int unlockConnectorReader (void);
        virtual int resetConnectorReader (void);
        void resetDecompStream (void);

        static void * alloc_mem (void *userdata, uint32_t items, uint32_t size);
        static void free_mem (void *userdata, void *data);

        unsigned char *_pInputBuffer;
        unsigned int _ulInBufSize;
        unsigned char *_pOutputBuffer;
        unsigned int _ulOutBufSize;
        lzma_stream _lzmaDecompStream;

        NOMADSUtil::Mutex _m;
    };


    inline const ProxyMessage::CompressionType LzmaConnectorReader::getCompressionFlag (void) const
    {
        return ProxyMessage::PMC_LZMACompressedData;
    }

    inline int LzmaConnectorReader::resetAndUnlockConnectorReader (void)
    {
        if (0 != resetConnectorReader()) {
            return -1;
        }

        return unlockConnectorReader();
    }

    inline int LzmaConnectorReader::lockConnectorReader (void)
    {
        return _m.lock();
    }

    inline int LzmaConnectorReader::unlockConnectorReader (void)
    {
        return _m.unlock();
    }

}

#endif // INCL_ZLIB_CONNECTOR_READER_H
