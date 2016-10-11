// CMockets.cpp
// C API wrapper for Mockets, for use with P/Invoke in .NET
// jk, 11/2008

#define CMOCKET_BUILD

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Mocket.h"
#include "ServerMocket.h"
#include "Logger.h"

typedef struct t_MocketCtx
{
    Mocket *m;
    uint32 timeoutMsec;
} MocketCtx;

typedef struct t_MocketServerCtx
{
    ServerMocket *s;
} MocketServerCtx;

#define MocketStatsCtx MocketStats

#include "MocketsCWrapper.h"

// Peer unreachable timeout handler callback
// Return true to close the mocket
bool MocketPeerUnreachableHandler (void *pCallbackArg, unsigned long ulTimeSinceLastContact)
{
    MocketCtx *pCtx = (MocketCtx *) pCallbackArg;
    if (ulTimeSinceLastContact > pCtx->timeoutMsec) {
        return true;
    }
    return false;
}

MocketCtx * MocketCreate()
{
#ifdef ENABLE_MOCKETS_LOGGING
    if (NOMADSUtil::pLogger == 0)
        {
        NOMADSUtil::pLogger = new NOMADSUtil::Logger();
        //NOMADSUtil::pLogger->enableScreenOutput();
		NOMADSUtil::pLogger->initLogFile("C:\\temp\\mockets.log", false);
		NOMADSUtil::pLogger->enableFileOutput();
        }
    
    NOMADSUtil::pLogger->setDebugLevel(NOMADSUtil::Logger::L_HighDetailDebug);
#endif

    MocketCtx *pCtx = (MocketCtx *) malloc (sizeof (MocketCtx));
    pCtx->m = new Mocket();
    pCtx->m->registerPeerUnreachableWarningCallback (&MocketPeerUnreachableHandler, pCtx);
    pCtx->timeoutMsec = MOCKET_DEFAULT_TIMEOUT_MSEC;
    return pCtx;
}

void MocketDestroy (MocketCtx *pCtx)
{
    delete pCtx->m;
    free (pCtx);
}

int MocketBind (MocketCtx *pCtx, const char* pszBindAddr, uint16 ui16BindPort)
{
    return pCtx->m->bind (pszBindAddr, ui16BindPort);
}

int MocketConnect (MocketCtx *pCtx, const char *pszRemoteHost, uint16 ui16RemotePort)
{
    return pCtx->m->connect (pszRemoteHost, ui16RemotePort);
}

int MocketConnectEx (MocketCtx *pCtx, const char *pszRemoteHost, uint16 ui16RemotePort, int64 i64Timeout)
{
    return pCtx->m->connect (pszRemoteHost, ui16RemotePort, i64Timeout);
}

uint32 MocketGetLocalAddress (MocketCtx *pCtx)
{
    return pCtx->m->getLocalAddress();
}

uint16 MocketGetLocalPort (MocketCtx *pCtx)
{
    return pCtx->m->getLocalPort();
}

int MocketIsConnected (MocketCtx *pCtx)
{
    return pCtx->m->isConnected();
}

int MocketClose (MocketCtx *pCtx)
{
    return pCtx->m->close();
}

int MocketEnableCrossSequencing (MocketCtx *pCtx, int bEnable)
{
    // Visual Studio will throw a warning if you pass an int into a bool directly
    return pCtx->m->enableCrossSequencing ((bEnable != 0));
}

int MocketSend (MocketCtx *pCtx, int bReliable, int bSequenced, const void *pBuf, uint32 ui32BufSize, 
                uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    // Visual Studio will throw a warning if you pass an int into a bool directly
    return pCtx->m->send ((bReliable != 0), (bSequenced != 0), pBuf, ui32BufSize, ui16Tag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
}

int MocketReceive (MocketCtx *pCtx, void *pBuf, uint32 ui32BufSize )
{
    return pCtx->m->receive (pBuf, ui32BufSize);
}

int MocketReceiveEx (MocketCtx *pCtx, void *pBuf, uint32 ui32BufSize, int64 i64Timeout )
{
    return pCtx->m->receive (pBuf, ui32BufSize, i64Timeout);
}

int MocketReceiveAlloc (MocketCtx *pCtx, void **ppBuf)
{
    return pCtx->m->receive (ppBuf);
}

int MocketReceiveAllocEx (MocketCtx *pCtx, void **ppBuf, int64 i64Timeout)
{
    return pCtx->m->receive (ppBuf, i64Timeout);
}

int MocketGetNextMessageSize (MocketCtx *pCtx)
{
    return pCtx->m->getNextMessageSize();
}

int MocketGetNextMessageSizeEx (MocketCtx *pCtx, int64 i64Timeout)
{
    return pCtx->m->getNextMessageSize (i64Timeout);
}

int MocketReplace (MocketCtx *pCtx, int bReliable, int bSequenced, const void *pBuf, uint32 ui32BufSize,
                   uint16 ui16OldTag, uint16 ui16NewTag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    return pCtx->m->replace ((bReliable != 0), (bSequenced != 0), pBuf, ui32BufSize,
                             ui16OldTag, ui16NewTag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
}

int MocketCancel (MocketCtx *pCtx, int bReliable, int bSequenced, uint16 ui16TagId)
{
    return pCtx->m->cancel ((bReliable != 0), (bSequenced != 0), ui16TagId);
}

uint16 MocketGetMTU (MocketCtx *pCtx)
{
    return pCtx->m->getMTU();
}

uint32 MocketGetRemoteAddress (MocketCtx *pCtx)
{
    return pCtx->m->getRemoteAddress();
}

uint16 MocketGetRemotePort (MocketCtx *pCtx)
{
    return pCtx->m->getRemotePort();
}

void MocketSetIdentifier (MocketCtx *pCtx, const char *pszIdentifier)
{
    pCtx->m->setIdentifier (pszIdentifier);
}

const char * MocketGetIdentifier (MocketCtx *pCtx)
{
    return pCtx->m->getIdentifier();
}

MocketStatsCtx * MocketGetStatistics (MocketCtx *pCtx)
{
    return pCtx->m->getStatistics();
}

// Timeout stuff implemented in the C wrapper, not in Mockets itself
void MocketSetTimeOut (MocketCtx *pCtx, uint32 ui32TimeoutMsec)
{
    pCtx->timeoutMsec = ui32TimeoutMsec;
}

uint32 MocketGetTimeOut (MocketCtx *pCtx)
{
    return pCtx->timeoutMsec;
}

// ****************************
// **************************** server stuff	
// ****************************

MocketServerCtx* MocketServerCreate()
{
    MocketServerCtx *pCtx = (MocketServerCtx *) malloc (sizeof (MocketServerCtx));
    pCtx->s = new ServerMocket();
    return pCtx;
}

void MocketServerDestroy (MocketServerCtx *pCtx)
{
    delete pCtx->s;
    free (pCtx);
}

int MocketServerListen (MocketServerCtx *pCtx, uint16 ui16Port)
{
    return pCtx->s->listen (ui16Port);
}

int MocketServerListenEx (MocketServerCtx *pCtx, uint16 ui16Port, const char *pszListenAddr)
{
    return pCtx->s->listen (ui16Port, pszListenAddr);
}

MocketCtx* MocketServerAccept (MocketServerCtx *pCtx)
{
    Mocket *m = pCtx->s->accept();
    if (m == 0) {
        return 0;
    }

    MocketCtx *pNewCtx = (MocketCtx *) malloc (sizeof (MocketCtx));
    pNewCtx->m = m;
    pNewCtx->m->registerPeerUnreachableWarningCallback (&MocketPeerUnreachableHandler, pNewCtx);
    pNewCtx->timeoutMsec = MOCKET_DEFAULT_TIMEOUT_MSEC;

    return pNewCtx;
}

int MocketServerClose (MocketServerCtx *pCtx)
{
    return pCtx->s->close();
}

// ****************************
// **************************** statistics stuff
// ****************************

uint32 MocketStatsGetRetransmitCount (MocketStatsCtx *pCtx)
{
    return pCtx->getRetransmitCount();
}

uint32 MocketStatsGetSentPacketCount (MocketStatsCtx *pCtx)
{
    return pCtx->getSentPacketCount();
}

uint32 MocketStatsGetSentByteCount (MocketStatsCtx *pCtx)
{
    return pCtx->getSentByteCount();
}

uint32 MocketStatsGetReceivedPacketCount (MocketStatsCtx *pCtx)
{
    return pCtx->getReceivedPacketCount();
}

uint32 MocketStatsGetReceivedByteCount (MocketStatsCtx *pCtx)
{
    return pCtx->getReceivedByteCount();
}

uint32 MocketStatsGetDuplicatedDiscardedPacketCount (MocketStatsCtx *pCtx)
{
    return pCtx->getDuplicatedDiscardedPacketCount();
}

uint32 MocketStatsGetNoRoomDiscardedPacketCount (MocketStatsCtx *pCtx)
{
    return pCtx->getNoRoomDiscardedPacketCount();
}

uint32 MocketStatsGetReassemblySkippedDiscardedPacketCount (MocketStatsCtx *pCtx)
{
    return pCtx->getReassemblySkippedDiscardedPacketCount();
}

float MocketStatsGetEstimatedRTT (MocketStatsCtx *pCtx)
{
    return pCtx->getEstimatedRTT();
}

uint32 MocketStatsGetPendingDataSize (MocketStatsCtx *pCtx)
{
    return pCtx->getPendingDataSize();
}

uint32 MocketStatsGetPendingPacketQueueSize (MocketStatsCtx *pCtx)
{
    return pCtx->getPendingPacketQueueSize();
}

uint32 MocketStatsGetReliableSequencedDataSize (MocketStatsCtx *pCtx)
{
    return pCtx->getReliableSequencedDataSize();
}

uint32 MocketStatsGetReliableSequencedPacketQueueSize (MocketStatsCtx *pCtx)
{
    return pCtx->getReliableSequencedPacketQueueSize();
}

uint32 MocketStatsGetReliableUnsequencedDataSize (MocketStatsCtx *pCtx)
{
    return pCtx->getReliableUnsequencedDataSize();
}

uint32 MocketStatsGetReliableUnsequencedPacketQueueSize (MocketStatsCtx *pCtx)
{
    return pCtx->getReliableUnsequencedPacketQueueSize();
}

uint16 MocketStatsGetHighestTag (MocketStatsCtx *pCtx)
{
    return pCtx->getHighestTag();
}

int MocketStatsIsTagUsed (MocketStatsCtx *pCtx, uint16 ui16Tag)
{
    return pCtx->isTagUsed (ui16Tag);
}
