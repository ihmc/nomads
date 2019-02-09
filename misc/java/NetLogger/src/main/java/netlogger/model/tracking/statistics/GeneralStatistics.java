package netlogger.model.tracking.statistics;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * General statistics comprising all sent/received data
 */
public class GeneralStatistics
{
    public GeneralStatistics (int totalSent, int totalReceived, String lastSent, String lastReceived, String mostSentToClient,
                              String mostReceivedFromClient, String leastSentToClient,
                              String leastReceivedFromClient, String mostSentTrackedType, String mostReceivedTrackedType,
                              String leastSentTrackedType, String leastReceivedTrackedType) {
        _totalSent = totalSent;
        _totalReceived = totalReceived;
        _lastSent = lastSent;
        _lastReceived = lastReceived;
        _mostSentToClient = mostSentToClient;
        _mostReceivedFromClient = mostReceivedFromClient;
        _leastSentToClient = leastSentToClient;
        _leastReceivedFromClient = leastReceivedFromClient;
        _mostSentTrackedType = mostSentTrackedType;
        _mostReceivedTrackedType = mostReceivedTrackedType;
        _leastSentTrackedType = leastSentTrackedType;
        _leastReceivedTrackedType = leastReceivedTrackedType;
    }

    public String getLastReceivedTime () {
        return _lastReceived;
    }

    public String getLastSentTime () {
        return _lastSent;
    }

    public String getLeastReceivedFromClient () {
        return _leastReceivedFromClient;
    }

    public String getLeastReceivedType () {
        return _leastReceivedTrackedType;
    }

    public String getLeastSentToClient () {
        return _leastSentToClient;
    }

    public String getLeastSentTrackedType () {
        return _leastSentTrackedType;
    }

    public String getMostReceivedFromClient () {
        return _mostReceivedFromClient;
    }

    public String getMostReceivedType () {
        return _mostReceivedTrackedType;
    }

    public String getMostSentToClient () {
        return _mostSentToClient;
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
    private String _lastSent;
    private String _lastReceived;

    private String _mostSentToClient;
    private String _mostReceivedFromClient;
    private String _leastSentToClient;
    private String _leastReceivedFromClient;

    private String _mostSentTrackedType;
    private String _mostReceivedTrackedType;
    private String _leastSentTrackedType;
    private String _leastReceivedTrackedType;

    private static final Logger _logger = LoggerFactory.getLogger(GeneralStatistics.class);
}
