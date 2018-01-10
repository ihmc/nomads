/*
 * MocketWrapper.h 
 * Mocket Wrapper for Golang
 *
 */
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
    typedef void* CMocket;
    CMocket MocketInit (const char *pszConfigFile);
    CMocket MocketInitDtls (const char *pszConfigFile, const char *pCertificatePath, const char *pPrivateKeyPath);
    int MocketConnectNative (CMocket pcmocket, const char* premoteHost, int iremotePort);
    int MocketConnectTimeout (CMocket pcmocket, const char* premoteHost, uint16_t iremotePort, int64_t i64Timeout);
    int MocketSend (CMocket pcmocket, int8_t bReliable, int8_t bSequenced, const void *pBuf, uint32_t ui32BufSize, uint16_t ui16Tag, uint8_t ui8Priority,
                  uint32_t ui32EnqueueTimeout, uint32_t ui32RetryTimeout);
    int MocketBind (CMocket pcmocket, const char *pszBindAddr, uint16_t ui16BindPort);
    void MocketUseTwoWayHandshake (CMocket pcmocket);
    int MocketReceive (CMocket pcmocket, void *pBuf, uint32_t ui32BufSize, int64_t i64Timeout);
    int MocketClose (CMocket pcmocket);
    void MocketSetIdentifier (CMocket pcmocket, const char *pszIdentifier);
    const char * MocketGetIdentifier (CMocket pcmocket);
    uint32_t MocketGetRemoteAddress (CMocket pcmocket);
    uint16_t MocketGetRemotePort (CMocket pcmocket);
    uint32_t MocketGetLocalAddress (CMocket pcmocket);
    uint16_t MocketGetLocalPort (CMocket pcmocket);
#ifdef __cplusplus
}
#endif

