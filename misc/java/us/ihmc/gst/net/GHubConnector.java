package us.ihmc.gst.net;

import java.io.IOException;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import us.ihmc.gst.sdi.Packet;
import us.ihmc.gst.util.ThreadHandler;

/**
 *
 * @author Giacomo Benincasa        (gbenincasa@ihmc.us)
 */
public class GHubConnector
{
    private enum Mode
    {
        TCP ("tcp"),
        UDP ("udp");

        public String _mode;

        Mode (String mode)
        {
            _mode = mode;
        }

        static public Mode parseMode (String mode)
        {
            if (mode == null) {
                throw new NullPointerException();
            }
            for (Mode m : Mode.values()) {
                if (m._mode.compareToIgnoreCase (mode) == 0) {
                    return m;
                }
            }
            throw new IllegalArgumentException();
        }
    }

    public static void runGHUBConnector (String host, int port, BlockingQueue<GHubObject> objects) throws IOException
    {
        runGHUBConnector(host, port, Mode.TCP, objects);
    }

    public static void runGHUBConnector (String host, BlockingQueue<GHubObject> objects) throws IOException
    {
        runGHUBConnector (host, -1, Mode.TCP, objects);
    }

    public static void runGHUBConnector (String host, Mode mode, BlockingQueue<GHubObject> objects) throws IOException
    {
        runGHUBConnector (host, -1, mode, objects);
    }

    public static void runGHUBConnector (String host, int port, Mode mode, BlockingQueue<GHubObject> objects) throws IOException
    {
        switch (mode) {
            case TCP:
                if (port != -1) {
                    _server = new TCPServer (port, _urls);
                }
                else {
                    _server = new TCPServer(_urls);
                }
                break;

            case UDP:
                if (port != -1) {
                    _server = new UDPServer (port, _urls);
                }
                else {
                    _server = new UDPServer(_urls);
                }
                break;

            default:
                _server = null;
                System.exit(-1);
        }

        _retriever = new DataRetriever (host, _urls, objects);
        Thread t = new Thread (_retriever);
        t.setUncaughtExceptionHandler (new ThreadHandler());
        (new Thread (_retriever)).start();

        t = new Thread (_server);
        t.start();
    }
            
    /**
     * @param args the command line arguments
     */
    public static void main (String[] args) throws Exception
    {
        if (args.length > 2) {
            throw new Exception ("Too many arguments");
        }

        Mode mode = Mode.TCP;
        int port = -1;

        switch (2 - args.length) {
            case 0:
                mode = Mode.parseMode (args[0]);

            case 1:
                port = Integer.parseInt(args[args.length-1]);
        }

        String host = "192.168.50.23";
        BlockingQueue<GHubObject> objects = new LinkedBlockingQueue<GHubObject>();
        runGHUBConnector (host, port, mode, objects);

        GHubObject obj;
        Packet p;
        byte[] data;
        while ((obj = objects.poll(Long.MAX_VALUE, TimeUnit.DAYS)) != null) {
            System.out.println(obj.getPacket().getHeader().toString());
            data = obj.getData();
            if (data != null && data.length > 0) {
                System.out.println("Data of size " + data.length);
            }
            else {
                System.out.println("Data of size 0");
            }
        }
    }

    static private ThreadHandler handler = new ThreadHandler();
    static private final BlockingQueue<Packet> _urls = new LinkedBlockingQueue<Packet>();
    static private DataRetriever _retriever;
    static private Server _server;
}
