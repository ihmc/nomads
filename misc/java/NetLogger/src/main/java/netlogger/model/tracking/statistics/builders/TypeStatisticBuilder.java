package netlogger.model.tracking.statistics.builders;

import netlogger.model.tracking.statistics.TypeStatistics;

public class TypeStatisticBuilder
{
    public TypeStatistics createTypeStatistic () {
        return new TypeStatistics(_lastSent, _lastReceived, _totalSent, _totalReceived, _type);
    }

    public TypeStatisticBuilder setLastReceived (long lastReceived) {
        _lastReceived = lastReceived;
        return this;
    }

    public TypeStatisticBuilder setLastSent (long lastSent) {
        _lastSent = lastSent;
        return this;
    }

    public TypeStatisticBuilder setTotalReceived (int totalReceived) {
        _totalReceived = totalReceived;
        return this;
    }

    public TypeStatisticBuilder setTotalSent (int totalSent) {
        _totalSent = totalSent;
        return this;
    }

    public void setType (String type) {
        _type = type;
    }

    private long _lastSent;
    private long _lastReceived;
    private int _totalSent;
    private int _totalReceived;
    private String _type;
}