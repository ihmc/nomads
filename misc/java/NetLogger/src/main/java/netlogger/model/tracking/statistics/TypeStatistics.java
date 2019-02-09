package netlogger.model.tracking.statistics;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TypeStatistics
{
    public TypeStatistics (long lastSent, long lastReceived, int totalSent, int totalReceived, String type) {
        _lastSent = lastSent;
        _lastReceived = lastReceived;
        _totalSent = totalSent;
        _totalReceived = totalReceived;
        _type = type;
    }

    public long getLastReceivedTime () {
        return _lastReceived;
    }

    public long getLastSentTime () {
        return _lastSent;
    }

    public int getTotalReceived () {
        return _totalReceived;
    }

    public int getTotalSent () {
        return _totalSent;
    }

    public String getType () {
        return _type;
    }

    private long _lastSent;
    private long _lastReceived;
    private int _totalSent;
    private int _totalReceived;
    private String _type;

    private static final Logger _logger = LoggerFactory.getLogger(TypeStatistics.class);
}
