/**
 * Filippo Poltronieri <fpoltronieri@ihmc.us>
 * ServerMocketWrapper.cpp MocketWrapper for Golang
 *
 */
#include <stdint.h>
#include "Mocket.h"
#include "ServerMocket.h"
#include "ServerMocketWrapper.h"


CServerMocket ServerMocketInit (const char* pszconfigFile)
{
	ServerMocket *psMocket = new ServerMocket(pszconfigFile);
	return (void*)psMocket;
}

CServerMocket ServerMocketInitDtls (const char *pszConfigFile, const char *pCertificatePath, const char *pPrivateKeyPath)
{
    	ServerMocket *psMocket = new ServerMocket(pszConfigFile, NULL, false, true, pCertificatePath, pPrivateKeyPath);
    	return (void*)psMocket;
}

int ServerMocketListen (CServerMocket pcSMocket, uint16_t ui16Port)
{
    ServerMocket *psmocket = (ServerMocket*)pcSMocket;
    if (psmocket == NULL) {
        printf("ServerMocketWrapper::ServerMocketListen psmocket is NULL, error!\n");
        return -24;
    }
    if (ui16Port < 0) {
        printf("ServerMocketWrapper::ServerMocketListen psmocket is < 0, error\n");
        return -25;
    }
    int rc = psmocket->listen((uint16)ui16Port);
    if (rc < 0) {
        printf("ServerMocketWrapper::ServerMocketListen listen(), error\n");
    }
    return rc;
}

int ServerMocketListenAddress (CServerMocket pcSMocket, uint16_t ui16Port, const char *pszListenAddr) {
    ServerMocket *psmocket = (ServerMocket*)pcSMocket;
        if (psmocket == NULL) {
            printf("ServerMocketWrapper::ServerMocketListenAddress psmocket is NULL, error!\n");
            return -26;
        }
        if (ui16Port < 0) {
            printf("ServerMocketWrapper::ServerMocketListenAddress psmocket is < 0, error\n");
            return -27;
        }
        if (pszListenAddr == NULL) {
            printf("ServerMocketWrapper::ServerMocketListenAddress pszListenAddr is NULL, error!\n");
            return -28;
        }
        int rc = psmocket->listen((uint16)ui16Port, pszListenAddr);
        if (rc < 0) {
            printf("ServerMocketWrapper::ServerMocketListen listen(), error\n");
        }
        return rc;
}

CMocket ServerMocketAccept (CServerMocket psmocket)
{
    ServerMocket *pcsmocket = (ServerMocket*)psmocket;
    if (pcsmocket == NULL) {
        printf("ServerMocketWrapper::ServerMocketAccept psmocket is NULL\n");
        return NULL;
    }
    return (void *)pcsmocket->accept();
}

CMocket ServerMocketAcceptPort (CServerMocket psmocket, uint16_t ui16PortForNewConnection)
{
    ServerMocket *pcsmocket = (ServerMocket*)psmocket;
    if (pcsmocket == NULL) {
        printf("ServerMocketWrapper::ServerMocketAcceptPort psmocket is NULL\n");
        return NULL;
    }
    if (ui16PortForNewConnection < 0) {
        printf("ServerMocketWrapper::ServerMocketAcceptPort ui16PortForNewConnection is not a valid port\n");
        return NULL;
    }
    return (void *)pcsmocket->accept((uint16)ui16PortForNewConnection);
}


int ServerMocketClose (CServerMocket pcmocket)
{
	ServerMocket *psmocket = (ServerMocket*)pcmocket;
	if (psmocket == NULL) {
		printf("ServerMocketWrapper::ServerMocketClose pcmocket is NULL, error!\n");
		return -23;
	}
	int rc = psmocket->close();
	if (rc < 0) {
		printf("ServerMocketWrapper::ServerMocketClose error calling pmocket->close()\n");
	}
	delete psmocket;
	return rc;
}



		





