/*
 * ConnectorReader.cpp
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

#include "ConnectorReader.h"
#include "CompressionSettings.h"
#include "ZLibConnectorReader.h"
#include "LzmaConnectorReader.h"


namespace ACMNetProxy
{
    // ConnectorReader Factory method
    ConnectorReader * const ConnectorReader::inizializeConnectorReader (const CompressionSettings & compressionSettings)
    {
        switch (compressionSettings.getCompressionType()) {
        case CompressionType::PMC_UncompressedData:
            return new ConnectorReader{compressionSettings};
        case CompressionType::PMC_ZLibCompressedData:
            return new ZLibConnectorReader{compressionSettings};
        #if !defined (ANDROID)
            case CompressionType::PMC_LZMACompressedData:
                return new LzmaConnectorReader{compressionSettings};
        #endif
        }

        return nullptr;
    }

    ConnectorReader * const ConnectorReader::getAndLockUDPConnectorReader (const CompressionSettings & compressionSettings)
    {
        switch (compressionSettings.getCompressionType()) {
        case CompressionType::PMC_UncompressedData:
            {
                if (!_pUDPConnectorReader) {
                    _pUDPConnectorReader = new ConnectorReader{compressionSettings};
                }
                _pUDPConnectorReader->lockConnectorReader();
                return _pUDPConnectorReader;
            }
        case CompressionType::PMC_ZLibCompressedData:
            {
                if (!_pUDPZLibConnectorReader) {
                    _pUDPZLibConnectorReader = new ZLibConnectorReader{compressionSettings};
                }
                _pUDPZLibConnectorReader->lockConnectorReader();
                return _pUDPZLibConnectorReader;
            }
        #if !defined (ANDROID)
        case CompressionType::PMC_LZMACompressedData:
            {
                if (!_pUDPLzmaConnectorReader) {
                    _pUDPLzmaConnectorReader = new LzmaConnectorReader{compressionSettings};
                }
                _pUDPLzmaConnectorReader->lockConnectorReader();
                return _pUDPLzmaConnectorReader;
            }
        #endif
        }

        return nullptr;
    }

    int ConnectorReader::receiveTCPDataProxyMessage (const uint8 *const ui8SrcData, uint16 ui16SrcLen, uint8 **pDest, uint32 &ui32DestLen)
    {
        *pDest = const_cast<uint8*> (ui8SrcData);
        ui32DestLen = ui16SrcLen;
        return 0;
    }


    ConnectorReader * ConnectorReader::_pUDPConnectorReader = nullptr;
    ZLibConnectorReader * ConnectorReader::_pUDPZLibConnectorReader = nullptr;
    LzmaConnectorReader * ConnectorReader::_pUDPLzmaConnectorReader = nullptr;
}
