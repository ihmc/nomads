package us.ihmc.aci.net;

import java.net.*;
import java.io.*;
import java.rmi.server.RMIClientSocketFactory;

public class CustomSocketFactory implements RMIClientSocketFactory, Serializable
{
    Socket createSocket(InetAddress host, int port)
        throws IOException    
    {
        return new DataStatsSocket(host, port);
    }
    
    Socket createSocket(InetAddress address, int port, InetAddress localAddress, int localPort)
        throws IOException    
    {
        return new DataStatsSocket (address, port, localAddress, localPort);
    }
    
    @Override
    public Socket createSocket(String host, int port)
        throws IOException    
    {
        return new DataStatsSocket (host, port);
    }
    
    Socket 	createSocket(String host, int port, InetAddress localHost, int localPort)
        throws IOException    
    {
        return new DataStatsSocket (host, port, localHost, localPort);
    }
    
    public static class DataStatsSocket extends Socket
    {
        DataStatsSocket()
        {
            super();
        }
        
        DataStatsSocket (InetAddress host, int port)
            throws IOException
        {
            super (host, port);
        }
        
        DataStatsSocket (InetAddress address, int port, InetAddress localAddress, int localPort)
            throws IOException        
        {
            super (address, port, localAddress, localPort);
        }
        
        DataStatsSocket (String host, int port)
            throws IOException        
        {
            super (host, port);
        }
        
        DataStatsSocket (String host, int port, InetAddress localHost, int localPort)
            throws IOException        
        {
            super (host, port, localHost, localPort);
        }
 
	@Override
        public InputStream getInputStream()
            throws IOException
        {
            if (_inputStream == null) {
                _inputStream = new DataStatsInputStream (super.getInputStream());
            }
            return _inputStream;
        }
        
	@Override
        public OutputStream getOutputStream()
            throws IOException        
        {
            if (_outputStream == null) {
                _outputStream = new DataStatsOutputStream (super.getOutputStream());
            }
            return _outputStream;
        }
        
        InputStream _inputStream = null;
        OutputStream _outputStream = null;
    }
    
    public static class DataStatsInputStream extends InputStream
    {
        DataStatsInputStream (InputStream is)
        {
            _is = is;
        }

	@Override
        public int read() throws IOException
        {
            int read = _is.read();

            if (read >= 0) {
                _dataRead++;
                System.out.println ("DataRead so far:: "  + _dataRead);
            }

            return read;
        }
        
	@Override
        public int available() throws IOException
        {
            return _is.available();
        }

	@Override
        public int read (byte[] buf, int off, int len) throws IOException
        {
            int read = _is.read (buf, off, len);
            if (read >= 0) {
                _dataRead += read;
                System.out.println ("DataRead so far:: "  + _dataRead);                
            }
            return read;
        }

        private int _dataRead = 0;
        private InputStream _is;
    } //DataStatsInputStream

    public static class DataStatsOutputStream extends OutputStream
    {
        DataStatsOutputStream (OutputStream os)
        {
            _os = os;
        }

	@Override
        public void write (int b)
            throws IOException
        {
            _os.write (b);
            _dataWritten++;
            System.out.println ("DataWritten so far:: " + _dataWritten);
        }
        
	@Override
        public void write (byte[] buf) throws IOException
        {
            _os.write (buf);
            if (buf != null) {
                _dataWritten += buf.length;
            }
        }           

        private int _dataWritten = 0;
        private OutputStream _os;
    } //DataStatsOutputStream
}
