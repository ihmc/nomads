#ifndef INCL_CONNECTOR_WRITER_H
#define INCL_CONNECTOR_WRITER_H

/*
 * ConnectorWriter.h
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
 * Base class for all the ConnectorWriters.
 * It provides the interface of a ConnectorWriter and the default functions
 * of a ConnectorWriter that doesn't perform any compression.
 */

#include "Mutex.h"
#include "DArray.h"

#include "ProxyMessages.h"
#include "CompressionSetting.h"


namespace ACMNetProxy
{
    class ConnectorWriter
    {
    public:
        ConnectorWriter (const CompressionSetting * const pCompressionSetting);
        virtual ~ConnectorWriter (void);

        virtual const ProxyMessage::CompressionType getCompressionFlag (void) const;
        const int8 getCompressionLevel (void) const;
        virtual const char * const getCompressionName (void) const;
        CompressionSetting const * const getCompressionSetting (void) const;
        const bool isFlushed (void) const;

        static ConnectorWriter *connectorWriterFactory (uint8 ui8CompressionTypeAndLevel);
        static ConnectorWriter *connectorWriterFactory (const CompressionSetting * const pCompressionSetting);
        static ConnectorWriter * const getAndLockUPDConnectorWriter (uint8 ui8CompressionTypeAndLevel);
        static ConnectorWriter * const getAndLockUPDConnectorWriter (const CompressionSetting * const pCompressionSetting);

        virtual int lockConnectorWriter (void);
        virtual int unlockConnectorWriter (void);
        virtual int flush (unsigned char **pDest, unsigned int &uiDestLen);
        virtual int writeData (const unsigned char *pSrc, unsigned int uiSrcLen, unsigned char **pDest, unsigned int &uiDestLen, bool bLocalFlush = true);
        virtual int writeDataAndResetWriter (const unsigned char *pSrc, unsigned int uiSrcLen, unsigned char **pDest, unsigned int &uiDestLen);


    protected:
        bool _bFlushed;


    private:
        const CompressionSetting * const _pCompressionSetting;

        static NOMADSUtil::DArray<ConnectorWriter *> _UDPConnectorWriters;
    };


    inline ConnectorWriter::ConnectorWriter (const CompressionSetting * const pCompressionSetting)
        : _pCompressionSetting (pCompressionSetting ? pCompressionSetting->clone() : (CompressionSetting *) 0)
    {
        _bFlushed = true;
    }

    inline ConnectorWriter::~ConnectorWriter (void)
    {
        if (_pCompressionSetting) {
            delete _pCompressionSetting;
        }
    }

    inline const ProxyMessage::CompressionType ConnectorWriter::getCompressionFlag (void) const
    {
        return ProxyMessage::PMC_UncompressedData;
    }

    inline const int8 ConnectorWriter::getCompressionLevel (void) const
    {
        return _pCompressionSetting->getCompressionLevel();
    }

    inline const char * const ConnectorWriter::getCompressionName (void) const
    {
        return _pCompressionSetting->getCompressionTypeAsString();
    }

    inline const CompressionSetting * const ConnectorWriter::getCompressionSetting (void) const
    {
        return _pCompressionSetting;
    }

    inline const bool ConnectorWriter::isFlushed (void) const
    {
        return _bFlushed;
    }

    inline ConnectorWriter *ConnectorWriter::connectorWriterFactory (uint8 ui8CompressionTypeAndLevel)
    {
        const CompressionSetting compressionSetting (ui8CompressionTypeAndLevel);

        return ConnectorWriter::connectorWriterFactory (&compressionSetting);
    }

    inline ConnectorWriter * const ConnectorWriter::getAndLockUPDConnectorWriter (const CompressionSetting * const pCompressionSetting)
    {
        if (!pCompressionSetting) {
            return nullptr;
        }

        return getAndLockUPDConnectorWriter (pCompressionSetting->getCompressionTypeAndLevel());
    }

    inline int ConnectorWriter::lockConnectorWriter (void)
    {
        return NOMADSUtil::Mutex::RC_Ok;
    }

    inline int ConnectorWriter::unlockConnectorWriter (void)
    {
        return NOMADSUtil::Mutex::RC_Ok;
    }

}

#endif      // #ifndef INCL_CONNECTOR_WRITER_H
