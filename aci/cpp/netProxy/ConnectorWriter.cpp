/*
 * ConnectorWriter.cpp
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
 */

#include "ConnectorWriter.h"
#include "CompressionSettings.h"
#include "LzmaConnectorWriter.h"
#include "ZLibConnectorWriter.h"


namespace ACMNetProxy
{
    ConnectorWriter * ConnectorWriter::connectorWriterFactory (const CompressionSettings & compressionSettings)
    {
        switch (compressionSettings.getCompressionType()) {
        case CompressionType::PMC_UncompressedData:
            return new ConnectorWriter{compressionSettings};
        case CompressionType::PMC_ZLibCompressedData:
            return new ZLibConnectorWriter{compressionSettings};
        #if !defined (ANDROID)
            case CompressionType::PMC_LZMACompressedData:
                return new LzmaConnectorWriter{compressionSettings};
        #endif
        }

        return nullptr;
    }

    ConnectorWriter * const ConnectorWriter::getAndLockUPDConnectorWriter (const CompressionSettings & compressionSettings)
    {
        auto *&pConnectorWriter = _UDPConnectorWriters[compressionSettings.getCompressionTypeAndLevel()];
        if (!pConnectorWriter) {
            pConnectorWriter = ConnectorWriter::connectorWriterFactory (compressionSettings);
        }
        pConnectorWriter->lockConnectorWriter();

        return pConnectorWriter;
    }

    int ConnectorWriter::flush (unsigned char ** pDest, unsigned int & uiDestLen) {
        *pDest = (unsigned char *) 0;
        uiDestLen = 0;
        _bFlushed = true;

        return 0;
    }

    int ConnectorWriter::writeData (const unsigned char * pSrc, unsigned int uiSrcLen, unsigned char ** pDest, unsigned int & uiDestLen, bool bLocalFlush)
    {
        (void) bLocalFlush;

        *pDest = const_cast<unsigned char *> (pSrc);
        uiDestLen = uiSrcLen;
        _bFlushed = false;

        return 0;
    }

    int ConnectorWriter::writeDataAndResetWriter (const unsigned char * pSrc, unsigned int uiSrcLen, unsigned char ** pDest, unsigned int & uiDestLen)
    {
        *pDest = const_cast<unsigned char *> (pSrc);
        uiDestLen = uiSrcLen;
        _bFlushed = true;

        return 0;
    }


    std::unordered_map<uint8, ConnectorWriter *> ConnectorWriter::_UDPConnectorWriters;
}
