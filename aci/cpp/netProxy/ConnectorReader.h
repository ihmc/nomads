#ifndef INCL_CONNECTOR_READER_H
#define INCL_CONNECTOR_READER_H

/*
 * ConnectorReader.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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
#include "CompressionSettings.h"


namespace ACMNetProxy
{
    class ZLibConnectorReader;
    class LzmaConnectorReader;


    class ConnectorReader
    {
    public:
        virtual ~ConnectorReader (void);

        virtual const CompressionType getCompressionFlag (void) const;
        const int8 getCompressionLevel (void) const;
        virtual const char * getCompressionName (void) const;
        const CompressionSettings & getCompressionSettings (void) const;

        virtual int receiveTCPDataProxyMessage (const uint8 *const ui8SrcData, uint16 ui16SrcLen, uint8 **pDest, uint32 &ui32DestLen);
        virtual int resetAndUnlockConnectorReader (void);

        static ConnectorReader * const inizializeConnectorReader (const CompressionSettings & compressionSettings);
        static ConnectorReader * const getAndLockUDPConnectorReader (const CompressionSettings & compressionSettings);


    protected:
        ConnectorReader (const CompressionSettings & compressionSettings);

        virtual void lockConnectorReader (void) const;
        virtual void unlockConnectorReader (void) const;
        virtual int resetConnectorReader (void);


    private:
        const CompressionSettings _compressionSettings;

        static ConnectorReader * _pUDPConnectorReader;
        static ZLibConnectorReader * _pUDPZLibConnectorReader;
        static LzmaConnectorReader * _pUDPLzmaConnectorReader;
    };


    inline ConnectorReader::~ConnectorReader (void) { }

    inline const CompressionType ConnectorReader::getCompressionFlag (void) const
    {
        return CompressionType::PMC_UncompressedData;
    }

    inline const int8 ConnectorReader::getCompressionLevel (void) const
    {
        return _compressionSettings.getCompressionLevel();
    }

    inline const char * ConnectorReader::getCompressionName (void) const
    {
        return _compressionSettings.getCompressionTypeAsString();
    }

    inline const CompressionSettings & ConnectorReader::getCompressionSettings (void) const
    {
        return _compressionSettings;
    }

    inline int ConnectorReader::resetAndUnlockConnectorReader (void)
    {
        return 0;
    }

    inline ConnectorReader::ConnectorReader (const CompressionSettings & compressionSettings)
        : _compressionSettings{compressionSettings} {}

    inline void ConnectorReader::lockConnectorReader (void) const { }

    inline void ConnectorReader::unlockConnectorReader (void) const { }

    inline int ConnectorReader::resetConnectorReader (void)
    {
        return 0;
    }
}

#endif      // #ifndef INCL_CONNECTOR_WRITER_H
