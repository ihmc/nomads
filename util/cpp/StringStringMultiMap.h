/*
 * StringStringMultiMap.h
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

#ifndef INCL_MULTI_MAP_H
#define INCL_MULTI_MAP_H

#include <string>
#include <map>

namespace NOMADSUtil
{

    using namespace std;

    /**
     * Maps a string key to multiple string values.
     */
    class StringStringMultiMap
    {
        public:
            void put (string key, string value);
            bool hasKey (string key);
            bool hasKeyValue (string key, string value);
            bool remove (string key);
            bool remove (string key, string value);

        private:
            multimap<string, string> _multimap;
    };

    inline void StringStringMultiMap::put (string key, string value)
    {
        _multimap.insert (pair<string, string> (key, value));
    }

    inline bool StringStringMultiMap::remove (string key)
    {
        multimap<string, string>::iterator pos;
        pos = _multimap.find (key);
        if (pos != _multimap.end()) { //key found
            do {
                _multimap.erase (pos++);
            }
            while (pos != _multimap.upper_bound (key));
        }
        return false;
    }

    inline bool StringStringMultiMap::remove (string key, string value)
    {
        multimap<string, string>::iterator pos;
        pos = _multimap.find (key);
        if (pos != _multimap.end()) { //key found
            do {
                if (pos->second == value) {
                    _multimap.erase (pos);
                    return true;
                }
                pos++;
            }
            while (pos != _multimap.upper_bound (key));
        }
        return false;
    }

    inline bool StringStringMultiMap::hasKey (string key)
    {
        multimap<string, string>::iterator p;
        p = _multimap.find (key);
        if (p != _multimap.end()) { //key found
            return true;
        }
        return false;
    }

    inline bool StringStringMultiMap::hasKeyValue (string key, string value)
    {
        multimap<string, string>::iterator p;
        p = _multimap.find (key);
        if (p != _multimap.end()) { //key found
            do {
                if (p->second == value) {
                    return true;
                }
                p++;
            }
            while (p != _multimap.upper_bound (key));
        }
        return false;
    }

}

#endif //INCL_MULTI_MAP_H
