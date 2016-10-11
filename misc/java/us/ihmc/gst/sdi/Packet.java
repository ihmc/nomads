package us.ihmc.gst.sdi;

import java.io.IOException;
import us.ihmc.gst.oa.OAMessageFactory;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;

import java.io.InputStream;
import java.util.zip.Checksum;
import mil.navy.nrlssc.gst.oa.OAMessage;

import us.ihmc.gst.util.GHubDataInputStream;

/**
 * NRL Geo-spatial Hub: Streaming Data Interface Packet
 *
 * @author Giacomo Benincasa            (gbenincasa@ihmc.us)
 */
public class Packet
{
    private PacketHeader _header;
    private OAMessage _payload;
    private PacketFooter _footer;

    public Packet()
    {
        _header = new PacketHeader();
        _payload = null;
        _footer = new PacketFooter();
    }

    public boolean deserialize (byte messageBuffer[]) throws IOException
    {
        return deserialize (new ByteArrayInputStream (messageBuffer));
    }

    public boolean deserialize (InputStream is) throws IOException
    {
        return deserialize (new DataInputStream (is));
    }

    public boolean deserialize (ByteArrayInputStream byteInput) throws IOException
    {
        return deserialize (new DataInputStream (byteInput));
    }

    public boolean deserialize (DataInputStream dataInput) throws IOException
    {
        return deserialize (new GHubDataInputStream (dataInput));
    }

    public boolean deserialize (GHubDataInputStream dataInput) throws IOException
    {
        // Read header
        if (!_header.deserialize (dataInput)) {
            return false;
        }

        // Read payload (it is a OA message)
        _payload = OAMessageFactory.getOAMessage (dataInput, _header.getMessageLength());
        if (_payload == null) {
            return false;
        }

        if (!_payload.deserialize (dataInput, _header.getMessageLength())) {
            return false;
        }

        // Get header + payload checksum
        Checksum checksum = dataInput.getChecksum();

        // Read footer
        if (!_footer.deserialize (dataInput)) {
            return false;
        }

        // Check header + payload checksum
        if (checksum.getValue() != _footer.getCRC32()) {
            return true /*false*/;
        }

        return true;
    }

    public PacketFooter getFooter()
    {
        return _footer;
    }

    public PacketHeader getHeader()
    {
        return _header;
    }

    public OAMessage getPayload()
    {
        return _payload;
    }
}
