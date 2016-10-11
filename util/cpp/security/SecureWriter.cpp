/*
 * SecureWriter.cpp
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

#include "SecureWriter.h"
#ifdef WIN32
    #include <winsock2.h>
#endif
#include "openssl/des.h"
#include "openssl/err.h"
#include "openssl/rsa.h"
#include "openssl/sha.h"

#include <memory.h>

using namespace CryptoUtils;

SecureWriter::SecureWriter (PublicKey *pPublicKey, Writer *pWriter, bool bDeleteWhenDone)
{
    _pPublicKey = pPublicKey;
    _pPrivateKey = NULL;
    _pSecretKey = NULL;
    _pWriter = pWriter;
    _bDeleteWriterWhenDone = bDeleteWhenDone;
    _iKeySize = RSA_size ((RSA*)pPublicKey->getKey());
    _iPaddingType = RSA_PKCS1_OAEP_PADDING;
    _iPaddingSize = SHA_DIGEST_LENGTH * 2 + 2;
    _pInBuf = malloc (_iKeySize);
    _ulInCount = 0;
    memset (_pInBuf, 0, _iKeySize);
    _pOutBuf = malloc (_iKeySize);
}

SecureWriter::SecureWriter (PrivateKey *pPrivateKey, Writer *pWriter, bool bDeleteWhenDone)
{
    _pPublicKey = NULL;
    _pPrivateKey = pPrivateKey;
    _pSecretKey = NULL;
    _pWriter = pWriter;
    _bDeleteWriterWhenDone = bDeleteWhenDone;
    _iKeySize = RSA_size ((RSA*)pPrivateKey->getKey());
    _iPaddingType = RSA_PKCS1_PADDING;
    _iPaddingSize = RSA_PKCS1_PADDING_SIZE;
    _pInBuf = malloc (_iKeySize);
    _ulInCount = 0;
    memset (_pInBuf, 0, _iKeySize);
    _pOutBuf = malloc (_iKeySize);
}

SecureWriter::SecureWriter (SecretKey *pSecretKey, Writer *pWriter, bool bDeleteWhenDone)
{
    _pPublicKey = NULL;
    _pPrivateKey = NULL;
    _pSecretKey = pSecretKey;
    _pWriter = pWriter;
    _bDeleteWriterWhenDone = bDeleteWhenDone;
    _iKeySize = DES_KEY_SZ;
    _iPaddingType = 0;
    _iPaddingSize = 0;
    _pInBuf = malloc (_iKeySize);
    _ulInCount = 0;
    memset (_pInBuf, 0, _iKeySize);
    _pOutBuf = malloc (_iKeySize);
}

SecureWriter::~SecureWriter (void)
{
    flush();

    if (_bDeleteWriterWhenDone) {
        delete _pWriter;
    }
    _pWriter = NULL;

    if (_pInBuf) {
        free (_pInBuf);
        _pInBuf = NULL;
    }
    if (_pOutBuf) {
        free (_pOutBuf);
        _pOutBuf = NULL;
    }
}

int SecureWriter::writeBytes (const void *pBuf, unsigned long ulCount)
{
    if (_iKeySize <= 0) {
        return -1;
    }

    while (ulCount > 0) {
        unsigned long ulBytesToProcess;
        unsigned long ulSpaceLeft = (_iKeySize - _iPaddingSize) - _ulInCount;
        if (ulCount > ulSpaceLeft) {
            ulBytesToProcess = ulSpaceLeft;
        }
        else {
            ulBytesToProcess = ulCount;
        }
        memcpy (((unsigned char*)_pInBuf)+_ulInCount, pBuf, ulBytesToProcess);
        _ulInCount += ulBytesToProcess;
        pBuf = ((unsigned char*)pBuf) + ulBytesToProcess;
        ulCount -= ulBytesToProcess;

        if (_ulInCount == (_iKeySize - _iPaddingSize)) {
            if (encryptBuffer()) {
                return -2;
            }
            if (_pWriter->writeBytes (_pOutBuf, _iKeySize)) {
                return -3;
            }
            _ulInCount = 0;
            memset (_pInBuf, 0, _iKeySize);
        }
    }

    return 0;
}

int SecureWriter::flush (void)
{
    if (_ulInCount > 0) {
        if (encryptBuffer()) {
            return -1;
        }
        if (_pWriter->writeBytes (_pOutBuf, _iKeySize)) {
            return -2;
        }
        _ulInCount = 0;
        memset (_pInBuf, 0, _iKeySize);
    }
    return 0;
}

int SecureWriter::encryptBuffer (void)
{
    int rc;
    if (_pPublicKey) {
        rc = RSA_public_encrypt (_ulInCount, (unsigned char*) _pInBuf, (unsigned char*) _pOutBuf,
                                 (RSA*) _pPublicKey->getKey(), _iPaddingType);
    }
    else if (_pPrivateKey) {
        rc = RSA_private_encrypt (_ulInCount, (unsigned char*) _pInBuf, (unsigned char*) _pOutBuf,
                                  (RSA*) _pPrivateKey->getKey(), _iPaddingType);
    }
    else if (_pSecretKey) {
        DES_ecb_encrypt ((DES_cblock*)_pInBuf, (DES_cblock*)_pOutBuf,
                         (DES_key_schedule*)_pSecretKey->getKey(), DES_ENCRYPT);
        rc = _iKeySize;
    }
    else {
        return -1;
    }
    if (rc != _iKeySize) {
        return -2;
    }
    return 0;
}
