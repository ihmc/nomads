package us.ihmc.aci.net;

import java.net.*;
import java.io.*;
import javax.net.ServerSocketFactory;
import java.rmi.server.RMIServerSocketFactory;

public class CustomServerSocketFactory extends ServerSocket
    implements RMIServerSocketFactory, Serializable
{
    public CustomServerSocketFactory() throws IOException
    {
        super();
    }
    
    public ServerSocket createServerSocket(int port)
        throws IOException
    {
        return new CustomServerSocket(port);
    }
    
    
    public static class CustomServerSocket extends ServerSocket
    {
        public CustomServerSocket(int port)
            throws IOException
        {
            super (port);
            System.out.println("CustomServerSocket :: port = " + port);
        }
    }

    public Socket accept() throws IOException
    {
        Socket s = new CustomSocketFactory.DataStatsSocket();
        implAccept (s);
        return s;
    }
    
}
