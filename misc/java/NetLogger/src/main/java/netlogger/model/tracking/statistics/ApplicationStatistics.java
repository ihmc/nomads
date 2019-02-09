package netlogger.model.tracking.statistics;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Application statistic is used to describe statistics about the application
 */
public class ApplicationStatistics
{
    public ApplicationStatistics (int totalSent, int totalReceived, long lastSent, long lastReceived, String mostSentTrackedType,
                                  String mostReceivedTrackedType, String leastSentTrackedType, String leastReceivedTrackedType) {
        _totalSent = totalSent;
        _totalReceived = totalReceived;
        _lastSent = lastSent;
        _lastReceived = lastReceived;
        _mostSentTrackedType = mostSentTrackedType;
        _mostReceivedTrackedType = mostReceivedTrackedType;
        _leastSentTrackedType = leastSentTrackedType;
        _leastReceivedTrackedType = leastReceivedTrackedType;
    }

    public long getLastReceivedTime () {
        return _lastReceived;
    }

    public long getLastSentTime () {
        return _lastSent;
    }

    public String getLeastReceivedType () {
        return _leastReceivedTrackedType;
    }

    public String getLeastSentTrackedType () {
        return _leastSentTrackedType;
    }

    public String getMostReceivedType () {
        return _mostReceivedTrackedType;
    }

    public String getMostSentTrackedType () {
        return _mostSentTrackedType;
    }

    public int getTotalReceived () {
        return _totalReceived;
    }

    public int getTotalSent () {
        return _totalSent;
    }

    private int _totalSent;
    private int _totalReceived;
    private long _lastSent;
    private long _lastReceived;
    private String _mostSentTrackedType;
    private String _mostReceivedTrackedType;
    private String _leastSentTrackedType;
    private String _leastReceivedTrackedType;

    private static final Logger _logger = LoggerFactory.getLogger(ApplicationStatistics.class);
}
