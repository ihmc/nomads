/*
 * CryptoUtils.cpp
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

#include "CryptoUtils.h"

#include <memory.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#ifdef WIN32
    #include <winsock2.h>
#endif

#include "openssl/aes.h"
#include "openssl/bio.h"
#include "openssl/des.h"
#include "openssl/evp.h"
#include "openssl/rsa.h"
#include "openssl/x509.h"
#include "openssl/sha.h"

#include "Base64Transcoders.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "security/SecureReader.h"
#include "security/SecureWriter.h"

using namespace NOMADSUtil;
using namespace CryptoUtils;

MD5Hash::MD5Hash (void)
{
    _pszHash = NULL;
    _pMDCTX = malloc (sizeof (EVP_MD_CTX));
    EVP_MD_CTX_init ((EVP_MD_CTX*)_pMDCTX);
    EVP_DigestInit_ex ((EVP_MD_CTX*)_pMDCTX, EVP_md5(), NULL);
    _bComputed = false;
}

MD5Hash::~MD5Hash (void)
{
    delete[] _pszHash;
    EVP_MD_CTX_cleanup ((EVP_MD_CTX*)_pMDCTX);
    free (_pMDCTX);
    _pMDCTX = NULL;
}

int MD5Hash::reinit (void)
{
    delete[] _pszHash;
    _pszHash = NULL;
    EVP_DigestInit_ex ((EVP_MD_CTX*)_pMDCTX, EVP_md5(), NULL);
    _bComputed = false;
    return 0;
}

int MD5Hash::update (const void *pBuf, unsigned long ulSize)
{
    EVP_DigestUpdate ((EVP_MD_CTX*)_pMDCTX, pBuf, ulSize);
    return 0;
}

const char * MD5Hash::getHashAsBase64String (void)
{
    if (_bComputed) {
        return _pszHash;
    }
    else {
        unsigned char auchMDValue [EVP_MAX_MD_SIZE];
        unsigned int uiMDLen;
        EVP_DigestFinal_ex ((EVP_MD_CTX*)_pMDCTX, auchMDValue, &uiMDLen);
        _pszHash = Base64Transcoders::encode (auchMDValue, uiMDLen);
        _bComputed = true;
        return _pszHash;
    }
}

/*
 *generate a sha256digest
 *This funcrion will be used to generate the AES256 key from
 *an input password
*/
void * CryptoUtils::sha256(const char *pszPassword) {

	unsigned char * hash = NULL;
	hash = (unsigned char *)malloc(sizeof(unsigned char *) * SHA256_DIGEST_LENGTH);
	memset(hash, 0, sizeof(hash));

	SHA256_CTX sha256;

	SHA256_Init(&sha256);

	SHA256_Update(&sha256, pszPassword, strlen(pszPassword) +1);

	SHA256_Final(hash, &sha256);

	return (void *)hash;
}

Key::KeyData::KeyData (Type type)
{
    _type = type;
    _pData = NULL;
    _uiLen = 0;
}

Key::KeyData::KeyData (Type type, const char *pData, unsigned int uiLen)
{
    _type = type;
    if ((pData == NULL) || (uiLen == 0)) {
        _pData = NULL;
        _uiLen = 0;
    }
    _pData = new char [uiLen];
    memcpy (_pData, pData, uiLen);
    _uiLen = uiLen;
}

Key::KeyData::KeyData (const KeyData &kd)
{
    _type = kd._type;
    if ((kd._pData == NULL) || (kd._uiLen == 0)) {
        _pData = NULL;
        _uiLen = 0;
    }
    _pData = new char [kd._uiLen];
    memcpy (_pData, kd._pData, kd._uiLen);
    _uiLen = kd._uiLen;
}

Key::KeyData::~KeyData (void)
{
    delete[] _pData;
    _uiLen = 0;
}

void Key::KeyData::setData (const char *pData, unsigned int uiLen)
{
    delete[] _pData;
    _uiLen = 0;
    if ((pData) && (uiLen > 0)) {
        _pData = new char [uiLen];
        memcpy (_pData, pData, uiLen);
        _uiLen = uiLen;
    }
}

Key::KeyData * Key::convertBIOToKeyData (KeyData::Type type, void *pBIO)
{
    int len = BIO_number_written ((BIO*)pBIO);
    if (len <= 0) {
        return NULL;
    }

    char *pBuf = new char [len];
    if (len != BIO_read ((BIO*)pBIO, pBuf, len)) {
        delete[] pBuf;
        return NULL;
    }

    KeyData *pKeyData = new KeyData (type, pBuf, (unsigned int) len);
    delete[] pBuf;

    return pKeyData;
}

Key::KeyData * Key::readKeyDataFromFile (KeyData::Type type, const char *pszFileName)
{
    struct stat statBuf;
    if (stat (pszFileName, &statBuf)) {
        return NULL;
    }
    if (statBuf.st_size <= 0) {
        return NULL;
    }

    FILE *pFile = fopen (pszFileName, "rb");
    if (pFile == NULL) {
        return NULL;
    }

    char *pBuf = new char [statBuf.st_size];

    if (fread (pBuf, statBuf.st_size, 1, pFile) != 1) {
        fclose (pFile);
        delete[] pBuf;
        return NULL;
    }

    KeyData *pKD = new KeyData (type, pBuf, statBuf.st_size);
    delete[] pBuf;
    return pKD;
}

int Key::writeKeyDataToFile (KeyData *pKD, const char *pszFileName)
{
    FILE *pFile = fopen (pszFileName, "wb");
    if (pFile == NULL) {
        return -1;
    }

    if (fwrite (pKD->getData(), pKD->getLength(), 1, pFile) != 1) {
        fclose (pFile);
        return -2;
    }

    fclose (pFile);
    return 0;
}

PublicKey::PublicKey (const PublicKey &key)
{
    if (key._pKey) {
        _pKey = RSAPublicKey_dup ((RSA*)key._pKey);
    }
    else {
        _pKey = NULL;
    }
}

PublicKey::~PublicKey (void)
{
    if (_pKey) {
        RSA_free ((RSA*)_pKey);
        _pKey = NULL;
    }
}

PublicKey & PublicKey::operator = (const PublicKey &rhsKey)
{
    if (_pKey) {
        RSA_free ((RSA*)_pKey);
        _pKey = NULL;
    }
    if (rhsKey._pKey) {
        _pKey = RSAPublicKey_dup ((RSA*)rhsKey._pKey);
    }
    return (*this);
}

PublicKey::KeyData * PublicKey::getKeyAsDEREncodedX509Data (void)
{
    EVP_PKEY *pEvpPKey = EVP_PKEY_new();
    if (pEvpPKey == NULL) {
        return NULL;
    }
    if (EVP_PKEY_set1_RSA (pEvpPKey, (RSA*)_pKey) <= 0) {
        EVP_PKEY_free (pEvpPKey);
        return NULL;
    }
    BIO *pBIO = BIO_new (BIO_s_mem());
    if (pBIO == NULL) {
        EVP_PKEY_free (pEvpPKey);
        return NULL;
    }
    if (i2d_PUBKEY_bio (pBIO, pEvpPKey) <= 0) {
        EVP_PKEY_free (pEvpPKey);
        BIO_free (pBIO);
        return NULL;
    }
    EVP_PKEY_free (pEvpPKey);

    KeyData *pKeyData = convertBIOToKeyData (KeyData::X509, pBIO);
    BIO_free (pBIO);

    return pKeyData;
}

int PublicKey::setKeyFromDEREncodedX509Data (KeyData *pKD)
{
    BIO *pBIO = BIO_new (BIO_s_mem());
    if (pBIO == NULL) {
        return -1;
    }
    if (BIO_write (pBIO, pKD->getData(), pKD->getLength()) != pKD->getLength()) {
        BIO_free (pBIO);
        return -2;
    }

    EVP_PKEY *pEvpPKey = d2i_PUBKEY_bio (pBIO, NULL);
    BIO_free (pBIO);

    if (pEvpPKey == NULL) {
        return -3;
    }

    if (_pKey) {
        RSA_free ((RSA*)_pKey);
        _pKey = NULL;
    }

    _pKey = EVP_PKEY_get1_RSA (pEvpPKey);
    EVP_PKEY_free (pEvpPKey);

    if (_pKey == NULL) {
        return -4;
    }

    return 0;
}

int PublicKey::storeKeyAsDEREncodedX509Data (const char *pszFileName)
{
    KeyData *pKD = getKeyAsDEREncodedX509Data();
    if (pKD == NULL) {
        return -1;
    }
    if (writeKeyDataToFile (pKD, pszFileName)) {
        delete pKD;
        return -2;
    }
    delete pKD;
    return 0;
}

int PublicKey::loadKeyFromDEREncodedX509Data (const char *pszFileName)
{
    KeyData *pKD = readKeyDataFromFile (KeyData::X509, pszFileName);
    if (pKD == NULL) {
        return -1;
    }
    if (setKeyFromDEREncodedX509Data (pKD)) {
        delete pKD;
        return -2;
    }
    delete pKD;
    return 0;
}

PrivateKey::PrivateKey (const PrivateKey &key)
{
    if (key._pKey) {
        _pKey = RSAPrivateKey_dup ((RSA*)key._pKey);
    }
    else {
        _pKey = NULL;
    }
}

PrivateKey::~PrivateKey (void)
{
    if (_pKey) {
        RSA_free ((RSA*)_pKey);
        _pKey = NULL;
    }
}

PrivateKey & PrivateKey::operator = (const PrivateKey &rhsKey)
{
    if (_pKey) {
        RSA_free ((RSA*)_pKey);
        _pKey = NULL;
    }
    if (rhsKey._pKey) {
        _pKey = RSAPrivateKey_dup ((RSA*)rhsKey._pKey);
    }
    return (*this);
}

PrivateKey::KeyData * PrivateKey::getKeyAsDEREncodedPKCS8Data (void)
{
    EVP_PKEY *pEvpPKey = EVP_PKEY_new();
    if (pEvpPKey == NULL) {
        return NULL;
    }
    if (EVP_PKEY_set1_RSA (pEvpPKey, (RSA*)_pKey) <= 0) {
        EVP_PKEY_free (pEvpPKey);
        return NULL;
    }
    BIO *pBIO = BIO_new (BIO_s_mem());
    if (pBIO == NULL) {
        EVP_PKEY_free (pEvpPKey);
        return NULL;
    }
    if (i2d_PKCS8PrivateKeyInfo_bio (pBIO, pEvpPKey) <= 0) {
        EVP_PKEY_free (pEvpPKey);
        BIO_free (pBIO);
        return NULL;
    }
    EVP_PKEY_free (pEvpPKey);

    KeyData *pKeyData = convertBIOToKeyData (KeyData::PKCS8, pBIO);
    BIO_free (pBIO);

    return pKeyData;
}

int PrivateKey::setKeyFromDEREncodedPKCS8Data (KeyData *pKD)
{
    BIO *pBIO = BIO_new (BIO_s_mem());
    if (pBIO == NULL) {
        return -1;
    }
    if (BIO_write (pBIO, pKD->getData(), pKD->getLength()) != pKD->getLength()) {
        BIO_free (pBIO);
        return -2;
    }

    PKCS8_PRIV_KEY_INFO *pPKCS8Info = d2i_PKCS8_PRIV_KEY_INFO_bio (pBIO, NULL);
    if (pPKCS8Info == NULL) {
        BIO_free (pBIO);
        return -3;
    }
    BIO_free (pBIO);

    EVP_PKEY *pEvpPKey = EVP_PKCS82PKEY (pPKCS8Info);
    if (pEvpPKey == NULL) {
        return -4;
    }
    PKCS8_PRIV_KEY_INFO_free (pPKCS8Info);

    if (_pKey) {
        RSA_free ((RSA*)_pKey);
        _pKey = NULL;
    }

    _pKey = EVP_PKEY_get1_RSA (pEvpPKey);
    EVP_PKEY_free (pEvpPKey);

    if (_pKey == NULL) {
        return -5;
    }

    return 0;
}

int PrivateKey::storeKeyAsDEREncodedPKCS8Data (const char *pszFileName)
{
    KeyData *pKD = getKeyAsDEREncodedPKCS8Data();
    if (pKD == NULL) {
        return -1;
    }
    if (writeKeyDataToFile (pKD, pszFileName)) {
        delete pKD;
        return -2;
    }
    delete pKD;
    return 0;
}

int PrivateKey::loadKeyFromDEREncodedPKCS8Data (const char *pszFileName)
{
    KeyData *pKD = readKeyDataFromFile (KeyData::PKCS8, pszFileName);
    if (pKD == NULL) {
        return -1;
    }
    if (setKeyFromDEREncodedPKCS8Data (pKD)) {
        delete pKD;
        return -2;
    }
    delete pKD;
    return 0;
}

PublicKeyPair::~PublicKeyPair (void)
{
    delete _pPublicKey;
    _pPublicKey = NULL;
    delete _pPrivateKey;
    _pPrivateKey = NULL;
}

// RSA_generate_key is a DEPRECATED METHOD, new version is RSA_generate_key_ex
int PublicKeyPair::generateNewKeyPair (unsigned short usKeyLength)
{	
	#if !defined (ANDROID)
    	RSA *pKey = RSA_generate_key (usKeyLength, RSA_F4, NULL, NULL);
	#else
		BIGNUM *oBigNbr = BN_new();
		BN_set_word(oBigNbr, RSA_F4);
		RSA *pKey = RSA_new();
		RSA_generate_key_ex(pKey, usKeyLength, oBigNbr, NULL);
	#endif
    if (pKey == NULL) {
        return -1;
    }

    if (_pPublicKey) {
        delete _pPublicKey;
        _pPublicKey = NULL;
    }
    if (_pPrivateKey) {
        delete _pPrivateKey;
        _pPrivateKey = NULL;
    }

    void *pRSAPublicKey = RSAPublicKey_dup (pKey);
    void *pRSAPrivateKey = RSAPrivateKey_dup (pKey);
    RSA_free (pKey);
    if ((pRSAPublicKey == NULL) || (pRSAPrivateKey == NULL)) {
        return -2;
    }

    _pPublicKey = new PublicKey (pRSAPublicKey);
    _pPrivateKey = new PrivateKey (pRSAPrivateKey);

    return 0;
}

AES256Key::AES256Key(const AES256Key &key)
{
    /*
     * The size of an AES256 is 32 byte
    */
    if (key._pKey) {
        _pKey = malloc (sKeySize);
        memcpy(_pKey, key._pKey, sKeySize);
    }
    else {
        _pKey = NULL;
    }
}

AES256Key & AES256Key::operator = (const AES256Key &rhsKey)
{
    if (_pKey) {
        free(_pKey);
        _pKey = NULL;
    }
    if (rhsKey._pKey) {
        _pKey = malloc(sKeySize);
        memcpy(_pKey, rhsKey._pKey, sKeySize);
    }
    return (*this);
}

AES256Key::~AES256Key(void)
{
    if (_pKey) {
        free(_pKey);
        _pKey = NULL;
    }
}

int AES256Key::initKey(const char *pszPassword)
{
    if (pszPassword == NULL) {
        return -1;
    }
    _pKey = sha256 (pszPassword);
    return 0;
}

int AES256Key::initKey (unsigned char *pchKey, uint32 ui32len)
{
	if (pchKey == NULL) {
		return -1;
	}
	if (ui32len != sKeySize) {
		return -2;
	}
    _pKey = (unsigned char*)malloc (sKeySize);
    memcpy (_pKey, pchKey, sKeySize);
    return 0;
} 

int AES256Key::initKeyFromFile (const char *pszFileName)
{
    FILE *pFile = fopen(pszFileName, "rb");
    if (pFile == NULL) {
        return -1;
    }
    unsigned char* pKeyData = (unsigned char*)malloc (sKeySize);
    if (fread (pKeyData, sKeySize, 1, pFile) != 1) {
        fclose (pFile);
        free (pKeyData);
        pKeyData = NULL;
        return -2;
    }
    _pKey = pKeyData;
    return 0;
}

SecretKey::SecretKey (const SecretKey &key)
{
    if (key._pKey) {
        _pKey = malloc (sizeof (DES_key_schedule));
        memcpy (_pKey, key._pKey, sizeof (DES_key_schedule));
    }
    else {
        _pKey = NULL;
    }
}

SecretKey & SecretKey::operator = (const SecretKey &rhsKey)
{
    if (_pKey) {
        free (_pKey);
        _pKey = NULL;
    }
    if (rhsKey._pKey) {
        _pKey = malloc (sizeof (DES_key_schedule));
        memcpy (_pKey, rhsKey._pKey, sizeof (DES_key_schedule));
    }
    return (*this);
}

SecretKey::~SecretKey (void)
{
    if (_pKey) {
        free (_pKey);
        _pKey = NULL;
    }
}

int SecretKey::initKey (const char *pszPassword)
{
    _pKey = malloc (sizeof (DES_key_schedule));
    unsigned char auchPassword[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int iPassLen = (int) strlen (pszPassword);
    if (iPassLen > 8) {
        iPassLen = 8;
    }
    for (int i = 0; i < iPassLen; i++) {
        auchPassword[i] = pszPassword[i];
    }
    DES_set_key_unchecked ((DES_cblock*)auchPassword, (DES_key_schedule*)_pKey);
    return 0;
}

int CryptoUtils::encryptDataUsingSecretKey (SecretKeyInterface *pKey, const void *pData, uint32 ui32DataLen, void *pDestBuf, uint32 ui32BufSize)
{
	//the effective encryption depend on the type of the key
	switch (pKey->getKeyType()) {
	case SecretKeyInterface::DES:
		{
			BufferWriter bw;
			SecureWriter sw((SecretKey *)pKey, &bw, false);
			if (sw.writeBytes(pData, ui32DataLen)) {
				return -1;
			}
			sw.flush();
			if (ui32BufSize < bw.getBufferLength()) {
				return -2;
			}
			memcpy(pDestBuf, bw.getBuffer(), bw.getBufferLength());
			return (int)bw.getBufferLength();
		}
	case SecretKeyInterface::AES256:
		{
			EVP_CIPHER_CTX * pctx = NULL;
			if (!(pctx = EVP_CIPHER_CTX_new())) {
				return -1;
			}
			int iLen = 0;
			int iEncryptedMsgLen;
			//From the man of the EVP function. This is the maximum size
			pDestBuf = (void *)malloc(ui32DataLen + AES_BLOCK_SIZE - 1);
			// The last parameter is NULL beacuse the encryption is performed in ecb MODE

			if (EVP_EncryptInit_ex(pctx, EVP_aes_256_cfb(), NULL, (const unsigned char *)pKey->getKey(), NULL) != 1) {
				return -2;
			}

			if (EVP_EncryptUpdate(pctx, (unsigned char *)pDestBuf, &iLen, (unsigned char *)pData, ui32DataLen) != 1) {
				return -3;
			}
			iEncryptedMsgLen = iLen;

			if (EVP_EncryptFinal_ex(pctx, (unsigned char *)pDestBuf + iLen, &iLen) != 1) {
				return -4;
			}
			iEncryptedMsgLen += iLen;
			EVP_CIPHER_CTX_cleanup(pctx);
			pctx = NULL;
			return iEncryptedMsgLen;
		}
		//the KeyType is not defined. The encrytpion cannot be performed
		default:
			return -6;
	}
}

void * CryptoUtils::encryptDataUsingSecretKey (SecretKeyInterface *pKey, const void *pData, uint32 ui32DataLen, uint32 *pui32BufLen)
{
	switch (pKey->getKeyType()) {
		case SecretKeyInterface::DES:
		{
			BufferWriter bw;
			SecureWriter sw((SecretKey *)pKey, &bw, false);
			if (sw.writeBytes(pData, ui32DataLen)) {
				return NULL;
			}
			sw.flush();
			*pui32BufLen = bw.getBufferLength();
			return bw.relinquishBuffer();
		}
		case SecretKeyInterface::AES256:
		{
			EVP_CIPHER_CTX * pctx = NULL;
			if (!(pctx = EVP_CIPHER_CTX_new())) {
				return NULL;
			}
			int iLen = 0;
			void *pDestBuf = NULL;
			//From the man of the EVP function. This is the maximum size
			pDestBuf = (void *)malloc(ui32DataLen + AES_BLOCK_SIZE - 1);
			/*
			* The last parameter is NULL beacuse the encryption is performed in ecb MODE, no padding
			*/
			if (EVP_EncryptInit_ex(pctx, EVP_aes_256_cfb(), NULL, (const unsigned char *)pKey->getKey(), NULL) != 1) {
				return NULL;
			}

			if (EVP_EncryptUpdate(pctx, (unsigned char *)pDestBuf, &iLen, (unsigned char *)pData, ui32DataLen) != 1) {
				return NULL;
			}
			*pui32BufLen = iLen;

			if (EVP_EncryptFinal_ex(pctx, (unsigned char *)pDestBuf + iLen, &iLen) != 1) {
				return NULL;
			}
			*pui32BufLen += iLen;
			EVP_CIPHER_CTX_cleanup(pctx);
			pctx = NULL;
			return pDestBuf;
		}
		//the KeyType is not defined. The encrytpion cannot be performed
		default:
			return NULL;
	}
}

int CryptoUtils::decryptDataUsingSecretKey (SecretKeyInterface *pKey, const void *pData, uint32 ui32DataLen, void *pDestBuf, uint32 ui32BufSize)
{
	switch (pKey->getKeyType()) {
		case SecretKeyInterface::DES:
		{
			BufferReader br(pData, ui32DataLen);
			SecureReader sr((SecretKey *)pKey, &br, false);
			BufferWriter bw;
			while (true) {
				char buf[128];
				int iBytesRead = sr.read(buf, sizeof(buf));
				if (iBytesRead <= 0) {
					break;
				}
				bw.writeBytes(buf, iBytesRead);
			}
			if (ui32BufSize < bw.getBufferLength()) {
				return -1;
			}
			memcpy(pDestBuf, bw.getBuffer(), bw.getBufferLength());
			return (int)bw.getBufferLength();
		 }
		case SecretKeyInterface::AES256:
		{
			EVP_CIPHER_CTX * pctx = NULL;
			if (!(pctx = EVP_CIPHER_CTX_new())) {
				return -1;
			}
			int iLen = 0;
			int iDecryptedMsgLen;
			//From the man of the EVP function. This is the maximum size
			pDestBuf = (void *)malloc(ui32DataLen + AES_BLOCK_SIZE);
			// The last parameter is NULL beacuse the encryption is performed in ecb MODE

			if (EVP_DecryptInit_ex(pctx, EVP_aes_256_cfb(), NULL, (const unsigned char *)pKey->getKey(), NULL) != 1) {
				return -2;
			}

			if (EVP_DecryptUpdate(pctx, (unsigned char *)pDestBuf, &iLen, (unsigned char *)pData, ui32DataLen) != 1) {
				return -3;
			}
			iDecryptedMsgLen = iLen;

			if (EVP_DecryptFinal_ex(pctx, (unsigned char *)pDestBuf + iLen, &iLen) != 1) {
				//reset the messages if the decrypt fails
				iDecryptedMsgLen = ui32DataLen;
				if (pDestBuf != NULL) {
					free(pDestBuf);
				}
				pDestBuf = (void *)malloc(ui32DataLen);				
				memcpy(pDestBuf, pData, ui32DataLen);
				EVP_CIPHER_CTX_cleanup(pctx);
				return ui32DataLen;
			}
			iDecryptedMsgLen += iLen;
			EVP_CIPHER_CTX_cleanup(pctx);
			pctx = NULL;
			return iDecryptedMsgLen;
		}
		//the KeyType is not defined. The encrytpion cannot be performed
		default:
			return -5;
	} 
}

void * CryptoUtils::decryptDataUsingSecretKey (SecretKeyInterface *pKey, const void *pData, uint32 ui32DataLen, uint32 *pui32BufLen)
{
	switch (pKey->getKeyType()) {
		case SecretKeyInterface::DES:
		{
			BufferReader br(pData, ui32DataLen);
			SecureReader sr((SecretKey *)pKey, &br, false);
			BufferWriter bw;
			while (true) {
				char buf[128];
				int iBytesRead = sr.read(buf, sizeof(buf));
				if (iBytesRead <= 0) {
					break;
				}
				bw.writeBytes(buf, iBytesRead);
			}
			*pui32BufLen = bw.getBufferLength();
			return bw.relinquishBuffer();
		}
		case SecretKeyInterface::AES256:
		{
			EVP_CIPHER_CTX * pctx = NULL;
			if (!(pctx = EVP_CIPHER_CTX_new())) {
				return NULL;
			}
			int iLen = 0;
			void *pDestBuf = NULL;
			//From the manual of the EVP function. This is the maximum size
			pDestBuf = (void *)malloc(ui32DataLen + AES_BLOCK_SIZE);
			/*
			* The last parameter is NULL beacuse the encryption is performed in ecb MODE
			*/
			if (EVP_DecryptInit_ex(pctx, EVP_aes_256_cfb(), NULL, (const unsigned char *)pKey->getKey(), NULL) != 1) {
				return NULL;
			}

			if (EVP_DecryptUpdate(pctx, (unsigned char *)pDestBuf, &iLen, (unsigned char *)pData, ui32DataLen) != 1) {
				return NULL;
			}
			*pui32BufLen = iLen;

			if (EVP_DecryptFinal_ex(pctx, (unsigned char *)pDestBuf + iLen, &iLen) != 1) {
				//if the decrypt fails just return the messages as it was
				*pui32BufLen = ui32DataLen;
				if (pDestBuf != NULL) {
					free(pDestBuf);
				}
				pDestBuf = (void *)malloc(ui32DataLen);				
				memcpy(pDestBuf, pData, ui32DataLen);
				EVP_CIPHER_CTX_cleanup(pctx);
				return pDestBuf; 
			}
			*pui32BufLen += iLen;
			EVP_CIPHER_CTX_cleanup(pctx);
			pctx = NULL;
			return pDestBuf;
		}
		//the KeyType is not define. The encrytpion cannot be performed
		default:
			return NULL;
	}
    
}

int CryptoUtils::encryptDataUsingPublicKey (PublicKey *pKey, const void *pData, uint32 ui32DataLen, void *pDestBuf, uint32 ui32BufSize)
{
    BufferWriter bw;
    SecureWriter sw (pKey, &bw, false);
    if (sw.writeBytes (pData, ui32DataLen)) {
        return -1;
    }
    sw.flush();
    if (ui32BufSize < bw.getBufferLength()) {
        return -2;
    }
    memcpy (pDestBuf, bw.getBuffer(), bw.getBufferLength());
    return (int) bw.getBufferLength();
}

void * CryptoUtils::encryptDataUsingPublicKey (PublicKey *pKey, const void *pData, uint32 ui32DataLen, uint32 *pui32BufLen)
{
    BufferWriter bw;
    SecureWriter sw (pKey, &bw, false);
    if (sw.writeBytes (pData, ui32DataLen)) {
        return NULL;
    }
    sw.flush();
    *pui32BufLen = bw.getBufferLength();
    return bw.relinquishBuffer();
}

int CryptoUtils::decryptDataUsingPrivateKey (PrivateKey *pKey, const void *pData, uint32 ui32DataLen, void *pDestBuf, uint32 ui32BufSize)
{
    BufferReader br (pData, ui32DataLen);
    SecureReader sr (pKey, &br, false);
    BufferWriter bw;
    while (true) {
        char buf[128];
        int iBytesRead = sr.read (buf, sizeof (buf));
        if (iBytesRead <= 0) {
            break;
        }
        bw.writeBytes (buf, iBytesRead);
    }
    if (ui32BufSize < bw.getBufferLength()) {
        return -1;
    }
    memcpy (pDestBuf, bw.getBuffer(), bw.getBufferLength());
    return (int) bw.getBufferLength();
}

void * CryptoUtils::decryptDataUsingPrivateKey (PrivateKey *pKey, const void *pData, uint32 ui32DataLen, uint32 *pui32BufLen)
{
    BufferReader br (pData, ui32DataLen);
    SecureReader sr (pKey, &br, false);
    BufferWriter bw;
    while (true) {
        char buf[128];
        int iBytesRead = sr.read (buf, sizeof (buf));
        if (iBytesRead <= 0) {
            break;
        }
        bw.writeBytes (buf, iBytesRead);
    }
    *pui32BufLen = bw.getBufferLength();
    return bw.relinquishBuffer();
}

