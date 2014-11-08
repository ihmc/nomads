// ManagedMocket.cs
// P/Invoke wrapper for C++ mockets library
// jk, 11/2008

using System;

namespace us.ihmc.mockets
{
    /// <summary>
    /// The us.ihmc.mockets namespace contains the Mockets API. Mockets provides
    /// flexible, efficient, connection-oriented message transport services over unreliable tactical networks.
    /// Reliability and sequencing can be enabled or disabled on a per-message basis. Mockets also contains support
    /// for removing and replacing messages in the transmit queue.
    /// </summary>
    /// <remarks>
    /// Mockets uses UDP internally, so Mockets port numbers are really UDP port numbers.
    /// </remarks>
    [System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    class NamespaceDoc
    {
    }

    /// <summary>
    /// Allows applications to connect to a ServerMocket and send and receive messages.
    /// </summary>
    /// <remarks>
    /// This class uses P/Invoke to wrap a C++ Mocket object. 
    /// </remarks>
    public class ManagedMocket: IDisposable
    {
        /// <summary>
        /// Constructor for a ManagedMocket.
        /// </summary>
        public ManagedMocket()
        {
            ctx = NativeMethods.MocketCreate();
        }

        /// <summary>
        /// Constructor for use by ManagedMessageServerMocket when the mocket handle has already been allocated
        /// by accept
        /// </summary>
        /// <param name="mocketCtx">An unmanaged handle to a mocket that has already been allocated</param>
        internal ManagedMocket(IntPtr mocketCtx)
        {
            ctx = mocketCtx;
        }

        /// <summary>
        /// Destructor
        /// </summary>
        ~ManagedMocket()
        {
            Dispose(false);
        }

        /// <summary>
        /// Deallocates the unmanaged mocket handle.
        /// </summary>
        /// <inheritdoc />
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Dispose implementation.
        /// </summary>
        /// <param name="disposing">True if dispose was explicitly called, false otherwise.</param>
        protected virtual void Dispose(bool disposing)
        {
            // MS says you should be able to safely call dispose multiple times
            if (ctx != IntPtr.Zero)
            {
                NativeMethods.MocketDestroy(ctx);
            }
            ctx = IntPtr.Zero;
        }
        
        /// <summary>
        /// Binds the local end point to a particular address (interface) and port.
        /// Calls to this method will work if invoked before calling 'connect()'.
        /// Otherwise, it will return an error code. 
        /// </summary>
        /// <param name="pszBindAddress">The local IP address or hostname to bind the mocket to.</param>
        /// <param name="ui16BindPort">The local UDP port to bind to.</param>
        /// <returns>0 if success, negative if error.</returns>
        public int bind(string pszBindAddress, UInt16 ui16BindPort)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketBind(ctx, pszBindAddress, ui16BindPort);
        }

        /// <summary>
        /// Attempt to connect to a ServerMocket at the specified remote host on the specified remote port using the default
        /// connect timeout (30 seconds.)
        /// </summary>
        /// <param name="pszRemoteHost">Address of the remote server. The address may be a hostname that can be resoved to an
        /// IP address or an IP address in string format (e.g. "127.0.0.1").</param>
        /// <param name="ui16RemotePort">The remote UDP port the server is listening on.</param>
        /// <returns>Returns 0 if successful or a negative value in case of failure.</returns>
        public int connect(string pszRemoteHost, UInt16 ui16RemotePort)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketConnect(ctx, pszRemoteHost, ui16RemotePort);
        }

        /// <summary>
        /// Attempt to connect to a ServerMocket at the specified remote host on the specified remote port using the default
        /// connect timeout (30 seconds.)
        /// </summary>
        /// <param name="pRemoteHost">The IP address of the remote server.</param>
        /// <param name="ui16RemotePort">The remote UDP port the server is listening on.</param>
        /// <returns>Returns 0 if successful or a negative value in case of failure.</returns>
        public int connect(System.Net.IPAddress pRemoteHost, UInt16 ui16RemotePort)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketConnect(ctx, pRemoteHost.ToString(), ui16RemotePort);
        }

        /// <summary>
        /// Attempt to connect to a ServerMocket at the specified remote host on the specified remote port
        /// with a specified timeout.
        /// </summary>
        /// <param name="pszRemoteHost">Address of the remote server. The address may be a hostname that can be resoved to an
        /// IP address or an IP address in string format (e.g. "127.0.0.1").</param>
        /// <param name="ui16RemotePort">The remote UDP port the server is listening on.</param>
        /// <param name="i64Timeout">Connect timeout, in milliseconds.</param>
        /// <returns>0 if successful, negative value if error.</returns>
        public int connect(string pszRemoteHost, UInt16 ui16RemotePort, Int64 i64Timeout)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketConnectEx(ctx, pszRemoteHost, ui16RemotePort, i64Timeout);
        }

        /// <summary>
        /// Attempt to connect to a ServerMocket at the specified remote host on the specified remote port
        /// with a specified timeout.
        /// </summary>
        /// <param name="pRemoteHost">The IP address of the remote server.</param>
        /// <param name="ui16RemotePort">The remote UDP port the server is listening on.</param>
        /// <param name="i64Timeout">Connect timeout, in milliseconds.</param>
        /// <returns></returns>
        public int connect(System.Net.IPAddress pRemoteHost, UInt16 ui16RemotePort, Int64 i64Timeout)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketConnectEx(ctx, pRemoteHost.ToString(), ui16RemotePort, i64Timeout);
        }

        /// <summary>
        /// Check if the mocket is connected.
        /// </summary>
        /// <returns>Returns true if the mocket is currently connected.</returns>
        public bool isConnected()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            int rval = NativeMethods.MocketIsConnected(ctx);

            if (rval != 0)
                return true;
            return false;
        }

        /// <summary>
        /// Closes the current open connection to a remote endpoint. 
        /// </summary>
        /// <returns>Returns 0 if successful or a negative value in case of error.</returns>
        public int close()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketClose(ctx);
        }

        /// <summary>
        /// Return the remote host address to which the connection has been established.
        /// </summary>
        /// <returns>The remote host address to which the connection has been established.</returns>
        public System.Net.IPAddress getRemoteAddress()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            UInt32 addr = NativeMethods.MocketGetRemoteAddress(ctx);
            //Int64 longAddr = addr;

            return new System.Net.IPAddress(addr);
        }

        /// <summary>
        /// Return the remote UDP port to which the connection has been established.
        /// </summary>
        /// <returns>The remote UDP port to which the connection has been established.</returns>
        public UInt16 getRemotePort()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketGetRemotePort(ctx);
        }

        /// <summary>
        /// Return the address on the local host to which this connection is bound to.
        /// </summary>
        /// <returns>The address on the local host to which this connection is bound to.</returns>
        public System.Net.IPAddress getLocalAddress()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            UInt32 addr = NativeMethods.MocketGetLocalAddress(ctx);
            //Int64 longAddr = addr;

            return new System.Net.IPAddress(addr);
            
        }

        /// <summary>
        /// Return the UDP port on the local host to which this connection is bound to. 
        /// </summary>
        /// <returns>The UDP port on the local host to which this connection is bound to.</returns>
        public UInt16 getLocalPort()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketGetLocalPort(ctx);
        }

        /// <summary>
        /// Enables or disables cross sequencing across the reliable sequenced and unreliable sequenced packets.
        /// </summary>
        /// <param name="bEnable">True to enable cross sequencing, false to disable.</param>
        /// <returns>0 if successful, negative value if error.</returns>
        public int enableCrossSequencing(bool bEnable)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            int val = 0;
            if (bEnable) val = 1;

            return NativeMethods.MocketEnableCrossSequencing(ctx, val);
        }

        // Obtains a new sender for the specified combination of reliability and sequencing parameters
        // Not wrapped at this time
        //ManagedMessageSender getSender (bool bReliable, bool bSequenced);

        /// <summary>
        /// Enqueues the specified data for transmission using the specified reliability and sequencing requirements
        /// </summary>
        /// <param name="bReliable">If true, use reliable transport. If false, use unreliable transport.</param>
        /// <param name="bSequenced">If true, use sequenced transport. If false, use unsequenced transport.</param>
        /// <param name="pBuf">Byte array containing the data to send.</param>
        /// <param name="ui32BufSize">The number of bytes to send.</param>
        /// <param name="ui16Tag">Tag identifying the type of packet. Used for cancelling or replacing the packet later.</param>
        /// <param name="ui8Priority">Priority of the packet. (FIXME: What is highest?)</param>
        /// <param name="ui32EnqueueTimeout">The length of time in milliseconds for which the method will wait
        /// if there is no room in the outgoing buffer (a zero value indicates wait forever).</param>
        /// <param name="ui32RetryTimeout">Currently not used, but in the future could be used to specify the length of time
        /// for which the transmitter will retransmit the packet to ensure successful delivery.</param>
        /// <returns>Returns 0 if successful or a negative value in case of error.</returns>
        public int send(bool bReliable, bool bSequenced, byte[] pBuf, UInt32 ui32BufSize, UInt16 ui16Tag, byte ui8Priority,
            UInt32 ui32EnqueueTimeout, UInt32 ui32RetryTimeout)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            int rval = 0;
            if (bReliable) rval = 1;
            int sval = 0;
            if (bSequenced) sval = 1;

            return NativeMethods.MocketSend(ctx, rval, sval, pBuf, ui32BufSize, ui16Tag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
        }

        /// <summary>
        /// Retrieves the data from next packet that is ready to be delivered to the application.
        /// Uses the default timeout. 
        /// The buffer to hold the data must be allocated by the caller. The caller
        /// can call getNextMessageSize() to determine the size of the next message so that a buffer
        /// of the proper size can be allocated. Remember that Mockets messages can be of arbitrary length
        /// and are not limited to 64K.
        /// </summary>
        /// <param name="pBuf">The byte array to hold the data.</param>
        /// <param name="ui32BufSize">The number of bytes to copy into pBuf. Any additional data in the packet that
        /// will not fit in the buffer is discarded.</param>
        /// <returns>Returns the number of bytes that were copied into the buffer, 0 in case of the connection
        /// being closed, and -1 in case no data is available within the specified timeout.</returns>
        public int receive(byte[] pBuf, UInt32 ui32BufSize)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketReceive(ctx, pBuf, ui32BufSize);
        }

        /// <summary>
        /// Retrieves the data from next packet that is ready to be delivered to the application.
        /// The buffer to hold the data must be allocated by the caller. The caller
        /// can call getNextMessageSize() to determine the size of the next message so that a buffer
        /// of the proper size can be allocated. Remember that Mockets messages can be of arbitrary length
        /// and are not limited to 64K.
        /// </summary>
        /// <param name="pBuf">The byte array to hold the data.</param>
        /// <param name="ui32BufSize">The number of bytes to copy into pBuf. Any additional data in the packet that
        /// will not fit in the buffer is discarded.</param>
        /// <param name="i64Timeout">Timeout in milliseconds. Not specifiying a timeout or a timeout of 0 implies that the default timeout
        /// should be used. A timeout of -1 implies wait indefinitely</param>
        /// <returns>Returns the number of bytes that were copied into the buffer, 0 in case of the connection
        /// being closed, and -1 in case no data is available within the specified timeout.</returns>
        public int receive(byte[] pBuf, UInt32 ui32BufSize, Int64 i64Timeout)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketReceiveEx(ctx, pBuf, ui32BufSize, i64Timeout);
        }

        /// <summary>
        /// Returns the size of the next message that is ready to be delivered to the application.
        /// If there are no messages in the incoming queue, this call will block until a message arrives
        /// or the default timeout expires.
        /// </summary>
        /// <returns>Size of next message in bytes. 0 in case of the connection being closed, and -1 in case no
        /// data is available within the specified timeout.</returns>
        public int getNextMessageSize()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketGetNextMessageSize(ctx);
        }

        /// <summary>
        /// Returns the size of the next message that is ready to be delivered to the application.
        /// If there are no messages in the incoming queue, this call will block until a message arrives
        /// or the specified timeout expires.
        /// </summary>
        /// <param name="i64Timeout">Timeout in milliseconds. A timeout of 0 implies that the default
        /// timeout should be used whereas a timeout of -1 implies wait indefinitely.</param>
        /// <returns>Size of next message in bytes. 0 in case of the connection being closed, and -1 in case no
        /// data is available within the specified timeout.</returns>
        public int getNextMessageSize(Int64 i64Timeout)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketGetNextMessageSizeEx(ctx, i64Timeout);
        }

        /// <summary>
        /// Retrieves the data from next message that is ready to be delivered to the application, allocating a new byte
        /// array for the message. Uses the default timeout.
        /// </summary>
        /// <remarks>
        /// This method will allocate a new array for every call to receive, which places a higher burden on the garbage collector
        /// compared to maintaining a single buffer in the application. The implementation of this function uses getNextMessageSize().
        /// </remarks>
        /// <returns>Returns the buffer with the message, or null in case no data is available within the specified timeout.</returns>
        public byte[] receive()
        {
            return receive(0);
        }

        /// <summary>
        /// Retrieves the data from next message that is ready to be delivered to the application, allocating a new byte
        /// array for the message. Uses a specified timeout.
        /// </summary>
        /// <remarks>
        /// This method will allocate a new array for every call to receive, which places a higher burden on the garbage collector
        /// compared to maintaining a single buffer in the application. The implementation of this function uses getNextMessageSize().
        /// </remarks>
        /// <param name="i64Timeout">Timeout in millseconds. A timeout of 0 implies that the default timeout should be used
        ///     whereas a timeout of -1 implies wait indefinitely.</param>
        /// <returns>Returns the buffer with the message, or null in case no data is available within the specified timeout.</returns>
        public byte[] receive(Int64 i64Timeout)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            int size = NativeMethods.MocketGetNextMessageSize(ctx);

            if (size <= 0)
                throw new System.Net.Sockets.SocketException();

            byte[] rval = new byte[size];

            int status = NativeMethods.MocketReceiveEx(ctx, rval, (uint)rval.Length, i64Timeout);

            if (status <= 0)
                throw new System.Net.Sockets.SocketException();

            return rval;
        }

        // Retrieves the data from the next message that is ready to be delivered to the application
        // Not specifiying a timeout or a timeout of 0 implies that the default timeout should be used
        //     whereas a timeout of -1 implies wait indefinitely
        // The data is scattered into the buffers that are passed into the method
        // The pointer to the buffer and the buffer size arguments must be passed in pairs
        // The last argument should be NULL
        // Returns the total number of bytes that were copied into all the buffers, 0 in case of the
        //     connection being closed, and -1 in case no data is available within the specified timeout
        // NOTE: If the caller passes in three buffers, (e.g., sreceive (-1, pBufOne, 8, pBufTwo, 1024, pBufThree, 4096)),
        //       and the method returns 4000, the implication is that 8 bytes were read into pBufOne, 1024 bytes into
        //       pBufTwo, and the remaining 2968 bytes into pBufThree.
        // NOTE: Any additional data in the packet that will not fit in the buffers is discarded
        // This is not yet wrapped because I don't know how to handle the variable arguments in a reasonable way.
        //int sreceive (Int64 i64Timeout, array<System::Byte> ^pBuf1, UInt32 ui32BufSize1, ... array<Object^>^ args);

        /// <summary>
        /// Replaces a message in the transmit queue.
        /// </summary>
        /// <remarks>
        /// <para>First cancels any previously enqueued messages that have been tagged with the specified OldTag value
        /// and then enqueues the new message for transmission using the specified parameters.
        /// Note that there may be no old messages to cancel - in which case this call behaves just like a send().
        /// See documentation for <see cref="cancel"/> and <see cref="send"/> for more details.</para>
        /// 
        /// <para>Note that Mockets maintains seperate queues for different combinations of reliability/sequencing settings. The values
        /// of the bReliable and bSequenced parameters are used to determine which queue to look for old messages in. This means you cannot
        /// use this function to change the reliability/sequencing settings of a message. If you want to do this, you will need
        /// to cancel the message (using the old reliability/sequencing parameters) and send it again using the new parameters.</para>
        /// </remarks>
        /// <param name="bReliable">Reliability flag of the original message.</param>
        /// <param name="bSequenced">Sequencing flag of the original message.</param>
        /// <param name="pBuf">Byte array containing the new message.</param>
        /// <param name="ui32BufSize">The number of bytes to send.</param>
        /// <param name="ui16OldTag">The tag to cancel. Any messages in the transmit queue matching this tag will be cancelled.</param>
        /// <param name="ui16NewTag">The tag of the new message.</param>
        /// <param name="ui8Priority">Priority of the new message. (FIXME: What is highest?)</param>
        /// <param name="ui32EnqueueTimeout">The length of time in milliseconds for which the method will wait
        /// if there is no room in the outgoing buffer (a zero value indicates wait forever).</param>
        /// <param name="ui32RetryTimeout">Currently not used, but in the future could be used to specify the length of time
        /// for which the transmitter will retransmit the packet to ensure successful delivery.</param>
        /// <returns>Returns 0 if successful or a negative value in case of error.</returns>
        public int replace(bool bReliable, bool bSequenced, byte[] pBuf, UInt32 ui32BufSize, UInt16 ui16OldTag, UInt16 ui16NewTag,
            byte ui8Priority, UInt32 ui32EnqueueTimeout, UInt32 ui32RetryTimeout)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            int rval = 0;
            if (bReliable) rval = 1;
            int sval = 0;
            if (bSequenced) sval = 1;

            return NativeMethods.MocketReplace(ctx, rval, sval, pBuf, ui32BufSize, ui16OldTag, ui16NewTag, ui8Priority, ui32EnqueueTimeout,
                ui32RetryTimeout);
        }

        /// <summary>
        /// Cancels (deletes) previously enqueued messages that have been tagged with the specified tag.
        /// </summary>
        /// <remarks>
        /// <para>Note that the messages may be pending transmission (which applies to all flows)
        /// or may have already been transmitted but not yet acknowledged (which only applies to
        /// reliable flows).</para>
        /// 
        /// <para>Note that Mockets maintains seperate queues for different combinations of reliability/sequencing settings. The values
        /// of the bReliable and bSequenced parameters are used to determine which queue to look for messages in. If these settings
        /// are incorrect messages will not be found and will not be cancelled.</para>
        /// </remarks>
        /// <param name="bReliable">Reliability flag of the original message.</param>
        /// <param name="bSequenced">Sequencing flag of the original message.</param>
        /// <param name="ui16TagId">Tag ID of the message to cancel.</param>
        /// <returns>Returns 0 if successful or a negative value in case of error.</returns>
        public int cancel(bool bReliable, bool bSequenced, UInt16 ui16TagId)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            int rval = 0;
            if (bReliable) rval = 1;
            int sval = 0;
            if (bSequenced) sval = 1;

            return NativeMethods.MocketCancel(ctx, rval, sval, ui16TagId);
        }

        /// <summary>
        /// Gets the MTU used by this Mocket when sending packets on the underlying network.
        /// </summary>
        /// <remarks>This is for information
        /// only and does not affect the size of messages that can be sent using the Mocket.</remarks>
        /// <returns>The current MTU in bytes.</returns>
        public int getMTU()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketGetMTU(ctx);
        }

        /// <summary>
        /// Returns a reference to the Statistics class that provides statistics
        /// about this mocket connection.
        /// </summary>
        /// <remarks>
        /// The statistics object maintains a reference to the mocket. If statistics objects for a Mocket
        /// are referenced by your program, that Mocket will not be garbage collected even if the Mocket itself
        /// is no longer being referenced. This can lead to possible resource leaks.
        /// </remarks>
        /// <returns>A pointer to the statistics object associated with the mocket.</returns>
        public ManagedMocketStats getStatistics()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            IntPtr statsCtx = NativeMethods.MocketGetStatistics(ctx);

            return new ManagedMocketStats(this, statsCtx);
        }

        /// <summary>
        /// Sets an identifier for the connection. This identifier is used by the IHMC logging tools.
        /// </summary>
        /// <param name="pszIdentifier">A string identifier. Doesn't have to be unique - just used for logging.</param>
        public void setIdentifier(string pszIdentifier)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            NativeMethods.MocketSetIdentifier(ctx, pszIdentifier);
        }

        /// <summary>
        /// Gets the current identifier for the connection.
        /// </summary>
        /// <returns>The current identifier for the connection.</returns>
        public string getIdentifier()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketGetIdentifier(ctx);
        }

        /// <summary>
        /// Sets the disconnect timeout for the connection.
        /// </summary>
        /// <remarks>
        /// If no traffic or keepalives are received during the timeout
        /// period the connection will be closed. This takes the place of the PeerUnreachableWarningCallback
        /// mechanism used by the C++ mockets library.
        /// </remarks>
        /// <param name="ui32TimeoutMsec">The new timeout in milliseconds</param>
        public void setTimeOut(UInt32 ui32TimeoutMsec)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            NativeMethods.MocketSetTimeOut(ctx, ui32TimeoutMsec);
        }

        /// <summary>
        /// Gets the current disconnect timeout for the connection.
        /// </summary>
        /// <returns>The current disconnect timeout in milliseconds.</returns>
        public UInt32 getTimeOut()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketGetTimeOut(ctx);
        }

        /// <summary>
        /// The unmanaged pointer to the actual mocket context from the DLL
        /// </summary>
        internal IntPtr ctx;
    }
}
