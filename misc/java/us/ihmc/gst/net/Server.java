package us.ihmc.gst.net;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.BlockingQueue;
import us.ihmc.gst.oa.XMLMessage;
import us.ihmc.gst.sdi.Packet;
import us.ihmc.gst.sdi.PacketHeader;

/**
 *
 * @author Giacomo Benincasa            (gbenincasa@ihmc.us)
 */
public abstract class Server implements Runnable
{
    private static final String ERR_MSG = "Could not deserialize packet";
    private final BlockingQueue<Packet> _packets;

    protected Server (BlockingQueue<Packet> packets)
    {
        _packets = packets;
    }

    protected void deserialize (byte[] bytes) throws IOException
    {
        deserialize (new ByteArrayInputStream (bytes));
    }

    protected void deserialize (InputStream is) throws IOException
    {
        while (true) {
            try {
                Packet packet = new Packet();
                if (!packet.deserialize (is)) {
                    break;
                }

                if (packet.getPayload() instanceof XMLMessage) {
                    XMLMessage msg = (XMLMessage) packet.getPayload();
                    System.out.println ("Added packet for object " + msg.getReferredObjectURL());
                    _packets.add (packet);
                }
                else {
                    System.out.println ("Added packet for object ");
                }
            }
            catch (IOException e) {
                
            }
        }
    }
}
