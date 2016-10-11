/*
 * ConfigManager.h
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

#ifndef INCL_CONFIG_MANAGER_H
#define INCL_CONFIG_MANAGER_H

#pragma warning (disable:4786)

#if defined (WIN32)
    #if !defined (PATH_MAX)
        #define PATH_MAX _MAX_PATH
    #endif
#endif

#include "FTypes.h"
#include "StringStringHashtable.h"
#include <stdlib.h>

namespace NOMADSUtil
{
    class Reader;
    class Writer;

    class ConfigManager
    {
        public:
            ConfigManager (void);
            ~ConfigManager (void);

            /**
             * init must be called before calling any of the read() methods.
             */
            int init (int iMaxLineLen = 1024);

            StringStringHashtable * getProperties (void);

            void display (void);

            /**
             * Added all the property in cfgMgr. If two properties have the same
             * key and different values an error is returned.
             */
            int addProperties (ConfigManager &cfgMgr);

            /**
             * - ui32Len is the length of the stream to be read 
             */
            int read (Reader *pReader, uint32 ui32Len, bool bBeTolerant=false);
            int readConfigFile (const char *pszFile, bool bBeTolerant = false);

            int write (Writer *pWriter);
            void writeConfigFile (const char *pszFilePath);
            void writeConfigFile (void);

            /**
             * Performs a simple variable substitution
             * Looks for values beginning with '$', treats the rest of the value
             * as a variable, looks up a new value with the key, and replaces the
             * value with the new value
             *
             * NOTE: Assumes that there can be only one variable as the value and
             * that the entire value following the '$' is the variable name
             */
            int performVariableSubstitution (void);

            bool hasValue (const char *pszKey) const;

            void setValue (const char *pszKey, const char *pszValue);
            void setValue (const char *pszKey, int iValue);

            const char * getValue (const char *pszKey) const;

            bool getValueAsBool (const char *pszKey) const;

            int getValueAsInt (const char *pszKey) const;

            int64 getValueAsInt64 (const char *pszKey) const;

            uint32 getValueAsUInt32 (const char *pszKey) const;
            uint64 getValueAsUInt64 (const char *pszKey) const;

            double getValueAsDouble (const char *pszKey) const;
            float getValueAsFloat (const char *pszKey) const;

            /**
             * Returns the value of pszKey if it exists, sets and returns the default
             * value otherwise.
             */
            const char * getValue (const char *pszKey, const char *pszDefault) const;
            int getValueAsInt (const char *pszKey, int iDefault) const;
            uint32 getValueAsUInt32 (const char *pszKey, uint32 ui32Default) const;
            int64 getValueAsInt64 (const char *pszKey, int64 i64Default) const;
            bool getValueAsBool (const char *pszKey, bool bDefault) const;

            const char * removeValue (const char *pszKey) const;

            StringStringHashtable::Iterator getAllElements (void);

            /**
             * Returns a null-terminated string representing the absolute path
             * of the default configuration file, if the path exists.
             * If the path does not exist, NULL is returned.
             *
             * NOTE: the returned string must be deallocated by the caller
             */
            static char * getDefaultConfigFileDirectory (const char *pszHomeDir);
            static char * getDefaultConfigFilePath (const char *pszHomeDir, const char *pszConfigFileName);

            static bool parseBool (const char *pszValue);

        protected:
            int _iMaxLineLen;
            char *_pszConfigFile;
            StringStringHashtable *_pSettings;
    };
}

#endif   // #ifndef INCL_CONFIG_MANAGER_H
