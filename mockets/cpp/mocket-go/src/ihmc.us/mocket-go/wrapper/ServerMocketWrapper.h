/*
 * Filippo Poltronieri <fpoltronieri@ihmc.us>
 * ServerMocketWrapper.h
 * Mocket Wrapper for Golang
 *
 */
#include <stdint.h>
#include "MocketWrapper.h"
#ifdef __cplusplus
extern "C" {
#endif
    typedef void* CServerMocket;
    CServerMocket ServerMocketInit (const char *pszConfigFile);
    CServerMocket ServerMocketInitDtls (const char *pszConfigFile, const char *pCertificatePath, const char *pPrivateKeyPath);
    int ServerMocketListen (CServerMocket pcSMocket, uint16_t ui16Port);
    int ServerMocketListenAddress (CServerMocket pcSMocket, uint16_t ui16Port, const char *pszListenAddr);
    CMocket ServerMocketAccept (CServerMocket psmocket);
    CMocket ServerMocketAcceptPort (CServerMocket psmocket, uint16_t ui16PortForNewConnection);
    int ServerMocketClose (CServerMocket pcmocket);
#ifdef __cplusplus
}
#endif

