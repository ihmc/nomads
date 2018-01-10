/**
 * MocketWrapper.cpp MocketWrapper for Golang
 *
 */
#include <stdint.h>
#include "Mocket.h"
#include "MocketWrapper.h"


CMocket MocketInit (const char* pszconfigFile)
{
	Mocket *pMocket = new Mocket(pszconfigFile);
	return (void*)pMocket;
}

CMocket MocketInitDtls (const char *pszConfigFile, const char *pCertificatePath, const char *pPrivateKeyPath)
{
    	Mocket *pMocket = new Mocket(pszConfigFile, NULL, false, true, pCertificatePath, pPrivateKeyPath);
    	return (void*)pMocket;
}


int MocketBind (CMocket pcmocket, const char *pszBindAddr, uint16_t ui16BindPort)
{
	Mocket *pmocket = (Mocket *)pcmocket;
	if (ui16BindPort < 0) {
		printf("MocketWrapper::MocketBind ui16BindPort < 0, error!\n");
		return -20;
	}
	if (pmocket == NULL) {
		printf("MocketWrapper::MocketBind mocket is NULL, error!\n");
		return -21;
	}
	if (pszBindAddr == NULL) {
		printf("MocketWrapper::MocketBind pszBindAddr is NULL, error\n");
		return -23;
	}
	int rc = pmocket->bind (pszBindAddr, (uint16) ui16BindPort);
	if (rc < 0) {
		printf("MocketWrapper:: error during bind()\n");
	}
	return rc;	
}

int MocketConnectNative (CMocket pcmocket, const char* premoteHost, int iremotePort)
{
	Mocket *pmocket = (Mocket *)pcmocket;
	if (iremotePort < 0) {
		printf("MocketWrapper::MocketConnectNative iremotePort < 0, error!\n");
		return -20;
	}
	if (pmocket == NULL) {
		printf("MocketWrapper::MocketConnectNative mocket is NULL, error!\n");
		return -21;
	}
	if (premoteHost == NULL) {
		printf("MocketWrapper::MocketConnectNative premoteHost is NULL, error!\n");
		return -22;
	}
	int rc = pmocket->connect (premoteHost, (uint16) iremotePort);
	if (rc < 0) {
		printf("MocketWrapper::MocketConnectNative connection failed, error!\n");
	}
	return rc;
}


int MocketConnectTimeout (CMocket pcmocket, const char* premoteHost, int iremotePort, int64_t i64Timeout) 
{
	Mocket *pmocket = (Mocket *)pcmocket;
	if (iremotePort < 0) {
		printf("MocketWrapper::MocketConnectTimeout iremotePort < 0, error!\n");
		return -23;
	}
	if (pmocket == NULL) {
		printf("MocketWrapper::MocketConnectTimeout mocket is NULL, error!\n");
		return -24;
	}
	if (premoteHost == NULL) {	
		printf("MocketWrapper::MocketConnectTimeout premoteHost is NULL, error!\n");
		return -25;
	}
	int rc = pmocket->connect (premoteHost, (uint16) iremotePort, (int64) i64Timeout);
	if (rc < 0) {
		printf("MocketWrapper::MocketConnectTimeout connection failed, error!\n");
	}
	return rc;
	
}

int MocketSend (CMocket pcmocket, int8_t bReliable, int8_t bSequenced, const void *pBuf, uint32_t ui32BufSize, uint16_t ui16Tag, uint8_t ui8Priority,
                  uint32_t ui32EnqueueTimeout, uint32_t ui32RetryTimeout)
                  {
	Mocket *pmocket = (Mocket *)pcmocket;
	if (pmocket == NULL) {
		printf("MocketWrapper::send pMocket is NULL\n");
		return -26;
	}
	if (pBuf == NULL) {
		printf("MocketWrapper::send pBuf is NULL\n");
		return -27;
	}
	int rc = pmocket->send((bool) bReliable, (bool) bSequenced, pBuf, (uint32) ui32BufSize, (uint16) ui16Tag, (uint8) ui8Priority, (uint32) ui32EnqueueTimeout, (uint32) ui32RetryTimeout);
	if (rc < 0) {
		printf("MocketWrapper::send send is failed, error!\n");
	}
	return rc;
	
}

int MocketReceive (CMocket pcmocket, void *pBuf, uint32_t ui32BufSize, int64_t i64Timeout) 
{
	Mocket *pmocket = (Mocket *)pcmocket;
	if (pmocket == NULL) {
		printf("MocketWrapper::send pMocket is NULL\n");
		return -28;
	}
	if (pBuf == NULL) {
		printf("MocketWrapper::send pBuf is NULL\n");
		return -29;
	}
	int rc = pmocket->receive (pBuf, (uint32) ui32BufSize, (int64) i64Timeout);
	if (rc < 0) {
		printf("MocketWrapper::receive Error in receive, error!\n");
	}
	return rc;
}

void MocketSetIdentifier (CMocket pcmocket, const char *pszIdentifier)
{
	Mocket *pmocket = (Mocket *)pcmocket;
	if (pmocket == NULL) {
		printf("MocketWrapper::MocketSetIdentifier pmocket is NULL\n");
		return;
	}
	pmocket->setIdentifier(pszIdentifier);
}

const char * MocketGetIdentifier (CMocket pcmocket)
{
	Mocket *pmocket = (Mocket *)pcmocket;
	if (pmocket == NULL) {
		printf("MocketWrapper::MocketGetIdentifier pmocket is NULL\n");
		return NULL;
	}
	return pmocket->getIdentifier();
}

uint32_t MocketGetRemoteAddress (CMocket pcmocket)
{
    Mocket *pmocket = (Mocket *)pcmocket;
    if (pmocket == NULL) {
        printf("MocketWrapper::MocketGetRemoteAddress pmocket is NULL\n");
                return NULL;
    }
    return (uint32_t)pmocket->getRemoteAddress();
}

uint16_t MocketGetRemotePort (CMocket pcmocket)
{
    Mocket *pmocket = (Mocket *)pcmocket;
    if (pmocket == NULL) {
        printf("MocketWrapper::MocketGetRemotePort pmocket is NULL\n");
                return NULL;
    }
    return (uint16_t)pmocket->getRemotePort();
}


uint32_t MocketGetLocalAddress (CMocket pcmocket)
{
    Mocket *pmocket = (Mocket *)pcmocket;
    if (pmocket == NULL) {
        printf("MocketWrapper::MocketGetLocalAddress pmocket is NULL\n");
                return NULL;
    }
    return (uint32_t)pmocket->getLocalAddress();
}

uint16_t MocketGetLocalPort (CMocket pcmocket)
{
    Mocket *pmocket = (Mocket *)pcmocket;
    if (pmocket == NULL) {
        printf("MocketWrapper::MocketGetLocalPort pmocket is NULL\n");
                return NULL;
    }
    return (uint16_t)pmocket->getLocalPort();
}

void MocketUseTwoWayHandshake (CMocket pcmocket)
{
	Mocket *pmocket = (Mocket *)pcmocket;
	if (pmocket == NULL) {
		printf("MocketWrapper::useTwoWayHandshake pMocket is NULL\n");
		return;
	}
	pmocket->useTwoWayHandshake();
}


int MocketClose (CMocket pcmocket) 
{
	Mocket *pmocket = (Mocket*)pcmocket;
	if (pmocket == NULL) {
		printf("MocketWrapper::MocketClose pmocket is NULL, error!\n");
		return -23;
	}
	int rc = pmocket->close();
	if (rc < 0) {
		printf("MocketWrapper::MocketClose error calling pmocket->close()\n");
	}
	delete pmocket;
	return rc;
}


		





