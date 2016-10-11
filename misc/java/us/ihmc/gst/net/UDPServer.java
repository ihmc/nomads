package us.ihmc.gst.net;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.util.concurrent.BlockingQueue;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.gst.sdi.Packet;

/**
 *
 * @author Giacomo Benincasa            (gbenincasa@ihmc.us)
 */
public class UDPServer extends Server
{
    public static final int UDP_PORT = 6262;

    private DatagramSocket _srvr;

    public UDPServer (BlockingQueue<Packet> urls) throws SocketException
    {
        this (UDP_PORT, urls);
    }

    public UDPServer (int port, BlockingQueue<Packet> urls) throws SocketException
    {
        super (urls);
        _srvr = new DatagramSocket (port);
    }

    @Override
    public void run()
    {
        while (true) {
            try {
                byte[] buf = new byte[4096];
                DatagramPacket packet = new DatagramPacket (buf, buf.length);
                _srvr.receive (packet);

                deserialize (packet.getData());
            }
            catch (IOException ex) {
                Logger.getLogger(TCPServer.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
    }
}
