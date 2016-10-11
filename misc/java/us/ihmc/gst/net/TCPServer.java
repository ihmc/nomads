package us.ihmc.gst.net;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.BlockingQueue;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.gst.sdi.Packet;

/**
 *
 * @author Giacomo Benincasa            (gbenincasa@ihmc.us)
 */
public class TCPServer extends Server
{
    public static final int TCP_PORT = 6264;

    private ServerSocket _srvr;

    public TCPServer (BlockingQueue<Packet> urls) throws IOException
    {
        this (TCP_PORT, urls);
    }

    public TCPServer (int port, BlockingQueue<Packet> urls) throws IOException
    {
        super (urls);
        _srvr = new ServerSocket (port);
    }

    @Override
    public void run()
    {
        while (true) {
            try {
                Socket sock = _srvr.accept();
                deserialize (sock.getInputStream());
            }
            catch (Exception ex) {
                Logger.getLogger(TCPServer.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
    }
}
