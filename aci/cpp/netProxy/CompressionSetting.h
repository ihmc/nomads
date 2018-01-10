#ifndef INCL_COMPRESSION_SETTING_H
#define INCL_COMPRESSION_SETTING_H

/*
 * CompressionSetting.h
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
 * Manages the type and the level of a compression.
 */

#include "FTypes.h"

#include "ProxyMessages.h"


namespace ACMNetProxy
{
    class CompressionSetting
    {
    public:
        CompressionSetting (void);
        CompressionSetting (const uint8 ui8CompressionTypeAndLevel);
        CompressionSetting (const ProxyMessage::CompressionType fCompressionTypeFlag, const int8 _ui8CompressionLevel);
        CompressionSetting (const char * const pCompressionTypeName, const int8 _ui8CompressionLevel);
        CompressionSetting (const CompressionSetting & compressionSetting);
        const CompressionSetting * const clone (void) const;

        const CompressionSetting & operator = (const CompressionSetting &rCompressionSetting);
        bool operator == (const CompressionSetting &rhCompressionSetting) const;
        bool operator != (const CompressionSetting &rhCompressionSetting) const;

        ProxyMessage::CompressionType getCompressionType (void) const;
        const char * const getCompressionTypeAsString (void) const;
        uint8 getCompressionLevel (void) const;
        uint8 getCompressionTypeAndLevel (void) const;

        void setCompressionType (const ProxyMessage::CompressionType fCompressionType);
        void setCompressionLevel (const uint8 ui8CompressionLevel);

        static bool isSpecifiedCompressionNameCorrect (const char * const pSpecifiedCompressionName);
        static bool isSpecifiedCompressionLevelCorrect (const int iSpecifiedCompressionLevel);

        static const int NO_COMPRESSION_LEVEL = 0;
        static const int DEFAULT_COMPRESSION_LEVEL = 1;
        static const int MAX_COMPRESSION_LEVEL = 9;
        static const uint8 MAX_COMPRESSION_TYPE_AND_LEVEL = ((uint8) ProxyMessage::PMC_LZMACompressedData) | ((uint8) CompressionSetting::MAX_COMPRESSION_LEVEL);

        static const CompressionSetting DefaultNOCompressionSetting;


    private:
        ProxyMessage::CompressionType _fCompressionType;
        uint8 _ui8CompressionLevel;
    };


    inline CompressionSetting::CompressionSetting (void) :
        _fCompressionType (ProxyMessage::PMC_UncompressedData), _ui8CompressionLevel (NO_COMPRESSION_LEVEL) { }

    inline CompressionSetting::CompressionSetting (const uint8 ui8CompressionTypeAndLevel) :
        _fCompressionType (ProxyMessage::CompressionType (ui8CompressionTypeAndLevel & COMPRESSION_TYPE_FLAGS_MASK)),
        _ui8CompressionLevel (ui8CompressionTypeAndLevel & COMPRESSION_LEVEL_FLAGS_MASK) { }

    inline CompressionSetting::CompressionSetting (const ProxyMessage::CompressionType fCompressionType, const int8 ui8CompressionLevel) :
        _fCompressionType (fCompressionType), _ui8CompressionLevel (ui8CompressionLevel) { }

    inline CompressionSetting::CompressionSetting (const CompressionSetting & compressionSetting) :
        _fCompressionType (compressionSetting._fCompressionType), _ui8CompressionLevel (compressionSetting._ui8CompressionLevel) { }

    inline const CompressionSetting * const CompressionSetting::clone (void) const
    {
        return new CompressionSetting (_fCompressionType, _ui8CompressionLevel);
    }

    inline const CompressionSetting & CompressionSetting::operator = (const CompressionSetting &rCompressionSetting)
    {
        _fCompressionType = rCompressionSetting._fCompressionType;
        _ui8CompressionLevel = rCompressionSetting._ui8CompressionLevel;

        return *this;
    }

    inline bool CompressionSetting::operator == (const CompressionSetting &rhCompressionSetting) const
    {
        return (_fCompressionType == rhCompressionSetting._fCompressionType) && (_ui8CompressionLevel == rhCompressionSetting._ui8CompressionLevel);
    }

    inline bool CompressionSetting::operator != (const CompressionSetting &rhCompressionSetting) const
    {
        return (_fCompressionType != rhCompressionSetting._fCompressionType) || (_ui8CompressionLevel != rhCompressionSetting._ui8CompressionLevel);
    }

    inline ProxyMessage::CompressionType CompressionSetting::getCompressionType (void) const
    {
        return _fCompressionType;
    }

    inline uint8 CompressionSetting::getCompressionLevel (void) const
    {
        return _ui8CompressionLevel;
    }

    inline uint8 CompressionSetting::getCompressionTypeAndLevel (void) const
    {
        return (uint8) (_fCompressionType | _ui8CompressionLevel);
    }

    inline void CompressionSetting::setCompressionType (const ProxyMessage::CompressionType fCompressionType)
    {
        _fCompressionType = fCompressionType;
    }

    inline void CompressionSetting::setCompressionLevel (const uint8 ui8CompressionLevel)
    {
        _ui8CompressionLevel = ui8CompressionLevel;
    }

    inline bool CompressionSetting::isSpecifiedCompressionNameCorrect (const char * const pSpecifiedCompressionName)
    {
        NOMADSUtil::String sCompressionTypeName (pSpecifiedCompressionName);
        return (sCompressionTypeName == "none") || (sCompressionTypeName == "plain") ||
               (sCompressionTypeName == "zlib") || (sCompressionTypeName == "lzma");
    }

    inline bool CompressionSetting::isSpecifiedCompressionLevelCorrect (const int iSpecifiedCompressionLevel)
    {
        return (iSpecifiedCompressionLevel >= NO_COMPRESSION_LEVEL) && (iSpecifiedCompressionLevel <= MAX_COMPRESSION_LEVEL);
    }

}

#endif  // INCL_COMPRESSION_SETTING_H
