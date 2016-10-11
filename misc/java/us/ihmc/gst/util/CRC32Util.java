package us.ihmc.gst.util;

import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.CRC32;

/**
 *
 * @author Giacomo Benincasa            (gbenincasa@ihmc.us)
 */
public class CRC32Util
{
    private static final CRC32 _checker = new CRC32();

    /**
     * Updates CRC-32 with specified array of bytes.
     *
     * @param b - the byte array to update the checksum with
     * @param off - the start offset of the data
     * @param len - the number of bytes to use for the update
     */
    public static boolean check (byte[] b, int off, int len, int expectedCRC)
    {
        if (b.length < (off + len)) {
            Logger.getLogger (CRC32Util.class.getName()).log (Level.SEVERE, null,
                              new Exception ("Tried to compute CRC on bytes " + off + " - "
                                            + off+len + " when the length of the buffer is only "
                                            + b.length) + " bytes");
            return false;
        }

        _checker.update (b, 0, len);
        if (_checker.getValue() != expectedCRC) {
            return false;
        }

        return true;
    }
}
