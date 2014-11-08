/*
 * SecureReader.h
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
 *
 * Defines the SecureReader class that is used to decrypt data being read in
 *
 */

#ifndef INCL_SECURE_READER_H
#define INCL_SECURE_READER_H

#include <stddef.h>

#include "Reader.h"

#include "CryptoUtils.h"

namespace CryptoUtils
{

    class SecureReader : public NOMADSUtil::Reader
    {
        public:
            // Uses the specified public key for decryption
            // Data must have been encrypted with the corresponding private key
            SecureReader (PublicKey *pPublicKey, NOMADSUtil::Reader *pReader, bool bDeleteWhenDone = false);

            // Uses the specified private key for decryption
            // Data must have been encrypted with the corresponding public key
            SecureReader (PrivateKey *pPrivateKey, NOMADSUtil::Reader *pReader, bool bDeleteWhenDone = false);

            // Uses the specified secret key for decryption
            // Data must have been encrypted using the same key
            SecureReader (SecretKey *pSecretKey, NOMADSUtil::Reader *pReader, bool bDeleteWhenDone = false);

            // Destructor
            ~SecureReader (void);

            // Reads upto the specified number of bytes into the specified buffer
            // NOTE: The actual number read might be less than the number requested
            // Returns the number of bytes read or < 0 in case of EOF or error
            int read (void *pBuf, int iCount);

            // Reads the specified number of bytes into the specified buffer
            // NOTE: Unlike read(), this one fails if the specified number could not be read
            // Returns 0 if successful or < 0 in case of EOF or error
            int readBytes (void *pBuf, unsigned long ulCount);

        protected:
            int readAndDecryptBlock (void);

        protected:
            NOMADSUtil::Reader *_pReader;
            bool _bDeleteReaderWhenDone;
            PublicKey *_pPublicKey;
            PrivateKey *_pPrivateKey;
            SecretKey *_pSecretKey;
            int _iKeySize;
            int _iPaddingType;
            int _iPaddingSize;
            void *_pInBuf;
            void *_pOutBuf;
            unsigned long _ulOutPos;
            unsigned long _ulOutCount;
    };

}

#endif   // #ifndef INCL_SECURE_READER_H
