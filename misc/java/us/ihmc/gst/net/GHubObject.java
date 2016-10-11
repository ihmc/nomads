package us.ihmc.gst.net;

import us.ihmc.gst.sdi.Packet;

/**
 *
 * @author Giacomo Benincasa        (gbenincasa@ihmc.us)
 */
public class GHubObject
{
    public GHubObject (Packet packet, byte[] date, String relatedLink, String fileName)
    {
        _packet = packet;
        _data = date;
        _relatedLink = relatedLink;
        _fileName = fileName;
    }

    public Packet getPacket()
    {
        return _packet;
    }

    public byte[] getData()
    {
        return _data;
    }

    public String getRelatedLink()
    {
        return _relatedLink;
    }

    public String getFileName()
    {
        return _fileName;
    }

    private final Packet _packet;
    private final byte[] _data;
    private final String _fileName;
    private final String _relatedLink;
}
