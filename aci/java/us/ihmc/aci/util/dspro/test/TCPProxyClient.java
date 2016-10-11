package us.ihmc.aci.util.dspro.test;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.net.Socket;

/**
 *
 * @author gbenincasa
 */
public class TCPProxyClient implements Runnable
{
    private final String _host;
    private final int _port;
    private TCPProxyListener _listener;
    private ObjectInputStream _in;

    public TCPProxyClient (String host, int port)
    {
        _host = host;
        _port = port;
    }

    public void addTCPProxyListener (TCPProxyListener listener)
    {
        _listener = listener;
    }

    public void run() {

        while (true) {
            try {
                Object obj = _in.readObject();
                if (obj instanceof Message) {
                    Message msg = (Message) obj;
                    _listener.messageArrived(msg);
                }
            }
            catch (IOException ex) {
              //  java.util.logging.Logger.getLogger(TCPProxyServer.class.getName()).log(Level.SEVERE, null, ex);
            }
            catch (ClassNotFoundException ex) {
                //java.util.logging.Logger.getLogger(TCPProxyServer.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
    }

    public void init() throws IOException
    {
        System.out.println ("Trying to connect to " + _host + ":" + _port);
        Socket sock = new Socket (_host, _port);
        new ObjectOutputStream (sock.getOutputStream()).flush();
        _in = new ObjectInputStream (sock.getInputStream());
    }
}
