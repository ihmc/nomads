// CMockets.h
// C API wrapper for Mockets, for use with P/Invoke in .NET
// jk, 11/2008

#ifndef INCL_MOCKETS_C_WRAPPER_H
#define INCL_MOCKETS_C_WRAPPER_H

#ifdef WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif

#ifdef __cplusplus
    extern "C"
    {
#endif

#define MOCKET_DEFAULT_TIMEOUT_MSEC 30000

#ifndef CMOCKET_BUILD
    typedef struct t_MocketCtx
    {
        void *pPlaceHolder;
    } MocketCtx;

    typedef struct t_MocketServerCtx
    {
        void *pPlaceHolder;
    } MocketServerCtx;

    typedef struct t_MocketStatsCtx
    {
        void *pPlaceHolder;
    } MocketStatsCtx;

#endif

#ifdef DEFINE_INTTYPES
    typedef char               int8;
    typedef unsigned char     uint8;
    typedef short             int16;
    typedef unsigned short   uint16;
    typedef int               int32;
    typedef unsigned int     uint32;

#if defined (WIN32)
    typedef __int64             int64;
    typedef unsigned __int64   uint64;
#elif defined (UNIX)
    typedef long long int       int64;
    typedef unsigned long long uint64;
#else
    struct int64
    {
        int32 lowWord;
        int32 highWord;
    };
#endif
#endif

    // ****************************
    // **************************** client stuff
    // ****************************

    EXPORT MocketCtx * MocketCreate();
    EXPORT void MocketDestroy (MocketCtx *ctx);

    EXPORT int MocketBind (MocketCtx *ctx, const char* pszBindAddr, uint16 ui16BindPort);
    EXPORT int MocketConnect (MocketCtx *ctx, const char *pszRemoteHost, uint16 ui16RemotePort);
    EXPORT int MocketConnectEx (MocketCtx *ctx, const char *pszRemoteHost, uint16 ui16RemotePort, int64 i64Timeout);
    EXPORT uint32 MocketGetLocalAddress (MocketCtx *ctx);
    EXPORT uint16 MocketGetLocalPort (MocketCtx *ctx);
    EXPORT int MocketIsConnected (MocketCtx *ctx);
    EXPORT int MocketClose (MocketCtx *ctx);
    EXPORT int MocketEnableCrossSequencing (MocketCtx *ctx, int bEnable);
    EXPORT int MocketSend (MocketCtx *ctx, int bReliable, int bSequenced, const void *pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
                           uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);
    EXPORT int MocketReceive (MocketCtx *ctx, void *pBuf, uint32 ui32BufSize );
    EXPORT int MocketReceiveEx (MocketCtx *ctx, void *pBuf, uint32 ui32BufSize, int64 i64Timeout );
    EXPORT int MocketReceiveAlloc (MocketCtx *ctx, void **ppBuf);
    EXPORT int MocketReceiveAllocEx (MocketCtx *ctx, void **ppBuf, int64 i64Timeout);
    EXPORT int MocketGetNextMessageSize (MocketCtx *ctx);
    EXPORT int MocketGetNextMessageSizeEx (MocketCtx *ctx, int64 i64Timeout);
    // sreceive not wrapped for now, not really sure how to handle it
    EXPORT int MocketReplace (MocketCtx *ctx, int bReliable, int bSequenced, const void *pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag, uint8 ui8Priority,
                              uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);
    EXPORT int MocketCancel (MocketCtx *ctx, int bReliable, int bSequenced, uint16 ui16TagId);
    EXPORT uint16 MocketGetMTU (MocketCtx *ctx);
    EXPORT uint32 MocketGetRemoteAddress (MocketCtx *ctx);
    EXPORT uint16 MocketGetRemotePort (MocketCtx *ctx);
    EXPORT void MocketSetIdentifier (MocketCtx *ctx, const char *pszIdentifier);
    EXPORT const char * MocketGetIdentifier (MocketCtx *ctx);
    EXPORT MocketStatsCtx * MocketGetStatistics (MocketCtx *ctx);

    // timeout is implemented in the C wrapper using the peerUnreachableWarningCallback mechanism
    // provided by mockets
    EXPORT void MocketSetTimeOut (MocketCtx *ctx, uint32 ui32TimeoutMsec);
    EXPORT uint32 MocketGetTimeOut (MocketCtx *ctx);

    // ****************************
    // **************************** server stuff	
    // ****************************

    EXPORT MocketServerCtx * MocketServerCreate();
    EXPORT void MocketServerDestroy (MocketServerCtx *ctx);
    EXPORT int MocketServerListen (MocketServerCtx *ctx, uint16 ui16Port);
    EXPORT int MocketServerListenEx (MocketServerCtx *ctx, uint16 ui16Port, const char *pszListenAddr);
    EXPORT MocketCtx * MocketServerAccept (MocketServerCtx *ctx);
    EXPORT int MocketServerClose (MocketServerCtx *ctx);

    // ****************************
    // **************************** statistics stuff
    // ****************************

    EXPORT uint32 MocketStatsGetRetransmitCount (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetSentPacketCount (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetSentByteCount (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetReceivedPacketCount (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetReceivedByteCount (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetDuplicatedDiscardedPacketCount (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetNoRoomDiscardedPacketCount (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetReassemblySkippedDiscardedPacketCount (MocketStatsCtx *ctx);
    EXPORT float MocketStatsGetEstimatedRTT (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetPendingDataSize (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetPendingPacketQueueSize (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetReliableSequencedDataSize (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetReliableSequencedPacketQueueSize (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetReliableUnsequencedDataSize (MocketStatsCtx *ctx);
    EXPORT uint32 MocketStatsGetReliableUnsequencedPacketQueueSize (MocketStatsCtx *ctx);
    EXPORT uint16 MocketStatsGetHighestTag (MocketStatsCtx *ctx);
    EXPORT int MocketStatsIsTagUsed (MocketStatsCtx *ctx, uint16 ui16Tag);

#ifdef __cplusplus
    }
#endif

#endif   // #ifndef INCL_MOCKETS_C_WRAPPER_H
