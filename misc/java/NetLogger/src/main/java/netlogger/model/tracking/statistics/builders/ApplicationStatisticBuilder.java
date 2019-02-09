package netlogger.model.tracking.statistics.builders;

import netlogger.model.tracking.statistics.ApplicationStatistics;

public class ApplicationStatisticBuilder
{
    public ApplicationStatistics createApplicationStatistic () {
        return new ApplicationStatistics(_totalSent, _totalReceived, _lastSent, _lastReceived, _mostSentTrackedType, _mostReceivedTrackedType, _leastSentTrackedType, _leastReceivedTrackedType);
    }

    public ApplicationStatisticBuilder setLastReceived (long lastReceived) {
        _lastReceived = lastReceived;
        return this;
    }

    public ApplicationStatisticBuilder setLastSent (long lastSent) {
        _lastSent = lastSent;
        return this;
    }

    public ApplicationStatisticBuilder setLeastReceivedTrackedType (String leastReceivedTrackedType) {
        _leastReceivedTrackedType = leastReceivedTrackedType;
        return this;
    }

    public ApplicationStatisticBuilder setLeastSentTrackedType (String leastSentTrackedType) {
        _leastSentTrackedType = leastSentTrackedType;
        return this;
    }

    public ApplicationStatisticBuilder setMostReceivedTrackedType (String mostReceivedTrackedType) {
        _mostReceivedTrackedType = mostReceivedTrackedType;
        return this;
    }

    public ApplicationStatisticBuilder setMostSentTrackedType (String mostSentTrackedType) {
        _mostSentTrackedType = mostSentTrackedType;
        return this;
    }

    public ApplicationStatisticBuilder setTotalReceived (int totalReceived) {
        _totalReceived = totalReceived;
        return this;
    }

    public ApplicationStatisticBuilder setTotalSent (int totalSent) {
        _totalSent = totalSent;
        return this;
    }

    private int _totalSent;
    private int _totalReceived;
    private long _lastSent;
    private long _lastReceived;
    private String _mostSentTrackedType;
    private String _mostReceivedTrackedType;
    private String _leastSentTrackedType;
    private String _leastReceivedTrackedType;
}