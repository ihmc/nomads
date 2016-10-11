package us.ihmc.gst.sdi;

import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.gst.util.BoundingBox;
import us.ihmc.gst.util.GHubDataInputStream;

/**
 * NRL Geo-spatial Hub: Streaming Data Interface Packet Header
 *
 * =============================================================================
 * Field                Type            Contents                        Offset
 * =============================================================================
 * Magic Number         Integer         Fixed value: 0x66696C65         0
 * Time Stamp           Long            Zulu time expressed as Unix     4
 *                                      millisecond representation
 * Min Latitude         Double          Decimal degrees, -90º to 90º    12
 * Min Longitude        Double          Decimal degrees, -180º to 180º  20
 * Max Latitude         Double          Decimal degrees, -90º to 90º    28
 * Max Longitude        Double          Decimal degrees, -180º to 180º  36
 * Filename             String(64)      Null-padded name of payload     44
 * GHub Path            String(256)     GHub path description           108
 * Data Type            String(32)      Null-padded GHub Data type      364
 * Classification       Char            Classification level of the     396
 *                                      embedded payload
 * Releasable To        String(20)      Space-padded list of country    397
 *                                      codes
 * Owner                String(24)      Space-padded descriptor of      417
 *                                      product owner
 * Length               Integer         Size of payload in bytes        441
 *                                      (excludes header/footer)
 * =============================================================================
 *                                                          Total Size  445
 *
 * @author Giacomo Benincasa            (gbenincasa@ihmc.us)
 */
public class PacketHeader
{
    public static final int GHUB_MAGIC_NUMBER = 0x66696C65;

    public static final int LENGTH = 445;
    private static final int FILE_NAME_LENGTH = 64;
    private static final int GHUB_PATH_LENGTH = 256;
    private static final int DATA_TYPE_LENGTH = 32;
    private static final int RELEASABLE_TO_LENGTH = 20;
    private static final int OWNER_LENGTH = 24;

    private long _timestamp;
    private BoundingBox _boundingbox;
    private String _fileName;
    private String _ghubPath;
    private String _dataType;
    private char _classification;
    private String _releasableTo;
    private String _owner;
    private int _length;

    public PacketHeader()
    {
    }

    public boolean deserialize (GHubDataInputStream dataInput)
    {
        try {
            // Magic Number
            if (!checkMagicNumber (dataInput.readInt())) {
                return false;
            }

            // Timestamp
            _timestamp = dataInput.readLong();

            // Bounding box
            double minLat = dataInput.readLatitude();
            double minLon = dataInput.readLongitude();
            double maxLat = dataInput.readLatitude();
            double maxLon = dataInput.readLongitude();
            _boundingbox = new BoundingBox (minLat, minLon, maxLat, maxLon);
            if (!_boundingbox.isValid()) {
                return false;
            }

            _fileName = dataInput.readNullPaddedString (FILE_NAME_LENGTH);
            _ghubPath = dataInput.readNullPaddedString (GHUB_PATH_LENGTH);
            _dataType = dataInput.readNullPaddedString (DATA_TYPE_LENGTH);
            _classification = dataInput.readChar();
            _releasableTo = dataInput.readNullPaddedString (RELEASABLE_TO_LENGTH);
            _owner = dataInput.readNullPaddedString (OWNER_LENGTH);
            _length = dataInput.readInt();
        }
        catch (Exception ex) {
            Logger.getLogger (Packet.class.getName()).log(Level.SEVERE, null, ex);
            return false;
        }

        return true;
    }

    public boolean checkMagicNumber (int readMagicNumber)
    {
        return (readMagicNumber == GHUB_MAGIC_NUMBER);
    }

    public BoundingBox getBoundingBox()
    {
        return _boundingbox;
    }

    public char getClassification()
    {
        return _classification;
    }

    public String getDataType()
    {
        return _dataType;
    }

    public String getFileName()
    {
        return _fileName;
    }

    public String getGHubPath()
    {
        return _ghubPath;
    }

    public int getHeaderLength()
    {
        return LENGTH;
    }

    public int getMessageLength()
    {
        return _length;
    }

    public String getOwner()
    {
        return _owner;
    }

    public String getResealableTo()
    {
        return _releasableTo;
    }

    public long getTimestamp()
    {
        return _timestamp;
    }
}
