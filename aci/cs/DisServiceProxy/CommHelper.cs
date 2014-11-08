/*
 * CommHelper.cs
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace us.ihmc.util
{
    public class CommHelper: IDisposable
    {
        public CommHelper()
        {
            _tcpClient = null;
        }

        public bool init(TcpClient client)
        {
            if (client == null) {
                return false;
            }

            _tcpClient = client;

            try {
                Stream stream = _tcpClient.GetStream();

                // !! DO NOT instantiate the LineReaderInputStream with a BufferedStream
                // this can cause very odd behaviour
                _inputReader = new LineReaderInputStream(stream);

                BufferedStream buffStream = new BufferedStream(stream);
                _outputWriter = new StreamWriter(buffStream);

                return true;
            }
            catch (SocketException ex) {
                // close the socket if an exception occurs getting its  stream
                closeConnection();                        
                if (_debug) {
                    Console.WriteLine(ex.ToString());
                }
            }

            return false;  
        }

        public string receiveLine()
        {
            return receiveLine(-1);
        }

        public string receiveLine(int timeout)
        {    
            String line;

            int oldTimeOut = 0;
            if (timeout != -1)
            {
                oldTimeOut = _inputReader.ReadTimeout;
                _inputReader.ReadTimeout = timeout;
            }

            try
            {
                line = _inputReader.readLine();
            }
            catch (IOException ex)
            {
                if (_debug)
                {
                    Console.WriteLine(ex.ToString());
                }

                if (ex.InnerException != null && ex.InnerException is SocketException &&
                    (ex.InnerException as SocketException).ErrorCode == 10060) // timeout
                {
                    throw new TimeoutException("Timed out waiting for response from DisServiceProxyServer.");
                }

                throw new CommException("unable to read a line from socket");
            }
            finally
            {
                if (timeout != -1)
                    _inputReader.ReadTimeout = oldTimeOut;
            }

            if (line == null) {
                throw new CommException("other end closed socket");
            }

            if (_debug) {
                Console.WriteLine("RECEIVED: [{0}]", line);
            }
            
            return line;
        }

        public string[] receiveParsed()
        {
            String line = null;
            try {
                line = this.receiveLine();
            }
            catch (IOException) {
                throw;
            }

            return parse(line);
        }

        public byte[] receiveBlob(int size)
        {
            byte[] buffer = new byte[size];
            int read = 0;
            int index = 0;

            do {
                read = _inputReader.Read(buffer, index, size - index);
                if (read == -1) {
                    throw new SocketException();
                }
                index += read;
            }
            while (index < size);

            return buffer;
        }

        public int receiveBlob(byte[] buf, int off, int len)
        {
            return _inputReader.Read(buf, off, len);
        }

        public byte[] receiveBlock()
        {
            uint uiBlockLen = read32();
            if (uiBlockLen == 0)
            {
                return null;
            }
            byte[] buf= new byte[uiBlockLen];
            receiveBlob(buf, 0, (int)uiBlockLen);
            return buf;
        }

        public void sendLine(String line)
        {       
            try {
                _outputWriter.Write(line);
                _outputWriter.Write("\n");
                _outputWriter.Flush();
            }
            catch (Exception) {
                throw new CommException();
            }

            if (_debug) {
                Console.WriteLine("SENT: {0}", line);
            }
        }

		public void sendStringBlock(String str)
		{
			uint uiLen = (str == null || str.Length < 0) ? 0 : (uint) str.Length;
			write32 (uiLen);
			if (uiLen > 0) {
                sendBlob (Encoding.ASCII.GetBytes(str));
			}
		}

        public void sendBlob(byte[] buff)
        {
            sendBlob(buff, 0, buff.Length);
        }

        public void sendBlob(byte[] buff, int off, int len)
        {
            try
            {
                _outputWriter.BaseStream.Write(buff, off, len);
                _outputWriter.BaseStream.Flush();
            }
            catch (Exception ex)
            {
                if (_debug)
                {
                    Console.WriteLine(ex.ToString());
                }

                throw new CommException();
            }
        }

        public void closeConnection()
        {
            // don't eat exception

            _tcpClient.Close();
        }

        protected static string[] parse(string str)
        {
            if (str == null) {
                return null;
            }

            char[] seps = new char[]{' '};
            string[] aux = str.Split(seps);

            return aux;
        }

        public void write8 (byte val)
        {
            byte[] aux = new byte[]{val};
            _outputWriter.BaseStream.Write(aux, 0, 1);
            _outputWriter.BaseStream.Flush();
        }

        public void write16 (ushort ui16Val)
        {
            ui16Val = (ushort) IPAddress.HostToNetworkOrder((short)ui16Val);

            byte[] aux = BitConverter.GetBytes (ui16Val);
            _outputWriter.BaseStream.Write(aux, 0, 2);
            _outputWriter.BaseStream.Flush();
        }

        public void write32 (uint ui32Val)
        {
            ui32Val = (uint)IPAddress.HostToNetworkOrder((int)ui32Val);

            byte[] aux = BitConverter.GetBytes(ui32Val);
            _outputWriter.BaseStream.Write(aux, 0, 4);
            _outputWriter.BaseStream.Flush();
        }

        public void write64(ulong ui64Val)
        {
            ui64Val = (ulong)IPAddress.HostToNetworkOrder((long)ui64Val);

            byte[] aux = BitConverter.GetBytes(ui64Val);
            _outputWriter.BaseStream.Write(aux, 0, 8);
            _outputWriter.BaseStream.Flush();
        }

        public byte read8()
        {
            int b = _inputReader.ReadByte();
            if (b < 0) {
                throw new CommException("Exception in read8");
            }

            return (byte)b;
        }

        public ushort read16()
        {
            byte[] buf = new byte[2];
            int index = 0;
            int read;

            while (index < 2) {
                read = _inputReader.Read (buf, index, 2 - index);
                if (read < 0) {
                    throw new CommException("Exception in read16");
                }
                index += read;
            }

            ushort aux = (ushort)BitConverter.ToUInt16(buf, 0);
            aux = (ushort)IPAddress.NetworkToHostOrder ((short)aux);
            return aux;
        }

        public uint read32()
        {
            byte[] buf = new byte[4];
            int index = 0;
            int read;

            while (index < 4)
            {
                read = _inputReader.Read(buf, index, 4 - index);
                if (read < 0) {
                    throw new CommException("Exception in read32");
                }
                index += read;
            }

            uint aux = (uint)BitConverter.ToUInt32(buf, 0);
            aux = (uint)IPAddress.NetworkToHostOrder((int)aux);

            return aux;
        }
        
        // ////////////////////////////////////////////////////////////////////
        private static readonly bool _debug = false;
        private TcpClient _tcpClient;
        
        private LineReaderInputStream _inputReader;
        private StreamWriter _outputWriter;

        ~CommHelper()
        {
            Dispose(false);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                _tcpClient.Close();
                _inputReader.Dispose();
                _outputWriter.Dispose();

                _tcpClient = null;
                _inputReader = null;
                _outputWriter = null;
            }
        }

        #region IDisposable Members

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        #endregion
    }

    public class LineReaderInputStream : Stream
    {
        // the stream that is passed cannot be a BufferedStream, given that 
        // this is now a BufferedStream. Passing a BufferedStream will
        // only cause an odd behaviour.
        // The Stream that is passed must be one that will return inmediately after
        // calling the Read(byte[], int, int) method, even if it didn't/couldn't read
        // all the bytes it was asked to.
        public LineReaderInputStream(Stream stream)
        {
            _stream = stream;

            _internalBuffer = new byte[INTERNAL_BUFFER_SIZE];
            _internalBuffCount = 0;
            _internalBuffIndex = 0;
        }
        
        public string readLine()
        {

            StringBuilder sb = new StringBuilder(10);

            while (true) {
                int read = this.read();
                if (read < 0) {
                    throw new IOException("End of stream reached.");
                }

                char readChar = (char) read;

                if (readChar == '\r') {
                    // found the end of the line.
                    // check if there is a \n also
                    byte nextByte = (byte) this.read();

                    if (nextByte != '\n') {
                        this.putBackByte(nextByte);
                    }

                    break;
                }
                else if (readChar == '\n') {
                    // We have received the end of a line
                    break;
                }
                
                sb.Append(readChar);
            }

            string line = sb.ToString();
            return line;
        }

		public int read()
        {
            // check first if the internal buffer has any data.
            // if it doesn't, try to fill it up.
            if (_internalBuffCount <= 0)
            {
                int read = _stream.Read(_internalBuffer, 0, INTERNAL_BUFFER_SIZE);

                if (read > 0)
                {
                    _internalBuffCount = read;
                    _internalBuffIndex = 0;
                }
                else
                {
                    _internalBuffCount = 0;
                    _internalBuffIndex = -1;
                }

                // if the buffer still doesn't contain data, means that we 
                // reached the end of the stream
                if (_internalBuffCount <= 0)
                {
                    Console.WriteLine("_internalBuffCount <= 0, returning -1");
                    return -1;
                }
            }

            byte returnByte = _internalBuffer[_internalBuffIndex];
            _internalBuffIndex++;
            _internalBuffCount--;

            return returnByte;
        }

        // ///////////////////////////////////////////////////////////////////////
        // methods to accomplish with the System.IO.Stream Interface. ////////////
        // ///////////////////////////////////////////////////////////////////////
        #region System.IO.Stream methods

        override public bool CanRead 
        {
            get {return true;}
        }

        override public bool CanWrite 
        {
            get {return false;}
        }

        override public bool CanSeek
        {
            get {return false;}
        }

        override public long Length
        {
            get {throw new NotSupportedException("getLength is not supported by MocketInputStream");}
        }

        override public void SetLength(long length)
        {
            throw new NotSupportedException("SetLength is not supported by MocketInputStream");
        }

        override public long Position
        {
            get {throw new NotSupportedException("get Position is not supported by MocketInputStream");}
            set {throw new NotSupportedException("set Position is not supported by MocketInputStream");}
        }

        override public long Seek(long offset, SeekOrigin origin)
        {
            throw new NotSupportedException("Seek is not supported by MocketInputStream");
        }

        override public void Flush()
        {
        }

        override public void Write(byte[] byteArr, int off, int count)
        {
            throw new NotSupportedException("Write is not supported by MocketInputStream");
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            int lim = offset + count;
            int numRead = 0;

            for (int i = offset; i < lim; i++) {
                int read = this.read();
                if (read < 0) {
                    break;
                }

                buffer[i] = (byte) read;
                numRead++;                
            }

            return numRead;
        }

        public override int ReadTimeout
        {
            get
            {
                return _stream.ReadTimeout;
            }
            set
            {
                _stream.ReadTimeout = value;
            }
        }

        #endregion

        // //////////////////////////////////////////////////////////////////////////
        // PRIVATE METHODS //////////////////////////////////////////////////////////
        // //////////////////////////////////////////////////////////////////////////
        
        private void putBackByte(byte b)
        {
            if (_internalBuffIndex < 1) {
                return;
            }

            _internalBuffCount++;
            _internalBuffIndex--;

            _internalBuffer[_internalBuffIndex] = b;
        }

        // /////////////////////////////////////////////////////////////////////

        private const int INTERNAL_BUFFER_SIZE = 2048;

        // /////////////////////////////////////////////////////////////////////
        private byte[] _internalBuffer;
        private int _internalBuffIndex;
        private int _internalBuffCount;

        private Stream _stream;
    }

    public class CommException : Exception
    {
        public CommException()
        {
        }

        public CommException(string msg): base(msg)
        {
        }
    }
}
