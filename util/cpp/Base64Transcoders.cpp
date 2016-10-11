/*
 * Base64Transcoders.cpp
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

#include "Base64Transcoders.h"
#ifdef WIN32
    #include <winsock2.h>
#endif
#include <openssl/evp.h>
#include <string.h>

using namespace NOMADSUtil;

char * Base64Transcoders::encode (const void *pBuf, unsigned int uiLen)
{
    if ((pBuf == NULL) || (uiLen == 0)) {
        return NULL;
    }

    // Compute the length of the Base64 encoded string - the ratio is 3:4
    unsigned int uiB64Len = (((uiLen + 2) / 3) * 4) + 1;
    char *pszRetBuf = (char *) malloc (uiB64Len);
    if (pszRetBuf == NULL) {
        return NULL;
    }
    int iBytesWritten = EVP_EncodeBlock ((unsigned char*) pszRetBuf, (unsigned char*) pBuf, uiLen);
    if (iBytesWritten < 0) {
        free (pszRetBuf);
        return NULL;
    }
    pszRetBuf[iBytesWritten] = '\0';
    return pszRetBuf;
}

void * Base64Transcoders::decode (const char *pszB64Buf, unsigned int *puiLen)
{
    if ((pszB64Buf == NULL) || (puiLen == NULL)) {
        return NULL;
    }

    // Compute the length of the decoded binary data the ratio is 4:3
    size_t stInputLen = strlen (pszB64Buf);
    unsigned int uiDecodedLen = (unsigned int) (((stInputLen + 3) / 4) * 3);

    unsigned char *puchRetBuf = (unsigned char *) malloc (uiDecodedLen);
    if (puchRetBuf == NULL) {
        return NULL;
    }

    int rc = EVP_DecodeBlock (puchRetBuf, (unsigned char*) pszB64Buf, (int) stInputLen);
    if (rc < 0) {
        free (puchRetBuf);
        return NULL;
    }
    else {
        *puiLen = rc;
        return puchRetBuf;
    }
}
