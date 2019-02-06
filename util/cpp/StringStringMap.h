/*
 * StringStringMap.h
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

#ifndef INCL_STRING_MAP_H
#define INCL_STRING_MAP_H

#pragma warning (disable:4786)

#include <map>
#include <string>

namespace NOMADSUtil
{

    class StringStringMap : public std::map<std::string,std::string>
    {
        public:
            const char * get (const char *pszKey);
    };

    inline const char * StringStringMap::get (const char *pszKey)
    {
        // The following part is a temporary hack since find() does
        // a case sensitive comparison - find out how to use case
        // insensitive comparisons in map
        char *pszLowerKey = new char [strlen(pszKey)+1];
        char *pszTemp = pszLowerKey;
        while (*pszTemp++ = tolower(*pszKey++))
            ;
        StringStringMap::iterator i = find (pszLowerKey);
        delete[] pszLowerKey;
        if (i != end()) {
            return i->second.c_str();
        }
        else {
            return NULL;
        }

        /*StringStringMap::iterator i = find (pszKey);
        if (i != end()) {
            return i->second.c_str();
        }
        else {
            return NULL;
        }*/
    }

}

#endif   // #ifndef INCL_STRING_MAP_H
