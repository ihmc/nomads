/*
* Dtls.cpp
* Author: rfronteddu@ihmc.us
* This file is part of the IHMC Mockets Library/Component
* Copyright (c) 2002-2014 IHMC.
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

#include "Dtls.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
using namespace NOMADSUtil;

namespace IHMC_DTLS
{
	bool DtlsEndpoint::fileExists(const char *pszFilePath)
	{
		if (FILE *pFile = fopen(pszFilePath, "r")) {
            fclose(pFile);
            return true;
        }
		else {
            checkAndLogMsg("DtlsEndpoint::fileExists", 
                Logger::L_SevereError, "Can't find: %s\n", pszFilePath);
            return false;
        }
	}

	int DtlsEndpoint::commonInitializationCTX(void)
	{
		int error;
		dtls_fields* commonHandler;
		int(*no_client_certificate)(int ok, X509_STORE_CTX* ctx) = noverify;
		commonHandler = new dtls_fields();

        if (getIsServer()) {
            _pServer = commonHandler;
            commonHandler->ctx = SSL_CTX_new(DTLSv1_2_server_method());
        }
		else {
            _pClient = commonHandler;
            commonHandler->ctx = SSL_CTX_new(DTLSv1_2_client_method());
        }

        if (!commonHandler->ctx) {
            checkAndLogMsg("DtlsEndpoint::commonInitializationCTX", Logger::L_SevereError, "ctx is NULL?\n");
            return -1;
        }
		if (SSL_CTX_set_cipher_list(commonHandler->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH") != 1) {
            checkAndLogMsg("DtlsEndpoint::commonInitializationCTX", Logger::L_SevereError, "SSL_CTX_set_cipher_list failed?\n");
            return -2;
        }
        //do not verify the client certificate for now
        SSL_CTX_set_verify(commonHandler->ctx, SSL_VERIFY_PEER, no_client_certificate);
        /*
        srtp has bee patched in the latest version of openssl
		bool useSRTP = false;
		if (useSRTP) {
			if (SSL_CTX_set_tlsext_use_srtp(commonHandler->ctx, "SRTP_AES128_CM_SHA1_80") != 0) {
				//cannot setup srtp
				return -4;
			}
		}
		*/

        if (loadCertificateAndPrivateKey(commonHandler, _cpPathToCertificate, _cpPathToPrivateKey) < 0) {  return -3; }

        if (SSL_CTX_check_private_key(commonHandler->ctx) != 1) {
            checkAndLogMsg("DtlsEndpoint::commonInitializationCTX", Logger::L_SevereError, "SSL_CTX_check_private_key failed?\n");
            return -4;
        }
        getIsServer() ? (sprintf(commonHandler->name, "+ %s", "server")) : (sprintf(commonHandler->name, "+ %s", "client"));
		return 0;
	}

    int DtlsEndpoint::loadCertificateAndPrivateKey(
        dtls_fields *pCommonHandler, 
        const char  *pszCertificate, 
        const char  *pszKey)
    {
        const char *pszMethodName = "DtlsEndpoint::loadCertificateAndPrivateKey";
        if (pszCertificate == NULL) {
            getIsServer() ? 
                (sprintf(_cCertfile, "%s-cert.pem", "server")) : 
                (sprintf(_cCertfile, "%s-cert.pem", "client")); 
        }
        else { 
            sprintf(_cCertfile, "%s", pszCertificate);
        }

        if (pszKey == NULL) { 
            getIsServer() ? 
                (sprintf(_cKeyfile, "%s-key.pem", "server")) : 
                (sprintf(_cKeyfile, "%s-key.pem", "client")); 
        }
        else { 
            sprintf(_cKeyfile, "%s", pszKey);
        }

        if (!fileExists(_cCertfile)) {
            return -1;   
        }

        if (!fileExists(_cKeyfile)) { 
            return -2;    
        }

        checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, 
            "Paths: Certificate: %s, Key: %s\n", _cCertfile, _cKeyfile);

        if (SSL_CTX_use_certificate_file(pCommonHandler->ctx, _cCertfile, SSL_FILETYPE_PEM) != 1) {
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "SSL_CTX_use_certificate_file failed?\n");
            return -3;
        }
        if (SSL_CTX_use_PrivateKey_file(pCommonHandler->ctx, _cKeyfile, SSL_FILETYPE_PEM) != 1) {
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "SSL_CTX_use_PrivateKey_file failed?\n");
            return -4;
        }
        return 0;
    }

	int DtlsEndpoint::commonInitializationSSL(void)
	{
        const char *pszMethodName = "DtlsEndpoint::commonInitializationSSL";
			dtls_fields *pCommonHandler;
			getIsServer() ? pCommonHandler = _pServer : pCommonHandler = _pClient;
            if (pCommonHandler == NULL) {
                checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                    "Ctx object is null?");
                return -1;
            }

            if (!(pCommonHandler->ssl = SSL_new(pCommonHandler->ctx))) {
                checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                    "Couldn't create new SSL, not enough memory?");
                return -2;
            }

            if (!(pCommonHandler->in_bio = BIO_new(BIO_s_mem()))) {
                checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                    "Couldn't create input BIO, not enough memory?");
                return -3;
            }

			// sets the behaviour of memory BIO when empty.
			BIO_set_mem_eof_return(pCommonHandler->in_bio, -1);

            if (!(pCommonHandler->out_bio = BIO_new(BIO_s_mem()))) {
                checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                    "Couldn't create output BIO, not enough memory?");
                return -4;
            }

			// sets the behaviour of memory BIO when empty.
			BIO_set_mem_eof_return(pCommonHandler->out_bio, -1);

			//set SSL to use input and output memory BIO
			SSL_set_bio(
                pCommonHandler->ssl, 
                pCommonHandler->in_bio, 
                pCommonHandler->out_bio);

			getIsServer() ? 
                SSL_set_accept_state(pCommonHandler->ssl) : 
                SSL_set_connect_state(pCommonHandler->ssl);
		return 0;
	}

	DtlsEndpoint::DtlsEndpoint(
        const char *pszPathToCertificate, 
        const char *pszPathToPrivateKey)
	{
		//Initialize OpenSSL
		SSL_load_error_strings();
		SSL_library_init();
		OpenSSL_add_all_algorithms();
		ERR_load_BIO_strings();
		_bHandShakeStarted = false;

		_pClient = NULL;
		_pServer = NULL;
		_bHandShakeFinished = 0;
		_iMtu = 3000; //this should be changed and not used to transfer the certificates

        pszPathToCertificate ? 
            (_cpPathToCertificate = strdup(pszPathToCertificate)) : 
            (_cpPathToCertificate = NULL);
        pszPathToPrivateKey ? 
            (_cpPathToPrivateKey = strdup(pszPathToPrivateKey)) : 
            (_cpPathToPrivateKey = NULL);
	}

	DtlsEndpoint::~DtlsEndpoint(void)
	{
		if(_bIsServer) {
			handlerCleanup(_pServer);
            delete _pServer;
		}
		else {
			handlerCleanup(_pClient);
            delete _pClient;
		}

		ERR_remove_state(0);
		ENGINE_cleanup();
		CONF_modules_unload(1);
		ERR_free_strings();
		EVP_cleanup();
		//this should be called but there is a problem which causes a double free I need to investigate.
		//sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
		CRYPTO_cleanup_all_ex_data();

        free(_cpPathToCertificate);
        free(_cpPathToPrivateKey);
	}

    int DtlsEndpoint::getEncryptedBuffer(dtls_fields *pConnection, char *pBuf)
    {
        int written = 0;
        int read = 0;
        int pending = BIO_ctrl_pending(pConnection->out_bio);
        if (pending > 0) {
            //write the content of the BIO in outbuf
            read = BIO_read(pConnection->out_bio, pBuf, _iMtu);
        }
        return read;
    }

    void DtlsEndpoint::handlerCleanup(dtls_fields *pHandler)
    {
        if (pHandler != NULL) {
            if (pHandler->ctx != NULL) {
                SSL_CTX_free(pHandler->ctx);
                pHandler->ctx = NULL;
            }
            if (pHandler->ssl != NULL) {
                SSL_free(pHandler->ssl);
                pHandler->ssl = NULL;
            }
        }
    }

	int DtlsEndpoint::handShake(char **ppMsg)
	{
        const char *pszMethodName = "DtlsEndpoint::handShake";
		int rc = 0;
		dtls_fields *pConnection;
		(getIsServer()) ? 
            (pConnection = _pServer) : 
            (pConnection = _pClient);

		if (getIsServer()) {
			checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Server performing handshake\n");
			return getEncryptedBuffer(pConnection, *ppMsg);
		}
		else {
			checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Client performing handshake\n");
			if (!handshakeIsStarted()) {
				rc = SSL_do_handshake(pConnection->ssl);
				if (rc == 0) {
					//Connection was not successful but was shut-down controlled
					checkAndLogMsg(pszMethodName, Logger::L_Warning,
						"Connection was not successful but was shut-down controlled\n");
					return -1;
				}

                if (rc == 1 || rc == -1) {
                    return getEncryptedBuffer(pConnection, *ppMsg);
                }             

				//do_handshake returns a negative number both if it 
                // needs to continue or if it gets an error, use SSL_get_error() 
                // to know what happened
                return -2;
			}
			//return getEncryptedBuffer(pConnection, *ppMsg);
		}
	}

    void DtlsEndpoint::init(bool bIsServer)
    {
        const char *pszMethodName = "DtlsEndpoint::init";
        int rc;
        setIsServer(bIsServer);
        if ((rc = commonInitializationCTX()) < 0) {
            checkAndLogMsg(pszMethodName, Logger::L_MildError, 
                "CTX Initialization failed!\n");
            exit(rc);
        }
        if ((rc = commonInitializationSSL()) < 0) {
            checkAndLogMsg(pszMethodName, Logger::L_MildError, 
                "SSL Initialization failed!\n");
            exit(rc);
        }
    }

	bool DtlsEndpoint::isHandshakeOver(void)
	{
		dtls_fields *pConnection;
		(getIsServer()) ? (pConnection = _pServer) : (pConnection = _pClient);	
        return SSL_is_init_finished(pConnection->ssl);
	}

    int verify_callback(int ok, X509_STORE_CTX *pCtx) // make this part of object
    {
        return 1;
    }

    int DtlsEndpoint::noverify(int ok, X509_STORE_CTX *pCtx)
    {
        /*
        const char* server_name = "123.456.789.323";
        const char* server_port = "1234";
        const char* remoteAddress = "123.456.789.323:1234";

        long res = 1;
        SSL_CTX* pctx    = NULL;
        BIO*     pbioWeb = NULL;
        BIO*     pbioOut = NULL;
        SSL*     psslSsl = NULL;

        const SSL_METHOD* psslmMethod = SSLv23_method();
        if (psslmMethod == NULL) {
            printf("ERROR method\n");
        }
        if (pctx == NULL) {
            printf("ERROR ctx\n");
        }

        SSL_CTX_set_verify(pctx, SSL_VERIFY_PEER, verify_callback);
        SSL_CTX_set_verify_depth(pctx, 4);

        const long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
            SSL_OP_NO_COMPRESSION;

        SSL_CTX_set_options(pctx, flags);
        res = SSL_CTX_load_verify_locations(pctx, "random-org-chain.pem", NULL);
        if (res != 1) {
            printf("Can't find certificate\n");
        }

        pbioWeb = BIO_new_ssl_connect(pctx);
        if (pbioWeb == NULL) {
            printf("pbioweb error\n");
        }

        res = BIO_set_conn_hostname(pbioWeb, remoteAddress);
        if (res != 1) {
            printf("Connection to remote Address failed\n");
        }

        BIO_get_ssl(pbioWeb, &psslSsl);
        if (psslSsl == NULL) {
            printf("Failed bio get ssl\n");
        }

        const char* const PREFERRED_CIPHERS = "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4";
        res = SSL_set_cipher_list(psslSsl, PREFERRED_CIPHERS);
        if (res != 1) {
            printf("Failed SS; set cipher\n");
        }

        res = SSL_set_tlsext_host_name(psslSsl, server_name);
        if (res != 1) {
            printf("Failed set_tlsext_host_name\n");
        }

        pbioOut = BIO_new_fp(stdout, BIO_NOCLOSE);
        if (pbioOut == NULL) {
            printf("Failed pbioOut\n");
        }
        res = BIO_do_connect(pbioWeb);
        if (res != 1) {
            printf("do connect failed\n");
        }

        res = BIO_do_handshake(pbioWeb);
        if (res != 1) {
            printf("BIO_do_handshake failed\n");
        }

        // Step 1: verify a server certificate was presented during the negotiation
        X509* cert = SSL_get_peer_certificate(psslSsl);
        if (cert) { X509_free(cert); } /* Free immediately
        if (cert == NULL) {
            printf("cert problem\n");
        }

        // Step 2: verify the result of chain verification
        // Verification performed according to RFC 4158
        res = SSL_get_verify_result(psslSsl);
        if (X509_V_OK != res) {
            printf("Verification failed");
        }
        else {
            printf("Success\n");
        }

        // Step 3: hostname verification
        //missing for now

        int len = 0;
        char buff[1536] = {};

        //do filippo protocol
        do
        {
            char buff[1536] = {};
            len = BIO_read(pbioWeb, buff, sizeof(buff));

            if (len > 0)
                BIO_write(pbioOut, buff, len);

        } while (len > 0 || BIO_should_retry(pbioWeb));

        if (pbioOut)
            BIO_free(pbioOut);

        if (pbioWeb != NULL)
            BIO_free_all(pbioWeb);

        if (NULL != ctx)
            SSL_CTX_free(pctx);
        */
        return 1;
    }

	int DtlsEndpoint::prepareDataForSending(
        const char *pMsg, 
        int msgSize, 
        char **ppEncrypted)
	{
		dtls_fields* connection;
		(getIsServer()) ? (connection = _pServer) : (connection = _pClient);
        int read = SSL_write(connection->ssl, pMsg, msgSize);
		//*ppEncrypted = new char[read + 38];
        *ppEncrypted = new char[3000];
		read = getEncryptedBuffer(connection, *ppEncrypted);
		return read;
	}

	int DtlsEndpoint::recoverData(
        const char *pEncrypted, 
        int enSize, 
        char **ppDecrypted)
	{
		dtls_fields *pConnection;
		(getIsServer()) ? (pConnection = _pServer) : (pConnection = _pClient);

		int written = 0;
		int read = 0;
		if (enSize > 0) {
			written = BIO_write(pConnection->in_bio, pEncrypted, enSize);
		}
		if (enSize > 0) {
			if (!SSL_is_init_finished(pConnection->ssl)) {
				//continue the handshake
				SSL_do_handshake(pConnection->ssl);
				return 0;
			}
			else {
				//decript the data from the out bio and copy it in out buf
				*ppDecrypted = new char[written];
				read = SSL_read(pConnection->ssl, *ppDecrypted, written);
				return read;
			}
		}
		return -1;
	}

}