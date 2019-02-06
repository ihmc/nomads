/*
 * Json.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */


#ifndef INCL_JSON_H
#define INCL_JSON_H

struct cJSON;

#include "StrClass.h"
#include <stddef.h>

namespace NOMADSUtil
{
    class JsonArray;
    class JsonObject;
    class Reader;
    class Writer;

    class Json
    {
        public:
            virtual ~Json (void);

            int init (const char *pszJson);

            Json * clone (void) const;

            /**
             * NB: JsonObject return a pointer to the search object. The returned
             *     pointer should not be modified (nor deallocated)
             */
            const JsonArray * getArray (const char *pszName) const;
            JsonObject * getObject (const char *pszName) const;

            /**
             */
            JsonArray * getArrayReference (const char *pszName) const;

            int getSize (void) const;
            cJSON * relinquishCJson (void);

        protected:
            friend class JsonArray;
            Json (cJSON *pRoot, bool bDeallocate);

            const bool _bDeallocate;
            cJSON *_pRoot;
    };

    class JsonObject : public Json
    {
        public:
            JsonObject (const char *pszJson = NULL);
            ~JsonObject (void);

            int init (const char *pszJson);

            void clear (void);

            // If bRelinquish is set on true, a copy of the value will be made
            int setObject (const char *pszName, Json *pValue);
            int setObject (const char *pszName, const Json *pValue);
            int setString (const char *pszName, const char *pszValue);
            int setStrings (const char *pszName, const char **ppszValues, int i);
            int setNumber (const char *pszName, int iValue);
            int setNumber (const char *pszName, uint32 ui32Value);
            int setNumber (const char *pszName, uint64 ui64Value);
            int setNumber (const char *pszName, int64 i64Value);
            int setNumber (const char *pszName, double dValue);
            int setBoolean (const char *pszName, bool bValue);

            int removeValue (const char *pszName);

            int getBoolean (const char *pszName, bool &bVal) const;
            int getNumber (const char *pszName, int &iVal) const;
            int getNumber (const char *pszName, uint16 &ui16Val) const;
            int getNumber (const char *pszName, uint32 &ui32Val) const;
            int getNumber (const char *pszName, uint64 &ui64Val) const;
            int getNumber (const char *pszName, int64 &i64Val) const;
            int getNumber (const char *pszName, double &dVal) const;
            int getString (const char *pszName, NOMADSUtil::String &sVal) const;

            int getAsString (const char *pszName, NOMADSUtil::String &sVal) const;

            bool hasObject (const char *pszName) const;
            String toString (bool bMinimized = false) const;

            int read (Reader *pReader, bool bCompressed);
            int write (Writer *pWriter, bool bCompressed);

        private:
            friend class Json;
            friend class JsonArray;
            JsonObject (cJSON *pRoot, bool bDeallocate);
    };

    class JsonArray : public Json
    {
        public:
            JsonArray (const char *pszJson = NULL);
            ~JsonArray (void);

            int addObject (JsonObject *pValue);
            int removeObject (int iIdx);
            int addString (const char *pszValue);
            int addNumber (uint32 ui32Value);
            int setObject (JsonObject *pValue);
            JsonObject * getObject (int iIdx) const;
            int getString (int iIdx, NOMADSUtil::String &sVal) const;

        private:
            friend class Json;
            JsonArray (cJSON *pRoot, bool bDeallocate);
    };
}

#endif  /* INCL_JSON_H */

