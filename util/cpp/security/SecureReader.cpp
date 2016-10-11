/*
 * SecureReader.cpp
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

#include "SecureReader.h"
#ifdef WIN32
    #include <winsock2.h>
#endif
#include "openssl/des.h"
#include "openssl/err.h"
#include "openssl/rsa.h"
#include "openssl/sha.h"

#include <memory.h>

using namespace CryptoUtils;

SecureReader::SecureReader (PublicKey *pPublicKey, Reader *pReader, bool bDeleteWhenDone)
{
    _pPublicKey = pPublicKey;
    _pPrivateKey = NULL;
    _pSecretKey = NULL;
    _pReader = pReader;
    _bDeleteReaderWhenDone = bDeleteWhenDone;
    _iKeySize = RSA_size ((RSA*)pPublicKey->getKey());
    _iPaddingType = RSA_PKCS1_PADDING;
    _iPaddingSize = RSA_PKCS1_PADDING_SIZE;
    _pInBuf = malloc (_iKeySize);
    _pOutBuf = malloc (_iKeySize);
    _ulOutPos = 0;
    _ulOutCount = 0;
}

SecureReader::SecureReader (PrivateKey *pPrivateKey, Reader *pReader, bool bDeleteWhenDone)
{
    _pPublicKey = NULL;
    _pPrivateKey = pPrivateKey;
    _pSecretKey = NULL;
    _pReader = pReader;
    _bDeleteReaderWhenDone = bDeleteWhenDone;
    _iKeySize = RSA_size ((RSA*)pPrivateKey->getKey());
    _iPaddingType = RSA_PKCS1_OAEP_PADDING;
    _iPaddingSize = SHA_DIGEST_LENGTH *2 + 2;
    _pInBuf = malloc (_iKeySize);
    _pOutBuf = malloc (_iKeySize);
    _ulOutPos = 0;
    _ulOutCount = 0;
}

SecureReader::SecureReader (SecretKey *pSecretKey, Reader *pReader, bool bDeleteWhenDone)
{
    _pPublicKey = NULL;
    _pPrivateKey = NULL;
    _pSecretKey = pSecretKey;
    _pReader = pReader;
    _bDeleteReaderWhenDone = bDeleteWhenDone;
    _iKeySize = DES_KEY_SZ;
    _iPaddingType = 0;
    _iPaddingSize = 0;
    _pInBuf = malloc (_iKeySize);
    _pOutBuf = malloc (_iKeySize);
    _ulOutPos = 0;
    _ulOutCount = 0;
}

SecureReader::~SecureReader (void)
{
    if (_bDeleteReaderWhenDone) {
        delete _pReader;
    }
    _pReader = NULL;

    if (_pInBuf) {
        free (_pInBuf);
        _pInBuf = NULL;
    }
    if (_pOutBuf) {
        free (_pOutBuf);
        _pOutBuf = NULL;
    }
}

int SecureReader::read (void *pBuf, int iCount)
{
    if (iCount <= 0) {
        return -1;
    }
    if (_ulOutPos < _ulOutCount) {
        unsigned long ulBytesAvail = _ulOutCount - _ulOutPos;
        int iBytesToReturn;
        if (iCount < (int) ulBytesAvail) {
            iBytesToReturn = iCount;
        }
        else {
            iBytesToReturn = (int) ulBytesAvail;
        }
        memcpy (pBuf, ((unsigned char*)_pOutBuf)+_ulOutPos, iBytesToReturn);
        _ulOutPos += iBytesToReturn;
        return iBytesToReturn;
    }
    else {
        int rc;
        if ((rc = readAndDecryptBlock()) <= 0) {
            return -3;
        }
        _ulOutPos = 0;
        _ulOutCount = rc;
        return read (pBuf, iCount);
    }
}

int SecureReader::readBytes (void *pBuf, unsigned long ulCount)
{
    unsigned long ulBytesRead = 0;
    while (ulBytesRead < ulCount) {
        int rc = read (((unsigned char*)pBuf)+ulBytesRead, ulCount-ulBytesRead);
        if (rc <= 0) {
            return -1;
        }
        ulBytesRead += rc;
    }
    return 0;
}

int SecureReader::readAndDecryptBlock (void)
{
    if (_pReader->readBytes (_pInBuf, _iKeySize)) {
        return -1;
    }
    int rc;
    if (_pPublicKey) {
        rc = RSA_public_decrypt (_iKeySize, (unsigned char*) _pInBuf, (unsigned char*) _pOutBuf,
                                 (RSA*)_pPublicKey->getKey(), _iPaddingType);
    }
    else if (_pPrivateKey) {
        rc = RSA_private_decrypt (_iKeySize, (unsigned char*) _pInBuf, (unsigned char*) _pOutBuf,
                                  (RSA*)_pPrivateKey->getKey(), _iPaddingType);
    }
    else if (_pSecretKey) {
        DES_ecb_encrypt ((DES_cblock*)_pInBuf, (DES_cblock*)_pOutBuf,
                         (DES_key_schedule*)_pSecretKey->getKey(), DES_DECRYPT);
        rc = _iKeySize;
    }
    else {
        return -2;
    }
    if (rc <= 0) {
        return -3;
    }
    return rc;
}
