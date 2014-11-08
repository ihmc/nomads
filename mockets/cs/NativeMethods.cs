using System;
using System.Runtime.InteropServices;


namespace us.ihmc.mockets
{
    internal static class NativeMethods
    {
        // ****************************
        // **************************** client stuff
        // ****************************

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr MocketCreate();

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void MocketDestroy(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketBind(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)]string pszBindAddr, UInt16 ui16BindPort);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketConnect(IntPtr ctx,
               [MarshalAs(UnmanagedType.LPStr)]string pszRemoteHost, UInt16 ui16RemotePort);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketConnectEx(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)]string pszRemoteHost, UInt16 ui16RemotePort, Int64 i64Timeout);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketGetLocalAddress(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt16 MocketGetLocalPort(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketIsConnected(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketClose(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketEnableCrossSequencing(IntPtr ctx, int bEnable);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketSend(IntPtr ctx, int bReliable, int bSequenced,
            [MarshalAs(UnmanagedType.LPArray)]byte[] pBuf, UInt32 ui32BufSize, UInt16 ui16Tag, byte ui8Priority,
            UInt32 ui32EnqueueTimeout, UInt32 ui32RetryTimeout);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketReceive(IntPtr ctx,
            [MarshalAs(UnmanagedType.LPArray)]byte[] pBuf, UInt32 ui32BufSize);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketReceiveEx(IntPtr ctx, [MarshalAs(UnmanagedType.LPArray)]byte[] pBuf,
            UInt32 ui32BufSize, Int64 i64Timeout);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketGetNextMessageSize(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketGetNextMessageSizeEx(IntPtr ctx, Int64 i64Timeout);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketReplace(IntPtr ctx, int bReliable, int bSequenced,
            [MarshalAs(UnmanagedType.LPArray)]byte[] pBuf, UInt32 ui32BufSize, UInt16 ui16OldTag, UInt16 ui16NewTag,
            byte ui8Priority, UInt32 ui32EnqueueTimeout, UInt32 ui32RetryTimeout);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketCancel(IntPtr ctx, int bReliable, int bSequenced, UInt16 ui16TagId);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt16 MocketGetMTU(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketGetRemoteAddress(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt16 MocketGetRemotePort(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void MocketSetIdentifier(IntPtr ctx, [MarshalAs(UnmanagedType.LPStr)]string pszIdentifier);

        // does this work right????? not sure how the return argument will get handled. the function returns a const char *
        // pointer to a char array somewhere inside the mockets class
        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        public static extern string MocketGetIdentifier(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr MocketGetStatistics (IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void MocketSetTimeOut(IntPtr ctx, UInt32 ui32TimeoutMsec);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketGetTimeOut(IntPtr ctx);

        // ****************************
        // **************************** server stuff	
        // ****************************

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr MocketServerCreate();

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void MocketServerDestroy(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketServerListen(IntPtr ctx, UInt16 ui16Port);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketServerListenEx(IntPtr ctx, UInt16 ui16Port, [MarshalAs(UnmanagedType.LPStr)]string pszListenAddr);

        // this returns an IntPtr to a MocketCtx, NOT a MocketServerCtx
        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr MocketServerAccept(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketServerClose(IntPtr ctx);

        // ****************************
        // **************************** statistics stuff
        // ****************************

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetRetransmitCount(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetSentPacketCount(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetSentByteCount(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetReceivedPacketCount(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetReceivedByteCount(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetDuplicatedDiscardedPacketCount(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetNoRoomDiscardedPacketCount(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetReassemblySkippedDiscardedPacketCount(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern float MocketStatsGetEstimatedRTT(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetPendingDataSize(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetPendingPacketQueueSize(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetReliableSequencedDataSize(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetReliableSequencedPacketQueueSize(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetReliableUnsequencedDataSize(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt32 MocketStatsGetReliableUnsequencedPacketQueueSize(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt16 MocketStatsGetHighestTag(IntPtr ctx);

        [DllImport("MocketsCWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int MocketStatsIsTagUsed(IntPtr ctx, UInt16 ui16Tag);
    }
}
