/*
 * CompressionSetting.cpp
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
 */

#include "StrClass.h"

#include "CompressionSetting.h"


namespace ACMNetProxy
{
    CompressionSetting::CompressionSetting (const char * const pCompressionTypeName, const int8 _ui8CompressionLevel)
    {
        this->_ui8CompressionLevel = _ui8CompressionLevel;

        NOMADSUtil::String sCompressionTypeName (pCompressionTypeName);
        sCompressionTypeName.convertToLowerCase();

        if (sCompressionTypeName == "zlib") {
            this->_fCompressionType = ProxyMessage::CompressionType (ProxyMessage::PMC_ZLibCompressedData & COMPRESSION_TYPE_FLAGS_MASK);
        }
    #if !defined (ANDROID)
        else if (sCompressionTypeName == "lzma") {
            this->_fCompressionType = ProxyMessage::CompressionType (ProxyMessage::PMC_LZMACompressedData & COMPRESSION_TYPE_FLAGS_MASK);
        }
    #endif
        else {
            this->_fCompressionType = ProxyMessage::CompressionType (ProxyMessage::PMC_UncompressedData & COMPRESSION_TYPE_FLAGS_MASK);
            this->_ui8CompressionLevel = NO_COMPRESSION_LEVEL;
        }
    }

    const char * const CompressionSetting::getCompressionTypeAsString (void) const
    {
        if (_fCompressionType == ProxyMessage::PMC_UncompressedData) {
            return "none";
        }
        if (_fCompressionType == ProxyMessage::PMC_ZLibCompressedData) {
            return "zlib";
        }
        if (_fCompressionType == ProxyMessage::PMC_LZMACompressedData) {
            return "lzma";
        }

        return (char *) 0;
    }
}
