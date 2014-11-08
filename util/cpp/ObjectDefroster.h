/*
 * ObjectDefroster.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#ifndef INCL_OBJECT_DEFROSTER_H
#define INCL_OBJECT_DEFROSTER_H

#include "ObjectFreezer.h"

#include "FTypes.h"
#include "Reader.h"

#include <stddef.h>

namespace NOMADSUtil
{

    class ObjectDefroster
    {
        public:
            ObjectDefroster (void);
            ~ObjectDefroster (void);
            int init (Reader *pr);
            int beginNewObject (const char *pszObjectType);
            int endObject (void);
            int getBool (bool &b);
            int getChar (char &ch);
            int getUnsignedChar (unsigned char &uch);
            int getInt16 (int16 &i16);
            int getUInt16 (uint16 &ui16);
            int getInt32 (int32 &i32);
            int getUInt32 (uint32 &ui32);
            int getInt64 (int64 &i64);
            int getFloat (float &f);
            int getDouble (double &d);
            int getString (char *&pszString);
            int getBlob (void *&pBlob);
            int operator >> (bool &b);
            int operator >> (char &ch);
            int operator >> (unsigned char &uch);
            int operator >> (int16 &i16);
            int operator >> (uint16 &ui16);
            int operator >> (int32 &i32);
            int operator >> (uint32 &ui32);
            int operator >> (int64 &i64);
            int operator >> (float &f);
            int operator >> (double &d);
            int operator >> (char *&pszString);
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
            int beginGet (Type t);
            int endGet (void);
        protected:
            bool bFailed;
            Reader *pReader;
            bool bSwapNeeded;
    };

    inline int ObjectDefroster::operator >> (bool &b)
    {
        return getBool (b);
    }

    inline int ObjectDefroster::operator >> (char &ch)
    {
        return getChar (ch);
    }

    inline int ObjectDefroster::operator >> (unsigned char &uch)
    {
        return getUnsignedChar (uch);
    }

    inline int ObjectDefroster::operator >> (int16 &i16)
    {
        return getInt16 (i16);
    }

    inline int ObjectDefroster::operator >> (uint16 &ui16)
    {
        return getUInt16 (ui16);
    }

    inline int ObjectDefroster::operator >> (int32 &i32)
    {
        return getInt32 (i32);
    }

    inline int ObjectDefroster::operator >> (uint32 &ui32)
    {
        return getUInt32 (ui32);
    }

    inline int ObjectDefroster::operator >> (int64 &i64)
    {
        return getInt64 (i64);
    }

    inline int ObjectDefroster::operator >> (float &f)
    {
        return getFloat (f);
    }

    inline int ObjectDefroster::operator >> (double &d)
    {
        return getDouble (d);
    }

    inline int ObjectDefroster::operator >> (char *&pszString)
    {
        return getString (pszString);
    }

    inline int ObjectDefroster::beginGet (Type t)
    {
        if (pReader == NULL) {
            bFailed = true;
            return -1;
        }
        #if defined (ERROR_CHECKING) || defined (STATE_ERROR_CHECKING)
            unsigned char uch = 0;
            if (pReader->readBytes (&uch, sizeof (unsigned char))) {
                bFailed = true;
                return -2;
            }
            if (uch != t) {
                bFailed = true;
                return -3;
            }
        #endif
        return 0;
    }

    inline int ObjectDefroster::endGet (void)
    {
        if (bFailed) {
            return -1;
        }
        return 0;
    }

}   // namespace NOMADSUtil

#endif   // #ifndef INCL_OBJECT_DEFROSTER_H
