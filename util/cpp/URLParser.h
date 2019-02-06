/*
 * URLParser.h
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

#ifndef INCL_URL_PARSER_H
#define INCL_URL_PARSER_H

#include "Exceptions.h"

#pragma warning (disable:4290)

namespace NOMADSUtil
{
    class URLParser
    {
        public:
            // Void Constructor
            // Must call parse() before calling any of the getXXX() methods
            URLParser (void);

            // Parses the given URL
            // Throws FormatException if the URL is not correctly formed
            URLParser (const char *pszURL) throw (FormatException);

            // Destructor
            ~URLParser (void);

            // Parses the given URL
            // Throws FormatException if the URL is not correctly formed
            void parse (const char *pszURL) throw (FormatException);

            // Returns the protocol for the URL (e.g., "http")
            // Throws InvalidStateException if a URL was not correctly parsed
            const char * getProtocol (void) throw (InvalidStateException);

            // Returns the host for the URL
            // Throws InvalidStateException if a URL was not correctly parsed
            const char * getHost (void) throw (InvalidStateException);

            // Returns the port number specified by the URL
            // Will return -1 if no port number was specified as part of the URL
            // Throws InvalidStateException if a URL was not correctly parsed
            int getPort (void) throw (InvalidStateException);

            // Returns the path specified by the URL
            // Returns "/" if no path was specified as part of the URL
            // Throws InvalidStateException if a URL was not correctly parsed
            const char * getPath (void) throw (InvalidStateException);

        protected:
            char *_pszBuf;
            char *_pszProtocol;
            char *_pszHost;
            int _iPort;
            char *_pszPath;
    };
}

#endif   // #ifndef INCL_URL_PARSER_H
