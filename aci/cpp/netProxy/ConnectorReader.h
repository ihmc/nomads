#ifndef INCL_CONNECTOR_READER_H
#define INCL_CONNECTOR_READER_H

/*
 * ConnectorReader.h
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
 * Base class of all the ConnectorReaders.
 * It provides the interface of a ConnectorReader and the default functions
 * of a ConnectorReader that doesn't perform any decompression.
 */

#include "Mutex.h"

#include "ProxyMessages.h"
#include "CompressionSetting.h"


namespace ACMNetProxy
{
    class ZLibConnectorReader;
    class LzmaConnectorReader;

    class ConnectorReader
    {
    public:
        virtual ~ConnectorReader (void);

        virtual const ProxyMessage::CompressionType getCompressionFlag (void) const;
        const int8 getCompressionLevel (void) const;
        virtual const char * getCompressionName (void) const;
        const CompressionSetting * const getCompressionSetting (void) const;

        static ConnectorReader * const inizializeConnectorReader (const CompressionSetting * const pCompressionSetting);
        static ConnectorReader * const getAndLockUDPConnectorReader (const uint8 ui8CompressionTypeAndLevel);
        static ConnectorReader * const getAndLockUDPConnectorReader (const CompressionSetting * const pCompressionSetting);

        virtual int receiveTCPDataProxyMessage (const uint8 *const ui8SrcData, uint16 ui16SrcLen, uint8 **pDest, uint32 &ui32DestLen);
        virtual int resetAndUnlockConnectorReader (void);


    protected:
        ConnectorReader (const CompressionSetting * const pCompressionSetting);


    private:
        virtual int lockConnectorReader (void);
        virtual int unlockConnectorReader (void);
        virtual int resetConnectorReader (void);

        const CompressionSetting * const _pCompressionSetting;

        static ConnectorReader *_pUDPConnectorReader;
        static ZLibConnectorReader *_pUDPZLibConnectorReader;
        static LzmaConnectorReader *_pUDPLzmaConnectorReader;
    };


    inline ConnectorReader::~ConnectorReader (void)
    {
        delete _pCompressionSetting;
    }

    inline const ProxyMessage::CompressionType ConnectorReader::getCompressionFlag (void) const
    {
        return ProxyMessage::PMC_UncompressedData;
    }

    inline const int8 ConnectorReader::getCompressionLevel (void) const
    {
        return _pCompressionSetting->getCompressionLevel();
    }

    inline const char * ConnectorReader::getCompressionName (void) const
    {
        return _pCompressionSetting->getCompressionTypeAsString();
    }

    inline const CompressionSetting * const ConnectorReader::getCompressionSetting (void) const
    {
        return _pCompressionSetting;
    }

    inline ConnectorReader * const ConnectorReader::getAndLockUDPConnectorReader (uint8 ui8CompressionTypeAndLevel)
    {
        const CompressionSetting compressionSetting (ui8CompressionTypeAndLevel);

        return getAndLockUDPConnectorReader (&compressionSetting);
    }

    inline int ConnectorReader::resetAndUnlockConnectorReader (void)
    {
        return NOMADSUtil::Mutex::RC_Ok;
    }

    inline ConnectorReader::ConnectorReader (const CompressionSetting * const pCompressionSetting)
        : _pCompressionSetting (pCompressionSetting->clone()) {}

    inline int ConnectorReader::lockConnectorReader (void)
    {
        return NOMADSUtil::Mutex::RC_Ok;
    }

    inline int ConnectorReader::unlockConnectorReader (void)
    {
        return NOMADSUtil::Mutex::RC_Ok;
    }

    inline int ConnectorReader::resetConnectorReader (void)
    {
        return 0;
    }

}

#endif      // #ifndef INCL_CONNECTOR_WRITER_H
