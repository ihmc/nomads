package netlogger.model.tracking.statistics.builders;

import netlogger.model.tracking.statistics.GeneralStatistics;

public class GeneralStatisticBuilder
{
    public GeneralStatistics createGeneralStatistic () {
        return new GeneralStatistics(_totalSent, _totalReceived, _lastSent, _lastReceived, _mostSentToClient, _mostReceivedFromClient, _leastSentToClient, _leastReceivedFromClient, _mostSentTrackedType, _mostReceivedTrackedType, _leastSentTrackedType, _leastReceivedTrackedType);
    }

    public GeneralStatisticBuilder setLastReceived (String lastReceived) {
        _lastReceived = lastReceived;
        return this;
    }

    public GeneralStatisticBuilder setLastSent (String lastSent) {
        _lastSent = lastSent;
        return this;
    }

    public GeneralStatisticBuilder setLeastReceivedFromClient (String leastReceivedFromClient) {
        _leastReceivedFromClient = leastReceivedFromClient;
        return this;
    }

    public GeneralStatisticBuilder setLeastReceivedType (String leastReceivedTrackedType) {
        _leastReceivedTrackedType = leastReceivedTrackedType;
        return this;
    }

    public GeneralStatisticBuilder setLeastSentToClient (String leastSentToClient) {
        _leastSentToClient = leastSentToClient;
        return this;
    }

    public GeneralStatisticBuilder setLeastSentType (String leastSentTrackedType) {
        _leastSentTrackedType = leastSentTrackedType;
        return this;
    }

    public GeneralStatisticBuilder setMostReceivedFromClient (String mostReceivedFromClient) {
        _mostReceivedFromClient = mostReceivedFromClient;
        return this;
    }

    public GeneralStatisticBuilder setMostReceivedType (String mostReceivedTrackedType) {
        _mostReceivedTrackedType = mostReceivedTrackedType;
        return this;
    }

    public GeneralStatisticBuilder setMostSentToClient (String mostSentToClient) {
        _mostSentToClient = mostSentToClient;
        return this;
    }

    public GeneralStatisticBuilder setMostSentType (String mostSentTrackedType) {
        _mostSentTrackedType = mostSentTrackedType;
        return this;
    }

    public GeneralStatisticBuilder setTotalReceived (int totalReceived) {
        _totalReceived = totalReceived;
        return this;
    }

    public GeneralStatisticBuilder setTotalSent (int totalSent) {
        _totalSent = totalSent;
        return this;
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
}