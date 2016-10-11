package us.ihmc.gst.sdi;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.gst.util.GHubDataInputStream;

/**
 * NRL Geo-spatial Hub: Streaming Data Interface Packet Footer
 *
 * =============================================================================
 * Field                Type            Contents                        Offset
 * =============================================================================
 * CRC32                Integer         Checksum of payload including   0
 *                                      header
 * =============================================================================
 *                                                          Total Size  8
 *
 * @author Giacomo Benincasa            (gbenincasa@ihmc.us)
 */
public class PacketFooter
{
    private int _crc;

    PacketFooter()
    {
        _crc = 0;
    }

    boolean deserialize (GHubDataInputStream dataInput)
    {
        try {
            _crc = dataInput.readInt();
            return true;

        }
        catch (IOException ex) {
            Logger.getLogger (PacketFooter.class.getName()).log(Level.SEVERE, null, ex);
            return false;
        }
    }

    public int getCRC32()
    {
        return _crc;
    }
}
