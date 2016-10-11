package us.ihmc.gst.util;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.zip.CRC32;
import java.util.zip.CheckedInputStream;
import java.util.zip.Checksum;

/**
 *
 * @author Giacomo Benincasa            (gbenincasa@ihmc.us)
 */
public class GHubDataInputStream extends DataInputStream
{
    protected CheckedInputStream _cin;

    public GHubDataInputStream (InputStream input)
    {
        super (new CheckedInputStream (new BufferedInputStream (input), new CRC32()));
        _cin = (CheckedInputStream) in;
    }

    public Checksum getChecksum()
    {
        return _cin.getChecksum();
    }

    public double readLatitude() throws IOException, DeserializationException
    {
        double lat = readDouble();
        if (!GeoCoordinates.isValidLatitude (lat)) {
            throw new DeserializationException (lat + " is not a valid value for latitude");
        }
        return lat;
    }

    public double readLongitude() throws IOException, DeserializationException
    {
        double lon = readDouble();
        if (!GeoCoordinates.isValidLongitude (lon)) {
            throw new DeserializationException (lon + " is not a valid value for longitude");
        }
        return lon;
    }

    public byte[] readByteArray (int len) throws IOException
    {
        byte bytes[] = new byte[len];
        readFully (bytes);
        return bytes;
    }

    public String readNullPaddedString (int maxLen) throws IOException
    {
        byte[] buf = new byte[256];
        readFully (buf, 0, maxLen);
        return new String(buf).trim();
    }
}
