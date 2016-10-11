#ifndef INCL_ZLIB_CONNECTOR_READER_H
#define INCL_ZLIB_CONNECTOR_READER_H

/*
 * ZLibConnectorReader.h
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
 * ConnectorReader decompresses data compressed using the ZLib library.
 */

#include "zlib.h"

#include "Mutex.h"
#include "ConnectorReader.h"

#define DEFAULT_IN_BUF_SIZE 1024U
#define DEFAULT_OUT_BUF_SIZE 4096U


namespace ACMNetProxy
{
    class ZLibConnectorReader : public ConnectorReader
    {
    public:
        virtual const ProxyMessage::CompressionType getCompressionFlag (void) const;
        using ConnectorReader::getCompressionLevel;
        using ConnectorReader::getCompressionName;

        virtual int receiveTCPDataProxyMessage (const uint8 *const ui8SrcData, uint16 ui16SrcLen, uint8 **pDest, uint32 &ui32DestLen);
        virtual int resetAndUnlockConnectorReader (void);


    private:
        friend class ConnectorReader;

        // Initializes the CompressedReader object
        // If bDeleteWhenDone is true, the reader that is passed into the constructor will be
        //     deleted when this object is deleted
        // If bNoWrapMode is true, the zlib library will not use a header or a checksum. This
        //     mode should be used for compatibility with .zip files
        // NOTE: If bNoWrapMode is set to true, then the data that is passed in must be padded with
        //       an extra byte in order to prevent the zlib library from complaining with a Z_BUF_ERROR
        ZLibConnectorReader (const CompressionSetting * const pCompressionSetting, unsigned int ulInBufSize = DEFAULT_IN_BUF_SIZE, unsigned int ulOutBufSize = DEFAULT_OUT_BUF_SIZE, bool bNoWrapMode = false);
        virtual ~ZLibConnectorReader (void);

        virtual int lockConnectorReader (void);
        virtual int unlockConnectorReader (void);
        virtual int resetConnectorReader (void);
        void resetDecompStream (void);

        unsigned char *_pInputBuffer;
        unsigned int _ulInBufSize;
        unsigned char *_pOutputBuffer;
        unsigned int _ulOutBufSize;
        const bool _bNoWrapMode;
        z_stream _zsDecompStream;

        NOMADSUtil::Mutex _m;
    };


    inline const ProxyMessage::CompressionType ZLibConnectorReader::getCompressionFlag (void) const
    {
        return ProxyMessage::PMC_ZLibCompressedData;
    }

    inline int ZLibConnectorReader::resetAndUnlockConnectorReader (void)
    {
        if (0 != resetConnectorReader()) {
            return -1;
        }

        return unlockConnectorReader();
    }

    inline int ZLibConnectorReader::lockConnectorReader (void)
    {
        return _m.lock();
    }

    inline int ZLibConnectorReader::unlockConnectorReader (void)
    {
        return _m.unlock();
    }

}

#endif // INCL_ZLIB_CONNECTOR_READER_H
