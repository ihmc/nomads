package netlogger.model.tracking;

import com.google.protobuf.Timestamp;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Data class for a tracking measure
 */
public class TrackingMeasure
{
    public TrackingMeasure (String srcApplication, String sourceModule, String destApplication, String destModule,
                            String prevLogUUID, String currLogUUID, String checksum, String type, String sourceClientID,
                            String destClientID, Timestamp measureTimestamp) {
        _sourceApplication = srcApplication;
        _sourceModule = sourceModule;
        _destApplication = destApplication;
        _destModule = destModule;
        _prevLogUUID = prevLogUUID;
        _currLogUUID = currLogUUID;
        _checksum = checksum;
        _measureTimestamp = measureTimestamp;
        _type = type;
        _sourceClientID = sourceClientID;
        _destClientID = destClientID;
    }

    public String getDestClientID () {
        return _destClientID;
    }

    public String getSourceApplication () {
        return _sourceApplication;
    }

    public String getSourceClientID () {
        return _sourceClientID;
    }

    public String getSourceModule () {
        return _sourceModule;
    }

    public String getDestModule () {
        return _destModule;
    }

    public String getPrevLogUUID () {
        return _prevLogUUID;
    }

    public String getCurrLogUUID () {
        return _currLogUUID;
    }

    public Timestamp getMeasureTimestamp () {
        return _measureTimestamp;
    }

    public String getChecksum () {
        return _checksum;
    }

    public String getDestApplication () {
        return _destApplication;
    }

    public String getType () {
        return _type;
    }

    private final String _sourceApplication;
    private final String _sourceModule;
    private final String _destApplication;
    private final String _destModule;
    private final String _prevLogUUID;
    private final String _currLogUUID;
    private final String _checksum;
    private final Timestamp _measureTimestamp;

    private final String _type;
    private final String _sourceClientID;
    private final String _destClientID;

    private static final Logger _logger = LoggerFactory.getLogger(TrackingMeasure.class);
}
