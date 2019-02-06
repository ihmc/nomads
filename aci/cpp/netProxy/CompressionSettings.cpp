/*
 * CompressionSettings.cpp
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

#include <string>

#include "CompressionSettings.h"
#include "Utilities.h"

namespace ACMNetProxy
{
    CompressionSettings::CompressionSettings (const char * const pCompressionTypeName, const int8 ui8CompressionLevel)
    {
        _ui8CompressionLevel = ui8CompressionLevel;
        ci_string sCompressionTypeName{nullprtToEmptyString (pCompressionTypeName)};

        if (sCompressionTypeName == "zlib") {
            _fCompressionType = CompressionType::PMC_ZLibCompressedData;
        }
    #if !defined (ANDROID)
        else if (sCompressionTypeName == "lzma") {
            _fCompressionType = CompressionType::PMC_LZMACompressedData;
        }
    #endif
        else {
            _fCompressionType = CompressionType::PMC_UncompressedData;
            _ui8CompressionLevel = NO_COMPRESSION_LEVEL;
        }
    }

    const char * const CompressionSettings::getCompressionTypeAsString (void) const
    {
        if (_fCompressionType == CompressionType::PMC_UncompressedData) {
            return "none";
        }
        if (_fCompressionType == CompressionType::PMC_ZLibCompressedData) {
            return "zlib";
        }
        if (_fCompressionType == CompressionType::PMC_LZMACompressedData) {
            return "lzma";
        }

        return nullptr;
    }


    const CompressionSettings CompressionSettings::DefaultNOCompressionSetting;
}
