// ManagedServerMocket.cs
// P/Invoke wrapper for C++ mockets library
// jk, 11/2008

using System;

namespace us.ihmc.mockets
{
    /// <summary>
    /// Allows applications to listen for and accept connections from client Mockets.
    /// </summary>
    /// <remarks>
    /// This class uses P/Invoke to wrap a C++ ServerMocket object. 
    /// </remarks>
    public class ManagedServerMocket : IDisposable
    {
        /// <summary>
        /// Initialize a new instance of the ManagedServerMocket class.
        /// </summary>
        public ManagedServerMocket()
        {
            ctx = NativeMethods.MocketServerCreate();
        }

        /// <summary>
        /// Destructor
        /// </summary>
        ~ManagedServerMocket()
        {
            Dispose(false);
        }

        /// <summary>
        /// Deallocates the unmanaged server mocket handle.
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
            if (ctx != IntPtr.Zero)
            {
                NativeMethods.MocketServerDestroy(ctx);
            }
            ctx = IntPtr.Zero;
        }

        /// <summary>
        /// Initialize the server mocket to accept incoming connections.
        /// </summary>
        /// <param name="ui16Port">Port to use. Specifying a 0 for the port causes a random port to be allocated.</param>
        /// <returns>Returns the port number that was assigned, or a negative value in case of error.</returns>
        public int listen(UInt16 ui16Port)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedServerMocket");

            return NativeMethods.MocketServerListen(ctx, ui16Port);
        }

        /// <summary>
        /// Initialize the server mocket to accept incoming connections at a specified address and port.
        /// </summary>
        /// <param name="ui16Port">Port to use.</param>
        /// <param name="pszListenAddr">Listen address to use.</param>
        /// <returns>Returns the port number that was assigned, or a negative value in case of error.</returns>
        public int listen(UInt16 ui16Port, string pszListenAddr)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedServerMocket");

            return NativeMethods.MocketServerListenEx(ctx, ui16Port, pszListenAddr);
        }

        /// <summary>
        /// Initialize the server mocket to accept incoming connections at a specified address and port.
        /// </summary>
        /// <param name="ui16Port">Port to use.</param>
        /// <param name="pListenAddr">Listen address to use.</param>
        /// <returns>Returns the port number that was assigned, or a negative value in case of error.</returns>
        public int listen(UInt16 ui16Port, System.Net.IPAddress pListenAddr)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedServerMocket");

            return NativeMethods.MocketServerListenEx(ctx, ui16Port, pListenAddr.ToString());
        }

        /// <summary>
        /// Initialize the server mocket to accept incoming connections at a specified address and port.
        /// </summary>
        /// <param name="pListenEndPoint">Listen endpoint to use (address and port).</param>
        /// <returns>Returns the port number that was assigned, or a negative value in case of error.</returns>
        public int listen(System.Net.IPEndPoint pListenEndPoint)
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedServerMocket");

            return NativeMethods.MocketServerListenEx(ctx, (UInt16)pListenEndPoint.Port, pListenEndPoint.Address.ToString());
        }

        /// <summary>
        /// Accepts the next incoming connection.
        /// </summary>
        /// <returns>A ManagedMocket for the next incoming connection, or null if error.</returns>
        public ManagedMocket accept()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedServerMocket");

            IntPtr theMocket = NativeMethods.MocketServerAccept(ctx);
            if (theMocket == IntPtr.Zero)
                return null;

            return new ManagedMocket(theMocket);
        }

        /// <summary>
        /// Stops listening for connections.
        /// </summary>
        /// <returns>0 if successful.</returns>
        public int close()
        {
            if (ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedServerMocket");

            return NativeMethods.MocketServerClose(ctx);
        }

        /// <summary>
        /// The unmanaged pointer to the actual server mocket context from the DLL
        /// </summary>
        private IntPtr ctx;
    }
}
