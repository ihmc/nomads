/*
 * ConnectorWriter.cpp
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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
#include "LzmaConnectorWriter.h"
#include "ZLibConnectorWriter.h"


namespace ACMNetProxy
{
    ConnectorWriter *ConnectorWriter::connectorWriterFactory (const CompressionSetting * const pCompressionSetting)
    {
        if (!pCompressionSetting) {
            return NULL;
        }

        switch (pCompressionSetting->getCompressionType()) {
            case ProxyMessage::PMC_UncompressedData:
                return new ConnectorWriter (pCompressionSetting);
            case ProxyMessage::PMC_ZLibCompressedData:
                return new ZLibConnectorWriter (pCompressionSetting);
            #if !defined (ANDROID)
                case ProxyMessage::PMC_LZMACompressedData:
                    return new LzmaConnectorWriter (pCompressionSetting);
            #endif
        }

        return NULL;
    }

    ConnectorWriter * const ConnectorWriter::getAndLockUPDConnectorWriter (uint8 ui8CompressionTypeAndLevel)
    {
        ConnectorWriter *&pConnectorWriter = _UDPConnectorWriters[ui8CompressionTypeAndLevel];
        if (!pConnectorWriter) {
            pConnectorWriter = ConnectorWriter::connectorWriterFactory (ui8CompressionTypeAndLevel);
        }
        pConnectorWriter->lockConnectorWriter();

        return pConnectorWriter;
    }

    int ConnectorWriter::flush (unsigned char **pDest, unsigned int &uiDestLen) {
        *pDest = (unsigned char *) 0;
        uiDestLen = 0;
        _bFlushed = true;
        return 0;
    }

    int ConnectorWriter::writeData (const unsigned char *pSrc, unsigned int uiSrcLen, unsigned char **pDest, unsigned int &uiDestLen, bool bLocalFlush)
    {
        *pDest = const_cast<unsigned char *> (pSrc);
        uiDestLen = uiSrcLen;
        _bFlushed = false;
        return 0;
    }

    int ConnectorWriter::writeDataAndResetWriter (const unsigned char *pSrc, unsigned int uiSrcLen, unsigned char **pDest, unsigned int &uiDestLen)
    {
        *pDest = const_cast<unsigned char *> (pSrc);
        uiDestLen = uiSrcLen;
        _bFlushed = true;
        return 0;
    }

}
