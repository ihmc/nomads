/*
 * Base64Transcoders.cpp
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "Base64Transcoders.h"

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
