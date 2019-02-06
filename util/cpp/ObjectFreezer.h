/*
 * ObjectFreezer.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_OBJECT_FREEZER_H
#define INCL_OBJECT_FREEZER_H

#include "Writer.h"

#include "FTypes.h"

#include <stddef.h>

namespace NOMADSUtil
{

    class ObjectFreezer
    {
        public:
            ObjectFreezer (void);
            ~ObjectFreezer (void);

            int init (Writer *pw);
            int beginNewObject (const char *pszObjectType);
            int endObject (void);
            int putBool (bool b);
            int putChar (char ch);
            int putUnsignedChar (unsigned char uch);
            int putInt16 (int16 i16);
            int putUInt16 (uint16 ui16);
            int putInt32 (int32 i32);
            int putUInt32 (uint32 ui32);
            int putInt64 (int64 i64);
            int putFloat (float f);
            int putDouble (double d);
            int putString (const char *pszString);
            int putBlob (void *pBlob, uint32 length);
            int operator << (bool b);
            int operator << (char ch);
            int operator << (unsigned char uch);
            int operator << (int16 i16);
            int operator << (uint16 ui16);
            int operator << (int32 i32);
            int operator << (uint32 ui32);
            int operator << (int64 i64);
            int operator << (float f);
            int operator << (double d);
            int operator << (const char *pszString);
            bool operationFailed (void);
        protected:
            enum Type {
                Bool = 1,
                Char,
                UnsignedChar,
                Int16,
                UInt16,
                Int32,
                UInt32,
                Int64,
                Float,
                Double,
                String,
                Blob
            };
            int beginPut (Type t);
            int endPut (void);
        protected:
            bool _bFailed;
            Writer *_pWriter;
    };

    inline int ObjectFreezer::operator << (bool b)
    {
        return putBool (b);
    }

    inline int ObjectFreezer::operator << (char ch)
    {
        return putChar (ch);
    }

    inline int ObjectFreezer::operator << (unsigned char uch)
    {
        return putUnsignedChar (uch);
    }

    inline int ObjectFreezer::operator << (int16 i16)
    {
        return putInt16 (i16);
    }

    inline int ObjectFreezer::operator << (uint16 ui16)
    {
        return putUInt16 (ui16);
    }

    inline int ObjectFreezer::operator << (int32 i32)
    {
        return putInt32 (i32);
    }

    inline int ObjectFreezer::operator << (uint32 ui32)
    {
        return putUInt32 (ui32);
    }

    inline int ObjectFreezer::operator << (int64 i64)
    {
        return putInt64 (i64);
    }

    inline int ObjectFreezer::operator << (float f)
    {
        return putFloat (f);
    }

    inline int ObjectFreezer::operator << (double d)
    {
        return putDouble (d);
    }

    inline int ObjectFreezer::operator << (const char *pszString)
    {
        return putString (pszString);
    }

    inline int ObjectFreezer::beginPut (Type t)
    {
        if (_pWriter == NULL) {
            _bFailed = true;
            return -1;
        }
        #if defined (ERROR_CHECKING) || defined (STATE_ERROR_CHECKING)
            unsigned char uch = t;
            if (_pWriter->writeBytes (&uch, sizeof (unsigned char))) {
                _bFailed = true;
                return -2;
            }
        #endif
        return 0;
    }

    inline int ObjectFreezer::endPut (void)
    {
        if (_bFailed) {
            return -1;
        }
        return 0;
    }

}   // namespace NOMADSUtil

#endif   // #ifndef INCL_OBJECT_FREEZER_H
