/*
 * CryptoUtils.h
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
 * Contains several utility classes related to cryptographic functionality
 * Also see SecureReader and SecureWriter
 */

#ifndef INCL_CRYPTO_UTILS_H
#define INCL_CRYPTO_UTILS_H

#include <stddef.h>

#include "FTypes.h"

namespace CryptoUtils
{

    // Utility class to be used for computing an MD5 hash value
    class MD5Hash
    {
        public:
            // Constructor
            MD5Hash (void);

            // Destructor
            virtual ~MD5Hash (void);

            // Call before reusing this instance to compute another hash
            int reinit (void);

            // Update the hash value with the specified data
            int update (const void *pBuf, unsigned long ulSize);

            // Get a pointer to the string containing the hash value
            const char * getHashAsBase64String (void);

        private:
            char *_pszHash;
            void *_pMDCTX;
            bool _bComputed;
    };

    /*
     * Defines the base class Key, which is subclassed by PublicKey and PrivateKey.
     * This class is not used directly by user code but it does define the nested
     * class KeyData that is used to hold encoded representation of keys.
     */
    class Key
    {
        public:
            // Constructor
            Key (void);

            // Destructor
            virtual ~Key (void);

            /*
             * Defines the KeyData class which is used to load and store keys
             * The data for encoded public and private keys can be retrieved as
             * instances of this class which then allows the caller to store the
             * data as desired. The keys can be recreated by recreating an instance
             * of the key data with the encoded data for the key and then passing
             * the instance of this class to PublicKey or PrivateKey
             */
            class KeyData
            {
                public:
                    enum Type {
                        Undefined,
                        X509,    // Format used to represent public keys
                        PKCS8    // Format used to represent private keys
                    };

                    // Constructor for an empty KeyData of the specified type
                    KeyData (Type type);

                    // Constructor that is used to create an initialized KeyData
                    // NOTE: The constructor makes a copy of the data that is passed in
                    KeyData (Type type, const char *pData, unsigned int uiLen);

                    // Copy constructor
                    KeyData (const KeyData &kd);

                    // Destructor
                    virtual ~KeyData (void);

                    // Set the data for the key
                    // NOTE: The function makes a copy of the data that is passed in
                    void setData (const char *pData, unsigned int uiLen);

                    // Get the type of the key whose data is stored in this object
                    Type getType (void);

                    // Returns a pointer to the key data
                    // NOTE: Caller must not deallocate this memory
                    const char * getData (void);

                    // Returns the length of the key data
                    unsigned int getLength (void);

                protected:
                    Type _type;
                    char *_pData;
                    unsigned int _uiLen;
            };

        protected:
            KeyData * convertBIOToKeyData (KeyData::Type type, void *pBIO);
            KeyData * readKeyDataFromFile (KeyData::Type type, const char *pszFileName);
            int writeKeyDataToFile (KeyData *pKD, const char *pszFileName);

    };

    /*
     * Defines the PublicKey class that is used to hold representations of a public key
     *
     */
    class PublicKey : public Key
    {
        public:
            // Constructor
            PublicKey (void);

            // Copy constructor
            PublicKey (const PublicKey &key);

            // Destructor
            virtual ~PublicKey (void);

            // Assignment operator
            PublicKey & operator = (const PublicKey &rhsKey);

            // Returns the data for this public key encoded as X.509 data
            KeyData * getKeyAsDEREncodedX509Data (void);

            // Reconstructs this public key based on the passed in data encoded in X.509 format
            int setKeyFromDEREncodedX509Data (KeyData *pKD);

            // Stores the data for this public key encoded as X.509 data to the specified file
            int storeKeyAsDEREncodedX509Data (const char *pszFileName);

            // Reads the specified file to get the encoded X.509 data to reconstruct this public key
            int loadKeyFromDEREncodedX509Data (const char *pszFileName);

            PublicKey (void *pKey);

        protected:
            friend class PublicKeyPair;
            friend class SecureReader;
            friend class SecureWriter;
            //PublicKey (void *pKey);
            void * getKey (void);

        protected:
            void *_pKey;
    };

    /*
     * Defines the PrivateKey class that is used to hold representations of a private key
     *
     */
    class PrivateKey : public Key
    {
        public:
            // Constructor
            PrivateKey (void);

            // Copy constructor
            PrivateKey (const PrivateKey &key);

            // Destructor
            virtual ~PrivateKey (void);

            // Assignment operator
            PrivateKey & operator = (const PrivateKey &rhsKey);

            // Returns the data for this private key encoded as PKCS#8 data
            KeyData * getKeyAsDEREncodedPKCS8Data (void);

            // Reconstructs this private key based on the passed in data in PKCS#8 format
            int setKeyFromDEREncodedPKCS8Data (KeyData *pKD);

            // Stores the data for this private key encoded as PKCS#8 data to the specified file
            int storeKeyAsDEREncodedPKCS8Data (const char *pszFileName);

            // Reads the specified file to get the encoded PKCS#8 data to reconstruct this private key
            int loadKeyFromDEREncodedPKCS8Data (const char *pszFileName);

        protected:
            friend class PublicKeyPair;
            friend class SecureReader;
            friend class SecureWriter;
            PrivateKey (void *pKey);
            void * getKey (void);

        protected:
            void *_pKey;
    };

    /*
     * Defines the PublicKeyPair class that is used to generate and hold a key pair
     * consisting of a public and a private key
     *
     */
    class PublicKeyPair
    {
        public:
            // Constructor
            PublicKeyPair (void);

            // Constructor that accepts a PublicKey and a PrivateKey instance
            // NOTE: The constructor makes duplicates of the passed in objects
            PublicKeyPair (PublicKey *pPublicKey, PrivateKey *pPrivateKey);

            // Destructor
            virtual ~PublicKeyPair (void);

            // Generates a new key pair with a default key length of 1024 bits
            // Returns 0 if successful or < 0 in case of errors
            int generateNewKeyPair (unsigned short usKeyLength = 1024);

            int setPublicKey (PublicKey *pKey);
            PublicKey * getPublicKey (void);

            int setPrivateKey (PrivateKey *pKey);
            PrivateKey * getPrivateKey (void);

        protected:
            PublicKey *_pPublicKey;
            PrivateKey *_pPrivateKey;
    };

    /*
     * Defines the SecretKey class that is used to hold a secret key for DES-style
     * cryptography
     *
     */
    class SecretKey
    {
        public:
            // Constructor
            SecretKey (void);

            // Copy constructor
            SecretKey (const SecretKey &key);

            // Destructor
            virtual ~SecretKey (void);

            // Assignment operator
            SecretKey & operator = (const SecretKey &rhsKey);

            // Initializes this key using the specified password
            // NOTE: If the password is longer than 8 characters, only the first 8 are used
            int initKey (const char *pszPassword);

        protected:
            friend class SecureReader;
            friend class SecureWriter;
            void * getKey (void);

        protected:
            void *_pKey;
    };

    // Utility Functions

    // Encrypts the data passed in pData using the specified key and writes the result into pDestBuf
    // Returns a negative value in case of a failure or if the destination buffer is too small
    int encryptDataUsingSecretKey (SecretKey *pKey, const void *pData, uint32 ui32DataLen, void *pDestBuf, uint32 ui32BufSize);

    // Encrypts the data passed in pData using the specified key and returns the result in a newly allocated buffer
    // The caller must deallocate the memory for the returned buffer using free() when done
    // The pointer pui32BufLen must point to a uint32, which is updated with the length of the encrypted buffer
    // Returns NULL in case of error
    void * encryptDataUsingSecretKey (SecretKey *pKey, const void *pData, uint32 ui32DataLen, uint32 *pui32BufLen);

    // Decrypts the data passed in pData using the specified key and write the result into pDestBuf
    // Returns a negative value in case of a failure or if the destination buffer is too small
    int decryptDataUsingSecretKey (SecretKey *pKey, const void *pData, uint32 ui32DataLen, void *pDestBuf, uint32 ui32BufSize);

    // Decrypts the data passed in pData using the specified key and returns the result in a newly allocated buffer
    // The caller must deallocate the memory for the returned buffer using free() when done
    // The pointer pui32BufLen must point to a uint32, which is updated with the length of the decrypted buffer
    // Returns NULL in case of error
    void * decryptDataUsingSecretKey (SecretKey *pKey, const void *pData, uint32 ui32DataLen, uint32 *pui32BufLen);

    int encryptDataUsingPublicKey (PublicKey *pKey, const void *pData, uint32 ui32DataLen, void *pDestBuf, uint32 ui32BufSize);
    void * encryptDataUsingPublicKey (PublicKey *pKey, const void *pData, uint32 ui32DataLen, uint32 *pui32BufLen);
    int decryptDataUsingPrivateKey (PrivateKey *pKey, const void *pData, uint32 ui32DataLen, void *pDestBuf, uint32 ui32BufSize);
    void * decryptDataUsingPrivateKey (PrivateKey *pKey, const void *pData, uint32 ui32DataLen, uint32 *pui32BufLen);

    // Implementations of inline functions follow

    inline Key::Key (void)
    {
    }

    inline Key::~Key (void)
    {
    }

    inline Key::KeyData::Type Key::KeyData::getType (void)
    {
        return _type;
    }

    inline const char * Key::KeyData::getData (void)
    {
        return _pData;
    }

    inline unsigned int Key::KeyData::getLength (void)
    {
        return _uiLen;
    }

    inline PublicKey::PublicKey (void)
    {
        _pKey = NULL;
    }

    inline PublicKey::PublicKey (void *pKey)
    {
        _pKey = pKey;
    }

    inline void * PublicKey::getKey (void)
    {
        return _pKey;
    }

    inline PrivateKey::PrivateKey (void)
    {
        _pKey = NULL;
    }

    inline PrivateKey::PrivateKey (void *pKey)
    {
        _pKey = pKey;
    }

    inline void * PrivateKey::getKey (void)
    {
        return _pKey;
    }

    inline PublicKeyPair::PublicKeyPair (void)
    {
        _pPublicKey = NULL;
        _pPrivateKey = NULL;
    }

    inline PublicKeyPair::PublicKeyPair (PublicKey *pPublicKey, PrivateKey *pPrivateKey)
    {
        if (pPublicKey) {
            _pPublicKey = new PublicKey (*pPublicKey);
        }
        if (pPrivateKey) {
            _pPrivateKey = new PrivateKey (*pPrivateKey);
        }
    }

    inline int PublicKeyPair::setPublicKey (PublicKey *pKey)
    {
        if (pKey) {
            _pPublicKey = pKey;
            return 0;
        }
        return -1;
    }

    inline PublicKey * PublicKeyPair::getPublicKey (void)
    {
        return _pPublicKey;
    }

    inline int PublicKeyPair::setPrivateKey (PrivateKey *pKey)
    {
        if (pKey) {
            _pPrivateKey = pKey;
            return 0;
        }

        return -1;
    }

    inline PrivateKey * PublicKeyPair::getPrivateKey (void)
    {
        return _pPrivateKey;
    }

    inline SecretKey::SecretKey (void)
    {
        _pKey = NULL;
    }

    inline void * SecretKey::getKey (void)
    {
        return _pKey;
    }

}

#endif   // #ifndef INCL_CRYPTO_UTILS_H
