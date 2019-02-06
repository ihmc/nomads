/*
 * HTTPClient.h
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

#ifndef INCL_HTTP_CLIENT_H
#define INCL_HTTP_CLIENT_H

#include "FTypes.h"
#include <string.h>

namespace NOMADSUtil
{

    //Forward declaration
    class TCPSocket;

    class HTTPClient
    {
        public:
            struct HTTPMetaData {
                enum ReturnCode {
                    RC_Unknown,
                    RC_Ok,
                    RC_InvalidNetworkAddr,
                    RC_InvalidPath,
                    RC_UnknownError
                };
                ReturnCode rc;
                uint32 ui32Length;
                HTTPMetaData (void);
            };
            struct HTTPData {
                HTTPMetaData metaData;
                char *pData;
                HTTPData (void);
            };
            struct BasicAuthorization
            {
                const char *pszUsername;
                const char *pszPassword;
            };
            static HTTPData postData (const char *pszHost, unsigned short usPort, const char *pszPath, const char *pszData, BasicAuthorization *pAuth = NULL);
            static HTTPData getData (const char *pszHost, unsigned short usPort, const char *pszPath);
            static HTTPData getData (const char *pszURL);
            static HTTPMetaData getDataIntoFile (const char *pszFile, const char *pszURL);
            static HTTPMetaData getDataIntoFile (const char *pszFile, const char *pszHost, unsigned short usPort, const char *pszPath);
        protected:
            static TCPSocket * openConnection (const char *pszHost, unsigned short usPort);
    };

    inline HTTPClient::HTTPMetaData::HTTPMetaData (void)
    {
        rc = RC_Unknown;
        ui32Length = 0;
    }

    inline HTTPClient::HTTPData::HTTPData (void)
    {
        pData = NULL;
    }

}

#endif   // #ifndef INCL_HTTP_CLIENT_H
