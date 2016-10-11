package us.ihmc.gst.net;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

import mil.navy.nrlssc.gst.oa.OAMessage;
import org.apache.log4j.Logger;
import us.ihmc.gst.oa.XMLMessage;
import us.ihmc.gst.sdi.Packet;
import us.ihmc.gst.util.GHubUtils;

/**
 * @author Giacomo Benincasa        (gbenincasa@ihmc.us)
 */
public class DataRetriever implements Runnable
{
    public DataRetriever (String host, BlockingQueue<Packet> packets, BlockingQueue<GHubObject> objects)
    {
        _host = host;
        _packets = packets;
        _objects = objects;
    }

    public void run ()
    {
        Packet packet;
        URL url;
        InputStream in;
        ByteArrayOutputStream out;
        GHubObject obj;
        long totLen;
        while (true) {
            try {
                packet = _packets.poll(Long.MAX_VALUE, TimeUnit.DAYS);
            }
            catch (InterruptedException ex) {
                System.err.println(ex.getMessage());
                continue;
            }
            OAMessage oaMsg = packet.getPayload();
            if (oaMsg == null) {
                log.warn("OAMessage is null, unable to parse Payload");
                continue;
            }
            if (oaMsg instanceof XMLMessage) {
                in = null;
                XMLMessage msg = (XMLMessage) oaMsg;
                try {
                    if (msg.getReferredObjectURL() != null) {
                        log.info("Message has a referred object URL: " + msg.getReferredObjectURL());
                        url = GHubUtils.getURL(msg.getReferredObjectURL(), _host);
                        log.info("URL transformed to: " + url + " - Trying to open connection...");
                        url.openConnection();
                        in = url.openStream();
                        out = new ByteArrayOutputStream();
                        totLen = 0;
                        for (int len; (len = in.read(_buf, 0, _buf.length)) > 0; ) {
                            out.write(_buf, 0, len);
                            totLen += len;
                        }
                        log.info("Retrieved object. host: " + url.getHost() + " - path: " + url.getPath());
                        String strUrl = url.toString();
                        String fileName = GHubUtils.getURLFileName(strUrl);
                        if (fileName == null) {
                            log.error("Unable to fetch file name of this referred object, setting null");
                        }
                        obj = new GHubObject(packet, out.toByteArray(), msg.getReferredObjectURL(), fileName);

                    }
                    else {
                        log.info("No referred object URL found, generating packet with no data");
                        obj = new GHubObject(packet, null, null, null);
                    }
                    if (_objects.add(obj)) {
                        log.info("Added object to shared queue");
                    }
                    else {
                        log.info("Object was not added to shared queue");
                    }
                }
                catch (Exception ex) {
                    System.err.println(ex.getMessage());
                    log.error("Could not retrieve " + msg.getReferredObjectURL(), ex);
                    obj = new GHubObject(packet, null, msg.getReferredObjectURL(), null);
                    if (_objects.add(obj)) {
                       log.info("Added object to shared queue");
                    }
                    else {
                        log.info("Object was not added to shared queue");
                    }
                }
                finally {
                    if (in != null) {
                        try {
                            in.close();
                        }
                        catch (IOException ex) {
                        }
                    }
                }
            }
            else {
                log.warn("OAMessage is not an XMLMessage");
            }
        }
    }

    private final String _host;
    private BlockingQueue<Packet> _packets;
    private BlockingQueue<GHubObject> _objects;
    private static final byte[] _buf = new byte[1024];

    private static final Logger log = Logger.getLogger(DataRetriever.class);
}
