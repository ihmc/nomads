/*
 * SecureWriter.h
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
 * Defines the SecureWriter class that is used to encrypt data being written out
 */

#ifndef INCL_SECURE_WRITER_H
#define INCL_SECURE_WRITER_H

#include <stddef.h>

#include "Writer.h"

#include "CryptoUtils.h"

namespace CryptoUtils
{

    class SecureWriter : public NOMADSUtil::Writer
    {
        public:
            // Uses the specified public key for encryption
            // Data must be decrypted with the corresponding private key
            SecureWriter (PublicKey *pPublicKey, NOMADSUtil::Writer *pWriter, bool bDeleteWhenDone);

            // Uses the specified private key for encryption
            // Data must be decrypted with the corresponding public key
            SecureWriter (PrivateKey *pPrivateKey, NOMADSUtil::Writer *pWriter, bool bDeleteWhenDone);

            // Uses the specified secret key for encryption
            // Data must be decrypted using the same key
            SecureWriter (SecretKey *pSecretKey, NOMADSUtil::Writer *pWriter, bool bDeleteWhenDone);

            // Destructor
            ~SecureWriter (void);

            // Writes the specified bytes
            // Returns 0 if successful and < 0 in case of error
            int writeBytes (const void *pBuf, unsigned long ulCount);

            // Flushes any buffered data
            // The encrypted block may be padded as necessary
            int flush (void);

        protected:
            int encryptBuffer (void);

        protected:
            NOMADSUtil::Writer *_pWriter;
            bool _bDeleteWriterWhenDone;
            PublicKey *_pPublicKey;
            PrivateKey *_pPrivateKey;
            SecretKey *_pSecretKey;
            int _iKeySize;
            int _iPaddingType;
            int _iPaddingSize;
            void *_pInBuf;
            unsigned long _ulInCount;
            void *_pOutBuf;
    };

}
 
#endif   // #ifndef INCL_SECURE_WRITER_H
