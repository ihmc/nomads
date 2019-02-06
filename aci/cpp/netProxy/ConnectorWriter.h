#ifndef INCL_CONNECTOR_WRITER_H
#define INCL_CONNECTOR_WRITER_H

/*
 * ConnectorWriter.h
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
 * Base class for all the ConnectorWriters.
 * It provides the interface of a ConnectorWriter and the default functions
 * of a ConnectorWriter that doesn't perform any compression.
 */

#include <unordered_map>

#include "Mutex.h"

#include "ProxyMessages.h"
#include "CompressionSettings.h"


namespace ACMNetProxy
{
    class ConnectorWriter
    {
    public:
        ConnectorWriter (const CompressionSettings & compressionSettings);
        virtual ~ConnectorWriter (void);

        virtual const CompressionType getCompressionFlag (void) const;
        const int8 getCompressionLevel (void) const;
        virtual const char * const getCompressionName (void) const;
        const CompressionSettings & getCompressionSetting (void) const;
        const bool isFlushed (void) const;

        virtual int lockConnectorWriter (void);
        virtual int unlockConnectorWriter (void);
        virtual int flush (unsigned char ** pDest, unsigned int & uiDestLen);
        virtual int writeData (const unsigned char * pSrc, unsigned int uiSrcLen, unsigned char ** pDest, unsigned int & uiDestLen, bool bLocalFlush = true);
        virtual int writeDataAndResetWriter (const unsigned char * pSrc, unsigned int uiSrcLen, unsigned char ** pDest, unsigned int & uiDestLen);

        static ConnectorWriter * connectorWriterFactory (const CompressionSettings & compressionSettings);
        static ConnectorWriter * const getAndLockUPDConnectorWriter (const CompressionSettings & compressionSettings);


    protected:
        bool _bFlushed;


    private:
        const CompressionSettings _compressionSettings;

        static std::unordered_map<uint8, ConnectorWriter *> _UDPConnectorWriters;
    };


    inline ConnectorWriter::ConnectorWriter (const CompressionSettings & compressionSettings) :
        _bFlushed{true}, _compressionSettings{compressionSettings}
    { }

    inline ConnectorWriter::~ConnectorWriter (void)
    { }

    inline const CompressionType ConnectorWriter::getCompressionFlag (void) const
    {
        return CompressionType::PMC_UncompressedData;
    }

    inline const int8 ConnectorWriter::getCompressionLevel (void) const
    {
        return _compressionSettings.getCompressionLevel();
    }

    inline const char * const ConnectorWriter::getCompressionName (void) const
    {
        return _compressionSettings.getCompressionTypeAsString();
    }

    inline const CompressionSettings & ConnectorWriter::getCompressionSetting (void) const
    {
        return _compressionSettings;
    }

    inline const bool ConnectorWriter::isFlushed (void) const
    {
        return _bFlushed;
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
