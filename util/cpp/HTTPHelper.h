/*
 * HTTPHelper.h
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

#ifndef INCL_HTTP_HELPER_H
#define INCL_HTTP_HELPER_H

#include "StringStringHashtable.h"
#include "StrClass.h"

namespace NOMADSUtil
{

    class CommHelper2;
    class Reader;
    class Socket;
    class Writer;

    class HTTPHelper
    {
        public:
            HTTPHelper (void);
            ~HTTPHelper (void);

            int init (Socket *pSock);
            int init (CommHelper2 *pch);
            int init (Reader *pReader, Writer *pWriter);

            int sendHeader (const char *pszMethodName, const char * pszResourceName, StringStringHashtable &headerFields);
            int sendField (const char* pszFieldName, const char* pszFieldValue);
            int sendResponseCode (uint16 ui16ResponseCode);
            int sendBlankLine (void);

            int setExpectedResponseCode (uint16 ui16ResponseCode);
            int expectResponseCode (uint16 ui16ResponseCode);
            int checkAndSendResponseCode (uint16 ui16ResponseCode);
            uint16 getExpectedResponseCode (void);
            int sendContent (void *pContent, uint32 ui32ContentLength);
            int getLastResponseCode (void);
            int receiveHeader (void);
            int receiveHeader (const char * pszProtoLine);
            uint32 getContentLength (void);
            void *receiveContent (void);
            int receiveContent (void *pContent, uint32 ui32ContentLength);
            bool hasHeaderFieldValue (const char *pszKey);
            String getHeaderFieldValue (const char *pszKey);

            String getResourceName (void);
            String getRequestMethod (void); // will return whether the request was a POST, GET, DELETE, etc, etc

            CommHelper2 * getCommHelperRef (void);
            int flush();

            static String getResponseCodeMessage (uint16 ui16CodeNum);

        private:
            static const char * getCodeMessage (uint16 ui16Code);

        private:
            CommHelper2 *_pch;
            bool _bDeleteCommHelper;
            bool _bExpectCode;
            uint16 _ui16ExpectedHTTPCode;
            void *_pContent;
            uint32 _ui32ContentLength;
            uint16 _ui16LastResponseCode;

            String _requestMethod;
            String _resourceName;

            StringStringHashtable *_pHeaderFieldsHT;
    };

    inline CommHelper2 * HTTPHelper::getCommHelperRef()
    {
        return _pch;
    }

    static const char* http_status_string[] = {
        "100 Continue",
        "101 Switching Protocols",
        "102 Processing",
    #define LEVEL_200  3
        "200 OK",
        "201 Created",
        "202 Accepted",
        "203 Non-Authoritative Information",
        "204 No Content",
        "205 Reset Content",
        "206 Partial Content",
        "207 Multi-Status",
    #define LEVEL_300 11
        "300 Multiple Choices",
        "301 Moved Permanently",
        "302 Found",
        "303 See Other",
        "304 Not Modified",
        "305 Use Proxy",
        "306 unused",
        "307 Temporary Redirect",
    #define LEVEL_400 19
        "400 Bad Request",
        "401 Authorization Required",
        "402 Payment Required",
        "403 Forbidden",
        "404 Not Found",
        "405 Method Not Allowed",
        "406 Not Acceptable",
        "407 Proxy Authentication Required",
        "408 Request Time-out",
        "409 Conflict",
        "410 Gone",
        "411 Length Required",
        "412 Precondition Failed",
        "413 Request Entity Too Large",
        "414 Request-URI Too Large",
        "415 Unsupported Media Type",
        "416 Requested Range Not Satisfiable",
        "417 Expectation Failed",
        "418 unused",
        "419 unused",
        "420 unused",
        "421 unused",
        "422 Unprocessable Entity",
        "423 Locked",
        "424 Failed Dependency",
        "425 No code",
        "426 Upgrade Required",
    #define LEVEL_500 46
        "500 Internal Server Error",
        "501 Method Not Implemented",
        "502 Bad Gateway",
        "503 Service Temporarily Unavailable",
        "504 Gateway Time-out",
        "505 HTTP Version Not Supported",
        "506 Variant Also Negotiates",
        "507 Insufficient Storage",
        "508 unused",
        "509 unused"
    };

    #define RESPONSE_CODES  56

}

#endif //INCL_HTTP_HELPER_H
