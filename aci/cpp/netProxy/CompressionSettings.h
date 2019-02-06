#ifndef INCL_COMPRESSION_SETTING_H
#define INCL_COMPRESSION_SETTING_H

/*
 * CompressionSettings.h
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
 * Manages the type and the level of a compression.
 */

#include "FTypes.h"

#include "ProxyMessages.h"
#include "Utilities.h"

namespace ACMNetProxy
{
    class CompressionSettings
    {
    public:
        CompressionSettings (void);
        CompressionSettings (const uint8 ui8CompressionTypeAndLevel);
        CompressionSettings (const CompressionType fCompressionTypeFlag, const uint8 ui8CompressionLevel);
        CompressionSettings (const char * const pCompressionTypeName, const int8 ui8CompressionLevel);
        CompressionSettings (const CompressionSettings & compressionSetting);

        const CompressionSettings & operator = (const CompressionSettings &rCompressionSetting);
        bool operator == (const CompressionSettings &rhCompressionSetting) const;
        bool operator != (const CompressionSettings &rhCompressionSetting) const;

        CompressionType getCompressionType (void) const;
        const char * const getCompressionTypeAsString (void) const;
        uint8 getCompressionLevel (void) const;
        uint8 getCompressionTypeAndLevel (void) const;

        void setCompressionType (const CompressionType fCompressionType);
        void setCompressionLevel (const uint8 ui8CompressionLevel);

        static bool isSpecifiedCompressionNameCorrect (const char * const pSpecifiedCompressionName);
        static bool isSpecifiedCompressionLevelCorrect (const int iSpecifiedCompressionLevel);

        static const int NO_COMPRESSION_LEVEL = 0;
        static const int DEFAULT_COMPRESSION_LEVEL = 1;
        static const int MAX_COMPRESSION_LEVEL = 9;
        static const uint8 MAX_COMPRESSION_TYPE_AND_LEVEL =
            static_cast<uint8> (static_cast<int> (CompressionType::PMC_LZMACompressedData) |
                                CompressionSettings::MAX_COMPRESSION_LEVEL);

        static const CompressionSettings DefaultNOCompressionSetting;


    private:
        CompressionType _fCompressionType;
        uint8 _ui8CompressionLevel;
    };


    inline CompressionSettings::CompressionSettings (void) :
        _fCompressionType{CompressionType::PMC_UncompressedData}, _ui8CompressionLevel{NO_COMPRESSION_LEVEL}
    { }

    inline CompressionSettings::CompressionSettings (const uint8 ui8CompressionTypeAndLevel) :
        _fCompressionType{CompressionType (ui8CompressionTypeAndLevel & ProxyMessage::COMPRESSION_TYPE_FLAGS_MASK)},
        _ui8CompressionLevel{static_cast<uint8> (ui8CompressionTypeAndLevel & ProxyMessage::COMPRESSION_LEVEL_FLAGS_MASK)}
    { }

    inline CompressionSettings::CompressionSettings (const CompressionType fCompressionType,
                                                     const uint8 ui8CompressionLevel) :
        _fCompressionType{fCompressionType}, _ui8CompressionLevel{ui8CompressionLevel}
    { }

    inline CompressionSettings::CompressionSettings (const CompressionSettings & compressionSetting) :
        _fCompressionType{compressionSetting._fCompressionType}, _ui8CompressionLevel{compressionSetting._ui8CompressionLevel}
    { }

    inline const CompressionSettings & CompressionSettings::operator = (const CompressionSettings &rCompressionSetting)
    {
        _fCompressionType = rCompressionSetting._fCompressionType;
        _ui8CompressionLevel = rCompressionSetting._ui8CompressionLevel;

        return *this;
    }

    inline bool CompressionSettings::operator == (const CompressionSettings &rhCompressionSetting) const
    {
        return (_fCompressionType == rhCompressionSetting._fCompressionType) &&
            (_ui8CompressionLevel == rhCompressionSetting._ui8CompressionLevel);
    }

    inline bool CompressionSettings::operator != (const CompressionSettings &rhCompressionSetting) const
    {
        return (_fCompressionType != rhCompressionSetting._fCompressionType) ||
            (_ui8CompressionLevel != rhCompressionSetting._ui8CompressionLevel);
    }

    inline CompressionType CompressionSettings::getCompressionType (void) const
    {
        return _fCompressionType;
    }

    inline uint8 CompressionSettings::getCompressionLevel (void) const
    {
        return _ui8CompressionLevel;
    }

    inline uint8 CompressionSettings::getCompressionTypeAndLevel (void) const
    {
        return static_cast<uint8> (_fCompressionType) | _ui8CompressionLevel;
    }

    inline void CompressionSettings::setCompressionType (const CompressionType fCompressionType)
    {
        _fCompressionType = fCompressionType;
    }

    inline void CompressionSettings::setCompressionLevel (const uint8 ui8CompressionLevel)
    {
        _ui8CompressionLevel = ui8CompressionLevel;
    }

    inline bool CompressionSettings::isSpecifiedCompressionNameCorrect (const char * const pSpecifiedCompressionName)
    {
        ci_string sCompressionTypeName (pSpecifiedCompressionName);
        return (sCompressionTypeName == "none") || (sCompressionTypeName == "plain") ||
               (sCompressionTypeName == "zlib") || (sCompressionTypeName == "lzma");
    }

    inline bool CompressionSettings::isSpecifiedCompressionLevelCorrect (const int iSpecifiedCompressionLevel)
    {
        return (iSpecifiedCompressionLevel >= NO_COMPRESSION_LEVEL) &&
            (iSpecifiedCompressionLevel <= MAX_COMPRESSION_LEVEL);
    }

}

#endif  // INCL_COMPRESSION_SETTING_H
